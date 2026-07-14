#include "pma-internal/vectors.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>
#include <sstream>
#include <utility>

namespace pma::vectors {
namespace {

using storage::Database;
using storage::Error;
using storage::ErrorCode;
using storage::Migration;
using storage::Statement;
using storage::Transaction;

const std::vector<Migration> migrations{{
    300,
    "vector_projection_registry",
    R"sql(
ALTER TABLE embedding_documents ADD COLUMN content_hash TEXT NOT NULL DEFAULT '';
ALTER TABLE embedding_documents ADD COLUMN renderer_version TEXT NOT NULL DEFAULT 'claim-v1';
ALTER TABLE embedding_documents ADD COLUMN active INTEGER NOT NULL DEFAULT 1 CHECK(active IN (0,1));
CREATE UNIQUE INDEX embedding_document_object ON embedding_documents(object_type,object_id);
CREATE TABLE provider_runtime_manifests(
  id TEXT PRIMARY KEY, provider_package TEXT NOT NULL, provider_version TEXT NOT NULL,
  node_version TEXT NOT NULL, os TEXT NOT NULL, architecture TEXT NOT NULL,
  lockfile_hash TEXT NOT NULL, backend TEXT NOT NULL, deterministic_settings TEXT NOT NULL
) STRICT;
CREATE TABLE embedding_profiles(
  id TEXT PRIMARY KEY, fingerprint TEXT NOT NULL UNIQUE, provider TEXT NOT NULL,
  runtime_type TEXT NOT NULL, model_hash TEXT NOT NULL, model_revision TEXT NOT NULL,
  tokenizer_hash TEXT NOT NULL, dimensions INTEGER NOT NULL CHECK(dimensions>0),
  data_type TEXT NOT NULL, pooling TEXT NOT NULL, normalized INTEGER NOT NULL CHECK(normalized IN (0,1)),
  query_prefix TEXT NOT NULL, document_prefix TEXT NOT NULL, max_input INTEGER NOT NULL,
  truncation TEXT NOT NULL, renderer_version TEXT NOT NULL, provider_options TEXT NOT NULL,
  runtime_manifest_id TEXT NOT NULL REFERENCES provider_runtime_manifests(id)
) STRICT;
CREATE TABLE vector_store_registry(
  id TEXT PRIMARY KEY, profile_id TEXT NOT NULL REFERENCES embedding_profiles(id),
  profile_fingerprint TEXT NOT NULL, generation INTEGER NOT NULL CHECK(generation>0),
  state TEXT NOT NULL CHECK(state IN ('building','ready','active','retired','failed')),
  relative_path TEXT NOT NULL UNIQUE, watermark INTEGER NOT NULL DEFAULT 0,
  runtime_manifest_id TEXT NOT NULL REFERENCES provider_runtime_manifests(id),
  created_at TEXT NOT NULL DEFAULT (strftime('%Y-%m-%dT%H:%M:%fZ','now')),
  UNIQUE(profile_id,generation)
) STRICT;
)sql"}};

void bind_profile(Statement &insert, const Profile &profile, std::string_view fingerprint,
                  std::string_view runtime_id) {
  insert.bind(1, profile.id);
  insert.bind(2, fingerprint);
  insert.bind(3, profile.provider);
  insert.bind(4, profile.runtime_type);
  insert.bind(5, profile.model_hash);
  insert.bind(6, profile.model_revision);
  insert.bind(7, profile.tokenizer_hash);
  insert.bind(8, profile.dimensions);
  insert.bind(9, profile.data_type);
  insert.bind(10, profile.pooling);
  insert.bind(11, static_cast<std::int64_t>(profile.normalized));
  insert.bind(12, profile.query_prefix);
  insert.bind(13, profile.document_prefix);
  insert.bind(14, profile.max_input);
  insert.bind(15, profile.truncation);
  insert.bind(16, profile.renderer_version);
  insert.bind(17, profile.provider_options);
  insert.bind(18, runtime_id);
}

Profile read_profile(Database &core, std::string_view id) {
  auto query = core.prepare(
      "SELECT id,provider,runtime_type,model_hash,model_revision,tokenizer_hash,dimensions,"
      "data_type,pooling,normalized,query_prefix,document_prefix,max_input,truncation,"
      "renderer_version,provider_options FROM embedding_profiles WHERE id=?1");
  query.bind(1, id);
  if (!query.step()) {
    throw Error(ErrorCode::constraint, 0, "embedding profile not found");
  }
  return {query.column_text(0),  query.column_text(1), query.column_text(2),
          query.column_text(3),  query.column_text(4), query.column_text(5),
          query.column_int64(6), query.column_text(7), query.column_text(8),
          query.column_int64(9) != 0, query.column_text(10), query.column_text(11),
          query.column_int64(12), query.column_text(13), query.column_text(14),
          query.column_text(15)};
}

std::vector<std::byte> pack(const std::vector<float> &values) {
  std::vector<std::byte> bytes(values.size() * sizeof(float));
  for (std::size_t index = 0; index < values.size(); ++index) {
    std::uint32_t bits{};
    std::memcpy(&bits, &values[index], sizeof(bits));
    for (std::size_t byte = 0; byte < 4; ++byte) {
      bytes[index * 4 + byte] = static_cast<std::byte>((bits >> (byte * 8)) & 0xffU);
    }
  }
  return bytes;
}

std::vector<float> unpack(const std::vector<std::byte> &bytes) {
  if (bytes.size() % sizeof(float) != 0) {
    return {};
  }
  std::vector<float> values(bytes.size() / sizeof(float));
  for (std::size_t index = 0; index < values.size(); ++index) {
    std::uint32_t bits{};
    for (std::size_t byte = 0; byte < 4; ++byte) {
      bits |= static_cast<std::uint32_t>(std::to_integer<unsigned char>(bytes[index * 4 + byte]))
              << (byte * 8);
    }
    std::memcpy(&values[index], &bits, sizeof(bits));
  }
  return values;
}

float cosine(const std::vector<float> &left, const std::vector<float> &right) {
  if (left.size() != right.size() || left.empty()) {
    return -1.0F;
  }
  double dot = 0.0;
  double left_norm = 0.0;
  double right_norm = 0.0;
  for (std::size_t index = 0; index < left.size(); ++index) {
    dot += static_cast<double>(left[index]) * right[index];
    left_norm += static_cast<double>(left[index]) * left[index];
    right_norm += static_cast<double>(right[index]) * right[index];
  }
  if (left_norm == 0.0 || right_norm == 0.0) {
    return -1.0F;
  }
  return static_cast<float>(dot / (std::sqrt(left_norm) * std::sqrt(right_norm)));
}

void create_vector_schema(Database &vectors, const StoreInfo &info, std::string_view core_id,
                          std::string_view runtime_id, const Profile &profile) {
  vectors.execute(
      "CREATE TABLE IF NOT EXISTS vector_metadata(key TEXT PRIMARY KEY,value TEXT NOT NULL) STRICT;"
      "CREATE TABLE IF NOT EXISTS vectors(document_id TEXT PRIMARY KEY,content_hash TEXT NOT NULL,"
      "dimensions INTEGER NOT NULL,vector BLOB NOT NULL) STRICT;");
  const std::vector<std::pair<std::string, std::string>> metadata{
      {"store_id", info.id},
      {"profile_id", info.profile_id},
      {"profile_fingerprint", info.fingerprint},
      {"generation", std::to_string(info.generation)},
      {"source_core_database_id", std::string(core_id)},
      {"vector_schema_version", "1"},
      {"runtime_manifest_id", std::string(runtime_id)},
      {"dimensions", std::to_string(profile.dimensions)},
      {"metric", "cosine"},
      {"normalization", profile.normalized ? "normalized" : "none"},
      {"data_type", profile.data_type},
      {"watermark", "0"}};
  for (const auto &[key, value] : metadata) {
    auto insert = vectors.prepare(
        "INSERT OR REPLACE INTO vector_metadata(key,value) VALUES(?1,?2)");
    insert.bind(1, key);
    insert.bind(2, value);
    insert.execute();
  }
}

std::string metadata(Database &vectors, std::string_view key) {
  auto query = vectors.prepare("SELECT value FROM vector_metadata WHERE key=?1");
  query.bind(1, key);
  if (!query.step()) {
    throw Error(ErrorCode::corruption, 0, "vector metadata is incomplete");
  }
  return query.column_text(0);
}

} // namespace

std::string Profile::fingerprint() const {
  std::ostringstream value;
  value << provider << '\x1f' << runtime_type << '\x1f' << model_hash << '\x1f'
        << model_revision << '\x1f' << tokenizer_hash << '\x1f' << dimensions << '\x1f'
        << data_type << '\x1f' << pooling << '\x1f' << normalized << '\x1f' << query_prefix
        << '\x1f' << document_prefix << '\x1f' << max_input << '\x1f' << truncation << '\x1f'
        << renderer_version << '\x1f' << provider_options;
  return storage::sha256(value.str());
}

std::vector<float> FakeEmbeddingProvider::embed(std::string_view text, const Profile &profile) {
  if (fail_after_.has_value() && calls_ >= *fail_after_) {
    throw Error(ErrorCode::constraint, 0, "fake embedding provider interrupted");
  }
  ++calls_;
  std::vector<float> result(static_cast<std::size_t>(profile.dimensions));
  const std::string seed = profile.document_prefix + std::string(text) + profile.fingerprint();
  for (std::size_t index = 0; index < result.size(); ++index) {
    const auto digest = storage::sha256(seed + std::to_string(index));
    const auto sample = static_cast<unsigned>(std::stoul(digest.substr(0, 8), nullptr, 16));
    result[index] = static_cast<float>(static_cast<int>(sample % 20001) - 10000) / 10000.0F;
  }
  if (profile.normalized) {
    double norm = 0.0;
    for (const float value : result) {
      norm += static_cast<double>(value) * value;
    }
    norm = std::sqrt(norm);
    if (norm > 0.0) {
      for (float &value : result) {
        value = static_cast<float>(value / norm);
      }
    }
  }
  return result;
}

void FakeEmbeddingProvider::fail_after(std::optional<std::size_t> calls) {
  fail_after_ = calls;
  calls_ = 0;
}

Manager::Manager(Database &core, std::filesystem::path bundle_root)
    : core_(&core), bundle_root_(std::move(bundle_root)) {}

void Manager::initialize(std::string_view build_version) {
  storage::migrate(*core_, migrations, build_version);
  auto documents = core_->prepare("SELECT id,text FROM embedding_documents WHERE content_hash='' ");
  while (documents.step()) {
    auto update = core_->prepare("UPDATE embedding_documents SET content_hash=?2 WHERE id=?1");
    update.bind(1, documents.column_text(0));
    update.bind(2, storage::sha256(documents.column_text(1)));
    update.execute();
  }
  std::filesystem::create_directories(bundle_root_ / "vectors");
}

void Manager::register_profile(const Profile &profile, const RuntimeManifest &runtime) {
  Transaction transaction(*core_);
  auto manifest = core_->prepare(
      "INSERT OR IGNORE INTO provider_runtime_manifests VALUES(?1,?2,?3,?4,?5,?6,?7,?8,?9)");
  manifest.bind(1, runtime.id);
  manifest.bind(2, runtime.provider_package);
  manifest.bind(3, runtime.provider_version);
  manifest.bind(4, runtime.node_version);
  manifest.bind(5, runtime.os);
  manifest.bind(6, runtime.architecture);
  manifest.bind(7, runtime.lockfile_hash);
  manifest.bind(8, runtime.backend);
  manifest.bind(9, runtime.deterministic_settings);
  manifest.execute();
  auto insert = core_->prepare(
      "INSERT OR IGNORE INTO embedding_profiles VALUES(?1,?2,?3,?4,?5,?6,?7,?8,?9,?10,?11,?12,?13,?14,?15,?16,?17,?18)");
  bind_profile(insert, profile, profile.fingerprint(), runtime.id);
  insert.execute();
  transaction.commit();
}

void Manager::render_claim(std::string_view claim_id, std::string_view operation_id,
                           std::string_view renderer_version) {
  auto query = core_->prepare(
      "SELECT s.canonical_name,p.name,COALESCE(o.canonical_name,v.text_value),c.status "
      "FROM claims c JOIN entities s ON s.id=c.subject_id JOIN predicates p ON p.id=c.predicate_id "
      "LEFT JOIN entities o ON o.id=c.object_entity_id LEFT JOIN typed_values v ON v.id=c.typed_value_id "
      "WHERE c.id=?1");
  query.bind(1, claim_id);
  if (!query.step()) {
    throw Error(ErrorCode::constraint, 0, "claim not found for rendering");
  }
  const std::string document_id = "embedding:" + std::string(claim_id);
  const std::string text = query.column_text(0) + " | " + query.column_text(1) + " | " +
                           query.column_text(2);
  const bool active = query.column_text(3) == "active";
  Transaction transaction(*core_);
  auto upsert = core_->prepare(
      "INSERT INTO embedding_documents(id,object_type,object_id,text,revision,content_hash,renderer_version,active) "
      "VALUES(?1,'claim',?2,?3,1,?4,?5,?6) ON CONFLICT(object_type,object_id) DO UPDATE SET "
      "text=excluded.text,revision=revision+1,content_hash=excluded.content_hash,"
      "renderer_version=excluded.renderer_version,active=excluded.active");
  upsert.bind(1, document_id);
  upsert.bind(2, claim_id);
  upsert.bind(3, text);
  upsert.bind(4, storage::sha256(text));
  upsert.bind(5, renderer_version);
  upsert.bind(6, static_cast<std::int64_t>(active));
  upsert.execute();
  auto event = core_->prepare(
      "INSERT INTO embedding_events(operation_id,object_id,action) VALUES(?1,?2,?3)");
  event.bind(1, operation_id);
  event.bind(2, document_id);
  event.bind(3, active ? "upsert" : "delete");
  event.execute();
  transaction.commit();
}

void Manager::deactivate_document(std::string_view document_id, std::string_view operation_id) {
  Transaction transaction(*core_);
  auto update = core_->prepare("UPDATE embedding_documents SET active=0,revision=revision+1 WHERE id=?1");
  update.bind(1, document_id);
  update.execute();
  auto event = core_->prepare(
      "INSERT INTO embedding_events(operation_id,object_id,action) VALUES(?1,?2,'delete')");
  event.bind(1, operation_id);
  event.bind(2, document_id);
  event.execute();
  transaction.commit();
}

StoreInfo Manager::build(std::string_view store_id, std::string_view profile_id,
                         std::int64_t generation, std::string_view runtime_id,
                         EmbeddingProvider &provider) {
  const Profile profile = read_profile(*core_, profile_id);
  const std::filesystem::path relative =
      std::filesystem::path("vectors") /
      (std::string(profile_id) + "-generation-" + std::to_string(generation) + ".sqlite");
  StoreInfo info{std::string(store_id), std::string(profile_id), profile.fingerprint(), generation,
                 "building", relative, 0, 0};
  auto registry = core_->prepare(
      "INSERT INTO vector_store_registry(id,profile_id,profile_fingerprint,generation,state,"
      "relative_path,runtime_manifest_id) VALUES(?1,?2,?3,?4,'building',?5,?6)");
  registry.bind(1, store_id);
  registry.bind(2, profile_id);
  registry.bind(3, info.fingerprint);
  registry.bind(4, generation);
  registry.bind(5, relative.generic_string());
  registry.bind(6, runtime_id);
  registry.execute();
  Database vectors(bundle_root_ / relative);
  create_vector_schema(vectors, info, core_->identity(), runtime_id, profile);
  catch_up(store_id, provider);
  const auto issues = validate(store_id);
  auto state = core_->prepare("UPDATE vector_store_registry SET state=?2 WHERE id=?1");
  state.bind(1, store_id);
  state.bind(2, issues.empty() ? "ready" : "failed");
  state.execute();
  return inspect(store_id);
}

void Manager::catch_up(std::string_view store_id, EmbeddingProvider &provider,
                       std::size_t batch_size) {
  const StoreInfo info = inspect(store_id);
  const Profile profile = read_profile(*core_, info.profile_id);
  if (profile.fingerprint() != info.fingerprint) {
    throw Error(ErrorCode::constraint, 0, "incompatible vector profile");
  }
  Database vectors(bundle_root_ / info.relative_path);
  std::int64_t watermark = std::stoll(metadata(vectors, "watermark"));
  while (true) {
    auto events = core_->prepare(
        "SELECT event_seq,object_id,action FROM embedding_events WHERE event_seq>?1 "
        "ORDER BY event_seq LIMIT ?2");
    events.bind(1, watermark);
    events.bind(2, static_cast<std::int64_t>(batch_size));
    struct Event {
      std::int64_t sequence;
      std::string document_id;
      std::string action;
    };
    std::vector<Event> batch;
    while (events.step()) {
      batch.push_back({events.column_int64(0), events.column_text(1), events.column_text(2)});
    }
    if (batch.empty()) {
      break;
    }
    Transaction transaction(vectors);
    for (const auto &event : batch) {
      if (event.action == "delete") {
        auto remove = vectors.prepare("DELETE FROM vectors WHERE document_id=?1");
        remove.bind(1, event.document_id);
        remove.execute();
      } else {
        auto document = core_->prepare(
            "SELECT text,content_hash,active FROM embedding_documents WHERE id=?1");
        document.bind(1, event.document_id);
        if (document.step() && document.column_int64(2) != 0) {
          const auto vector = provider.embed(document.column_text(0), profile);
          if (vector.size() != static_cast<std::size_t>(profile.dimensions)) {
            throw Error(ErrorCode::constraint, 0, "embedding provider returned wrong dimensions");
          }
          auto upsert = vectors.prepare(
              "INSERT INTO vectors(document_id,content_hash,dimensions,vector) VALUES(?1,?2,?3,?4) "
              "ON CONFLICT(document_id) DO UPDATE SET content_hash=excluded.content_hash,"
              "dimensions=excluded.dimensions,vector=excluded.vector");
          upsert.bind(1, event.document_id);
          upsert.bind(2, document.column_text(1));
          upsert.bind(3, profile.dimensions);
          upsert.bind_blob(4, pack(vector));
          upsert.execute();
        }
      }
      watermark = event.sequence;
    }
    auto mark = vectors.prepare(
        "UPDATE vector_metadata SET value=?1 WHERE key='watermark'");
    mark.bind(1, std::to_string(watermark));
    mark.execute();
    transaction.commit();
    auto registry = core_->prepare("UPDATE vector_store_registry SET watermark=?2 WHERE id=?1");
    registry.bind(1, store_id);
    registry.bind(2, watermark);
    registry.execute();
  }
}

std::vector<ValidationIssue> Manager::validate(std::string_view store_id) {
  std::vector<ValidationIssue> issues;
  StoreInfo info;
  try {
    info = inspect(store_id);
  } catch (...) {
    return {{"missing_registry", std::string(store_id)}};
  }
  if (!std::filesystem::exists(bundle_root_ / info.relative_path)) {
    return {{"missing_file", std::string(store_id)}};
  }
  try {
    Database vectors(bundle_root_ / info.relative_path);
    if (metadata(vectors, "store_id") != info.id ||
        metadata(vectors, "profile_fingerprint") != info.fingerprint ||
        metadata(vectors, "source_core_database_id") != core_->identity()) {
      issues.push_back({"metadata_mismatch", info.id});
    }
    const Profile profile = read_profile(*core_, info.profile_id);
    auto rows = vectors.prepare("SELECT document_id,content_hash,dimensions,vector FROM vectors");
    while (rows.step()) {
      const std::string document_id = rows.column_text(0);
      auto document = core_->prepare(
          "SELECT content_hash,active FROM embedding_documents WHERE id=?1");
      document.bind(1, document_id);
      if (!document.step() || document.column_int64(1) == 0) {
        issues.push_back({"orphan_vector", document_id});
        continue;
      }
      if (document.column_text(0) != rows.column_text(1)) {
        issues.push_back({"content_hash_mismatch", document_id});
      }
      const auto values = unpack(rows.column_blob(3));
      if (rows.column_int64(2) != profile.dimensions ||
          values.size() != static_cast<std::size_t>(profile.dimensions)) {
        issues.push_back({"wrong_dimensions", document_id});
      } else if (std::any_of(values.begin(), values.end(),
                             [](float value) { return !std::isfinite(value); })) {
        issues.push_back({"non_finite", document_id});
      }
    }
    auto active_documents = core_->prepare("SELECT id FROM embedding_documents WHERE active=1");
    while (active_documents.step()) {
      const auto document_id = active_documents.column_text(0);
      auto vector = vectors.prepare("SELECT 1 FROM vectors WHERE document_id=?1");
      vector.bind(1, document_id);
      if (!vector.step()) {
        issues.push_back({"missing_vector", document_id});
      }
    }
    auto maximum = core_->prepare("SELECT COALESCE(max(event_seq),0) FROM embedding_events");
    (void)maximum.step();
    if (std::stoll(metadata(vectors, "watermark")) < maximum.column_int64(0)) {
      issues.push_back({"stale_watermark", info.id});
    }
  } catch (...) {
    issues.push_back({"corrupt_file", info.id});
  }
  return issues;
}

void Manager::activate(std::string_view store_id) {
  if (!validate(store_id).empty()) {
    throw Error(ErrorCode::corruption, 0, "vector store validation failed");
  }
  const auto selected = inspect(store_id);
  Transaction transaction(*core_);
  auto retire_old = core_->prepare(
      "UPDATE vector_store_registry SET state='retired' WHERE profile_id=?1 AND state='active'");
  retire_old.bind(1, selected.profile_id);
  retire_old.execute();
  auto activate_new = core_->prepare(
      "UPDATE vector_store_registry SET state='active' WHERE id=?1 AND state IN ('ready','active')");
  activate_new.bind(1, store_id);
  activate_new.execute();
  transaction.commit();
}

void Manager::retire(std::string_view store_id) {
  auto update = core_->prepare("UPDATE vector_store_registry SET state='retired' WHERE id=?1");
  update.bind(1, store_id);
  update.execute();
}

StoreInfo Manager::inspect(std::string_view store_id) {
  auto query = core_->prepare(
      "SELECT id,profile_id,profile_fingerprint,generation,state,relative_path,watermark "
      "FROM vector_store_registry WHERE id=?1");
  query.bind(1, store_id);
  if (!query.step()) {
    throw Error(ErrorCode::constraint, 0, "vector store not found");
  }
  StoreInfo info{query.column_text(0), query.column_text(1), query.column_text(2),
                 query.column_int64(3), query.column_text(4), query.column_text(5),
                 query.column_int64(6), 0};
  if (std::filesystem::exists(bundle_root_ / info.relative_path)) {
    try {
      Database vectors(bundle_root_ / info.relative_path);
      auto count = vectors.prepare("SELECT count(*) FROM vectors");
      if (count.step()) {
        info.vector_count = count.column_int64(0);
      }
      info.watermark = std::stoll(metadata(vectors, "watermark"));
    } catch (...) {
    }
  }
  return info;
}

std::vector<SearchResult> Manager::search(std::string_view store_id,
                                          const std::vector<float> &query,
                                          std::size_t limit) {
  const auto info = inspect(store_id);
  const auto profile = read_profile(*core_, info.profile_id);
  if (query.size() != static_cast<std::size_t>(profile.dimensions)) {
    throw Error(ErrorCode::constraint, 0, "query vector has incompatible dimensions");
  }
  Database vectors(bundle_root_ / info.relative_path);
  auto rows = vectors.prepare("SELECT document_id,vector FROM vectors");
  std::vector<SearchResult> results;
  while (rows.step()) {
    results.push_back({rows.column_text(0), cosine(query, unpack(rows.column_blob(1)))});
  }
  std::sort(results.begin(), results.end(), [](const auto &left, const auto &right) {
    return left.score > right.score;
  });
  if (results.size() > limit) {
    results.resize(limit);
  }
  return results;
}

std::string Manager::reconstruction_plan(std::string_view store_id) {
  const auto info = inspect(store_id);
  auto query = core_->prepare(
      "SELECT r.provider_package,r.provider_version,r.node_version,r.os,r.architecture,"
      "r.lockfile_hash,r.backend,r.deterministic_settings,p.model_hash,p.tokenizer_hash "
      "FROM vector_store_registry s JOIN provider_runtime_manifests r ON r.id=s.runtime_manifest_id "
      "JOIN embedding_profiles p ON p.id=s.profile_id WHERE s.id=?1");
  query.bind(1, store_id);
  if (!query.step()) {
    throw Error(ErrorCode::constraint, 0, "runtime manifest not found");
  }
  std::ostringstream output;
  output << "provider=" << query.column_text(0) << '@' << query.column_text(1)
         << ";node=" << query.column_text(2) << ";platform=" << query.column_text(3) << '/'
         << query.column_text(4) << ";lockfile=" << query.column_text(5)
         << ";backend=" << query.column_text(6) << ";settings=" << query.column_text(7)
         << ";model_hash=" << query.column_text(8)
         << ";tokenizer_hash=" << query.column_text(9)
         << ";compatibility=conformance-required";
  return output.str();
}

} // namespace pma::vectors
