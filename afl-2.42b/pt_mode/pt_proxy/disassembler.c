#ifdef KAFL_MODE

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <elf.h>
#include <unistd.h>
#include <string.h>

#include <capstone/capstone.h>
#include <capstone/x86.h>

#include "disassembler.h"

#define LOOKUP_TABLES		5
#define IGN_MOD_RM			0
#define IGN_OPODE_PREFIX	0
#define MODRM_REG(x)		(x << 3)
#define MODRM_AND			0b00111000


/* http://stackoverflow.com/questions/29600668/what-meaning-if-any-does-the-mod-r-m-byte-carry-for-the-unconditional-jump-ins */
/* conditional branch */
cofi_ins cb_lookup[] = {
	{X86_INS_JAE,		IGN_MOD_RM,	IGN_OPODE_PREFIX},
	{X86_INS_JA,		IGN_MOD_RM,	IGN_OPODE_PREFIX},
	{X86_INS_JBE,		IGN_MOD_RM,	IGN_OPODE_PREFIX},
	{X86_INS_JB,		IGN_MOD_RM,	IGN_OPODE_PREFIX},
	{X86_INS_JCXZ,		IGN_MOD_RM,	IGN_OPODE_PREFIX},
	{X86_INS_JECXZ,		IGN_MOD_RM,	IGN_OPODE_PREFIX},
	{X86_INS_JE,		IGN_MOD_RM,	IGN_OPODE_PREFIX},
	{X86_INS_JGE,		IGN_MOD_RM,	IGN_OPODE_PREFIX},
    {X86_INS_JG,		IGN_MOD_RM,	IGN_OPODE_PREFIX},
    {X86_INS_JLE,		IGN_MOD_RM,	IGN_OPODE_PREFIX},
    {X86_INS_JL,		IGN_MOD_RM,	IGN_OPODE_PREFIX},
    {X86_INS_JNE,		IGN_MOD_RM,	IGN_OPODE_PREFIX},
    {X86_INS_JNO,		IGN_MOD_RM,	IGN_OPODE_PREFIX},
    {X86_INS_JNP,		IGN_MOD_RM,	IGN_OPODE_PREFIX},
    {X86_INS_JNS,		IGN_MOD_RM,	IGN_OPODE_PREFIX},
    {X86_INS_JO,		IGN_MOD_RM,	IGN_OPODE_PREFIX},
    {X86_INS_JP,		IGN_MOD_RM,	IGN_OPODE_PREFIX},
    {X86_INS_JRCXZ,		IGN_MOD_RM,	IGN_OPODE_PREFIX},
    {X86_INS_JS,		IGN_MOD_RM,	IGN_OPODE_PREFIX},
	{X86_INS_LOOP,		IGN_MOD_RM,	IGN_OPODE_PREFIX},
	{X86_INS_LOOPE,		IGN_MOD_RM,	IGN_OPODE_PREFIX},
	{X86_INS_LOOPNE,	IGN_MOD_RM,	IGN_OPODE_PREFIX},
};

/* unconditional direct branch */
cofi_ins udb_lookup[] = {
	{X86_INS_JMP,		IGN_MOD_RM,	0xe9},
	{X86_INS_JMP,		IGN_MOD_RM, 0xeb},
	{X86_INS_CALL,		IGN_MOD_RM,	0xe8},	
};

/* indirect branch */
cofi_ins ib_lookup[] = {
	{X86_INS_JMP,		MODRM_REG(4),	0xff},
	{X86_INS_CALL,		MODRM_REG(2),	0xff},	
};

/* near ret */
cofi_ins nr_lookup[] = {
	{X86_INS_RET,		IGN_MOD_RM,	0xc3},
	{X86_INS_RET,		IGN_MOD_RM,	0xc2},
};
 
