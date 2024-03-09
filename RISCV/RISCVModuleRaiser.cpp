 
#include "RISCVModuleRaiser.h"

void registerRISCVModuleRaiser() {
  // NOTE: raising of RISCV32 binaries not yet supported
  // registerRISCV32ModuleRaiser();
  registerRISCV64ModuleRaiser();
}
