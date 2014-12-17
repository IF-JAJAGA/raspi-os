#include <stdint.h>

#define PAGE_SIZE		4*1000/8 //4kB <=> 500 octets
#define SECOND_LVL_TT_COUN 	256 // 2⁸ avec 8 = 19-12+1 (taille de Second Level tabe index dans l'adresse logique)
#define SECOND_LVL_TT_SIZE	(SECOND_LVL_TT_COUN*4) // 4096 o
#define FIRST_LVL_TT_COUN	4096 // 2¹² avec 12 = 31-20+1 (taille de First Level tabe index dans l'adresse logique)
#define FIRST_LVL_TT_SIZE	(FIRST_LVL_TT_COUN*4) // 16 384 o 
#define TOTAL_TT_SIZE		(FIRST_LVL_TT_SIZE + SECOND_LVL_TT_SIZE*FIRST_LVL_TT_COUN) // 16 793 600 o
#define SECTION_SIZE		1048576 // 1Mo
#define NB_PAGE_TP2		4096

#define MMUTABLEBASE		0x48000 // Adresse de la première table de page de niveau 1 dans la mémoire

unsigned int init_kern_translation_table (void);

void start_mmu_C();

void configure_mmu_C();
