/*
 * Copyright (c) 2018 Paul van Haastrecht.
 *
 * This program can decode the TID depending on the header e.g. E20034120156FF00062F8029010E0154700D5FFBFFFFDC00
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 * Paulvh : April 2018
 * Initial version.
 */

#include <stdint.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

// color display enable
#define RED     1
#define GREEN   2
#define YELLOW  3
#define BLUE    4
#define WHITE   5

#define REDSTR "\e[1;31m%s\e[00m"
#define GRNSTR "\e[1;92m%s\e[00m"
#define YLWSTR "\e[1;93m%s\e[00m"
#define BLUSTR "\e[1;34m%s\e[00m"

// display without color
int NoColor = 0;

// Hold the tid (max 0xD0 or 208 bits or 52 nibbles) in ascii
char tid[110];

// hold the tid in binary
uint8_t res[60];

typedef struct CmdToTxt
{
  /** command code */
  uint16_t cmd;
  /** command description */
  char cmdtxt[75];
} CmdToTxt;


// source : https://www.gs1.org/epcglobal/standards/mdid
// 9-bit MDID
struct CmdToTxt manufacturer[60] =
{
 { 0x01, "Impinj"},
 { 0x02, "Texas Instruments"},
 { 0x03, "Alien Technology"},
 { 0x04, "Intelleflex"},
 { 0x05, "Atmel"},
 { 0x06, "NXP Semiconductors"},
 { 0x07, "ST Microelectronics"},
 { 0x08, "EP Microelectronics"},
 { 0x09, "Motorola (formerly Symbol Technologies)"},
 { 0x0a, "Sentech Snd Bhd"},
 { 0x0b, "EM Microelectronics"},
 { 0x0c, "Renesas Technology Corp"},
 { 0x0d, "Mstar"},
 { 0x0e, "Tyco International"},
 { 0x0f, "Quanray Electronics"},
 { 0x10, "Fujitsu"},
 { 0x11, "LSIS"},
 { 0x12, "CAEN RFID srl"},
 { 0x13, "Productivity Engineering Gesellschaft fuer IC Design mbH"},
 { 0x14, "Federal Electric Corp."},
 { 0x15, "ON Semiconductor"},
 { 0x16, "Ramtron"},
 { 0x17, "Tego"},
 { 0x18, "Ceitec S.A."},
 { 0x19, "CPA Wernher von Braun"},
 { 0x1a, "TransCore"},
 { 0x1b, "Nationz"},
 { 0x1c, "Invengo"},
 { 0x1d, "Kiloway"},
 { 0x1e, "Longjing Microelectronics Co. Ltd."},
 { 0x1f, "Chipus Microelectronics "},
 { 0x20, "ORIDAO"},
 { 0x21, "Maintag"},
 { 0x22, "Yangzhou Daoyuan Microelectronics Co. Ltd"},
 { 0x23, "Gate Elektronik"},
 { 0x24, "RFMicron, Inc."},
 { 0x25, "RST-Invent LLC"},
 { 0x26, "Crystone Technology"},
 { 0x27, "Shanghai Fudan Microelectronics Group"},
 { 0x28, "Farsens"},
 { 0x29, "Giesecke & Devrient GmbH"},
 { 0x2a, "AWID"},
 { 0x2b, "Unitec Semicondutores S/A"},
 { 0x2c, "Q-Free ASA"},
 { 0x2d, "Valid S.A."},
 { 0x2e, "Fraunhofer IPMS"},
 { 0x2f, "ams AG"},
 { 0x30, "Angstrem JSC"},
 { 0x31, "Honeywell"},
 { 0x32, "Huada Semiconductor Co. Ltd (HDSC)"},
 { 0x33, "Lapis Semiconductor Co., Ltd."},
 { 0x34, "PJSC Mikron"},
 { 0x35, "Hangzhou Landa Microelectronics Co., Ltd"},
 { 0x36, "Nanjing NARI Micro-Electronic Technology Co., Ltd"},
 { 0x37, "Southwest Integrated Circuit Design Co., Ltd."},
 { 0xff, "unknown"}
};


