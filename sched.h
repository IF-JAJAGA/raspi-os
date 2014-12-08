#ifndef SCHED_H
#define SCHED_H

#include <stdlib.h>
#include <stdint.h>

typedef void (*func_t) (void *);

enum state_e {STATE_NEW, STATE_EXECUTING, STATE_WAITING, STATE_PAUSED, STATE_ZOMBIE};

extern struct pcb_s *current_ps;

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
	
	//compteur de quantum avant réveil si process en PAUSE
	unsigned int qtCount;
};

// GLOBAL
const unsigned int WORD_SIZE;
const unsigned int NUMBER_REGISTERS;

void
create_process(func_t f, void *args, unsigned int stack_size_words);

void
ctx_switch();

void
ctx_switch_from_irq();

void
start_sched();

void
end_sched();

#endif

