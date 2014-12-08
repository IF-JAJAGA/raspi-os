#ifndef SCHED_H
#define SCHED_H

#include <stdlib.h>
#include <stdint.h>

typedef void (*func_t) (void *);

enum state_e {STATE_NEW, STATE_EXECUTING, STATE_PAUSED, STATE_ZOMBIE};

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
};

// GLOBAL
const unsigned int WORD_SIZE;
const unsigned int NUMBER_REGISTERS;

struct pcb_s *
create_process(func_t f, void *args, unsigned int stack_size_words, unsigned int priority);

void
ctx_switch();

void
ctx_switch_from_irq();

void
start_sched();

void
end_sched();

#endif