/* far transfers */ 
cofi_ins ft_lookup[] = {
	{X86_INS_INT3,		IGN_MOD_RM,	IGN_OPODE_PREFIX},
	{X86_INS_INT,		IGN_MOD_RM,	IGN_OPODE_PREFIX},
	{X86_INS_INT1,		IGN_MOD_RM,	IGN_OPODE_PREFIX},
	{X86_INS_INTO,		IGN_MOD_RM,	IGN_OPODE_PREFIX},
	{X86_INS_IRET,		IGN_MOD_RM,	IGN_OPODE_PREFIX},
	{X86_INS_IRETD,		IGN_MOD_RM,	IGN_OPODE_PREFIX},
	{X86_INS_IRETQ,		IGN_MOD_RM,	IGN_OPODE_PREFIX},
	{X86_INS_JMP,		IGN_MOD_RM,		0xea},
	{X86_INS_JMP,		MODRM_REG(5),	0xff},
	{X86_INS_CALL,		IGN_MOD_RM,		0x9a},
	{X86_INS_CALL,		MODRM_REG(3),	0xff},
	{X86_INS_RET,		IGN_MOD_RM,		0xcb},
	{X86_INS_RET,		IGN_MOD_RM,		0xca},
	{X86_INS_SYSCALL,	IGN_MOD_RM,	IGN_OPODE_PREFIX},
	{X86_INS_SYSENTER,	IGN_MOD_RM,	IGN_OPODE_PREFIX},
	{X86_INS_SYSEXIT,	IGN_MOD_RM,	IGN_OPODE_PREFIX},
	{X86_INS_SYSRET,	IGN_MOD_RM,	IGN_OPODE_PREFIX},
	{X86_INS_VMLAUNCH,	IGN_MOD_RM,	IGN_OPODE_PREFIX},
	{X86_INS_VMRESUME,	IGN_MOD_RM,	IGN_OPODE_PREFIX},
};

uint16_t cmp_lookup[] = {
	X86_INS_CMP,
	X86_INS_CMPPD,
	X86_INS_CMPPS,
	X86_INS_CMPSB,
	X86_INS_CMPSD,
	X86_INS_CMPSQ,
	X86_INS_CMPSS,
	X86_INS_CMPSW,
	X86_INS_CMPXCHG16B,
	X86_INS_CMPXCHG,
	X86_INS_CMPXCHG8B,
};


cofi_ins* lookup_tables[] = {
	cb_lookup,
	udb_lookup,
	ib_lookup,
	nr_lookup,
	ft_lookup,
};

uint8_t lookup_table_sizes[] = {
	22,
	3,
	2,
	2,
	19
};

//Convert digital string into unsigned 64 int
static inline uint64_t fast_strtoull(const char *hexstring){
	uint64_t result = 0;
	uint8_t i = 0;
	if (hexstring[1] == 'x' || hexstring[1] == 'X')
		i = 2;
	for (; hexstring[i]; i++)
		result = (result << 4) + (9 * (hexstring[i] >> 6) + (hexstring[i] & 017));
	return result;
}

static inline uint64_t hex_to_bin(char* str){
	//return (uint64_t)strtoull(str, NULL, 16);
	return fast_strtoull(str);
}


