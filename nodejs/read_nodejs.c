/**
 * Sample program is the original sample program that is provided as a
 * sample with the MercuryAPI 29.4.34 from THingMagic (c).
 * It reads all memory bank data of GEN2 tags and prints the data.
 *
 * Modified to use with command input
 * @file readallmembank-GEN2.c
 */

/**
 * This sample program is modified to read and write through named pipes
 * with a nodejs program which provides a server that acts as a passthrough
 * to the network.
 *
 * This has been tested under the following conditions
 *   Ubuntu 16.4 LTS / Raspberry Pi (Jessie)
 *   nodejs v4.2.6 / V9.9.0
 *   MercuryAPI 29.4.34
 *   Modified readall.c example
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 * Paulvh : March 2018
 * Initial version.
 */

#include <tm_reader.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <inttypes.h>
#include <stdarg.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <poll.h>


/***************************************
 * needed for nodejs communication
 ***************************************/
#define BUFSIZE 512     // internal buffer size
int VERBOSE = 0;        // 1 = progress info, 2 = 1 + low level communication with reader

// definitions
char name_pipe_w[25] =  "./tmr_from";   // can be overruled from command line
char name_pipe_r[25] = "./tmr_to";      // can be overruled from command line
#define RETRY 5                         // default read retry count
#define TIMEOUT 1000                    // default read timeout (1000 ms)
#define READ_ABORT 0x999                // used in case a new command was received while tag-reading

// variables
int p_fd_r = 0;                         // pipe handles
int p_fd_w = 0;
int cancel_pending = 0;                 // in case cancel was sent
struct  pollfd fd;                      // needed for polling

int read_timeout = TIMEOUT;             // timeout during reading
int read_retry = RETRY;                 // read retry for tag (0 = continue until one found)
int retval;                             // result value to return
uint8_t readLength = 0x00;              // differs per reader type
/**************************************/

/// antenna list creation instead of asking (THIS WORKS FOR M6E NANO)
uint8_t aList[] = {1};          // 1 antenna
uint8_t *antennaList =aList;
uint8_t antennaCount = 0x1;

/** Enable this to use transportListener + VERBOSE == 2 */
#ifndef USE_TRANSPORT_LISTENER
#define USE_TRANSPORT_LISTENER 1
#endif

#define usage() {errx(1, "Please provide reader URL, such as:\n"\
                         "tmr:///dev/ttyUSB0."\
                         "tmr://my-reader.example.com or tmr://my-reader.example.com\n");}

void errx(int exitval, const char *fmt, ...)
{
  va_list ap;

  va_start(ap, fmt);
  vfprintf(stdout, fmt, ap);

  exit(exitval);
}

void checkerr(TMR_Reader* rp, TMR_Status ret, int exitval, const char *msg)
{
  if (TMR_SUCCESS != ret)
  {
    errx(exitval, "Error %s: %s\n", msg, TMR_strerr(rp, ret));
  }
}

void serialPrinter(bool tx, uint32_t dataLen, const uint8_t data[],
                   uint32_t timeout, void *cookie)
{
  FILE *out = cookie;
  uint32_t i;

  if (VERBOSE < 2) return;

  fprintf(out, "%s", tx ? "Sending: " : "Received:");
  for (i = 0; i < dataLen; i++)
  {
    if (i > 0 && (i & 15) == 0)
    {
      fprintf(out, "\n         ");
    }
    fprintf(out, " %02x", data[i]);
  }

  fprintf(out, "\n");

  fflush(out);
}

void stringPrinter(bool tx,uint32_t dataLen, const uint8_t data[],uint32_t timeout, void *cookie)
{
  FILE *out = cookie;

  if (VERBOSE < 2) return;

  fprintf(out, "%s", tx ? "Sending: " : "Received:");
  fprintf(out, "%s\n", data);

  fflush(out);
}

/** display message and add timestamp */
void verbose(char *format, ...)
{
    time_t  ltime; /* calendar time */
    va_list arg;
    char    *res, *tt;

    if (VERBOSE == 0 ) return;

    //allocate memory
    res = malloc(strlen(format) + 50);

    ltime=time(NULL); /* get current cal time */
    tt = asctime(localtime(&ltime));

    // remove \n
    tt[strlen(tt) - 1] = 0x0;

    sprintf(res,"reader %s: %s\n", tt, format);

    va_start (arg, format);
    vfprintf (stdout, res, arg);
    va_end (arg);

    fflush(stdout);

    // release memory
    free(res);
}

