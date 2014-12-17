#include "sched.h"
#include "hw.h"
#include "phyAlloc.h"

// GLOBAL
const unsigned int WORD_SIZE = 4; // ARM word size (32 bits)
const unsigned int NUMBER_REGISTERS = 14; // 13 all purpose registers (r0-r12) + lr

// STATIC

static const unsigned int CPSR_SVC_NO_IRQ = 0x53; // Supervisor mode (with no interrupts)
static const unsigned int CPSR_SVC_IRQ = 0x13; // Supervisor mode (with interrupts)

// Initialized to NULL (when not allocated)
static struct pcb_s *init_ps    = NULL;

static unsigned int process_count = 0;

struct pcb_s *current_ps = NULL;

static void
start_current_process()
{
	current_ps->state = STATE_EXECUTING;

	current_ps->entry_point(current_ps->args);

	current_ps->state = STATE_ZOMBIE;
	ctx_switch_from_irq();
}

static struct pcb_s *
init_pcb(func_t f, void *args, unsigned int stack_size_words)
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
	pcb->qtCount = 0;

	return pcb;
}

static void
free_process(struct pcb_s *zombie)
{
	// Deallocating
	phyAlloc_free(zombie->stack, zombie->stack_size_words);
	phyAlloc_free(zombie, sizeof(zombie));
}

static void
elect()
{
	struct pcb_s *first = current_ps;

	if (STATE_EXECUTING == current_ps->state) {
		current_ps->state = STATE_WAITING;
	}
	// Switching to the next element in the circular list
	current_ps = current_ps->next;

	if (STATE_ZOMBIE == current_ps->state) {
		while (STATE_ZOMBIE == current_ps->state && current_ps != first)
		{
			// Deleting current_ps from the list
			current_ps->previous->next = current_ps->next;
			current_ps->next->previous = current_ps->previous;

			struct pcb_s *next = current_ps->next;

			// Deallocating the memory of the ZOMBIE
			free_process(current_ps);

			// Switching to the next element
			current_ps = next;
		}

		if (STATE_ZOMBIE == current_ps->state) {
			// There are no process that wants to execute anything
			// TOO MANY ZOMBIES!!

			// We tell init_ps to free the last zombie
			init_ps->next = current_ps;
			current_ps = init_ps;
		}
	}
	else if (STATE_PAUSED == current_ps->state){
		current_ps->qtCount--;
		if (current_ps->qtCount){
			elect();
		}
	}

	current_ps->state = STATE_EXECUTING;
}

//------------------------------------------------------------------------

void
create_process(func_t f, void *args, unsigned int stack_size_words)
{
	DISABLE_IRQ();
	struct pcb_s *new_ps = init_pcb(f, args, stack_size_words);

	if (!current_ps)
	{
		// Single element list
		current_ps           = new_ps;
		current_ps->previous = new_ps;
		current_ps->next     = new_ps;
	}
	else
	{
		// Inserting new_ps BEFORE current_ps
		new_ps->previous = current_ps->previous;
		new_ps->next = current_ps;
		current_ps->previous->next = new_ps;
		current_ps->previous = new_ps;
	}
	ENABLE_IRQ();
}

void
__attribute__((naked)) ctx_switch_from_handler()
{
	__asm("sub lr, lr, #4");
	__asm("srsdb sp!, #0x13");

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

void
__attribute__((naked)) ctx_switch_from_irq()
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
	init_ps = init_pcb(NULL, NULL, stack_size_words);

	init_ps->next = current_ps;
	current_ps = init_ps;

	set_tick_and_enable_timer();
	ENABLE_IRQ();
}

void
end_sched()
{
	free_process(init_ps);
}