//initialize the disassembler based on the elf file
bool init_disassembler(char* elfpath,  disassembler_t *disassembler){
	int elffd; 
	int index; 
	bool ret; 
	int ix; 

#ifdef DEBUGMSG
	int dbgfd; 
	char msg[256];
	dbgfd = open("/tmp/dbg.log", O_WRONLY | O_APPEND);
#endif

#ifdef ARCH_32
	Elf32_Ehdr elfhdr;
	Elf32_Phdr *phdrs 
#else
	Elf64_Ehdr elfhdr;
	Elf64_Phdr *phdrs; 
#endif	

	disassembler->code = NULL;
	disassembler->min_addr = 0;
	disassembler->max_addr = 0;
	disassembler->cfg_cache = 0;

	//init the tip info
	disassembler->tip_info_map.prev_tip = 0;
	disassembler->tip_info_map.prev_ip = 0;
	disassembler->tip_info_map.cur_tip = 0;

	//init the tnt cache
	disassembler->tnt_cache_map.counter = 0;
	memset(disassembler->tnt_cache_map.tnt, 0, MAX_TNT_SIZE);

	elffd = open(elfpath, O_RDONLY);

	if(elffd <= 0){
#ifdef DEBUGMSG
		snprintf(msg, 256, "Cannot open target file %s\n", elfpath);
#endif
		ret = false;
		goto out; 
	}

	//read program header
	if(read(elffd, &elfhdr, 
#ifdef ARCH_32
				sizeof(Elf32_Ehdr)
#else
				sizeof(Elf64_Ehdr)
#endif	
	       ) <= 0 ){ 
#ifdef DEBUGMSG
		snprintf(msg, 256, "Cannot read program header from target file %s\n", elfpath);
#endif
		ret = false; 
		goto out; 
	}	

	// check if the file is ELF
	if(elfhdr.e_ident[0]!=0x7f ||
			elfhdr.e_ident[1]!='E' ||
			elfhdr.e_ident[2]!='L' ||
			elfhdr.e_ident[3]!='F'){
#ifdef DEBUGMSG
		snprintf(msg, 256, "The target file %s is not an elf\n", elfpath);
#endif
		ret = false; 
		goto out; 
	}

	//read segment table
	phdrs = malloc(
#ifdef ARCH_32
			sizeof(Elf32_Phdr) 
#else 	
			sizeof(Elf64_Phdr) 			
#endif
			* elfhdr.e_phnum);

	if(!phdrs){
#ifdef DEBUGMSG
		snprintf(msg, 256, "Cannot allocate mem for program headers when reading from target file %s\n", elfpath);
#endif
		ret = false; 
		goto out;
	}

	lseek(elffd, elfhdr.e_phoff, SEEK_SET);

	if(read(elffd, phdrs, 
#ifdef ARCH_32
				sizeof(Elf32_Phdr) 
#else 	
				sizeof(Elf64_Phdr) 			
#endif
				* elfhdr.e_phnum) <= 0){

#ifdef DEBUGMSG
		snprintf(msg, 256, "Cannot read program headers when reading from target file %s\n", elfpath);
#endif
		ret = false; 
		goto free_header; 
	}

	for(index = 0; index < elfhdr.e_phnum; index++){
		//code segment is to be loaded and executable
		if(phdrs[index].p_type & PT_LOAD
				&& phdrs[index].p_flags & PF_X){
			break;
		}
	}

	//Did not find the executable segment 
	if(index == elfhdr.e_phnum){
#ifdef DEBUGMSG
		snprintf(msg, 256, "Cannot find executable segment from target file %s\n", elfpath);
#endif
		ret = false; 
		goto free_header; 
	}

	//Now we can finally initializ the disassembler 
	disassembler->code =(uint8_t *)malloc(phdrs[index].p_memsz);
	if(!disassembler->code){
#ifdef DEBUGMSG
		snprintf(msg, 256, "Cannot allocate mem for code segment when reading from target file %s\n", elfpath);
#endif
		ret = false; 
		goto free_header; 

	}	

	memset(disassembler->code, 0, phdrs[index].p_memsz);
	lseek(elffd, phdrs[index].p_offset, SEEK_SET);

	if(read(elffd, disassembler->code, phdrs[index].p_filesz)<=0){
#ifdef DEBUGMSG
		snprintf(msg, 256, "Cannot read code segment when reading from target file %s\n", elfpath);
#endif
		ret = false; 
		goto free_code; 
	}

	disassembler->min_addr = phdrs[index].p_vaddr; 
	disassembler->max_addr = phdrs[index].p_vaddr + phdrs[index].p_memsz; 

#ifdef DEBUGMSG
	snprintf(msg, 256, "Starting address %llx and ending address %llx\n", disassembler->min_addr, disassembler->max_addr);
#endif

	 disassembler->cfg_cache =(cft_target_t *)malloc(phdrs[index].p_memsz * sizeof(cft_target_t));

	if(!disassembler->cfg_cache){
#ifdef DEBUGMSG
		snprintf(msg, 256, "Cannot allocate cache mem for target file %s\n", elfpath);
#endif
		ret = false; 
		goto free_code; 
	}


	memset(disassembler->cfg_cache, 0, phdrs[index].p_memsz * sizeof(cft_target_t));
	ret = true; 
	goto out; 

free_code:
	free(disassembler->code);

free_header: 
	free(phdrs); 
out:
#ifdef DEBUGMSG
	write(dbgfd, msg, strlen(msg));
	close(dbgfd);
#endif
	return ret; 
}