/** read memory banks */
int readAllMemBanks(TMR_Reader *rp, uint8_t antennaCount, uint8_t *antennaList, TMR_TagOp *op, TMR_TagFilter *filter, char * outbuf)
{
    TMR_ReadPlan plan;

    TMR_Status ret;
    TMR_uint8List dataList;
    uint8_t data[BUFSIZE];
    dataList.len = dataList.max = BUFSIZE;
    dataList.list = data;

    TMR_TagReadData trd;
    uint8_t dataBuf[BUFSIZE];
    uint8_t dataBuf1[BUFSIZE];
    uint8_t dataBuf2[BUFSIZE];
    uint8_t dataBuf3[BUFSIZE];
    uint8_t dataBuf4[BUFSIZE];
    char epcStr[128];
    char dataStr[BUFSIZE];

    int retry_count;
    int check;

    // set return
    outbuf[0]=0x0;

  // create a new readplan and read with GEN2
  // weight = 1000 = not used (only in multiplans)
  TMR_RP_init_simple(&plan, antennaCount, antennaList, TMR_TAG_PROTOCOL_GEN2, 1000);

  // add requested filter for correct TAG (EPC). Might be NULL for NO filter
  ret = TMR_RP_set_filter(&plan, filter);
  if (TMR_SUCCESS != ret)
  {
    sprintf(outbuf, "Error setting tag filter: %s", TMR_strerr(rp, ret));
    verbose("%s", outbuf);
    return(ret);
  }

  // add the tag operation requested (e.g. which userbank to read)
  ret = TMR_RP_set_tagop(&plan, op);
  if (TMR_SUCCESS != ret)
  {
    sprintf(outbuf, "Error setting tagop: %s", TMR_strerr(rp, ret));
    verbose("%s", outbuf);
    return(ret);
  }

  /* Commit read plan */
  ret = TMR_paramSet(rp, TMR_PARAM_READ_PLAN, &plan);

  if (TMR_SUCCESS != ret)
  {
    sprintf(outbuf, "Error setting read plan: %s", TMR_strerr(rp, ret));
    verbose("%s", outbuf);
    return(ret);
  }

  // if read_retry is 0, keep trying until succesfull
  if (read_retry > 0) retry_count = read_retry;
  else retry_count = 1;

  while (1)
  {
      // check if new data was sent by nodejs
      check = poll(&fd,1,50);
      if (check > 0)
      {
          verbose("New command received");
          return READ_ABORT;
      }
      else if (check < 0) 
      {
          verbose("read pipe broken");
          return TMR_ERROR_DEVICE_RESET;
      }
      
      /* read the RFID tag*/
      ret = TMR_read(rp, read_timeout, NULL);

      if (ret != TMR_SUCCESS)
      {
        sprintf(outbuf, "Error reading tags: %s", TMR_strerr(rp, ret));
        verbose("%s", outbuf);
        return(ret);
      }

      // if no tags found
      if (TMR_ERROR_NO_TAGS == TMR_hasMoreTags(rp))
      {
        // if not endless loop
        if (read_retry > 0)
        {
            // done all the retry ?
            if (retry_count-- == 0)
            {
                sprintf(outbuf, "Retry count exceeded");
                verbose("%s", outbuf);
                return(TMR_ERROR_NO_TAGS_FOUND);
            }
        }
      }
      else      // got a tag
      {
        break;
      }
    }   // end retry loop

    /* init data storage areas */
    ret = TMR_TRD_init_data(&trd, sizeof(dataBuf)/sizeof(uint8_t), dataBuf);

    if (TMR_SUCCESS != ret)
    {
        sprintf(outbuf, "Error creating tag read data: %s", TMR_strerr(rp, ret));
        verbose("%s", outbuf);
        return(ret);
    }

    trd.userMemData.list = dataBuf1;
    trd.epcMemData.list = dataBuf2;
    trd.reservedMemData.list = dataBuf3;
    trd.tidMemData.list = dataBuf4;

    trd.userMemData.max = BUFSIZE;
    trd.userMemData.len = 0;
    trd.epcMemData.max = BUFSIZE;
    trd.epcMemData.len = 0;
    trd.reservedMemData.max = BUFSIZE;
    trd.reservedMemData.len = 0;
    trd.tidMemData.max = BUFSIZE;
    trd.tidMemData.len = 0;

    int bank = op->u.gen2.u.readData.bank;

    /* get the tag information */
    ret = TMR_getNextTag(rp, &trd);

    if (TMR_SUCCESS != ret)
    {
        sprintf(outbuf, "Error fetching tag %s", TMR_strerr(rp, ret));
        verbose("%s", outbuf);
        return(ret);
    }

    // get EPC
    TMR_bytesToHex(trd.tag.epc, trd.tag.epcByteCount, epcStr);

    verbose("EPC : %s", epcStr);

    // make ASCII of data
    if (0 < trd.data.len)
    {
       TMR_bytesToHex(trd.data.list, trd.data.len, dataStr);
    }
    else
        sprintf(dataStr,"%s", "empty data/ could not read");

    // if only ONE bank requested
    if (bank == TMR_GEN2_BANK_USER)
    {
        sprintf(outbuf, "USER(%d): %s", trd.data.len, dataStr);
    }

    else if (bank == TMR_GEN2_BANK_TID || bank == TMR_GEN2_BANK_TID_ENABLED)
    {
        sprintf(outbuf, "TID(%d): %s", trd.data.len, dataStr);
    }

    else if (bank == TMR_GEN2_BANK_RESERVED || bank == TMR_GEN2_BANK_RESERVED_ENABLED)
    {
        sprintf(outbuf, "RESERVERD(%d): %s", trd.data.len, dataStr);
    }

    // sending the EPC instead of the EPC bank content ( crc + PW + EPC)
    else if (bank == TMR_GEN2_BANK_EPC || bank == TMR_GEN2_BANK_EPC_ENABLED)
    {
        sprintf(outbuf, "EPC(%d): %s", (int) strlen(epcStr), epcStr);
    }

    else    // readall
    {
        if (0 < trd.userMemData.len)
        {
          TMR_bytesToHex(trd.userMemData.list, trd.userMemData.len, dataStr);
          sprintf(outbuf, "USER(%d): %s", trd.userMemData.len, dataStr);
        }

        // sending the EPC instead of the EPC bank content ( crc + PW + EPC)
        if (0 < trd.epcMemData.len)
        {
          // TMR_bytesToHex(trd.epcMemData.list, trd.epcMemData.len, dataStr);

          if (strlen(outbuf) == 0)
             sprintf(outbuf, "EPC(%d): %s", (int) strlen(epcStr), epcStr);
          else
             sprintf(outbuf, "%s,EPC(%d): %s", outbuf, (int) strlen(epcStr), epcStr);
        }

        if (0 < trd.reservedMemData.len)
        {
          TMR_bytesToHex(trd.reservedMemData.list, trd.reservedMemData.len, dataStr);

          if (strlen(outbuf) == 0)
             sprintf(outbuf, "RESERVERD(%d): %s", trd.reservedMemData.len, dataStr);
          else
             sprintf(outbuf, "%s,RESERVERD(%d): %s", outbuf, trd.reservedMemData.len, dataStr);
        }

        if (0 < trd.tidMemData.len)
        {
          TMR_bytesToHex(trd.tidMemData.list, trd.tidMemData.len, dataStr);

          if (strlen(outbuf) == 0)
             sprintf(outbuf, "TID(%d): %s", trd.tidMemData.len, dataStr);
          else
             sprintf(outbuf, "%s,TID(%d): %s", outbuf,trd.tidMemData.len, dataStr);
        }
    }

    verbose("Result of Tag reading : %s", outbuf);

    return(TMR_SUCCESS);
}

