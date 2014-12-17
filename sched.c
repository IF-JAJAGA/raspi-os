#include "sched.h"
#include "constants.h"
#include "hw.h"
#include "phyAlloc.h"

// GLOBAL
#define NB_PRIORITY 256

// STATIC
static struct pcb_s * priority_array[NB_PRIORITY] = {NULL};

static const unsigned int CPSR_SVC_NO_IRQ = 0x53; // Supervisor mode (with no interrupts)
static const unsigned int CPSR_SVC_IRQ = 0x13; // Supervisor mode (with interrupts)

// Initialized to NULL (when not allocated)
static struct pcb_s *current_ps = NULL;
static struct pcb_s *idle_ps    = NULL;

static unsigned int process_count = 0;

// Starts the process in the global `current_process` variable
static void
start_current_process() {
	current_ps->state = STATE_EXECUTING;

	current_ps->entry_point(current_ps->args);

	current_ps->state = STATE_ZOMBIE;

	// Switching to next process, but never returning (zombie will be erased)
	ctx_switch();
}

// Creates the necessary data structure for the process (PCB + stack)
static struct pcb_s *
init_pcb(func_t f, void *args, unsigned int stack_size_words, unsigned int priority) {
	struct pcb_s *pcb = (struct pcb_s *) phyAlloc_alloc(sizeof(struct pcb_s));

	pcb->pid = ++process_count;
	pcb->state = STATE_NEW;
	pcb->stack_size_words = stack_size_words;

	// The stack base points to the lowest address (last octet cell of the stack)
	uint8_t *stack_base = (uint8_t *) phyAlloc_alloc(stack_size_words * WORD_SIZE_OCT);

	// Positioning the pointer to the first (word) cell of the stack (highest address)
	stack_base += stack_size_words * WORD_SIZE_OCT - WORD_SIZE_OCT;
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
	pcb->qt_count = 0;

	return pcb;
}

// Removes the (zombie) process from the priority data structure
static void
remove_priority(struct pcb_s *zombie) {
	struct pcb_s **first = &(priority_array[NB_PRIORITY - 1 - zombie->priority]);

	// Removing the zombie process from the priority array
	// If alone, we set the cell to NULL
	if (zombie->next == zombie) {
		*first = NULL;
	}
	else {
		// We delete the zombie from the list
		zombie->previous->next = zombie->next;
		zombie->next->previous = zombie->previous;

		if (zombie == *first) {
			*first = zombie->next; 
		}
	}
}

// Deallocating the memory of a process (PCB + stack)
static void
free_process(struct pcb_s *zombie) {
	phyAlloc_free(zombie->stack, zombie->stack_size_words);
	phyAlloc_free(zombie, sizeof(zombie));
}

// Returns the process with highest priority, or NULL if CPU is idle
static struct pcb_s *
highest_priority() {
	struct pcb_s *process_prio = NULL;
	// Finding the first process in the priority array (most prioritary)
	for (int i = 0; process_prio == NULL; ++i) {
		process_prio = priority_array[i];
	}

	return process_prio;
}

/**
 * Chooses the next process alive (not STATE_ZOMBIE) in the linked list given.
 * If there is no next process alive (all STATE_ZOMBIE), it returns NULL.
 * Note: all STATE_ZOMBIE process encountered during the search are DEALLOCATED
 */
static struct pcb_s *
next_alive(struct pcb_s *first_pcb) {
	struct pcb_s *iterator_ps = first_pcb->next;
	if (STATE_ZOMBIE == iterator_ps->state) {
		while (STATE_ZOMBIE == iterator_ps->state && iterator_ps != first_pcb) {
			// Deleting iterator_ps from the list
			remove_priority(iterator_ps);
			
			struct pcb_s *next = iterator_ps->next;

			// Deallocating the memory of the ZOMBIE
			free_process(iterator_ps);

			// Switching to the next element
			iterator_ps = next;
		}

		if (STATE_ZOMBIE == iterator_ps->state) {
			remove_priority(first_pcb);
			free_process(first_pcb);

			// There are no process that wants to execute anything
			// TOO MANY ZOMBIES!!
			return NULL;
		}
	}
	return iterator_ps;
}

