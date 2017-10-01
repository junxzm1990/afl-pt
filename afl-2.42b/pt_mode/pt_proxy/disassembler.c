#ifdef KAFL_MODE

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <elf.h>
#include <unistd.h>
#include <string.h>

#include "disassembler.h"

#define LOOKUP_TABLES		5
#define IGN_MOD_RM			0
#define IGN_OPODE_PREFIX	0
#define MODRM_REG(x)		(x << 3)
#define MODRM_AND			0b00111000


#if 0 
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

#endif


//initialize the disassembler based on the elf file
bool init_disassembler(char* elfpath,  disassembler_t *disassembler){
	int elffd; 
	int index; 
	bool ret; 
	int ix; 

#ifdef DEBUGMSG
	int dbgfd; 
	char msg[256];
	dbgfd = open("/tmp/dbg.log", O_RDWR | O_CREAT, 0777);
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

#endif