/** set the requested region.
 ** It will check whether the region is supported by the reader
 **
 ** return
 **  OK = TMR_SUCCESS
 **  Error = TMR_ERROR_INVALID_REGION (not supported)
 */
int set_region(TMR_Reader *rp, TMR_Region requestRegion)
{
    TMR_RegionList regions;
    TMR_Region _regionStore[32];
    TMR_Status ret;
    int i;

    // init list
    regions.list = _regionStore;
    regions.max = sizeof(_regionStore)/sizeof(_regionStore[0]);
    regions.len = 0;

    // obtain list with supported regions
    ret = TMR_paramGet(rp, TMR_PARAM_REGION_SUPPORTEDREGIONS, &regions);
    if (TMR_SUCCESS != ret)
    {
      errx(1, "Error getting supported regions: %s\n", TMR_strerr(rp, __LINE__));
    }

    if (regions.len < 1)
    {
      errx(1, "Reader doesn't support any regions", TMR_strerr(rp, __LINE__));
    }

    // check the requested region is supported
    for (i = 0; i < regions.len ;i++)
    {
        if (regions.list[i] == requestRegion)
        {
            ret = TMR_paramSet(rp, TMR_PARAM_REGION_ID, &requestRegion);

            if (TMR_SUCCESS != ret)
            {
                errx(1, "Error setting region: %s", TMR_strerr(rp, ret));
            }

            return(ret);
        }
    }

    return(TMR_ERROR_INVALID_REGION);
}

