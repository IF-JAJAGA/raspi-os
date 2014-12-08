#include "syscall.h"
#include "sched.h"

void __attribute__ ((naked)) sys_reboot(){
	//Ecrit dans r0 le numéro de l'appel système sys_reboot, i.e. 1
	__asm("mov r0, %0" : : "r"(1) : "r0");
	//Déclenche une interruption logicielle
	__asm("SWI 0" : : : "lr");
}

void __attribute__ ((naked)) sys_wait(unsigned int nbQuantums){
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
		case 2 : doSysCallWait();
					break;
	}
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
	unsigned int nbQuantums=0;
	//Récupérer le nombre de quantums à attendre
	__asm("mov %0, r1" : "=r"(nbQuantums));
	//Mise en pause du current process
	current_process->state = PAUSED;
	current_process->qtCount = nbQuantums;
}
