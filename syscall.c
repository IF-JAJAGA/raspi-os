#include "hw.h"
#include "syscall.h"
#include "sched.h"

void PUT32 ( unsigned int, unsigned int );

void sys_reboot(){
	DISABLE_IRQ();
	
	//Ecrit dans r0 le numéro de l'appel système sys_reboot, i.e. 1
	__asm("mov r0, %0" : : "r"(1) : "r0");
	//Déclenche une interruption logicielle
	__asm("SWI 0" : : : "lr");
}

void sys_wait(unsigned int nbQuantums){
	DISABLE_IRQ();
	
	//Ecrit l'adresse de retour dans r2
	__asm("mov r2, lr");
	//Ecrit dans r1 nbQuantums
	__asm("mov r1, %0" : : "r"(nbQuantums) : "r1");
	//Ecrit dans r0 le numéro de l'appel système sys_wait, i.e. 2
	__asm("mov r0, %0" : : "r"(2) : "r0");
	//Déclenche une interruption logicielle
	__asm("SWI 0" : : : "lr");
}

void __attribute__ ((naked)) SWIHandler(){
	int numAppelSys;
	//Récupération du numéro de l'appel système
	__asm("mov %0, r0" : "=r"(numAppelSys));
	switch(numAppelSys){
		case 1 : doSysCallReboot();
					break;
		case 2 : 
			doSysCallWait();
			break;
	}
	ENABLE_IRQ();
}

void __attribute__ ((naked)) doSysCallReboot(){
	const int PM_RSTC = 0x2010001c;
	const int PM_WDOG = 0x20100024;
	const int PM_PASSWORD = 0x5a000000;
	const int PM_RSTC_WRCFG_FULL_RESET = 0x00000020;
	
	PUT32(PM_WDOG, PM_PASSWORD | 1);
	PUT32(PM_RSTC, PM_PASSWORD | PM_RSTC_WRCFG_FULL_RESET);
	while(1);
}

void __attribute__ ((naked)) doSysCallWait(){
	unsigned int nbQuantums = 0;
	//func_t instruction = NULL; 
	//Récupérer le nombre de quantums à attendre
	__asm("mov %0, r1" : "=r"(nbQuantums));
	//Récupère l'instruction à exécuter
	//__asm("mov %0, r2" : "= r" (instruction));	
	//Empilement de l'adresse de retour
	
	__asm("push {r2,lr}");
	set_current_paused(nbQuantums);
	__asm("pop {r2,lr}");
	
	ctx_switch();
	
	__asm("bx r2");
}