/** execute a received command
 ** rp      : constructor to connected reader
 ** cmd     : command to execute  received
 ** output  : store the result of the execution
 */
int tag_cmd(TMR_Reader *rp, char *cmd, char *outbuf)
{
    TMR_TagOp tagop;

    if (strcmp(cmd, "READALL") == 0)
    {
        verbose("Perform embedded tag operation - read all memory bank without filter");
        TMR_TagOp_init_GEN2_ReadData(&tagop, (TMR_GEN2_BANK_USER | TMR_GEN2_BANK_EPC_ENABLED | TMR_GEN2_BANK_RESERVED_ENABLED |TMR_GEN2_BANK_TID_ENABLED |TMR_GEN2_BANK_USER_ENABLED), 0, readLength);
        return readAllMemBanks(rp, antennaCount, antennaList, &tagop, NULL, outbuf);
    }
    else if (strcmp(cmd, "READTID") == 0)
    {
        verbose("Perform embedded tag operation - read only TID memory without filter pos 0");
        TMR_TagOp_init_GEN2_ReadData(&tagop, TMR_GEN2_BANK_TID, 0, readLength);
        return readAllMemBanks(rp, antennaCount, antennaList, &tagop, NULL, outbuf);
    }

    else if (strcmp(cmd, "READRESERVERD") == 0)
    {
        verbose("Perform embedded  tag operation - reserved memory without filter");
        TMR_TagOp_init_GEN2_ReadData(&tagop, TMR_GEN2_BANK_RESERVED_ENABLED, 0, readLength);
        return readAllMemBanks(rp, antennaCount, antennaList, &tagop, NULL, outbuf);
    }

    else if (strcmp(cmd, "READUSER") == 0)
    {
        verbose("Perform embedded tag operation - read only user memory without filter pos 0");
        TMR_TagOp_init_GEN2_ReadData(&tagop, TMR_GEN2_BANK_USER, 0, readLength);
        return readAllMemBanks(rp, antennaCount, antennaList, &tagop, NULL, outbuf);
    }

    else if (strcmp(cmd, "READEPC") == 0)
    {
        verbose("Perform embedded and standalone tag operation - read only EPC memory without filter pos 0");
        TMR_TagOp_init_GEN2_ReadData(&tagop, TMR_GEN2_BANK_EPC_ENABLED , 0, readLength);
        return readAllMemBanks(rp, antennaCount, antennaList, &tagop, NULL, outbuf);
    }
    else     // unknown request
    {
        sprintf(outbuf, "Illegal / unknown command %s", cmd);
        verbose("%s", outbuf);
        return(TMR_ERROR_INVALID_OPCODE);
    }
}

/** open pipes to nodejs */
void connect_pipes(TMR_Reader *rp)
{
    // open input pipe from nodejs (open O_RDWR to prevent blocking)
    if ( (p_fd_r = open(name_pipe_r,O_RDWR)) < 0)
    {
        printf("can not open named pipe %s\n", name_pipe_r);
        TMR_destroy(rp);
        exit(1);
    }

    // open return pipe to nodejs (open O_RDWR to prevent blocking)
    if ( (p_fd_w = open(name_pipe_w,O_RDWR)) < 0)
    {
        printf("can not open named pipe %s\n", name_pipe_w);
        TMR_destroy(rp);
        exit(1);
    }
    
    // initialize polling structure to check whether a new command was
    // sent while performing retry count in readAllMembanks().
    fd.fd = p_fd_r;
    fd.events = POLLIN;
}

