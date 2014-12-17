#ifndef VMEM_H
#define VMEM_H

#include "constants.h"

// Functions
// ---------

unsigned int
init_kern_translation_table (void);

void
start_mmu_C();

void
configure_mmu_C();

unsigned int
translate(unsigned int va);

#endif

