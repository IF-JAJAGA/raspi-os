#include <stdlib.h>

#include "hw.h"
#include "sched.h"

// Stack size in words (divide by WORD_SIZE if necessary)
const unsigned int STACK_SIZE_WORDS = 16384; // 4kB
const unsigned int TOTAL_NB_PS = 5;

void
funcA(void *a)
{
	int *nb_done = (int*) a;
	long cptA = 1;

	while (cptA < 10) {
		cptA += 2;
	}
	cptA = 0;
	++(*nb_done);
}

void
funcB(void *a)
{
	int *nb_done = (int*) a;
	int cptB = 1;

	while (cptB < 20) {
		cptB += 2;
	}
	++(*nb_done);
}

void
funcC(void *a)
{
	int *nb_done = (int*) a;

	// We create two more processes
	create_process(funcA, a, STACK_SIZE_WORDS, 15);
	create_process(funcA, a, STACK_SIZE_WORDS, 4);

	++(*nb_done);
}
//------------------------------------------------------------------------

int
kmain ( void )
{
	// Initialize hardware
	init_hw();

	int nb_done = 0;

	// Initialize all ctx
	create_process(funcA, &nb_done, STACK_SIZE_WORDS, 4);
	create_process(funcB, &nb_done, STACK_SIZE_WORDS, 5);
	create_process(funcC, &nb_done, STACK_SIZE_WORDS, 10);

	start_sched(STACK_SIZE_WORDS);

	while (nb_done < TOTAL_NB_PS) {}

	char a[] = "Youpi!!";

	return 0;
}

