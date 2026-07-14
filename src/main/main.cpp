#include "pma-internal/maintenance.hpp"
#include "pma-internal/service.hpp"

int main(int argc, char **argv) {
  if (argc > 1) return pma::maintenance::run(argc, argv);
  return pma::service::run_stdio();
}
