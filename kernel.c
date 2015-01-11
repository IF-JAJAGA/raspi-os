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

void alloc_test(void *unused)
{
	uint8_t *p = vmem_alloc(0);
	uint8_t *p2;

	p = vmem_alloc(1);
	*p = 0x42;
	vmem_free(p, 1);
	p = vmem_alloc(12);
	p2 = vmem_alloc(2);
	vmem_free(p, 12);
	vmem_free(p2, 2);
}

//------------------------------------------------------------------------

int
kmain ( void )
{
	// Initialize hardware
	init_hw();
	configure_mmu_C();
	init_kern_translation_table();
	FramebufferInitialize();

	// Normally works, commented out just to make sure you have no problem:
//	start_mmu_C();

	// Initialize all ctx
	create_process(funcA, NULL, STACK_SIZE_WORDS, 4);
	create_process(funcB, NULL, STACK_SIZE_WORDS, 5);
	create_process(funcC, NULL, STACK_SIZE_WORDS, 10);
	// create_process(funcWait, NULL, STACK_SIZE_WORDS, 3);
	// create_process(funcReboot, NULL, STACK_SIZE_WORDS, 3);

	drawBlue();
	//start_sched(STACK_SIZE_WORDS, nothing, NULL);
	struct Point p1, p2;
	p1.x=20;
	p1.y=20;
	p2.x=60;
	p2.y=20;
	//drawLine(p1, p2);

	for(uint8 t = 1; t<10 ; t++){
		drawCharacter(t);
	}

	return 0;
}

