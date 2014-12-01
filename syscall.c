#include "syscall.h"
#include "hw.h"
#include <stdio.h>

void __attribute__ ((naked)) sys_reboot(){
	//Ecrit dans r0 le numéro de l'appel système sys_reboot, i.e. 1
	__asm("mov r0, %0" : : "r"(1) : "r0");
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
		case 2 : //"Pas encore implémenté\n";
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