/** sent data back to node.js
 ** format '{"ret":"xxx","val":"yyyyyyyyyy"}'
 ** xxx   = error code (or success)
 ** yyyyy = content to return
 **
 ** DO NOT CHANGE THE DATA FORMAT HERE, NEEDED FOR IPC
 ** potentially change the needed format in handle_responds() in the 
 ** nodejs script
 */
void sent_to_node(char *buf)
{
    char sent_buf[BUFSIZE + 4];

    sprintf(sent_buf, "{\"ret\":\"%x\",\"val\":\"%s\"}", TMR_ERROR_GET_CODE(retval), buf);

    if (write(p_fd_w, sent_buf, strlen(sent_buf)) != strlen(sent_buf))
    {
        printf("Error during writing to node\n");
        exit(1);
    }

    // debug message
    verbose("sent to nodejs %s", sent_buf);
}

/** search for options in the received command
 **
 ** input
 **  hay    : line to search in
 **  needle : argument to look for
 ** output
 **  res    : value following argument, null terminated
 **
 ** return
 **  NULL   : not found
 **  else   : last position of value in hay
 */
char * parse_buf(char *hay, char *needle, char *res)
{
    char *p, *r = res;

    // search argument
    if ((p = strstr(hay,needle)) != NULL)
    {
        // skip needle
        p += strlen(needle);

        // copy
        while (*p != ',' && *p != 0x0) *r++ = *p++;
    }

    // terminate
    *r = 0x0;

    return(p);
}

/** main communication loop with nodejs */
void comm_node(TMR_Reader *rp)
{
    int n, ret;
    char buf[BUFSIZE];          // received commandfrom nodejs
    char ret_buf[BUFSIZE];      // sent to nodejs
    char cmd[20]= {0};          // command to execute
    char *p, tmp[20];

    while(1)
    {
        verbose("Wait input from server");

        n = read(p_fd_r, buf, BUFSIZE);
        
        // Any input ?
        if (n > 0)
        {
            // terminate received buffer
            buf[n] = 0x0;

            verbose("received %s, with length %d", buf, n);

            // sent back to server that command has been received
            retval = TMR_SUCCESS;
            sent_to_node(buf);

            /// Look for arguments in request string (CAPITALS)
            /// cmd is a MUST have !!
            // other to consider: filter, read byte Length, read position start etc.
            if ((p = parse_buf(buf, "CMD=", cmd)) == NULL)
            {
                verbose("NO command received in %s", buf);

                retval = TMR_ERROR_INVALID_OPCODE;
                sent_to_node("missing command");

                continue; // skip rest
            }
            else
                verbose("command set to : %s", cmd);

            // check on timeout (optional else default)
            if ((p = parse_buf(buf, "TIMEOUT=", tmp)) == NULL)
            {
                read_timeout = TIMEOUT;        // set default timeout
            }
            else
            {
                // convert timeout to int
                read_timeout = (int) strtod(tmp, NULL);
                verbose("Read timeout set to %d ms", read_timeout);
            }

            // check on retry count (optional else default)
            if ((p = parse_buf(buf, "RETRY=", tmp)) == NULL)
            {
                read_retry = RETRY;            // set default retry
            }
            else
            {
                // convert retry to int
                read_retry = (int) strtod(tmp, NULL);
                verbose("Read retry count set to %d", read_retry);
            }

            if (strcmp(cmd,"CANCEL") == 0)
            {
                /* maybe more actions are needed. cancel_pending is 
                 * set to prevent sending output of current action */
                cancel_pending = 1;
            }
            else
            {   /// new command received !
                // reset cancel_pending
                cancel_pending = 0;

                // sent to reader program to execute the command
                ret = tag_cmd(rp, cmd, ret_buf);

                // indication that reader program has stopped
                if (ret < 0)
                {
                    retval = TMR_ERROR_SYSTEM_UNKNOWN_ERROR;

                    // sent to node.js to indicate status
                    sent_to_node("!STOPPING!");

                    // maybe more actions are needed
                    cancel_pending = 1;
                }
                
                // READ_ABORT is returned in case new data was sent
                // while performing retry count in readAllMembanks()
                else if (ret != READ_ABORT)
                {
                    /* return the result of doing something
                     * If an earlier cancel command was received: neglect it.
                     */
                    if (cancel_pending == 0)
                    {
                        retval = ret;
                        sent_to_node(ret_buf);
                    }
                }
            }

        }
        
        else if (n < 0)
        {
            verbose("reset/closing pipe");
            
            // reinit... as pipe connection seems to have been lost
            close(p_fd_r);
            close(p_fd_w);
    
            connect_pipes(rp);
    }
  } // while
}