static cofi_type opcode_analyzer(cs_insn *ins){
	uint8_t i, j;
	cs_x86 details = ins->detail->x86;
	
	for (i = 0; i < LOOKUP_TABLES; i++){
		for (j = 0; j < lookup_table_sizes[i]; j++){
			if (ins->id == lookup_tables[i][j].opcode){
				
				/* check MOD R/M */
				if (lookup_tables[i][j].modrm != IGN_MOD_RM && lookup_tables[i][j].modrm != (details.modrm & MODRM_AND))
						continue;	
						
				/* check opcode prefix byte */
				if (lookup_tables[i][j].opcode_prefix != IGN_OPODE_PREFIX && lookup_tables[i][j].opcode_prefix != details.opcode[0])
						continue;
				return i;
			}
		}
	}
	return NO_COFI_TYPE;
}

addr_t get_next_target(disassembler_t* disassembler, addr_t start, bool tnt){

	//start with the beginning address, 
	//linear scan until encounter of conditional jump 
	//Fucked up cases: 
		//Encountered indirect jump...
		//Linear scan error
	addr_t offset;
	addr_t retaddr; 
	const uint8_t* code;	// = self->code + (base_address-self->min_addr);
	size_t code_size;	// = (self->max_addr-base_address);
	uint64_t address;	// = base_address;
	csh handle;
	cs_insn *insn;
	cofi_type type;

	if(start < disassembler->min_addr || start > disassembler->max_addr)
		return 0;

	offset = start  - disassembler->min_addr; 	
	
	//check cache at first 	
	if(tnt){
		if(disassembler->cfg_cache[offset].true_br)
			return disassembler->cfg_cache[offset].true_br;	
	}else{
		if(disassembler->cfg_cache[offset].false_br)
			return disassembler->cfg_cache[offset].false_br;	
	}	
	//cache missed
	//starting raw disassembling

	if (cs_open(CS_ARCH_X86, 
#ifdef	ARCH_32	
	CS_MODE_32, 
#else
	CS_MODE_64,	
#endif
	&handle) != CS_ERR_OK)
		return false;

	cs_option(handle, CS_OPT_DETAIL, CS_OPT_ON);
	insn = cs_malloc(handle);
	
	code = disassembler->code + offset;
	code_size = disassembler->max_addr - start; 
	address = start; 

#ifdef DEBUGMSG
	int dbgfd; 
	char msg[256];
	dbgfd = open("/tmp/dbg.log", O_WRONLY | O_APPEND);
#endif

	#ifdef DEBUGMSG
		snprintf(msg,256, "------ Start point %llx with tnt %d\n",start, tnt);
		write(dbgfd, msg, strlen(msg));
	#endif

	while(cs_disasm_iter(handle, &code, &code_size, &address, insn)){

	#ifdef DEBUGMSG
		snprintf(msg, 256, "Address of current instruction %llx\n", insn->address);
		write(dbgfd, msg, strlen(msg));
	#endif

		type = opcode_analyzer(insn);	

		if(type == NO_COFI_TYPE) //not control flow transfer
			continue; 	
		
		//direct jump: adjust the target and continue; 
		if(type == COFI_TYPE_UNCONDITIONAL_DIRECT_BRANCH){
			address =  hex_to_bin(insn->op_str);
			code_size = disassembler->max_addr - address; 
			code =  disassembler->code + address - disassembler->min_addr; 
			continue;
		}	

		//conditional jump 
		//True branch: the target
		//False branch: the next branch
		if(type == COFI_TYPE_CONDITIONAL_BRANCH){
			addr_t toffset = insn->address - disassembler->min_addr;
		
			disassembler->cfg_cache[toffset].true_br = hex_to_bin(insn->op_str);
			disassembler->cfg_cache[toffset].false_br = insn->address + insn->size;
			
			if(tnt)
				retaddr = disassembler->cfg_cache[toffset].true_br;
			else
				retaddr = disassembler->cfg_cache[toffset].false_br;
			
			break;
		}

		//Other cases? Must be indirect jump. Right?
		retaddr = 0;
		break; 	
	}	
	#ifdef DEBUGMSG
		snprintf(msg,256, "------ End point %llx\n",retaddr);
		write(dbgfd, msg, strlen(msg));
	#endif	
	cs_free(insn, 1);
	cs_close(&handle);
#ifdef DEBUGMSG
	close(dbgfd);
#endif

	return retaddr;
}
#endif

