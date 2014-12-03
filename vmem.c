#define PAGE_SIZE		4*1000/8 //4kB <=> 500 octets
#define SECOND_LVL_TT_COUN 	1024 // 2¹⁰ avec 8 = 19-12+1+2 (taille de Second Level tabe index dans l'adresse logique)
#define SECOND_LVL_TT_SIZE	SECOND_LVL_TT_COUN*4 // 4096 o
#define FIRST_LVL_TT_COUN	4096 // 2¹² avec 12 = 31-20+1 (taille de First Level tabe index dans l'adresse logique)
#define FIRST_LVL_TT_SIZE	FIRST_LVL_TT_COUN*4 // 16 384 o 
#define TOTAL_TT_SIZE		FIRST_LVL_TT_SIZE + SECOND_LVL_TT_SIZE*FIRST_LVL_TT_COUN // 16 793 600 o

uint32_t device_flags;


