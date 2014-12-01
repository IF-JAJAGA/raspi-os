/**
 * Fonction qui déclenche une interruption logicielle
 * qui reboot le matériel et le noyau de notre mini-OS
 */
void sys_reboot();

/**
 * Le processus qui effectue cet appel attend nbQuantums 
 * avant d’être à nouveau éligible par l’ordonnanceur.
 */
void sys_wait(unsigned int nbQuantums);


/**
 * Handler des appels système
 */
void SWIHandler();
 
 /**
  * Fonction qui reboot le système
  */
void doSysCallReboot();
