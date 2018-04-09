/*PT packet decoding code extract from GRIFFIN:
  https://www.microsoft.com/en-us/research/wp-content/uploads/2017/01/griffin-asplos17.pdf
  with some customization
*/
#include "../../types.h"
#include <fcntl.h>
#include <string.h>
#include <stdio.h>



#define BIT_RANGE 0b1111111111111111111

extern u8 *__afl_area_ptr;
extern u8 *__afl_pt_fav_ptr;
extern u64 rand_map[];


/* decode context, needs to be preserved between any two runs of the parsing function     */
extern u64 ctx_curr_ip;
extern u64 ctx_last_ip;
extern u64 ctx_last_tip_ip;
extern u64 ctx_tnt_long;
extern u32 ctx_bit_selector;
extern u32 ctx_tnt_counter;
extern u64 ctx_curr_tnt_prod;
extern u64 ctx_tnt_container;
extern u8  ctx_tnt_short;
extern u8  ctx_tnt_go;
extern u8  ctx_tnt_lock;
extern u8  ctx_tip_counter;
extern u8  ctx_curr_tnt_cnt;


// static const u8 log_map[1<<21+1] = {

//     [0]                 = 0,
//     [1 ... 2]           = 1,
//     [3 ... 4]           = 2,
//     [5 ... 10]          = 3,
//     [11 ... 31]         = 4,
//     [32 ... 63]         = 5,
//     [64 ... 127]        = 6,
//     [128 ... 255]       = 7,
//     [256 ... 511]       = 8,
//     [512 ... 1023]      = 9,
//     [1024 ... 2047]     = 10,
//     [2048 ... 4095]     = 11,
//     [4096 ... 8191]     = 12,
//     [8192 ... 16383]    = 13,
//     [16384 ... 32767]   = 14,
//     [32768 ... 65535]   = 15,
//     [65536 ... 131071]  = 16,
//     [131072 ... 262143] = 17,
//     [262144 ... 524287] = 18,
//     [524288 ... 1048575]= 19,
//     [1048576 ... 1<<21] = 20

// };

static void
writeout_packet(s32 fd, const char *type ,long value){
    char buf[128]={};
    snprintf(buf, 127,"TYPE:%s %lx\n", type, value);
    write(fd, buf, strlen((char *)buf));
}

//look up rand_map and map the 8-bit val to a random number
static u32
inline map_8(u8 val){
    return rand_map[val];
}

//look up rand_map and map the 16-bit val to a random number
static u32
inline map_16(u16 val){
    return rand_map[val];
}


//look up rand_map and map the 32-bit val to a random number
// static u32
// inline map_32(u32 val){
//   u8 i = 0;
//   u32 res = 0;
//   for (;i < 2;++i){
// 	  res ^= rand_map[((u16)val)];	
// 	  val = val >> 0x10; 
//   }
//   return res;
// }

static u32
inline map_32(u32 val){
  u8 i = 0;
  u32 res = 0;
  res ^= rand_map[(val & 0xffffff)];	
  val = val >> 0x18; 
  res ^= rand_map[(val & 0xff)];	
  return res;
}


//look up rand_map and map the 64-bit val to a random number
static u32
inline map_64(u64 val){

    return ((u32)val) & ((u32)(val>>32)) & BIT_RANGE;

	// fitting into the bit map 19 bit in total

	//map a kep to a 18 bit value

    srand(((u32)val) ^ (u32) (val>>32));
    return rand() & BIT_RANGE;
	
    u32 index = 0;

    index ^= rand_map[val & BIT_RANGE];
    index ^= rand_map[(val >> 19) & BIT_RANGE];
    index ^= rand_map[(val >> 38 ) & BIT_RANGE];
    index ^= rand_map[(val >> 57) & BIT_RANGE];

    return rand_map[index & BIT_RANGE];

/*
    u8 i = 0;
    u32 res = 0;
    res ^= rand_map[val & 0xfffff];
    val = val >> 20;
    res ^= rand_map[val & 0xfffff];
    val = val >> 20;
    res ^= rand_map[val & 0xfffff];
    val = val >> 20;
    res ^= rand_map[val & 0xf];
    for (;i < 4;++i){
	  res ^= rand_map[((u16)val)];	
	  val = val >> 0x10; 
    }
    return res;
*/
}