/* Display in color
 * @param format : Message to display and optional arguments
 *                 same as printf
 * @param level :  1 = RED, 2 = GREEN, 3 = YELLOW 4 = BLUE 5 = WHITE
 *
 *  if NoColor was set, output is always WHITE.
 */
void p_printf(int level, char *format, ...)
{
    char    *col;
    int     coll=level;
    va_list arg;


    //allocate memory
    col = malloc(strlen(format) + 20);

    if (NoColor) coll = WHITE;

    switch(coll)
    {
        case RED:
            sprintf(col,REDSTR, format);
            break;
        case GREEN:
            sprintf(col,GRNSTR, format);
            break;
        case YELLOW:
            sprintf(col,YLWSTR, format);
            break;
        case BLUE:
            sprintf(col,BLUSTR, format);
            break;
        default:
            sprintf(col,"%s",format);
    }

    va_start (arg, format);
    vfprintf (stdout, col, arg);
    va_end (arg);

    fflush(stdout);

    // release memory
    free(col);
}

/* lookup the text description from the command */
char *get_description(int cmd,struct CmdToTxt *p)
{
    // as long as not end of input
    while (p->cmd != 0xff)
    {
        if (p->cmd == (cmd & 0x1ff))
            break;
        p++;
    }

    return(p->cmdtxt);
}

/* converter a line with ascii to bin
 * line: input ascii hex character
 * data: output hex bytes
 *
 * return :
 * 0 = error
 * else number of bytes in data
 */
int ascii_to_hex(char *line_in, uint8_t *data)
{
    char *line = line_in;
    int inlen = strlen(line);
    int i, outlen;
    char c;
    uint8_t val[2];
//printf( "line Ascii_to hex %s, len %d\n", line, inlen);
    outlen = 0;
    i = 0;

    while (inlen > 0 && *line != '\0')
    {
        inlen--;
        c = *line++;

        // skip CR and LF and trailing double quote (as some
        // trace files have that)
        if ( c == 0xd || c == 0xa || c == ':')   continue;

        /* some trace files do NOT not print double character
         * like 0 instead of 00 or 1 instead of 01
         */

        if (c == 0x20)      // space in between bytes
        {
             // if new byte had not started
             if (i == 0)    continue;

             // move to right place & include leading 0
             val[1] = val[0];
             val[0] = '0';

             // set write to buffer
             i = 2;
        }
        else
        {
            // for each character (2 nibbles)
            if (c >= '0' && c <= '9')
                val[i++] = c - '0';
            else if (c >= 'a' && c <= 'f')
                val[i++] = 10 + c - 'a';
            else if (c >= 'A' && c <= 'F')
                val[i++] = 10 + c - 'A';
            else
            {
                printf ("Invalid parse because of:: %c %x, in line:: %s\n",c,c, line_in);
                return 0;
            }
        }

        if (i == 2)
        {
            // create 1 output byte from 2 nibbles
            *data = (val[0] << 4) | val[1];

            // increment length
            outlen++;

            // next output pos
            data++;

            i = 0;
        }
    }

    return(outlen);
}



/* get bits from the TID, maximum 64 bits
 * bit_start : start location bit
 * bit_cnt : number of bits to include
 * val : store the value extracted
 *
 * return code : next BIT position in TID
 */
int get_from_tid(int bit_start, int bit_cnt, uint64_t *val)
{
    uint8_t tt;
    int vi, ti, i;
    *val = 0x0;         //  initialize return value

    if (bit_start + bit_cnt > strlen(tid) * 4)
    {
        p_printf(RED,"error. To many bits asked. Available %d, Requested end %d\n", (int) strlen(tid) * 4, bit_start+bit_cnt);
        exit(EXIT_FAILURE);
    }

    i =  bit_start / 8;     // determine starting byte location
    vi = bit_start % 8;     // starting offset in first byte
    tt = res[i++];          // get starting byte

    // get all the bits
    for (ti = 0 ;ti < bit_cnt; ti++, vi++)
    {
        // if we tested last bit of current byte
        if (vi == 8)
        {
            tt = res[i++];  // get next byte
            vi = 0;         // reset check bit
        }

        *val <<= 1;

        // if bit is set in tid, also set it in the return value
        if ((tt >> (7-vi)) & 1) *val |= 0x1;
    }

    return(ti);
}

