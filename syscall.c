#include "syscall.h"

void PUT32 ( unsigned int, unsigned int );

extern struct pcb_s *current_ps;

void sys_reboot(){
	DISABLE_IRQ();
	
	//Ecrit dans r0 le numéro de l'appel système sys_reboot, i.e. 1
	__asm("mov r0, %0" : : "r"(SYS_REBOOT) : "r0");
	//Déclenche une interruption logicielle
	__asm("SWI 0" : : : "lr");
}

void __attribute__((naked)) sys_wait(unsigned int nbQuantums){
	DISABLE_IRQ();
	
	uint32_t sp;
	func_t lr;
	
	//Sauvegarde des registres
	__asm("srsdb sp!, #0x13");
	__asm("push {r0-r12,lr}");
	
	__asm("mov %0, sp" :"=r"(sp));
	__asm("mov %0, lr" :"=r"(lr));
	
	store_sp_and_lr(sp, lr);
	
	//Ecrit l'adresse de retour dans r2
	__asm("mov r2, lr");
	//Ecrit dans r1 nbQuantums
	__asm("mov r1, %0" : : "r"(nbQuantums) : "r1");
	//Ecrit dans r0 le numéro de l'appel système sys_wait, i.e. 2
	__asm("mov r0, %0" : : "r"(SYS_WAIT) : "r0");
	//Déclenche une interruption logicielle
	__asm("SWI 0" : : : "lr");
}

void __attribute__ ((naked)) SWIHandler(){
	int numAppelSys;
	int nbQuantums;
	//Récupération du numéro de l'appel système
	__asm("mov %0, r0" : "=r"(numAppelSys));
	
	__asm("cps #0x13");
	
	
	if (numAppelSys == SYS_REBOOT){
		doSysCallReboot();
	}
	else if (numAppelSys == SYS_WAIT){
		//doSysCallWait();
		//Récupérer le nombre de quantums à attendre
		__asm("mov %0, r1" : "=r"(nbQuantums));
		ctx_switch_from_handler();
	}
	ENABLE_IRQ();
}

void __attribute__ ((naked)) doSysCallReboot(){
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
	
	__asm("push {r0-r12,lr}");
	set_current_paused(nbQuantums);
	__asm("pop {r0-r12,lr}");
	
	//__asm("push {r0-r12,lr}");
	ctx_switch_from_handler();
	//__asm("pop {r0-r12,lr}");
	
	__asm("bx r2");
}
