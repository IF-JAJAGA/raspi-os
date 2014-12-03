#include "hw.h"

// STACK_SIZE doit être suffisament grand pour contenir les éléments de contexte,
// les éventuels paramètres des différentes fonctions ainsi que le code lui-même
// Puisqu'on y trouve des boucles infinies, on choisit une variable assez grande
// pour contenir un bon nombre d'informations et pas trop grande pour pouvoir tourner
// sur le Raspberry Pi.
#define STACK_SIZE 1024

#define NULL 0

typedef void (*func_t) (void*);
typedef enum {NEW, READY, RUNNING, WAITING, TERMINATED} processState;

// structure concernant le PCB
struct pcb_s {
	// id du processus concerné
	int pid;
	// état du processus concerné
	processState state;
	// contexte lié au processus concerné
	// pointeur de pile
	unsigned int sp;
	// adresse de l'instruction en cours
	unsigned int lr;
	//fonction associée au contexte
	func_t f_ctx;
	//ainsi que ses arguments
	void* f_args;
	//pointeur vers le prochain pcb
	struct pcb_s* nextProcess;
	//Priorite associee au processus
	unsigned int priority;
};

//pointeur sur la tête de la liste
struct pcb_s* first_process;
//pointeur sur le processus courant
struct pcb_s* current_process;
//pointeur sur le processus idle
struct pcb_s* process_idle;

void init_pcb (struct pcb_s* pcb, void* args, func_t f, unsigned int stack_size);

void destroy_pcb (struct pcb_s* pcb);

void create_process (func_t f, void* args, unsigned int stack_size );

void start_current_process();

void start_sched();

void __attribute__ ((naked)) ctx_switch_from_irq();

//void idle();