/* in case this is Impinj tag
 *
 * Monza Monza R6-P E2801170
 * Monza Monza R6-A E2801171
 * Monza Monza S6-C E2801173
 * Monza Monza R6   E2801160
 * Monza Monza 5    E2801130
 * Monza Monza 4D   E2801100
 * Monza Monza 4E   E280110C
 * Monza Monza 4QT  E2801105
 * Monza Monza 4i   E2801114
 * MonzaX Monza X-2K E2801140
 * MonzaX Monza X-8K E2801150
 *
 */

void disp_TMN_Impinj(uint16_t TMN_id)
{
    char    model[10];

    switch(TMN_id)
    {
        case 0x170:
            strcpy(model,"R6-P");
            break;
        case 0x171:
            strcpy(model,"R6-A");
            break;
        case 0x173:
            strcpy(model,"S6-C");
            break;
        case 0x160:
            strcpy(model,"R6");
            break;
        case 0x130:
            strcpy(model,"5");
            break;
        case 0x100:
            strcpy(model,"4D (TID: SN=48 bits / XTID=16 bits/ TMN=32 bits, EPC: 128 bits, USER memory: 32)");
            break;
        case 0x10C:
            strcpy(model,"4E (TID: SN=48 bits / XTID=16 bits/ TMN=32 bits, EPC: 496 bits, USER memory: 128)");
            break;
        case 0x105:
            strcpy(model,"4QT (TID: TMN=32 bits, EPC: 96 bits, USER memory: 512)");
            break;
        case 0x114:
            strcpy(model,"4i");
            break;
        case 0x140:
            strcpy(model,"X-2K");
            break;
        case 0x150:
            strcpy(model,"X-8K");
            break;
    }

    printf(":  model Monza %s\n", model);
}


/* display Extended Tag Identification
 *
 *
 * 15 14 13 12 11 10 9-1 0
 * !   !  !  !  !  ! RFU !
 * !   !  !  !  !  !     Extended Header Present (XHP)
 * !   !  !  !  !  User Memory and Block Perma Lock Segment Present (UMP)
 * !   !  !  !  BlockWrite and BlockErase Segment Present  (BSP)
 * !   !  !  Optional Command Support segment  (OPS)
 * !   !  !
 * Serialization  (SER)
 */