int main(int argc, char *argv[])
{
    TMR_Reader r, *rp;
    TMR_Status ret;
    TMR_ReadPlan plan;
    int c;

    // needed to determine model and read length   
    TMR_String model;
    char str[64];
    
    // parse command line
    while ((c = getopt(argc, argv, "v:w:r:")) != -1)
    {
        switch (c) {
            case 'v':       // set debug / verbose messages
              VERBOSE = (int) strtod(optarg,NULL);
              break;

            case 'r':       // pipe to read from nodejs
              strcpy(name_pipe_r, optarg);
              break;

            case 'w':       // pipe to write to nodejs
              strcpy(name_pipe_w, optarg);
              break;

            default: /* '?' */
              printf("Usage: %s [-v #] [-r pipename] [ -w pipename] device\n", argv[0]);
              exit(EXIT_FAILURE);
        }
    }

    argc -= optind;
    argv += optind;

    if (argv[0] == NULL)
    {
        printf("Error: missing device name\n");
        exit(EXIT_FAILURE);
    }

    // create reader
    rp = &r;
    ret = TMR_create(rp, argv[0]);
    checkerr(rp, ret, 1, "creating reader");

    //in case we want to debug the transport layer (set in top of code)
#ifdef USE_TRANSPORT_LISTENER
    TMR_TransportListenerBlock tb;

    if (TMR_READER_TYPE_SERIAL == rp->readerType)
    {
        tb.listener = serialPrinter;
    }
    else
    {
        tb.listener = stringPrinter;
    }

    tb.cookie = stdout;

    TMR_addTransportListener(rp, &tb);
#endif

    // connect
    ret = TMR_connect(rp);
    checkerr(rp, ret, 1, "connecting reader");

    // set region to Europe (change what ever region you are in the world)
    if (set_region(rp, TMR_REGION_EU3) != TMR_SUCCESS) exit(1);

    /**
    * for antenna configuration we need two parameters
    * 1. antennaCount : specifies the no of antennas should
    *    be included in the read plan, out of the provided antenna list.
    * 2. antennaList  : specifies  a list of antennas for the read plan.
    **/

    /* initialize the read plan */
    ret = TMR_RP_init_simple(&plan, antennaCount, antennaList, TMR_TAG_PROTOCOL_GEN2, 1000);
    checkerr(rp, ret, 1, "initializing the read plan");

    /* Commit read plan */
    ret = TMR_paramSet(rp, TMR_PARAM_READ_PLAN, &plan);
    checkerr(rp, ret, 1, "setting read plan");

    /* Use first antenna for tag operation */
    if (NULL != antennaList)
    {
        ret = TMR_paramSet(rp, TMR_PARAM_TAGOP_ANTENNA, &antennaList[0]);
        checkerr(rp, ret, 1, "setting tagop antenna");
    }

    // set readlength
    model.value = str;
    model.max = 64;
    TMR_paramGet(rp, TMR_PARAM_VERSION_MODEL, &model);

    if ((0 == strcmp("M6e", model.value)) || (0 == strcmp("M6e PRC", model.value)) || (0 == strcmp("M6e Nano", model.value))
      || (0 == strcmp("M6e Micro", model.value)) || (0 == strcmp("Mercury6", model.value))
      || (0 == strcmp("Astra-EX", model.value)) || (0 == strcmp("M6e JIC", model.value)) || (0 == strcmp("Sargas", model.value)))
    {
      /**
      * Specifying the readLength = 0 will return full Memory bank data for any
      * tag read in case of M6e and its varients and M6 reader.
      **/
      readLength = 0;
    }
    else
    {
      /*In other case readLen is minimum.i.e 2 words*/
      readLength = 2;
    }

    // initialize pipe connections with nodejs
    connect_pipes(rp);

    // sent sign of live even if verbose was not set
    printf("Reader program: Ready to communicate\n ");
    fflush(stdout);

    // continued read/ main loop
    comm_node(rp);

    return 0;
}
