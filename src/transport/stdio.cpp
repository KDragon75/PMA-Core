#include "pma-internal/service.hpp"

#include <exception>
#include <iostream>
#include <string>

namespace pma::service {

int run_stdio() {
  Dispatcher dispatcher;
  std::string line;
  try {
    while (!dispatcher.shutdown_requested() && std::getline(std::cin, line)) {
      const auto response = dispatcher.handle_line(line);
      if (response.has_value()) {
        std::cout << *response << '\n' << std::flush;
      }
    }
    return 0;
  } catch (const std::exception &error) {
    std::cerr << "PMA stdio host failed: " << error.what() << '\n';
    return 1;
  } catch (...) {
    std::cerr << "PMA stdio host failed with an unknown internal error\n";
    return 1;
  }
}

} // namespace pma::service