void display_xtid(uint16_t MDID_id)
{
    uint64_t x_header, tmp;
    uint64_t s_tmp;
    int XHP=0, UMP=0, BSP=0, OPS=0, SER=0;
    int offset = 0x30;          // start position after XTid - header

     p_printf(RED, "\nXTID Extended Tag identifier\n");

    // get XTIDheader
    get_from_tid(0x20,16,(uint64_t *) &x_header);

    // parse XTID header
    if  (x_header & (1 << 0))  XHP = 1;
    if  (x_header & (1 << 10)) UMP = 1;
    if  (x_header & (1 << 11)) BSP = 1;
    if  (x_header & (1 << 12)) OPS = 1;

    SER = (x_header & 0xE000) >> 13;

    /* Extended Header Present (XHP)
     * If non-zero, specifies that additional XTID header bits are present beyond the 16 XTID header
     * bits specified herein. This provides a mechanism to extend the XTID in future versions
     * of the EPC Tag Data Standard.
     * This bit SHALL be set to zero to comply with this version of the EPC Tag Data Standard.
     * If zero, specifies that the XTID header only contains the 16 bits  defined herein */
    if (XHP == 1)
    {
        printf("Additional XTID header available. Interpretation not supported (yet)\n");
    }

    // if serial number is available
    if (SER > 0){

        SER = 48 + (16 * (SER-1));         // # of bits in serial

        // start at offset (0x30)
        offset = get_from_tid(offset, SER, &s_tmp);

        p_printf(YELLOW, "XTID: Serial number %d bits : 0x%lx (hex) %ld (dec)\n", SER, s_tmp, s_tmp);
    }

    /* Optional command support
     * BIT 4 – 0 Max EPC Size. This five bit field shall indicate the maximum size that can be
     *           programmed into the first five bits of the PC.
     * BIT 5     Recom Support If this bit is set the tag supports recommissioning as specified
     *           in [UHFC1G2].
     * BIT 6     Access. If this bit is set the it indicates that the tag supports the
     *           access command.
     * BIT 7     Separate Lockbits. If this bit is set it means that the tag supports lock bits
     *           for each memory bank rather than the simplest implementation of a single lock
     *           bit for the entire tag.
     * BIT 8     Auto UMI Support If this bit is set it means that the tag automatically sets
     *           its user memory indicator bit in the PC word.
     * BIT 9     PJM Support If this bit is set it indicates that the tag supports phase jitter
     *           modulation. This is an optional modulation mode supported only in Gen 2 HF tags.
     * BIT 10    BlockErase Supported. If set this indicates that the tag supports the BlockErase
     *           command. How the tag supports the BlockErase command is described in Section 16.2.4.
     *           A manufacture may choose to set this bit, but not include the BlockWrite and BlockErase
     *           field if how to use the command needs further explanation through a database lookup.
     * BIT 11    BlockWrite Supported. If set this indicates that the tag supports the BlockWrite command.
     *           How the tag supports the BlockErase command is described in Section 16.2.4. A manufacture
     *           may choose to set this bit, but not include the BlockWrite and BlockErase field if how to
     *           use the command needs further explanation through a database lookup.
     * BIT 12    BlockPermaLock Supported. If set this indicates that the tag supports the BlockPermaLock
     *           command. How the tag supports the BlockPermaLock command is described in Section 16.2.5.
     *           A manufacture may choose to set this bit, but not include the BlockPermaLock and User Memory
     *           field if how to use the command needs further explanation through a database lookup.
     * BIT 15 – 13 [RFU] These bits are RFU and should be set to zero.
     */
    if (OPS > 0) {

         // start at offset and read 16 bits
         offset = get_from_tid(offset, 0xf, (uint64_t *) &tmp);

         p_printf(YELLOW, "XTID: Optional Command Support: 0x%lx (hex)\n",tmp);
    }

    /* BlockWrite and BlockErase Segment
     * BIT 7 – 0 Block Write Size Max block size that the tag supports for the BlockWrite command.
     *           This value should be between 1-255 if the BlockWrite command is described in this field.
     * BIT 8     Variable Size Block Write. This bit is used to indicate if the tag supports BlockWrite
     *           commands with variable sized blocks. If the value is zero the tag only supports writing
     *           blocks exactly the maximum block size indicated in bits [7-0]. If the value is one the tag
     *           supports writing blocks less than the maximum block size indicated in bits [7-0].
     * BIT 16 – 9 Block Write EPC Address Offset This indicates the starting word address of the first full
     *           block that may be written to using BlockWrite in the EPC memory bank.
     * BIT 17    No Block Write EPC address alignment. This bit is used to indicate if the tag memory
     *           architecture has hard block boundaries in the EPC memory bank. If the value is zero the tag
     *           has hard block boundaries in the EPC memory bank. The tag will not accept BlockWrite commands
     *           that start in one block and end in another block. These block boundaries are determined by the
     *           max block size and the starting address of the first full block. All blocks have the same maximum size.
     *           If the value is one the tag has no block boundaries in the EPC memory bank. It will accept all
     *           BlockWrite commands that are within the memory bank.
     * BIT 25 – 18 Block Write User Address Offset. This indicates the starting word address of the first full block
     *           that may be written to using BlockWrite in the User memory.
     * BIT 26    No Block Write User Address Alignment This bit is used to indicate if the tag memory architecture has
     *           hard block boundaries in the USER memory bank. If the value is zero the tag has hard block boundaries
     *           in the USER memory bank. The tag will not accept BlockWrite commands that start in one block and end
     *           in another block. These block boundaries are determined by the max block size and the starting address
     *           of the first full block. All blocks have the same maximum size. If the value is one the tag has no block
     *           boundaries in the USER memory bank. It will accept all BlockWrite commands that are within the memory bank.
     * BIT 31 – 27 [RFU] These bits are RFU and should be set to zero.
     * BIT 39 – 32 Size of Block Erase Max block size that the tag supports for the BlockErase command. This value should
     *           be between 1-255 if the BlockErase command is described in this field.
     * BIT 40    Variable Size Block Erase. This bit is used to indicate if the tag supports BlockErase commands with variable
     *           sized blocks. If the value is zero the tag only supports erasing blocks exactly the maximum block size indicated
     *           in bits [39-32]. If the value is one the tag supports erasing blocks less than the maximum block size indicated
     *           in bits [39-32].
     * BIT 48 – 41 Block Erase EPC Address Offset. This indicates the starting address of the first full block that may be
     *           erased in EPC memory bank.
     * BIT 49    No Block Erase EPC Address Alignment. This bit is used to indicate if the tag memory architecture has hard
     *           block boundaries in the EPC memory bank. If the value is zero the tag has hard block boundaries in the EPC
     *           memory bank. The tag will not accept BlockErase commands that start in one block and end in another block.
     *           These block boundaries are determined by the max block size and the starting address of the first full block.
     *           All blocks have the same maximum size. If the value is one the tag has no block boundaries in the EPC memory
     *           bank. It will accept all BlockErase commands that are within the memory bank.
     * BIT 57 – 50 Block Erase User Address Offset This indicates the starting address of the first full block that may be
     *           erased in User memory bank.
     * BIT 58    No Block Erase User Address Alignment.This bit is used to indicate if the tag memory architecture has
     *           hard block boundaries in the USER memory bank. If the value is zero the tag has hard block boundaries in the
     *           USER memory bank. The tag will not accept BlockErase commands that start in one block and end in another block.
     *           These block boundaries are determined by the max block size and the starting address of the first full block.
     *           All blocks have the same maximum size. If the value is one the tag has no block boundaries in the USER memory
     *           bank. It will accept all BlockErase commands that are within the memory bank.
     * BIT 63 – 59 [RFU] These bits are reserved for future use and should be set to zero.
     */
    if (BSP > 0) {

         // start at pos 0x60 (if all options were included)
         offset = get_from_tid(offset, 0x3f, (uint64_t *) &tmp);

         p_printf(YELLOW, "XTID: BlockWrite and BlockErase Segment 0x%lx (hex)\n",tmp);
    }

    /* User Memory and Block Perma Lock Segment
     * BIT 15 – 0 User Memory Size Number of 16-bit words in user memory.
     * BIT 31 –16 BlockPermaLock Block Size If non-zero, the size in words of each block that may be block permalocked.
     *            That is, the block permalock feature allows blocks of N*16 bits to be locked, where N is the value of this field.
     *            If zero, then the XTID does not describe the block size for the BlockPermaLock feature. The tag may or may not
     *            support block permalocking. This field SHALL be zero if the Optional Command Support Segment (Section 16.2.3) is
     *            present and its BlockPermaLockSupported bit is zero.
     */
    if (UMP > 0) {

         // start at pos 0xB0 (if all options were included)
         offset = get_from_tid(offset, 0x1f, (uint64_t *) &tmp);

         p_printf(YELLOW, "XTID:User Memory and Block Perma Lock Segment 0x%lx (hex)\n",tmp);
    }

}

