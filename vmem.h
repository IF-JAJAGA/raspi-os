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

uint8_t *
vmem_alloc(unsigned int nbPages);

void
vmem_free(uint8_t *to_free, unsigned int nbPages);

#endif