enum pt_packet_kind {
    PT_PACKET_ERROR = -1,
    PT_PACKET_NONE,
    PT_PACKET_TNTSHORT,
    PT_PACKET_TNTLONG,
    PT_PACKET_TIP,
    PT_PACKET_TIPPGE,
    PT_PACKET_TIPPGD,
    PT_PACKET_FUP,
    PT_PACKET_PIP,
    PT_PACKET_MODE,
    PT_PACKET_TRACESTOP,
    PT_PACKET_CBR,
    PT_PACKET_TSC,
    PT_PACKET_MTC,
    PT_PACKET_TMA,
    PT_PACKET_CYC,
    PT_PACKET_VMCS,
    PT_PACKET_OVF,
    PT_PACKET_PSB,
    PT_PACKET_PSBEND,
    PT_PACKET_MNT,
    PT_PACKET_PAD,
};
/* extern s32 packet_fd; */
static inline u64
pt_get_and_update_ip(u8 *packet, u32 len, u64 *last_ip)
{
    u64 ip;

    switch (len) {
    case 1:
        ip = 0;
        break;
    case 3:
        ip = ((*last_ip) & 0xffffffffffff0000) |
            *(u16 *)(packet+1);
        break;
    case 5:
        ip = ((*last_ip) & 0xffffffff00000000) |
            *(u32 *)(packet+1);
        break;
    case 7:
        if (((*packet) & 0x80) == 0) {
            *(u32 *)&ip = *(u32 *)(packet+1);
            *((int *)&ip+1) = (int)*(short *)(packet+5);
        } else {
            *(u32 *)&ip = *(u32 *)(packet+1);
            *((u32 *)&ip+1) = ((u32)
                               *((u16 *)last_ip+3) << 16 |
                               (u32)*(u16 *)(packet+5));
        }
        break;
    case 9:
        ip = *(u64 *)(packet+1);
        break;
    default:
        ip = 0;
        break;
    }

    return ip;
}

