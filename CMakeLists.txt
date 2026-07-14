cmake_minimum_required(VERSION 3.25)
project(PMA_Core VERSION 0.1.0 LANGUAGES C CXX)

include(CTest)

option(PMA_WARNINGS_AS_ERRORS "Treat PMA-owned warnings as errors" OFF)
option(PMA_ENABLE_SANITIZERS "Enable AddressSanitizer and UndefinedBehaviorSanitizer" OFF)

add_library(pma_project_options INTERFACE)
target_compile_features(pma_project_options INTERFACE cxx_std_20)

if(MSVC)
  target_compile_options(pma_project_options INTERFACE /W4 /permissive-)
  if(PMA_WARNINGS_AS_ERRORS)
    target_compile_options(pma_project_options INTERFACE /WX)
  endif()
else()
  target_compile_options(pma_project_options INTERFACE -Wall -Wextra -Wpedantic)
  if(PMA_WARNINGS_AS_ERRORS)
    target_compile_options(pma_project_options INTERFACE -Werror)
  endif()
endif()

if(PMA_ENABLE_SANITIZERS)
  if(MSVC)
    message(FATAL_ERROR "PMA_ENABLE_SANITIZERS is currently supported only by GCC and Clang")
  endif()
  target_compile_options(pma_project_options INTERFACE -fsanitize=address,undefined -fno-omit-frame-pointer)
  target_link_options(pma_project_options INTERFACE -fsanitize=address,undefined)
endif()

add_library(pma_sqlite STATIC third_party/sqlite/sqlite3.c)
target_include_directories(pma_sqlite PRIVATE third_party/sqlite)
target_compile_definitions(
  pma_sqlite
  PRIVATE SQLITE_ENABLE_FTS5 SQLITE_THREADSAFE=1 SQLITE_DEFAULT_FOREIGN_KEYS=1
)
if(MSVC)
  target_compile_options(pma_sqlite PRIVATE /W0)
else()
  target_compile_options(pma_sqlite PRIVATE -w)
endif()

add_library(pma_sha256 STATIC third_party/sha256/sha-256.c)
target_include_directories(pma_sha256 PRIVATE third_party/sha256)
if(MSVC)
  target_compile_options(pma_sha256 PRIVATE /W0)
else()
  target_compile_options(pma_sha256 PRIVATE -w)
endif()

add_library(pma_storage STATIC src/storage/storage.cpp)
target_include_directories(pma_storage PUBLIC include PRIVATE third_party/sqlite third_party/sha256)
target_link_libraries(pma_storage PUBLIC pma_project_options PRIVATE pma_sqlite pma_sha256)

add_library(pma_graph STATIC src/application/graph.cpp)
target_include_directories(pma_graph PUBLIC include)
target_link_libraries(pma_graph PUBLIC pma_project_options pma_storage)

add_library(pma_lifecycle STATIC src/application/lifecycle.cpp)
target_include_directories(pma_lifecycle PUBLIC include)
target_link_libraries(pma_lifecycle PUBLIC pma_project_options pma_graph)

add_library(pma_vectors STATIC src/vectors/vectors.cpp)
target_include_directories(pma_vectors PUBLIC include)
target_link_libraries(pma_vectors PUBLIC pma_project_options pma_lifecycle)

add_library(pma_retrieval STATIC src/retrieval/retrieval.cpp)
target_include_directories(pma_retrieval PUBLIC include)
target_link_libraries(pma_retrieval PUBLIC pma_project_options pma_vectors)

add_library(pma_provider_process STATIC src/platform/provider_process.cpp)
target_include_directories(pma_provider_process PUBLIC include)
target_link_libraries(pma_provider_process PUBLIC pma_project_options)

find_package(nlohmann_json CONFIG REQUIRED)
add_library(pma_maintenance STATIC src/application/maintenance.cpp)
target_include_directories(pma_maintenance PUBLIC include)
target_link_libraries(pma_maintenance PUBLIC pma_project_options pma_storage)

add_library(pma_service STATIC src/application/service.cpp src/transport/stdio.cpp)
target_include_directories(pma_service PUBLIC include)
target_link_libraries(
  pma_service PUBLIC pma_project_options PRIVATE pma_retrieval pma_provider_process nlohmann_json::nlohmann_json
)

add_executable(pma-core src/main/main.cpp)
target_link_libraries(pma-core PRIVATE pma_project_options pma_storage pma_service pma_maintenance)

