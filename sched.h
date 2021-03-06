#ifndef SCHED_H
#define SCHED_H

#include <stdlib.h>
#include <stdint.h>
#include "constants.h"
#include "hw.h"
#include "phyAlloc.h"

typedef void (*func_t) (void *);

enum state_e {STATE_NEW, STATE_EXECUTING, STATE_WAITING, STATE_PAUSED, STATE_ZOMBIE};

struct pcb_s {
	// Stored in a circular doubly linked list
	struct pcb_s *previous;
	struct pcb_s *next;

	unsigned int  pid;
	enum state_e  state;

	uint32_t      *stack;

	// The stack size (in words)
	unsigned int  stack_size_words;

	// First instruction to execute (when started)
	func_t        entry_point;
	// Pointer to the current instruction (lr register)
	func_t        instruction;
	void         *args;

	// Priority assigned to the process 0 is lowest, NB_PRIORITY - 1 highest
	unsigned int priority;
	
	// Quantum counter before waking when ps is STATE_PAUSED
	unsigned int qt_count;
};

void
create_process(func_t f, void *args, unsigned int stack_size_words, unsigned int priority);
/*
void
set_current_paused(unsigned int qt_count, func_t instr);
*/

void
set_current_paused(unsigned int qt_count);

void
store_sp_and_lr(uint32_t sp, func_t lr);

void
ctx_switch();

void
ctx_switch_from_handler();

void
ctx_switch_from_irq();

void
infinite_switching(void *unused);

void
nothing(void *unused);

void
start_sched(unsigned int stack_size_words, func_t idle_handler, void *args);

#endif