/*  class_id 0xE200
 *  Per EPC Tag Data Standard 1.11, effective as of EPC Tag Data Standard 1.10:
 *
 *  An EPC TID memory bank shall contain an 8 bit ISO/IEC 15963 allocation class identifier of E2h at memory locations 00h to 07h.
 *  TID memory above location 07h SHALL be configured as follows:
 *
 *  08h: XTID (X) indicator (whether a Tag implements Extended Tag Identification, XTID)
 *  09h: Security (S) indicator (whether a Tag supports the Authenticate and/or Challenge commands)
 *  0Ah: File (F) indicator (whether a Tag supports the FileOpen command)
 *  0Bh to 13h: a 9-bit mask-designer identifier (MDID) available from GS1
 *  14h to 1Fh: a 12-bit, Tag-manufacturer-defined Tag Model Number (TMN)
 *  above 1Fh: as defined in TDS section ‎16.2
 *
 * EPCglobal will assign two MDIDs to each mask designer, one with bit 08 h equal to one and one with bit 08 h equal to zero.
 * Readers and applications that are not configured to handle the extended TID will treat both of these numbers as a 12 bit MDID.
 * Readers and applications that are configured to handle the extended TID will recognize the TID memory location 08 h as the
 * Extended Tag Identification bit. The value of this bit indicates the format of the rest of the TID. A value of zero indicates
 * a short TID in which the values beyond address 1F h are not defined. A value of one indicates an Extended Tag Identification
 * (XTID) in which the memory locations beyond 1F h contain additional data as specified in Section 16.2.
 */
