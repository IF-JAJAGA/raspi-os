#include "sched.h"
#include "hw.h"
#include "phyAlloc.h"

// GLOBAL
#define NB_PRIORITY 256

const unsigned int WORD_SIZE = 4; // ARM word size (32 bits)
const unsigned int NUMBER_REGISTERS = 14; // 13 all purpose registers (r0-r12) + lr

// STATIC
static struct pcb_s * priority_array[NB_PRIORITY] = {NULL};

static const unsigned int CPSR_SVC_NO_IRQ = 0x53; // Supervisor mode (with no interrupts)
static const unsigned int CPSR_SVC_IRQ = 0x13; // Supervisor mode (with interrupts)

// Initialized to NULL (when not allocated)
static struct pcb_s *current_ps = NULL;
static struct pcb_s *init_ps    = NULL;

static unsigned int process_count = 0;

static void
start_current_process()
{
	current_ps->state = STATE_EXECUTING;

	current_ps->entry_point(current_ps->args);

	current_ps->state = STATE_ZOMBIE;
}

static struct pcb_s *
init_pcb(func_t f, void *args, unsigned int stack_size_words, unsigned int priority)
{
	struct pcb_s *pcb = (struct pcb_s *) phyAlloc_alloc(sizeof(struct pcb_s));

	pcb->pid = process_count++;
	pcb->state = STATE_NEW;
	pcb->stack_size_words = stack_size_words;

	// The stack base points to the lowest address (last octet cell of the stack)
	uint8_t *stack_base = (uint8_t *) phyAlloc_alloc(stack_size_words * WORD_SIZE);

	// Positioning the pointer to the first (word) cell of the stack (highest address)
	stack_base += stack_size_words * WORD_SIZE - WORD_SIZE;
	pcb->stack = (uint32_t *) stack_base;

	// Initializing the first cell of the stack to the supervisor execution mode
	*pcb->stack = CPSR_SVC_IRQ; // with interrupts enabled

	// As the process is STATE_NEW, pushing the address of the code launching new processes
	--(pcb->stack);
	*pcb->stack = (uint32_t) &start_current_process;

	// Leaving space for the registers
	pcb->stack -= NUMBER_REGISTERS;

	pcb->entry_point = f;
	pcb->instruction = NULL;
	pcb->args = args;
	pcb->priority = priority;

	return pcb;
}

static void
remove_priority(struct pcb_s *zombie)
{
	//On s'enlève de la liste des priorités.
	//Si on est le seul avec notre priorité, on met la case à NULL.
	if(zombie -> next == zombie){
		priority_array[NB_PRIORITY - 1 - zombie->priority] = NULL;
	}
	else{
		struct pcb_s *pcb_same_prio = priority_array[NB_PRIORITY - 1 - zombie->priority];
		while(pcb_same_prio -> next != zombie){
			pcb_same_prio = pcb_same_prio -> next;
		}
		//On se situe juste avant le processus à supprimer.
		pcb_same_prio -> next = zombie -> next;
	}
}

static void
free_process(struct pcb_s *zombie)
{
	// Removes the process from the priority array
	remove_priority(zombie);

	// Deallocating
	phyAlloc_free(zombie->stack, zombie->stack_size_words);
	phyAlloc_free(zombie, sizeof(zombie));
}

static struct pcb_s *
highest_priority(unsigned int i)
{
	//Parcours dans la liste de priorité. Dès que l'on a trouvé une case non vide, on élit un process
	//parmi la liste chainée.
	//TODO : Question d'utiliser current_process si pas null ?
	struct pcb_s *process_prio = NULL;
	while(process_prio == NULL){
		if(priority_array[i] != NULL){
			process_prio = priority_array[i];
		}
		else {
			i++;
		}
	}
	return process_prio;
}

/**
 * Chooses the next process alive (not STATE_ZOMBIE) in the linked list given.
 * If there is no next process alive (all STATE_ZOMBIE), it returns NULL
 */
