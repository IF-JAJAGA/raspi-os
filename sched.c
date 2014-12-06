#include "sched.h"
#include "phyAlloc.h"

#define REGISTERS_NUMBER 13
#define NB_PRIORITY 256

static int pid_gb = 0;
static struct pcb_s * priority_array[NB_PRIORITY] = {NULL};

void init_pcb (struct pcb_s* pcb, void* args, func_t f, unsigned int stack_size, unsigned int priority) {
	pcb->state = NEW;
	pcb->pid = pid_gb;
	pid_gb++;

	pcb->sp = (unsigned int) phyAlloc_alloc ( stack_size );
	//Pour l'allocation de mémoire pour SP, le pointeur se retrouve tout en bas
	//de la pile. On se déplace donc en haut en ajoutant la taille de la pile
	//puis on descend d'un cran (taille d'un int) puis on laisse la place pour
	//l'ensemble des registres (REGISTERS_NUMBER)
	pcb->sp += stack_size - (1+REGISTERS_NUMBER)*sizeof(int);

	pcb->f_ctx = f;
	pcb->lr = (unsigned int) f;
	pcb->f_args = args;
	pcb->priority = priority;

	//Ajout du process dans la liste des priorités
	if(priority_array[NB_PRIORITY-1-priority]==NULL){
		priority_array[NB_PRIORITY-1-priority] = pcb;
	}
	else{
		//On se rajoute à la liste de processus de même priorité.
		struct pcb_s *pcb_same_prio_first = priority_array[NB_PRIORITY-1-priority];
		struct pcb_s *pcb_same_prio = pcb_same_prio_first;
		while(pcb_same_prio->nextProcess != pcb_same_prio_first){
			pcb_same_prio = pcb_same_prio -> nextProcess;
		}
		//On doit être à la fin de la liste.
		pcb -> nextProcess = pcb_same_prio_first;
		pcb_same_prio -> nextProcess = pcb; 
	}
}

void destroy_pcb (struct pcb_s* pcb){
	//On s'enlève de la liste des priorités.
	//Si on est le seul avec notre priorité, on met la case à NULL.
	if(pcb -> nextProcess == pcb){
		priority_array[NB_PRIORITY - 1 - pcb->priority] = NULL;
	}
	else{
		struct pcb_s *pcb_same_prio = priority_array[NB_PRIORITY - 1 - pcb->priority];
		while(pcb_same_prio -> nextProcess != pcb){
			pcb_same_prio = pcb_same_prio -> nextProcess;		
		}
		//On se situe juste avant le processus à supprimer.
		pcb_same_prio -> nextProcess = pcb -> nextProcess;
	}

	//libération de l'espace alloué pour le processus supprimé
	phyAlloc_free ((void*)pcb -> sp, sizeof (STACK_SIZE));
	phyAlloc_free (pcb, sizeof(struct pcb_s));
}

void create_process (func_t f, void* args, unsigned int stack_size, unsigned int priority){
	struct pcb_s* pcb = (struct pcb_s *) phyAlloc_alloc(sizeof(struct pcb_s));
	init_pcb(pcb, args, f, stack_size, priority);

	/*	Remplacé par la liste de priorité et la fonction elect.
	//Ajout du nouveau processus à la suite de la tête de liste
	if ( first_process != NULL ){
		//orienter le pointeur nextProcess du nouveau processus vers le
		//processus que pointait first_process
		pcb->nextProcess = first_process->nextProcess;
		//ré-orienter pointeur nextProcess du first_process vers le processus
		//nouvellement créé
		first_process->nextProcess = pcb;
	}
	else{
		//Création du first_process qui boucle sur lui-même vu qu'il est tout seul
		pcb->nextProcess = pcb;
		first_process = pcb;
	}
	*/
}

void start_current_process() {
	//Lancement initial de la fonction
	current_process->state = RUNNING;
	current_process->f_ctx(current_process->f_args);
	current_process->state = TERMINATED;
}

void elect (){
	//Parcours dans la liste de priorité. Que l'on a trouvé une case non vide, on élit un process
	//parmi la liste chainée.
	//TODO : Question d'utiliser current_process si pas null ?
	struct pcb_s *process_prio = NULL;
	int i = 0;
	while(process_prio == NULL){
		if(priority_array[i] != NULL){
			process_prio = priority_array[i];
		}
		else {
			i++;
		}
	}
	
	//TODO : code à adapter selon la liste de priorité
	while(current_process -> nextProcess -> state == TERMINATED){
		struct pcb_s* dead_process = current_process -> nextProcess;

		if (dead_process == first_process){
			//Réhabilitation du first_process dans le cas où celui-ci doit être supprimé
			first_process = first_process->nextProcess;
		}

		if (dead_process == current_process && current_process->state == TERMINATED){
			//Retour sur process_idle lorsqu'il ne reste plus qu'un seul processus et que celui-ci est à l'état TERMINATED
			current_process = process_idle;
			current_process -> nextProcess = current_process;
		}
		else{
			//ré-orienter pointeur nextProcess de current_processus vers
			//le processus que pointait le pointeur nextProcess du processus
			//à détruire
			current_process -> nextProcess = dead_process -> nextProcess;
		}
		destroy_pcb (dead_process);

	}
	current_process->state = WAITING;
	current_process = current_process -> nextProcess;
	current_process->state = RUNNING;
}

void start_sched(){
	//Création du process_idle qui permet l'initialisation de la chaîne de processus
	process_idle = phyAlloc_alloc(sizeof(struct pcb_s));
	// Idle ne se sert pas de sa priorite: il n'est pas ordonne de la meme facon que les autres
	init_pcb(process_idle, NULL, NULL, STACK_SIZE, 0);
	process_idle->nextProcess = first_process;
	current_process = process_idle->nextProcess;

	//Activation du timer de l'interruption
	ENABLE_IRQ();
}

void ctx_switch_from_irq(){
	//Désactivation du timer de l'interruption
	DISABLE_IRQ();

	//Déplace le lr modifié par l'interruption pour pouvoir lancé notre ancien
	//ctx_switch()
	__asm("sub lr, lr, #4");
	__asm("srsdb sp!, #0x13");
	__asm("cps #0x13");

	//Sauvegarde le contexte du processus en cours d'exécution
	//Sauvegarder les variables locales en pushant tous les registres
	__asm("push {R0-R12}");

	if (current_process->state == RUNNING){
		//Sauvegarder le contexte courant
		__asm ("mov %0, sp" : "=r"(current_process->sp));
		__asm ("mov %0, lr" : "=r"(current_process->lr));
	}

	//Demande au scheduler d'élire un nouveau processus
	elect();

	//Restaurer le nouveau contexte
	__asm ("mov sp , %0" : : "r"(current_process->sp));
	__asm ("mov lr , %0" : : "r"(current_process->lr));

	//Restituer toutes les variables globales contenues dans les registres
	__asm("pop {R0-R12}");

	//réactivation du timer de l'interruption
	ENABLE_IRQ();

	//Lance la fonction associé au processus current_process
	//Ou reprend là où on s'est arrêté
	if (current_process->state == NEW){
		start_current_process();
	}

	//Opération inverse de srsdb (decrement before), rfeia (increment after)
	//équivalent au précédent bx lr mais avec le sp sauvegardé au cours de
	//l'interruption
	__asm("rfeia sp!");
}