void handle_class_id()
{
    uint64_t class_id;              // bit 0 - 7
    uint64_t xtid,s_ind,f_ind;      // 8h, 9h, Ah
    uint64_t MDID_id, TMN_id;        // bit Bh - 13h, 14h - 1Fh
    uint64_t serial;

    // get class id 8 bit ISO/IEC 15963
    // start bit 0, count = 8
    get_from_tid(0,8, &class_id);

    // EPCglobal
    if (class_id == 0xe2)
    {
        p_printf(RED, "class access identifier 0x%02lx EPC global\n", class_id);

        // a 9-bit mask-designer identifier (MDID)
        get_from_tid(0xB,9, &MDID_id);
        printf("9-bit mask-designer identifier (MDID) 0x%03lx: %s\n", MDID_id, get_description(MDID_id, manufacturer));

        // check XTID
        get_from_tid(0x8,1,  &xtid);
        printf("xtid available bit \t\t%lx\t", xtid);
        if (xtid == 1) printf("Extended tag identification available\n");
        else printf("Extended tag identification NOT available\n");

        get_from_tid(0x9,1, &s_ind);
        printf("Security (S) indicator bit\t%lx\t", s_ind);
        if (s_ind) printf("Tag supports the Authenticate and/or Challenge commands\n");
        else printf("Tag does NOT support the Authenticate and/or Challenge commands\n");

        get_from_tid(0xa,1, &f_ind);
        printf("File (F) indicator bit\t\t%lx\t", f_ind);
        if (f_ind) printf("Tag supports the FileOpen command\n");
        else printf("Tag does NOT support the FileOpen command\n");

        // TID memory locations 14 h to 1F h SHALL contain a 12-bit vendor-defined Tag model number (TMN)
        get_from_tid(0x14,12,&TMN_id);

        printf("Tag-manufacturer-defined 12-bit Tag model number 0x%03lx ", TMN_id);

        // in case this is Impinj
        if (MDID_id == 1)   disp_TMN_Impinj(TMN_id);

        // others can be included here

        // else show default
        else printf(", %ld (dec)\n", TMN_id);

        // depending on bit 8 !
        if (xtid == 1) display_xtid(MDID_id);
    }

    /* Tags whose AC identifier is E0 h, the ISO/IEC 15963 requires that the TID
     * memory comprise of an 8-bit tag manufacturer ID and a 48-bit tag serial number.
     * Furthermore, the standard requires that the TID memory be permalocked */

    else if (class_id == 0xe0)
    {
        p_printf(RED, "class access identifier 0x%02lx EPC global\n", class_id);

        // 0x08 - 0x0f
        get_from_tid(0x8,8,  &MDID_id);
        printf("MDID %03lx : %s\n\n", MDID_id, get_description(MDID_id, manufacturer));

        // 0x10 - 0x3f
        get_from_tid(0x10,48, &serial);
        printf("Tag-manufacturer 48 bit serial number 0x%08lx / %ld (dec)\n", serial, serial);
    }
    else
        printf("unknown tag structure\n");
}

int main(int argc, char *argv[])
{
    int  len;

    // check for correct usage
    if (argc != 2)
    {
      fprintf(stderr, "Usage: tid [-v] tidstring\n");
      exit(EXIT_FAILURE);
    }

    len = strlen(argv[1]);

    if (len < 8)
    {
        p_printf(RED,"TID not long enough : %d\n", len);
        exit(EXIT_FAILURE);
    }

    else if (len > 52)
    {
        p_printf(RED,"TID too long : %d\n", len);
        exit(EXIT_FAILURE);
    }

    strncpy(tid, argv[1],len);

    // ascii to hex
    if (ascii_to_hex(tid,res)== 0)
    {
        p_printf(RED,"Error during conversion\n");
        exit(EXIT_FAILURE);
    }

    handle_class_id();

    exit(EXIT_SUCCESS);
}