static struct pcb_s *
next_alive(struct pcb_s *first_pcb)
{
	struct pcb_s *iterator_ps = first_pcb->next;
	if (STATE_ZOMBIE == iterator_ps->state) {
		while (STATE_ZOMBIE == iterator_ps->state && iterator_ps != first_pcb) {
			// Deleting current_ps from the list
			current_ps->previous->next = current_ps->next;
			current_ps->next->previous = current_ps->previous;

			struct pcb_s *next = iterator_ps->next;

			// Deallocating the memory of the ZOMBIE
			free_process(iterator_ps);

			// Switching to the next element
			iterator_ps = next;
		}

		if (STATE_ZOMBIE == iterator_ps->state) {
			// There are no process that wants to execute anything
			// TOO MANY ZOMBIES!!
			return NULL;
		}
	}
	return iterator_ps;
}

static void
elect()
{
	current_ps->state = STATE_PAUSED;
	struct pcb_s *previous_ps = current_ps;

	do {
		current_ps = highest_priority(0);
		current_ps = next_alive(current_ps->previous);
	} while (current_ps == NULL);

	// Should the highest priority process be the same as before
	if (previous_ps == current_ps) {
		// We elect the next (non zombie) element in the circular list
		// Note: it cannot be NULL as there is at least one alive (see above)
		current_ps = next_alive(current_ps);
	}

	current_ps->state = STATE_EXECUTING;
}

//------------------------------------------------------------------------

struct pcb_s *
create_process(func_t f, void *args, unsigned int stack_size_words, unsigned int priority)
{
	DISABLE_IRQ();

	struct pcb_s *new_ps = init_pcb(f, args, stack_size_words, priority);

	struct pcb_s *pcb_first = priority_array[NB_PRIORITY - 1 - new_ps->priority];
	// Adding process to priority array
	if (pcb_first == NULL) {
		priority_array[NB_PRIORITY - 1 - new_ps->priority] = new_ps;
		new_ps->next = new_ps;
		new_ps->previous = new_ps;
	}
	else {
		// If there is already someone, we insert ourself to the list
		// Inserting BEFORE current_ps
		new_ps->previous = current_ps->previous;
		new_ps->next = current_ps;
		current_ps->previous->next = new_ps;
		current_ps->previous = new_ps;
	}

	ENABLE_IRQ();

	return new_ps;
}

void
__attribute__((naked)) ctx_switch()
{
	__asm("sub lr, lr, #4");
	__asm("srsdb sp!, #0x13");

	// Saving current context
	__asm("push {r0-r12, lr}");
	__asm("mov %0, lr" : "= r" (current_ps->instruction));
	__asm("mov %0, sp" : "= r" (current_ps->stack));

	// Electing the next current_ps
	elect();

	// Restoring context
	__asm("mov sp, %0" : : "r" (current_ps->stack));
	__asm("mov lr, %0" : : "r" (current_ps->instruction));
	__asm("pop {r0-r12, lr}");

	__asm("rfeia sp!");
}

void
ctx_switch_from_irq()
{
	DISABLE_IRQ();

	__asm("sub lr, lr, #4");
	__asm("srsdb sp!, #0x13");
	__asm("cps #0x13");

	// Saving current context
	__asm("push {r0-r12, lr}");
	__asm("mov %0, lr" : "= r" (current_ps->instruction));
	__asm("mov %0, sp" : "= r" (current_ps->stack));

	// Electing the next current_ps
	elect();

	// Restoring context
	__asm("mov sp, %0" : : "r" (current_ps->stack));
	__asm("mov lr, %0" : : "r" (current_ps->instruction));
	__asm("pop {r0-r12, lr}");

	set_tick_and_enable_timer();
	ENABLE_IRQ();

	__asm("rfeia sp!");
}

void
start_sched(unsigned int stack_size_words)
{
	init_ps = create_process(NULL, NULL, stack_size_words, 0);

	current_ps = init_ps;

	set_tick_and_enable_timer();
	ENABLE_IRQ();
}

void
end_sched()
{
	free_process(init_ps);
}

