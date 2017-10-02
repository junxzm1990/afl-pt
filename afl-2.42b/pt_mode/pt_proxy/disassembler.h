#ifdef KAFL_MODE
#ifndef DISASSEMBLER_H
#define DISASSEMBLER_H

#include<stdbool.h>


//What is the longest TNT sequence? 
//Let's make it 100K? 
#define MAX_TNT_SIZE 20000


#ifdef ARCH_32
typedef uint64_t addr_t; 
#else
typedef uint32_t addr_t;
#endif

typedef struct cft_target{
	addr_t true_br;
	addr_t false_br; 
}cft_target_t;

typedef struct tip_info{
	addr_t prev_tip;
	addr_t prev_ip;
	addr_t cur_tip; 
}tip_info_t; 

typedef struct tnt_cache{
	size_t counter; 
	uint8_t tnt[MAX_TNT_SIZE];
}tnt_cache_t; 


///The major data structure for the disassembler
typedef struct disassembler_s{

	//Pointer to buffer storing the code
	uint8_t* code;
	//minimal address of code
	uint64_t min_addr;
	//maximal address of code
	uint64_t max_addr;
	
	cft_target_t *cfg_cache; 

	tnt_cache_t tnt_cache_map;
	tip_info_t tip_info_map;
	
	//khash_t(ADDR0) *map;
} disassembler_t;

bool init_disassembler(char* elfpath, disassembler_t *disassembler);

#endif

#endif

