#include <stdlib.h>

#include "hw.h"
#include "sched.h"
#include "syscall.h"
#include "fb.h"
#include "vmem.h"

const unsigned int TOTAL_NB_PS = 5;

void
funcA(void *a)
{
	long cptA = 1;

	while (cptA < 10) {
		cptA += 2;
	}
	cptA = 0;
	drawRed();
}

void
funcB(void *a)
{
	int cptB = 1;

	while (cptB < 20) {
		cptB += 2;
	}
	drawBlue();
}

void
funcC(void *a)
{
	// We create two more processes
	create_process(funcA, a, STACK_SIZE_WORDS, 15);
	create_process(funcA, a, STACK_SIZE_WORDS, 4);
	drawYellow();
}

void
funcWait(void *a)
{
	int b;
	// Call wait
	sys_wait(2);
	b = 42;
}

void
funcReboot(void *a)
{
	sys_reboot();
}

//------------------------------------------------------------------------

typedef enum bool_e {false, true} bool;

bool debug_ok = true;
int
kmain ( void )
{
	// Initialize hardware
	init_hw();
	init_kern_translation_table();

	for (unsigned int i = 0; i < 0x500000; ++i) {
		if (i != translate(i)) {
			debug_ok = false;
		}
	}
	for (unsigned int i = 0x500000; i < 0x20000000; ++i) {
		if (0 != translate(i)) {
			debug_ok = false;
		}
	}
	for (unsigned int i = 0x20000000; i < 0x20FFFFFF; ++i) {
		if (i != translate(i)) {
			debug_ok = false;
		}
	}
	for (unsigned int i = 0x20FFFFFF; i <= 0xFFFFFFFF; ++i) {
		if (0 != translate(i)) {
			debug_ok = false;
		}
	}

	debug_ok = false;

	// Initialize all ctx
	create_process(funcA, NULL, STACK_SIZE_WORDS, 4);
	create_process(funcB, NULL, STACK_SIZE_WORDS, 5);
	create_process(funcC, NULL, STACK_SIZE_WORDS, 10);
	// create_process(funcWait, NULL, STACK_SIZE_WORDS, 3);
	// create_process(funcReboot, NULL, STACK_SIZE_WORDS, 3);

	start_sched(STACK_SIZE_WORDS, nothing, NULL);

	return 0;
}