//only when this flag is on
//   and the input fd > 0, packets will be written to /tmp/packet.log
/* #define DEBUG_PACKET */
/* #define DEBUG */
static inline enum pt_packet_kind
pt_get_packet(u8 *buffer, u64 size, u64 *len)
{
    enum pt_packet_kind kind;
    u8 first_byte;
    u8 second_byte;
    u64 cyc_len;
    static u64 ipbytes_plus_one[8] = {1, 3, 5, 7, 7, 1, 9, 1};

#ifdef DEBUG
    if (!buffer || !size) {
        *len = 0;
        return PT_PACKET_NONE;
    }
#endif

    first_byte = *buffer;

    if ((first_byte & 0x1) == 0) { // ???????0
        if ((first_byte & 0x2) == 0) { // ??????00
            if (first_byte == 0) {
                kind = PT_PACKET_PAD;
                *len = 1;
            } else {
                kind = PT_PACKET_TNTSHORT;
                *len = 1;
            }
        } else { // ??????10
            if (first_byte != 0x2) {
                kind = PT_PACKET_TNTSHORT;
                *len = 1;
            } else {
#ifdef DEBUG
                if (size < 2) {
                    kind = PT_PACKET_NONE;
                    *len = 0;
                } else {
#endif
                    second_byte = *(buffer + 1);
                    if ((second_byte & 0x1) == 0) { // ???????0
                        if ((second_byte & 0x2) == 0) { // ??????00
#ifdef DEBUG
                            if (second_byte != 0xc8)
                                return PT_PACKET_ERROR;
#endif
                            kind = PT_PACKET_VMCS;
                            *len = 7;
                        } else { // ??????10
#ifdef DEBUG
                            if (second_byte != 0x82)
                                return PT_PACKET_ERROR;
#endif
                            kind = PT_PACKET_PSB;
                            *len = 16;
                        }
                    } else { // ???????1
                        if ((second_byte & 0x10) == 0) { // ???0???1
                            if ((second_byte & 0x20) == 0) { // ??00???1
                                if ((second_byte & 0x40) == 0) { // ?000???1
                                    if ((second_byte & 0x80) == 0) { // 0000???1
#ifdef DEBUG
                                        if (second_byte != 0x3)
                                            return PT_PACKET_ERROR;
#endif
                                        kind = PT_PACKET_CBR;
                                        *len = 4;
                                    } else { // 1000???1
#ifdef DEBUG
                                        if (second_byte != 0x83)
                                            return PT_PACKET_ERROR;
#endif
                                        kind = PT_PACKET_TRACESTOP;
                                        *len = 2;
                                    }
                                } else { // ??10???1
                                    if ((second_byte & 0x80) == 0) { // 0100???1
#ifdef DEBUG
                                        if (second_byte != 0x43)
                                            return PT_PACKET_ERROR;
#endif
                                        kind = PT_PACKET_PIP;
                                        *len = 8;
                                    } else { // 1100???1
#ifdef DEBUG
                                        if (second_byte != 0xc3)
                                            return PT_PACKET_ERROR;
#endif
                                        kind = PT_PACKET_MNT;
                                        *len = 11;
                                    }
                                }
                            } else { // ??10???1
                                if ((second_byte & 0x80) == 0) { // 0?10???1
#ifdef DEBUG
                                    if (second_byte != 0x23)
                                        return PT_PACKET_ERROR;
#endif
                                    kind = PT_PACKET_PSBEND;
                                    *len = 2;
                                } else { // 1?10???1
#ifdef DEBUG
                                    if (second_byte != 0xa3)
                                        return PT_PACKET_ERROR;
#endif
                                    kind = PT_PACKET_TNTLONG;
                                    *len = 8;
                                }
                            }
                        } else { // ???1???1
                            if ((second_byte & 0x80) == 0) { // 0??1???1
#ifdef DEBUG
                                if (second_byte != 0x73)
                                    return PT_PACKET_ERROR;
#endif
                                kind = PT_PACKET_TMA;
                                *len = 7;
                            } else { // 1??1???1
#ifdef DEBUG
                                if (second_byte != 0xf3)
                                    return PT_PACKET_ERROR;
#endif
                                kind = PT_PACKET_OVF;
                                *len = 2;
                            }
                        }
                    }
#ifdef DEBUG
                }
#endif
            }
        }
    } else { // ???????1
        if ((first_byte & 0x2) == 0) { // ??????01
            if ((first_byte & 0x4) == 0) { // ?????001
                if ((first_byte & 0x8) == 0) { // ????0001
                    if ((first_byte & 0x10) == 0) { // ???00001
                        kind = PT_PACKET_TIPPGD;
                        *len = ipbytes_plus_one[first_byte>>5];
                    } else { // ???10001
                        kind = PT_PACKET_TIPPGE;
                        *len = ipbytes_plus_one[first_byte>>5];
                    }
                } else { // ????1001
                    if ((first_byte & 0x40) == 0) { // ?0??1001
                        if ((first_byte & 0x80) == 0) { // 00??1001
#ifdef DEBUG
                            if (first_byte != 0x19)
                                return PT_PACKET_ERROR;
#endif
                            kind = PT_PACKET_TSC;
                            *len = 8;
                        } else { // 10??1001
#ifdef DEBUG
                            if (first_byte != 0x99)
                                return PT_PACKET_ERROR;
#endif
                            kind = PT_PACKET_MODE;
                            *len = 2;
                        }
                    } else { // ?1??1001
#ifdef DEBUG
                        if (first_byte != 0x59)
                            return PT_PACKET_ERROR;
#endif
                        kind = PT_PACKET_MTC;
                        *len = 2;
                    }
                }
            } else { // ?????101
#ifdef DEBUG
                if ((first_byte & 0x8) == 0)
                    return PT_PACKET_ERROR;
#endif
                if ((first_byte & 0x10) == 0) { // ???0?101
                    kind = PT_PACKET_TIP;
                    *len = ipbytes_plus_one[first_byte>>5];
                } else { // ???1?101
                    kind = PT_PACKET_FUP;
                    *len = ipbytes_plus_one[first_byte>>5];
                }
            }
        } else { // ??????11
            if ((first_byte & 0x4) == 0) {
                kind = PT_PACKET_CYC;
                *len = 1;
            } else {
                for (cyc_len = 2; cyc_len <= size; cyc_len ++) {
                    if (buffer[cyc_len-1] & 0x1) {
                        cyc_len ++;
                    } else {
                        break;
                    }
                }
#ifdef DEBUG
                if (cyc_len > size) {
                    kind = PT_PACKET_NONE;
                    *len = 0;
                } else {
#endif
                    kind = PT_PACKET_CYC;
                    *len = cyc_len;
#ifdef DEBUG
                }
#endif
            }
        }
    }
    /* writeout_packet(packet_fd, "len:", *len); */

    return kind;
}

//#define DEBUG_PACKET
inline
static u64 hash_func(u64 hash, char * str, size_t num){
	
	size_t index = 0;

	while(index < num){
		hash  = str[index] + (hash<<6) + (hash << 16) - hash;
		index++;
	}

	return hash;
}


