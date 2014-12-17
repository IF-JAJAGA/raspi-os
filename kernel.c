#include <stdlib.h>

#include "hw.h"
#include "sched.h"

// Stack size in words (divide by WORD_SIZE if necessary)
const unsigned int STACK_SIZE_WORDS = 16384; // 4kB
const unsigned int TOTAL_NB_PS = 5;

void
funcA(void *a)
{
	long cptA = 1;

	while (cptA < 10) {
		cptA += 2;
	}
	cptA = 0;
}

void
funcB(void *a)
{
	int cptB = 1;

	while (cptB < 20) {
		cptB += 2;
	}
}

void
funcC(void *a)
{
	// We create two more processes
	create_process(funcA, a, STACK_SIZE_WORDS, 15);
	create_process(funcA, a, STACK_SIZE_WORDS, 4);
}


//------------------------------------------------------------------------

int
kmain ( void )
{
	// Initialize hardware
	init_hw();

	// Initialize all ctx
	create_process(funcA, NULL, STACK_SIZE_WORDS, 4);
	create_process(funcB, NULL, STACK_SIZE_WORDS, 5);
	create_process(funcC, NULL, STACK_SIZE_WORDS, 10);

	start_sched(STACK_SIZE_WORDS);

	return 0;
}

