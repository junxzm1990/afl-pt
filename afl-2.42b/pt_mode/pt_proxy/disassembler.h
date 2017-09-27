#ifdef KAFL_MODE

#ifndef DISASSEMBLER_H
#define DISASSEMBLER_H





#ifdef ARCH_32
typedef uint64_t addr_t; 
#else
typedef uint32_t addr_t;
#endif

typedef struct cft_target{
	addr_t true_br;
	addr_t false_br; 
}cft_target_t;

///The major data structure for the disassembler
typedef struct disassembler_s{

	//Pointer to buffer storing the code
	uint8_t* code;
	//minimal address of code
	uint64_t min_addr;
	//maximal address of code
	uint64_t max_addr;
	
	cfg_target_t *cfg_cache; 
	
	//khash_t(ADDR0) *map;
} disassembler_t;

#endif

#endif

