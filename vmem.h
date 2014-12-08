#define PAGE_SIZE		4*1000/8 //4kB <=> 500 octets
#define SECOND_LVL_TT_COUN 	256 // 2⁸ avec 8 = 19-12+1 (taille de Second Level tabe index dans l'adresse logique)
#define SECOND_LVL_TT_SIZE	SECOND_LVL_TT_COUN*4 // 4096 o
#define FIRST_LVL_TT_COUN	4096 // 2¹² avec 12 = 31-20+1 (taille de First Level tabe index dans l'adresse logique)
#define FIRST_LVL_TT_SIZE	FIRST_LVL_TT_COUN*4 // 16 384 o 
#define TOTAL_TT_SIZE		FIRST_LVL_TT_SIZE + SECOND_LVL_TT_SIZE*FIRST_LVL_TT_COUN // 16 793 600 o

#define MMUTABLEBASE		0x48000 // Adresse de la première table de page de niveau 1 dans la mémoire

uint32_t device_flags =
	1    << 0  | //bit de code exécutable
	1    << 1  | //bit à 1 par défaut
	0b01 << 2  | //2 bits correspondants à C & B
	0b00 << 4  | //2 bits correspondants à AP
	000  << 6  | //3 bits correspondants à TEX
	0    << 9  | //bit correspondant à APX
	0    << 10 | //bit correspondant à S (share)
	0    << 11 ; //bit correspondant à nG (not general)

uint32_t normal_flags =
	1     << 0  | //Xn
	1     << 1  | //default
	0b00  << 2  | //C & B
	0b11  << 4  | //AP
	0b001 << 6  | //TEX
	0     << 9  | //APX
	1     << 10 | //S
	0     << 11 ; //nG


unsigned int init_kern_translation_table (void);

void start_mmu_C();

void configure_mmu_C();
