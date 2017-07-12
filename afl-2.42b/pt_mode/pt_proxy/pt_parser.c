/*PT packet decoding code extract from GRIFFIN:
  https://www.microsoft.com/en-us/research/wp-content/uploads/2017/01/griffin-asplos17.pdf
  with some customization
*/
#include "../../types.h"



#ifdef DEBUG_PACKET
const char* debug_packet_path = "/tmp/packet.log"
static void
writeout_packet(u32 fd, const char *type ,u64 value){
    char buf[128];
    write(fd, snprintf(buf, 127,"TYPE:%s %lx\n", type, value), strlen(buf));
}
#endif



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
        *last_ip = ip;
        break;
    case 5:
        ip = ((*last_ip) & 0xffffffff00000000) |
            *(u32 *)(packet+1);
        *last_ip = ip;
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
        *last_ip = ip;
        break;
    case 9:
        ip = *(u64 *)(packet+1);
        *last_ip = ip;
        break;
    default:
        ip = 0;
        *last_ip = 0;
        break;
    }

    return ip;
}
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

    return kind;
}

#define MAX_TNT_BITS (1 << 12)
inline void
pt_parse_packet(char *buffer, size_t size){

    //since the first packet will always be TIP.PGE, in the parsing process
    // whenever we get a TIP packet, we try calculate the shm index.
    // and reset the TNT calc result afterwards
    u8 *packet;
    u64 bytes_remained;


    u64 curr_ip = 0;
    u64 last_ip = 0;
    u64 packet_len;
    u8 bit_selector;
    u8 mask;
    enum pt_packet_kind kind;
    packet = buffer;
    bytes_remained = size;
    
#ifdef DEBUG_PACKET
    fd = open(debug_packet_path, O_RDWR);
#endif

#define NEXT_PACKET()                                               \
    do {                                                            \
        bytes_remained -= packet_len;                               \
        packet += packet_len;                                       \
        kind = pt_get_packet(packet, bytes_remained, &packet_len);  \
    } while (0)

    while (bytes_remained > 0) {
        kind = pt_get_packet(packet, bytes_remained, &packet_len);

        switch (kind) {
        case PT_PACKET_TNTSHORT:
            mask = (u8)*packet;
            bit_selector = 1 << ((32 - __builtin_clz(mask)) - 1);
            do {
                if (mask & (bit_selector >>= 1)) {
                    //TNT is 1, record the bit 
                } else {
                    //TNT is 0, record the bit 
                }
            } while (bit_selector != 2);
#ifdef DEBUG_PACKET
            writeout_packet(fd, "TNTSHORT", mask);
#endif
            break;

        case PT_PACKET_TIP:
            curr_ip = pt_get_and_update_ip(packet, packet_len, &last_ip);
#ifdef DEBUG_PACKET
            writeout_packet(fd, "TIP", curr_ip);
#endif
            break;

        case PT_PACKET_TIPPGE:
            curr_ip = pt_get_and_update_ip(packet, packet_len, &last_ip);
#ifdef DEBUG_PACKET
            writeout_packet(fd, "TIP.PGE", curr_ip);
#endif
            break;

        case PT_PACKET_TIPPGD:
            pt_get_and_update_ip(packet, packet_len, &last_ip);
#ifdef DEBUG_PACKET
            writeout_packet(fd, "TIP.PGD", curr_ip);
#endif
            break;

        case PT_PACKET_FUP:
            curr_ip = pt_get_and_update_ip(packet, packet_len, &last_ip);
#ifdef DEBUG_PACKET
            writeout_packet(fd, "FUP", curr_ip);
#endif
            break;

        case PT_PACKET_PSB:
            last_ip = 0;
            do {
                NEXT_PACKET();
                if (kind == PT_PACKET_FUP)
                    curr_ip=pt_get_and_update_ip(packet, packet_len, &last_ip);
            } while (kind != PT_PACKET_PSBEND && kind != PT_PACKET_OVF);
#ifdef DEBUG_PACKET
            writeout_packet(fd, "PSB", 0);
#endif
            break;

#ifdef DEBUG
        //simply ignore all MODE packets unless later cause us trouble
        case PT_PACKET_MODE:
            mode_payload = *(packet+1);
            switch ((mode_payload >> 5)) {
            case 0: /* MODE.Exec */
                pt_on_mode(mode_payload, arg);
                break;
            case 1: /* MODE.TSX */
                PFATAL("unwanted pt packet type: MODE.TSX");
                /* do { */
                /*     NEXT_PACKET(); */
                /* } while (kind != PT_PACKET_FUP); */
                /* curr_ip = pt_get_and_update_ip(packet, packet_len, &last_ip); */

                /* switch ((mode_payload & (u8)0x3)) { */
                /* case 0: */
                /*     pt_on_xcommit(arg); */
                /*     break; */
                /* case 1: */
                /*     pt_on_xbegin(arg); */
                /*     break; */
                /* case 2: */
                /*     pt_on_xabort(arg); */
                /*     curr_block = NULL; */
                /*     break; */
                default:
                    break;
                }
            break;
#endif

        case PT_PACKET_OVF:
            do {
                NEXT_PACKET();
            } while (kind != PT_PACKET_FUP);
            curr_ip = pt_get_and_update_ip(packet, packet_len, &last_ip);
            break;
#ifdef DEBUG_PACKET
            writeout_packet(fd, "OVF", 0);
#endif

        default:
            break;
        }

        bytes_remained -= packet_len;
        packet += packet_len;
    }
}