if(BUILD_TESTING)
  find_package(Catch2 CONFIG REQUIRED)
  find_package(benchmark CONFIG REQUIRED)
  find_program(NODE_EXECUTABLE node REQUIRED)
  add_executable(pma-smoke-test tests/unit/smoke_test.cpp)
  target_link_libraries(pma-smoke-test PRIVATE pma_project_options Catch2::Catch2WithMain)
  add_executable(pma-storage-test tests/storage/storage_test.cpp)
  target_link_libraries(pma-storage-test PRIVATE pma_storage Catch2::Catch2WithMain)
  add_executable(pma-maintenance-test tests/storage/maintenance_test.cpp)
  target_link_libraries(pma-maintenance-test PRIVATE pma_maintenance Catch2::Catch2WithMain)
  add_executable(pma-graph-test tests/graph/graph_test.cpp)
  target_link_libraries(pma-graph-test PRIVATE pma_graph Catch2::Catch2WithMain)
  add_executable(pma-lifecycle-test tests/lifecycle/lifecycle_test.cpp)
  target_link_libraries(pma-lifecycle-test PRIVATE pma_lifecycle Catch2::Catch2WithMain)
  add_executable(pma-vector-test tests/vectors/vector_test.cpp)
  target_link_libraries(pma-vector-test PRIVATE pma_vectors Catch2::Catch2WithMain)
  add_executable(pma-retrieval-test tests/retrieval/retrieval_test.cpp)
  target_link_libraries(pma-retrieval-test PRIVATE pma_retrieval Catch2::Catch2WithMain)
  add_executable(pma-release-gate-test tests/integration/release_gate_test.cpp)
  target_link_libraries(
    pma-release-gate-test PRIVATE pma_retrieval pma_maintenance Catch2::Catch2WithMain
  )
  add_executable(pma-learning-recall-simulation-test
                 tests/integration/learning_recall_simulation_test.cpp)
  target_link_libraries(
    pma-learning-recall-simulation-test PRIVATE pma_retrieval Catch2::Catch2WithMain
  )
  add_executable(pma-provider-process-test tests/providers/provider_process_test.cpp)
  target_compile_definitions(
    pma-provider-process-test
    PRIVATE PMA_NODE_EXECUTABLE="${NODE_EXECUTABLE}"
            PMA_FAKE_PROVIDER_SCRIPT="${CMAKE_CURRENT_SOURCE_DIR}/tests/providers/fake-provider.mjs"
  )
  target_link_libraries(
    pma-provider-process-test PRIVATE pma_provider_process nlohmann_json::nlohmann_json Catch2::Catch2WithMain
  )
  add_executable(pma-benchmark tests/performance/pma_benchmark.cpp)
  target_link_libraries(pma-benchmark PRIVATE pma_retrieval benchmark::benchmark)
  add_executable(pma-service-test tests/protocol/service_test.cpp)
  target_compile_definitions(
    pma-service-test PRIVATE PMA_PROTOCOL_FIXTURE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/protocol/pma-service/fixtures"
      PMA_NODE_EXECUTABLE="${NODE_EXECUTABLE}"
      PMA_FAKE_PROVIDER_PATH="${CMAKE_CURRENT_SOURCE_DIR}/tests/providers/fake-provider.mjs"
  )
  target_link_libraries(
    pma-service-test PRIVATE pma_service nlohmann_json::nlohmann_json Catch2::Catch2WithMain
  )
  include(Catch)
  catch_discover_tests(pma-smoke-test)
  catch_discover_tests(pma-storage-test)
  catch_discover_tests(pma-maintenance-test)
  catch_discover_tests(pma-graph-test)
  catch_discover_tests(pma-lifecycle-test)
  catch_discover_tests(pma-vector-test)
  catch_discover_tests(pma-retrieval-test)
  catch_discover_tests(pma-release-gate-test)
  catch_discover_tests(pma-learning-recall-simulation-test)
  catch_discover_tests(pma-provider-process-test)
  catch_discover_tests(pma-service-test)

  add_test(
    NAME pma-stdio-node-integration
    COMMAND ${NODE_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/protocol/stdio.integration.mjs
            $<TARGET_FILE:pma-core>
  )
endif()

include(GNUInstallDirs)
install(TARGETS pma-core RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES LICENSE THIRD_PARTY_NOTICES.md
              docs/07-delivery/COMPATIBILITY_AND_LIMITATIONS.md
              docs/07-delivery/MIGRATION_MATRIX.md
              docs/07-delivery/SECURITY_REVIEW_0.1.0.md
              docs/07-delivery/UPGRADE_AND_RECOVERY.md
        DESTINATION ${CMAKE_INSTALL_DOCDIR})
set(CPACK_PACKAGE_NAME "pma-core")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_VENDOR "PMA-Core")
set(CPACK_PACKAGE_FILE_NAME "pma-core-${PROJECT_VERSION}-${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR}")
set(CPACK_GENERATOR "ZIP;TGZ")
include(CPack)
