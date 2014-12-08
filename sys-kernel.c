#include "sched.h"
#include "hw.h"
#include "syscall.h"

#include <stdlib.h> 

#define STACK_SIZE 1024


void funcA()
{
	int cptA = 0;
	while ( 1 ) {
		cptA ++;
		sys_wait(2);
	}
}
void funcB()
{
	int cptB = 1;
	while ( 1 ) {
		cptB += 2 ;
	}
}
//------------------------------------------------------------------------
int kmain ( void )
{
	init_hw();
	create_process(funcB, NULL, STACK_SIZE);
	create_process(funcA, NULL, STACK_SIZE);
	start_sched();
	while(1);
	//sys_reboot();
	/* Pas atteignable vu nos 2 fonctions */
	return 0;
}