inline void
pt_parse_packet(char *buffer, size_t size, int dfd, int rfd){

    //since the first packet will always be TIP.PGE, in the parsing process
    // whenever we get a TIP packet, we try calculate the shm index.
    // and reset the TNT calc result afterwards
    u8 *packet;
    s64 bytes_remained;
    u64 packet_len;
    enum pt_packet_kind kind;
    packet = buffer;
    bytes_remained = size;


#define MAX_TNT_LEN 4096
#define UPDATE_TNT_PROD(BIT)                                        \
    do {                                                            \
      if(likely(ctx_tnt_go) && !ctx_tnt_lock){                      \
            ctx_tnt_container |= (BIT<<ctx_curr_tnt_cnt);           \
            if(++ctx_curr_tnt_cnt % 64 == 0){                       \
              ctx_last_tip_ip = hash_func(ctx_last_tip_ip, (char*)&ctx_tnt_container, sizeof(ctx_tnt_container));} \
              ctx_tnt_container = ctx_curr_tnt_cnt = 0;             \
              if (ctx_tnt_counter>=MAX_TNT_LEN){                    \
                ctx_tnt_lock = 1;                                   \
              }                                                     \
            }                                                       \
    } while (0)


#define MAX_TIP_LEN 5 
#define UPDATE_TRACEBITS_IDX()                                          \
    do {                                                                \
	if(ctx_curr_tnt_cnt){ctx_last_tip_ip = hash_func(ctx_last_tip_ip, (char*)&ctx_tnt_container, ctx_curr_tnt_cnt);}       \
      ctx_last_tip_ip = hash_func(ctx_last_tip_ip,(char*)&ctx_curr_ip, sizeof(ctx_curr_ip));\
      __afl_pt_fav_ptr[((ctx_curr_ip ^ ctx_last_ip)&BIT_RANGE) >> 3] |= (1<<((ctx_curr_ip ^ ctx_last_ip) & BIT_RANGE)); \
      ctx_tnt_counter= 0;                                               \
      ctx_tnt_lock= 0;                                                  \
      ctx_tnt_container= 0;                                             \
      ctx_curr_tnt_cnt= 0;                                              \
      if (++ctx_tip_counter >= MAX_TIP_LEN)                             \
        {                                                               \
          __afl_area_ptr[ map_64(ctx_last_tip_ip) >> 3 ] |= (1 << (map_64(ctx_last_tip_ip) & 0b111)) ; \
          ctx_tip_counter = 0;                                          \
          ctx_last_tip_ip = 0;                                          \
        }                                                               \
  } while (0)



#define NEXT_PACKET()                                                \
    do {                                                             \
        bytes_remained -= packet_len;                                \
        packet += packet_len;                                        \
        kind = pt_get_packet(packet, bytes_remained, &packet_len);   \
    } while (0)

    while (bytes_remained > 0) {
        kind = pt_get_packet(packet, bytes_remained, &packet_len);
// #ifdef DEBUG_PACKET
        /* writeout_packet(fd, "BYTE remained:", bytes_remained); */
        /* writeout_packet(fd, "packet addr:", packet); */
// #endif
        switch (kind) {
        case PT_PACKET_TNTSHORT:
            ctx_tnt_short = (u8)*packet;
            ctx_bit_selector = 1 << ((32 - __builtin_clz(ctx_tnt_short)) - 1);
            ctx_tnt_counter += ((32 - __builtin_clz(ctx_tnt_short)) - 1) - 1;
             do {
                 if((ctx_tnt_short & (ctx_bit_selector >>= 1)))
                     UPDATE_TNT_PROD(1);
                 else
                     UPDATE_TNT_PROD(0);
             } while (ctx_bit_selector != 2);
#ifdef DEBUG_PACKET
            /* writeout_packet(dfd, "TNTSHORT container", ctx_tnt_container); */
            writeout_packet(dfd, "TNT:", ctx_tnt_short);
#endif
            break;

        case PT_PACKET_TNTLONG:
            ctx_tnt_long = (u64)*packet;
            ctx_curr_tnt_prod ^= map_64(ctx_tnt_long);
#ifdef DEBUG_PACKET
            writeout_packet(dfd, "TNTLONG", ctx_tnt_long);
#endif
            break;

        case PT_PACKET_TIP:
            ctx_tnt_go=1;
            ctx_last_ip = ctx_curr_ip;
            ctx_curr_ip = pt_get_and_update_ip(packet, packet_len, &ctx_last_ip);
#ifdef DEBUG_PACKET
            writeout_packet(dfd, "TNTCOUNT", ctx_tnt_counter);
            writeout_packet(dfd, "TNTPROD", ctx_curr_tnt_prod);
            writeout_packet(dfd, "TIP", ctx_curr_ip);
#endif

            UPDATE_TRACEBITS_IDX();
            break;

        case PT_PACKET_TIPPGE:
            ctx_last_ip = ctx_curr_ip;
            ctx_curr_ip = pt_get_and_update_ip(packet, packet_len, &ctx_last_ip);
            /* UPDATE_TRACEBITS_IDX(); */
#ifdef DEBUG_PACKET
            writeout_packet(dfd, "TIP.PGE", ctx_curr_ip);
#endif
            break;

        case PT_PACKET_TIPPGD:
            ctx_last_ip = ctx_curr_ip;
            pt_get_and_update_ip(packet, packet_len, &ctx_last_ip);
            /* UPDATE_TRACEBITS_IDX(); */
#ifdef DEBUG_PACKET
            writeout_packet(dfd, "TIP.PGD", ctx_curr_ip);
#endif
            break;

        case PT_PACKET_FUP:
            ctx_last_ip = ctx_curr_ip;
            ctx_curr_ip = pt_get_and_update_ip(packet, packet_len, &ctx_last_ip);
            /* UPDATE_TRACEBITS_IDX(); */
#ifdef DEBUG_PACKET
            writeout_packet(dfd, "FUP", ctx_curr_ip);
#endif
            break;

        case PT_PACKET_PSB:
            ctx_last_ip = 0;
#ifdef DEBUG_PACKET
            writeout_packet(dfd, "PSB", 0);
#endif
            do {
                NEXT_PACKET();
                if (kind == PT_PACKET_FUP){
                    ctx_last_ip = ctx_curr_ip;
                    ctx_curr_ip=pt_get_and_update_ip(packet, packet_len, &ctx_last_ip);
                    /* UPDATE_TRACEBITS_IDX(); */
                }
            } while (kind != PT_PACKET_PSBEND && kind != PT_PACKET_OVF);
#ifdef DEBUG_PACKET
	    if (kind == PT_PACKET_PSBEND)
               writeout_packet(dfd, "PSBEND", 0);
	    else if (kind == PT_PACKET_OVF)
               writeout_packet(dfd, "OVF", 0);
 	    else
		assert(0 && "fuck");
#endif
            break;

#ifdef DEBUG
        //simply ignore all MODE packets unless later cause us trouble
        case PT_PACKET_MODE:
            /* mode_payload = *(packet+1); */
            /* switch ((mode_payload >> 5)) { */
            /* case 0: /\* MODE.Exec *\/ */
            /*     pt_on_mode(mode_payload, arg); */
            /*     break; */
            /* case 1: /\* MODE.TSX *\/ */
            /*     PFATAL("unwanted pt packet type: MODE.TSX"); */
            /*     /\* do { *\/ */
            /*     /\*     NEXT_PACKET(); *\/ */
            /*     /\* } while (kind != PT_PACKET_FUP); *\/ */
            /*     /\* curr_ip = pt_get_and_update_ip(packet, packet_len, &last_ip); *\/ */

            /*     /\* switch ((mode_payload & (u8)0x3)) { *\/ */
            /*     /\* case 0: *\/ */
            /*     /\*     pt_on_xcommit(arg); *\/ */
            /*     /\*     break; *\/ */
            /*     /\* case 1: *\/ */
            /*     /\*     pt_on_xbegin(arg); *\/ */
            /*     /\*     break; *\/ */
            /*     /\* case 2: *\/ */
            /*     /\*     pt_on_xabort(arg); *\/ */
            /*     /\*     curr_block = NULL; *\/ */
            /*     /\*     break; *\/ */
            /*     default: */
            /*         break; */
            /*     } */
            break;
#endif

        case PT_PACKET_OVF:
#ifdef DEBUG_PACKET
            writeout_packet(dfd, "OVF", 0);
#endif
            do {
                NEXT_PACKET();
            } while (kind != PT_PACKET_FUP);
            ctx_last_ip = ctx_curr_ip;
            ctx_curr_ip = pt_get_and_update_ip(packet, packet_len, &ctx_last_ip);
#ifdef DEBUG_PACKET
            writeout_packet(dfd, "FUP", 0);
#endif
            /* UPDATE_TRACEBITS_IDX(); */
            break;

        default:
            break;
        }

        bytes_remained -= packet_len;
        packet += packet_len;
    }
#ifdef DEBUG_PACKET
    write(dfd, "=======\n", 8);
#endif
}