// Changes the global `current_ps` variable to the next (according to BPF policy)
static void
elect() {
	if (STATE_ZOMBIE != current_ps->state) current_ps->state = STATE_WAITING;
	struct pcb_s *previous_ps = current_ps;

	do {
		current_ps = highest_priority();
		current_ps = current_ps == NULL ? idle_ps : current_ps;
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

// Creates a process with its stack and code and inserts it according to its priority
void
create_process(func_t f, void *args, unsigned int stack_size_words, unsigned int priority) {
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
		// Inserting BEFORE pcb_first
		new_ps->previous = pcb_first->previous;
		new_ps->next = pcb_first;
		pcb_first->previous->next = new_ps;
		pcb_first->previous = new_ps;
	}

	set_tick_and_enable_timer();
	ENABLE_IRQ();
}

// Sets the `current_ps` to STATE_PAUSED (but does not switch)
void
set_current_paused(unsigned int qt_count, func_t instr) {
	current_ps->state = STATE_PAUSED;
	current_ps->qt_count = qt_count;
	current_ps->instruction = instr;
}

void
__attribute__((naked)) ctx_switch_from_handler()
{
	__asm("sub lr, lr, #4");
	__asm("srsdb sp!, #0x13");
	__asm("cps #0x13");

	// Saving current context
	__asm("push {r0-r12}");
	__asm("mov %0, sp" : "= r" (current_ps->stack));

	// Electing the next current_ps
	elect();

	// Restoring context
	__asm("mov sp, %0" : : "r" (current_ps->stack));
	__asm("mov lr, %0" : : "r" (current_ps->instruction));
	__asm("pop {r0-r12}");

	__asm("rfeia sp!");

}

/**
 * Switches from one process to the next, using elect to find the next ps.
 * Note: when switching, the state of the current_ps is saved so as to return later.
 * When this does happen, the code starts again AFTER the calling of `ctx_switch()`.
 */
void
__attribute__((naked)) ctx_switch() {
	DISABLE_IRQ();

	__asm("srsdb sp!, #0x13");
	__asm("cps #0x13");

	// Saving current context
	__asm("push {r0-r12,lr}");
	__asm("mov %0, lr" : "= r" (current_ps->instruction));
	__asm("mov %0, sp" : "= r" (current_ps->stack));

	// Electing the next current_ps
	elect();

	// Restoring context
	__asm("mov sp, %0" : : "r" (current_ps->stack));
	__asm("mov lr, %0" : : "r" (current_ps->instruction));
	__asm("pop {r0-r12,lr}");

	set_tick_and_enable_timer();
	ENABLE_IRQ();

	__asm("rfeia sp!");
}

/**
 * Switches from one process to the next, using elect to find the next ps.
 * Note: when switching, the state of the current_ps is saved so as to return later.
 * When this does happen, the code starts again WHERE IT WAS INTERRUPTED.
 */
void
ctx_switch_from_irq() {
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

// Loops infinitely, switching from current_ps to the next ps
void
infinite_switching(void *unused) {
	for (;;) {
		// If idle becomes a zombie; check next_alive (does not support it yet)
		ctx_switch();
	}
}

// Does nothing
void nothing(void *unused) { }

// Launching the scheduling of all ps (when no ps, goes to idle_handler)
void
start_sched(unsigned int stack_size_words, func_t idle_handler, void *args) {
	// Default handler: infinite_switching
	idle_handler = idle_handler == NULL ? infinite_switching : idle_handler;

	// Creating the process, without inserting it in the priority array
	// (priority is not used)
	idle_ps = init_pcb(NULL, NULL, stack_size_words, 0);
	idle_ps->pid = 0;
	idle_ps->previous = idle_ps;
	idle_ps->next = idle_ps;

	current_ps = idle_ps;

	ctx_switch();
	idle_handler(args);
}

