#include "pma-internal/provider_process.hpp"

#include <atomic>
#include <mutex>
#include <stdexcept>
#include <utility>

#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#else
#include <csignal>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

namespace pma::providers {
namespace {

std::string cancellation(std::string_view id) {
  return "{\"jsonrpc\":\"2.0\",\"method\":\"provider.cancel\",\"params\":{\"id\":\"" +
         std::string(id) + "\"}}";
}

} // namespace

struct Process::Impl {
  explicit Impl(std::vector<std::string> value) : command(std::move(value)) {}
  std::vector<std::string> command;
  mutable std::mutex state_mutex;
  std::mutex write_mutex;
  std::mutex read_mutex;
#ifdef _WIN32
  PROCESS_INFORMATION process{};
  HANDLE input_write{INVALID_HANDLE_VALUE};
  HANDLE output_read{INVALID_HANDLE_VALUE};
#else
  pid_t pid{-1};
  int input_write{-1};
  int output_read{-1};
#endif
};

Process::Process(std::vector<std::string> command)
    : impl_(std::make_unique<Impl>(std::move(command))) {
  if (impl_->command.empty()) {
    throw std::invalid_argument("provider command cannot be empty");
  }
}
Process::Process(Process &&) noexcept = default;
Process &Process::operator=(Process &&) noexcept = default;
Process::~Process() { stop(); }

#ifdef _WIN32
namespace {
std::wstring widen(std::string_view value) {
  if (value.empty()) {
    return {};
  }
  const int size = MultiByteToWideChar(CP_UTF8, 0, value.data(), static_cast<int>(value.size()),
                                       nullptr, 0);
  std::wstring result(static_cast<std::size_t>(size), L'\0');
  MultiByteToWideChar(CP_UTF8, 0, value.data(), static_cast<int>(value.size()), result.data(), size);
  return result;
}

std::wstring quote(std::string_view value) {
  std::wstring input = widen(value);
  std::wstring output = L"\"";
  std::size_t slashes = 0;
  for (const wchar_t character : input) {
    if (character == L'\\') {
      ++slashes;
    } else if (character == L'\"') {
      output.append(slashes * 2 + 1, L'\\');
      output.push_back(character);
      slashes = 0;
    } else {
      output.append(slashes, L'\\');
      slashes = 0;
      output.push_back(character);
    }
  }
  output.append(slashes * 2, L'\\');
  output.push_back(L'\"');
  return output;
}
} // namespace
#endif

void Process::start() {
  std::scoped_lock state(impl_->state_mutex);
#ifdef _WIN32
  if (impl_->process.hProcess != nullptr) {
    DWORD code{};
    if (GetExitCodeProcess(impl_->process.hProcess, &code) && code == STILL_ACTIVE) {
      return;
    }
    CloseHandle(impl_->process.hThread);
    CloseHandle(impl_->process.hProcess);
    impl_->process = {};
  }
  SECURITY_ATTRIBUTES attributes{sizeof(SECURITY_ATTRIBUTES), nullptr, TRUE};
  HANDLE child_input_read = INVALID_HANDLE_VALUE;
  HANDLE child_output_write = INVALID_HANDLE_VALUE;
  if (!CreatePipe(&child_input_read, &impl_->input_write, &attributes, 0) ||
      !CreatePipe(&impl_->output_read, &child_output_write, &attributes, 0)) {
    throw std::runtime_error("failed to create provider pipes");
  }
  SetHandleInformation(impl_->input_write, HANDLE_FLAG_INHERIT, 0);
  SetHandleInformation(impl_->output_read, HANDLE_FLAG_INHERIT, 0);
  std::wstring command_line;
  for (const auto &argument : impl_->command) {
    if (!command_line.empty()) {
      command_line.push_back(L' ');
    }
    command_line += quote(argument);
  }
  STARTUPINFOW startup{};
  startup.cb = sizeof(startup);
  startup.dwFlags = STARTF_USESTDHANDLES;
  startup.hStdInput = child_input_read;
  startup.hStdOutput = child_output_write;
  startup.hStdError = GetStdHandle(STD_ERROR_HANDLE);
  const BOOL created = CreateProcessW(nullptr, command_line.data(), nullptr, nullptr, TRUE,
                                      CREATE_NO_WINDOW, nullptr, nullptr, &startup, &impl_->process);
  CloseHandle(child_input_read);
  CloseHandle(child_output_write);
  if (!created) {
    CloseHandle(impl_->input_write);
    CloseHandle(impl_->output_read);
    impl_->input_write = impl_->output_read = INVALID_HANDLE_VALUE;
    throw std::runtime_error("failed to start provider process");
  }
#else
  if (impl_->pid > 0 && kill(impl_->pid, 0) == 0) {
    return;
  }
  int input_pipe[2]{};
  int output_pipe[2]{};
  if (pipe(input_pipe) != 0 || pipe(output_pipe) != 0) {
    throw std::runtime_error("failed to create provider pipes");
  }
  const pid_t child = fork();
  if (child == 0) {
    dup2(input_pipe[0], STDIN_FILENO);
    dup2(output_pipe[1], STDOUT_FILENO);
    close(input_pipe[0]);
    close(input_pipe[1]);
    close(output_pipe[0]);
    close(output_pipe[1]);
    std::vector<char *> arguments;
    for (auto &argument : impl_->command) {
      arguments.push_back(argument.data());
    }
    arguments.push_back(nullptr);
    execvp(arguments[0], arguments.data());
    _exit(127);
  }
  if (child < 0) {
    throw std::runtime_error("failed to fork provider process");
  }
  close(input_pipe[0]);
  close(output_pipe[1]);
  impl_->pid = child;
  impl_->input_write = input_pipe[1];
  impl_->output_read = output_pipe[0];
#endif
}

void Process::stop() {
  if (impl_ == nullptr) {
    return;
  }
  std::scoped_lock state(impl_->state_mutex);
#ifdef _WIN32
  if (impl_->input_write != INVALID_HANDLE_VALUE) {
    CloseHandle(impl_->input_write);
    impl_->input_write = INVALID_HANDLE_VALUE;
  }
  if (impl_->output_read != INVALID_HANDLE_VALUE) {
    CloseHandle(impl_->output_read);
    impl_->output_read = INVALID_HANDLE_VALUE;
  }
  if (impl_->process.hProcess != nullptr) {
    if (WaitForSingleObject(impl_->process.hProcess, 1000) == WAIT_TIMEOUT) {
      TerminateProcess(impl_->process.hProcess, 1);
      WaitForSingleObject(impl_->process.hProcess, 1000);
    }
    CloseHandle(impl_->process.hThread);
    CloseHandle(impl_->process.hProcess);
    impl_->process = {};
  }
#else
  if (impl_->input_write >= 0) {
    close(impl_->input_write);
    impl_->input_write = -1;
  }
  if (impl_->output_read >= 0) {
    close(impl_->output_read);
    impl_->output_read = -1;
  }
  if (impl_->pid > 0) {
    int status{};
    if (waitpid(impl_->pid, &status, WNOHANG) == 0) {
      kill(impl_->pid, SIGTERM);
      waitpid(impl_->pid, &status, 0);
    }
    impl_->pid = -1;
  }
#endif
}

void Process::restart() {
  stop();
  start();
}

bool Process::running() const {
  std::scoped_lock state(impl_->state_mutex);
#ifdef _WIN32
  if (impl_->process.hProcess == nullptr) {
    return false;
  }
  DWORD code{};
  return GetExitCodeProcess(impl_->process.hProcess, &code) && code == STILL_ACTIVE;
#else
  return impl_->pid > 0 && kill(impl_->pid, 0) == 0;
#endif
}

std::string Process::request(std::string_view json_line) {
  if (!running()) {
    throw std::runtime_error("provider process is not running");
  }
  const std::string framed = std::string(json_line) + "\n";
  {
    std::scoped_lock write(impl_->write_mutex);
#ifdef _WIN32
    DWORD written{};
    if (!WriteFile(impl_->input_write, framed.data(), static_cast<DWORD>(framed.size()), &written,
                   nullptr) || written != framed.size()) {
      throw std::runtime_error("provider write failed");
    }
#else
    std::size_t offset = 0;
    while (offset < framed.size()) {
      const auto count = write(impl_->input_write, framed.data() + offset, framed.size() - offset);
      if (count <= 0) {
        throw std::runtime_error("provider write failed");
      }
      offset += static_cast<std::size_t>(count);
    }
#endif
  }
  std::scoped_lock read_lock(impl_->read_mutex);
  std::string response;
  char character{};
  while (character != '\n') {
#ifdef _WIN32
    DWORD count{};
    if (!ReadFile(impl_->output_read, &character, 1, &count, nullptr) || count == 0) {
      throw std::runtime_error("provider closed output");
    }
#else
    if (read(impl_->output_read, &character, 1) != 1) {
      throw std::runtime_error("provider closed output");
    }
#endif
    if (character != '\n') {
      response.push_back(character);
    }
  }
  return response;
}

void Process::cancel(std::string_view request_id) {
  const std::string line = cancellation(request_id) + "\n";
  std::scoped_lock write(impl_->write_mutex);
#ifdef _WIN32
  DWORD written{};
  if (!WriteFile(impl_->input_write, line.data(), static_cast<DWORD>(line.size()), &written,
                 nullptr)) {
    throw std::runtime_error("provider cancellation write failed");
  }
#else
  if (write(impl_->input_write, line.data(), line.size()) < 0) {
    throw std::runtime_error("provider cancellation write failed");
  }
#endif
}

} // namespace pma::providers
