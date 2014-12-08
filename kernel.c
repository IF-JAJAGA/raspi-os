#include <stdlib.h>

#include "hw.h"
#include "sched.h"

// Stack size in words (divide by WORD_SIZE if necessary)
const unsigned int STACK_SIZE_WORDS = 16384; // 4kB

struct done_s {
	int max;
	int curr;
};

void
funcA(void *a)
{
	struct done_s *d = (struct done_s *) a;
	long cptA = 1;

	for (;cptA > 0;) {
		cptA += 32;
	}
	cptA = 0;
	d->curr++;
}

void
funcB(void *a)
{
	struct done_s *d = (struct done_s *) a;
	int cptB = 1;

	for (;cptB < 2147483647;) {
		cptB += 2;
	}
	d->curr++;
}

//------------------------------------------------------------------------

int
kmain ( void )
{
	// Initialize hardware
	init_hw();

	struct done_s done;
	done.max = 2;
	done.curr = 0;

	// Initialize all ctx
	create_process(funcA, &done, STACK_SIZE_WORDS, 1);
	create_process(funcB, &done, STACK_SIZE_WORDS, 2);

	start_sched(STACK_SIZE_WORDS);

	while (done.curr < done.max) {}

	char a[] = "Youpi!!";

	return 0;
}

