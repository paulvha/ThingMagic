#ifndef WIN32
#define TMR_ENABLE_GETOPT
#define TMR_ENABLE_READLINE
#endif

#include <ctype.h>
#include <errno.h>
#ifdef TMR_ENABLE_GETOPT
#include <getopt.h>
#endif /* TMR_ENABLE_GETOPT */ 

#include <inttypes.h>

#include <time.h>

#include <stdint.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <signal.h>
#ifdef TMR_ENABLE_READLINE
  #include <unistd.h>
  #include <readline/readline.h>
  #include <readline/history.h>
#endif /* TMR_ENABLE_READLINE */

#ifdef WIN32
#define strcasecmp _stricmp
#define strncasecmp _strnicmp
//#define strtok_r strtok_s
#define strdup _strdup
#endif /* WIN32 */

#include "tm_reader.h"

#include "serial_reader_imp.h"

bool connected;

TMR_Reader r, *rp;
TMR_ReadPlan tmpPlan;
uint8_t ant_list[]= {1};

TMR_TransportListenerBlock tb, *listener;
TMR_TagReadData tag;

void readcmd(int argc, char *argv[]);
void readintocmd(int argc, char *argv[]);
void writeepccmd(int argc, char *argv[]);
void readmemwordscmd(int argc, char *argv[]);
void readmembytescmd(int argc, char *argv[]);
void writememwordscmd(int argc, char *argv[]);
void writemembytescmd(int argc, char *argv[]);
void lockcmd(int argc, char *argv[]);
void killcmd(int argc, char *argv[]);
void gposetcmd(int argc, char *argv[]);
void gpigetcmd(int argc, char *argv[]);
void loadfwcmd(int argc, char *argv[]);
void getcmd(int argc, char *argv[]);
void setcmd(int argc, char *argv[]);
void listcmd(int argc, char *argv[]);
void readintocmd(int argc, char *argv[]);
void helpcmd(int argc, char *argv[]);
void tracecmd(int argc, char *argv[]);
void echocmd(int argc, char *argv[]);
void sleepcmd(int argc, char *argv[]);
void savedConfig(int argc, char *argv[]);
void regulatoryTestCmds(int argc, char *argv[]);
void blockWriteCmds(int argc, char *argv[]);
void blockPermalockCmds(int argc, char *argv[]);
void tagMetaData(int argc, char *argv[]);
void gpiodirection(int argc, char *argv[]);
void closeout();
void errx(int exitval, const char *fmt, ...);
void serialPrinter(bool tx, uint32_t dataLen, const uint8_t data[],
                   uint32_t timeout, void *cookie);
void stringPrinter(bool tx, uint32_t dataLen, const uint8_t data[],
                   uint32_t timeout, void *cookie);

void runcmd(int argc, char *argv[]);
int parseEpc(const char *arg, TMR_TagData *tag, int *nchars);
int parseFilter(const char *arg, TMR_TagFilter **filterp, int *nchars);
int parseTagop(const char *arg, TMR_TagOp **tagopp, int *nchars);
int parseU32List(const char *arg, TMR_uint32List *list, int *nchars);
int parseU8List(const char *arg, TMR_uint8List *list, int *nchars);
int parseWords(const char *arg, TMR_uint16List *list);
int parseBytes(const char *arg, TMR_uint8List *list);
int parseReadPlan(const char *arg, TMR_ReadPlan *plan, int *nchars);
int parseLockAction(const char *arg, TMR_TagLockAction *action);
void printU8List(TMR_uint8List *list);
void printU32List(TMR_uint32List *list);
void printPortValueList(TMR_PortValueList *list);
void printReadPlan(TMR_ReadPlan *plan);
void printFilter(TMR_TagFilter *filter);
void printTagop(TMR_TagOp *tagop);
const char *listname(const char *list[], int listlen, unsigned int id);
int listid(const char *list[], int listlen, const char *name);

#ifdef TMR_ENABLE_READLINE
char *command_generator(const char *, int);
char **demo_completion(const char *, int, int);
#endif /* TMR_ENABLE_READLINE */

#define numberof(x) (sizeof((x))/sizeof((x)[0]))

#ifdef TMR_ENABLE_READLINE
char *getcommand_interactive();
char *getcommand_noninteractive();
char *getcommand_interactive()
{
  return readline(">>> ");
}
#endif /* TMR_ENABLE_READLINE */

char *getcommand_noninteractive()
{
  char buf[1024];
  char *ret;

  //printf(">>> ");
  //fflush(stdout);

  ret = fgets(buf, 1024, stdin);
  if (NULL == ret)
  {
    return NULL;
  }

  ret = strdup(buf);

  /* Trim newlines and carriage returns */
  while (0 < strlen(ret))
  {
    char* lastp = &ret[strlen(ret) - 1];
    if ((*lastp == '\n') || (*lastp == '\r'))
    {
      *lastp = '\0';
    }
    else
    {
      break;
    }
  }

  return ret;
}

/* check for Nano model
 * 
 * return 
 * OK = TMR_SUCCESS
 * Error TMR_ERROR_BL_INVALID_APP_END_ADDR
 */
int check_model(TMR_Reader *rp)
{
  TMR_String model;         // check for model
  char str[64];  

  model.value = str;
  model.max = 64;
  TMR_paramGet(rp, TMR_PARAM_VERSION_MODEL, &model);

  if (0 != strcmp("M6e Nano", model.value))
  {
    fprintf(stdout, "This is not a M6e Nano but an %s\n", model.value);
    return(TMR_ERROR_BL_INVALID_APP_END_ADDR);
  }

  return(TMR_SUCCESS);
}

/*
 * sets antennacount for Nano to 1 (Tx & RXport = 1) and power to requested levels
 * 
 * return
 * OK = TMR_SUCCESS
 *  
 */

int set_antenne_power(TMR_Reader *rp,int16_t read_Power, int16_t write_Power)
{
    TMR_Status ret;
    int16_t maxpower, minpower;
    int16_t setpower; 
    
     /* Init new readplan: 
     * 1 antenna, ant_list: 1, set for GEN2
     * weight = 1 (ignore for simple plan)
     * 
     * !!!!!
     * When trying to set a new read-plan from a subroutine, the antenna list 
     * got scrambled with random number after return from the subroutine.
     * 
     * The reason is that TMR_RP_init_simple() stores the pointer to the
     * ant_list provided. If that list is defined as a local variable (e.g. 
     * within the subroutine) it is in temporarily data space for as long as the
     * sub routine lives. On return that data location can be used for some other
     * variable, and hence the list is not valid anymore. Same is true for readplan!
     * 
     * hence both tmpplan and ant_list are defined global
     * !!!!!
     */
        
    // initialize the read plan 
    TMR_RP_init_simple(&tmpPlan, 1,ant_list, TMR_TAG_PROTOCOL_GEN2, 1) ;
   
    // commit new readplan
    ret = TMR_paramSet(rp, TMR_PARAM_READ_PLAN, &tmpPlan);

    if (TMR_SUCCESS != ret)
    {
        errx(1, "error committing read plan : %s\n", TMR_strerr(rp,ret));
        closeout();
    }
    
    // get antenna power limits
    ret = TMR_paramGet(rp, TMR_PARAM_RADIO_POWERMAX, &maxpower);
    if (TMR_SUCCESS != ret)
    {
        errx(1, "error getting maximum power: %s\n", TMR_strerr(rp,ret));
        return(ret);
    }

    ret = TMR_paramGet(rp, TMR_PARAM_RADIO_POWERMIN, &minpower);
    if (TMR_SUCCESS != ret)
    {
        errx(1, "error getting minimum power: %s\n", TMR_strerr(rp,ret));
        return(ret);
    }    
  
    // set for requested power (within limits)
    setpower = read_Power;   // readpower
    if (setpower > maxpower) setpower = maxpower;
    else if (setpower < minpower) setpower = minpower;

    ret = TMR_paramSet(rp, TMR_PARAM_RADIO_READPOWER, &setpower);
    if (TMR_SUCCESS != ret)
    {
        errx(1, "error setting read power: %s\n", TMR_strerr(rp,ret));
        return(ret);
    } 
    
    setpower = write_Power;  // writepower
    if ( setpower > maxpower) setpower = maxpower;
    else if (setpower < minpower) setpower = minpower;

    ret = TMR_paramSet(rp, TMR_PARAM_RADIO_WRITEPOWER, &setpower);
    if (TMR_SUCCESS != ret)
    {
        errx(1, "error setting write power: %s\n", TMR_strerr(rp,ret));
        return(ret);
    } 

    return(ret);
}


/* set the requested region and check this is supported by the reader
 * return 
 * OK = TMR_SUCCESS
 * Error = TMR_ERROR_INVALID_REGION ( not supported)
 */
 
int set_region(TMR_Reader *rp, TMR_Region requestRegion)
{
    TMR_RegionList regions;
    TMR_Region _regionStore[32];
    TMR_Status ret;
    int i;

    regions.list = _regionStore;
    regions.max = sizeof(_regionStore)/sizeof(_regionStore[0]);
    regions.len = 0;

    // obtain list with supported regions
    ret = TMR_paramGet(rp, TMR_PARAM_REGION_SUPPORTEDREGIONS, &regions);
    if (TMR_SUCCESS != ret)
    {
      errx(1, "Error getting supported region: %s\n", TMR_strerr(rp, __LINE__));
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
                errx(1, "Error setting region: %s\n", TMR_strerr(rp, ret));
            }
            
            return(ret);
        }
    }
    
    return(TMR_ERROR_INVALID_REGION);
}

/*********************************************************************
** catch signals to close out correctly 
**********************************************************************/
void signal_handler(int sig_num)
{
	switch(sig_num)
	{
		case SIGKILL:
		case SIGABRT:
		case SIGINT:
		case SIGTERM:
			printf("\nStopping UHF demo\n");
			closeout();
			break;
		default:
			printf("\nneglecting signal %d.\n",sig_num);
	}
}
/*********************************************************************
** setup signals 
**********************************************************************/
void set_signals()
{
	struct sigaction act;
	
	memset(&act, 0x0,sizeof(act));
	act.sa_handler = &signal_handler;
	sigemptyset(&act.sa_mask);
	
	sigaction(SIGTERM,&act, NULL);
	sigaction(SIGINT,&act, NULL);
	sigaction(SIGABRT,&act, NULL);
	sigaction(SIGSEGV,&act, NULL);
	sigaction(SIGKILL,&act, NULL);
}



int
main(int argc, char *argv[])
{
  char *uri;
#ifdef TMR_ENABLE_GETOPT
  int opt;
#endif /* TMR_ENABLE_GETOPT */
  int verbose;
  TMR_Status ret;
      
  char *saveptr, *arg, *args[16];
  char *line;
  char *(*getcommand)();
  verbose = 0;

#ifdef TMR_ENABLE_GETOPT
  while ((opt = getopt(argc, argv, "v")) != -1)
  {
    switch (opt) {
    case 'v':
      verbose = 1;
      break;
    default: /* '?' */
      fprintf(stderr, "Usage: %s [-v] uri\n",
              argv[0]);
      exit(EXIT_FAILURE);
    }
  }
  
  argc -= optind;
  argv += optind;
#else /* TMR_ENABLE_GETOPT not defined */
  {

    int i;
    // Skip options (args starting with '-')
    for (i=1; i<argc; i++)
    {
      if (argv[i][0] != '-')
      {
        break;
      }
      if (argv[i][0] != '\0')
      {
        switch (argv[i][1])
        {
        case 'v':
          verbose = 1;
        }
      }
    }
    argc -= i;
    argv += i;
  }
#endif /* TMR_ENABLE_GETOPT */

  if (argc < 1)
  {
    printf("Usage: demo reader-uri <command> [args]\n"
           "  (URI: 'tmr:///COM1' or 'tmr://astra-2100d3/' "
           "or 'tmr:///dev/ttyS0')\n\n");
    exit(1);
  }
 // catch signals
 set_signals();
  
  uri = argv[0];

  // open port and create the reader structure
  rp = &r;
  ret = TMR_create(rp, uri);
  if (TMR_SUCCESS != ret)
  {
    errx(1, "Error creating reader: %s\n", TMR_strerr(rp, ret));
  }

  // set output
  if (TMR_READER_TYPE_SERIAL == rp->readerType)
  {
    tb.listener = serialPrinter;
  }
  else
  {
    tb.listener = stringPrinter;
  }

  tb.cookie = stdout;

  if (verbose)
  {
    listener = &tb;
    TMR_addTransportListener(rp, listener);
  }
  


  // connect to reader on port
  ret = TMR_connect(rp);
/* 
 ret = TMR_reboot(rp);
 if (TMR_SUCCESS != ret)

{
    printf("success\n");
}
else
 printf("no success\n");
TMR_destroy(rp);
exit(0);
*/ 
  if (TMR_SUCCESS != ret)
  {
    switch (ret)
    {
      case TMR_ERROR_BL_INVALID_IMAGE_CRC:
      case TMR_ERROR_BL_INVALID_APP_END_ADDR:
        fprintf(stderr, "Error: App image corrupt.  Call firmware load\n");
        goto LEVEL;
        break;
      default:
        errx(1, "Error connecting reader: %s\n", TMR_strerr(rp, ret));
        break;
    }
  }
  
  // set connected flag
  connected = true;

    // check for Nano
    if (check_model(rp) != TMR_SUCCESS)  closeout();
    
    // set region to Europe
    if (set_region(rp, TMR_REGION_EU3) != TMR_SUCCESS) closeout();
 
    // set antenne_power
    if (set_antenne_power(rp, 2000, 2000) != TMR_SUCCESS) exit(3);

LEVEL:
#ifdef TMR_ENABLE_READLINE
  if (isatty(STDIN_FILENO))
  {
    getcommand = getcommand_interactive;
    rl_readline_name = "TMDemo";
    rl_attempted_completion_function = demo_completion;
  }
  else
#endif /* TMR_ENABLE_READLINE */
  {
    getcommand = getcommand_noninteractive;
  }
  if (argc > 1)
  {
    runcmd(argc - 1, argv + 1);
  }
  else
  {
    while (NULL != (line = getcommand()))
    {
      if (*line == '\0')
      {
        continue;
      }

#ifdef TMR_ENABLE_READLINE
      add_history(line);
#endif /* TMR_ENABLE_READLINE */

      arg = strtok_r(line, " \t", &saveptr);
      for (argc = 0; arg && argc < 16; argc++)
      {
        if ('#' == arg[0])
        {
          break;
        }
        args[argc] = arg;
        arg = strtok_r(NULL, " \t", &saveptr);
      }

      if (argc == 0)
      {
        continue;
      }

      runcmd(argc, args);
      free(line);
    }
  }


  TMR_destroy(rp);
  return 0;
}

void
errx(int exitval, const char *fmt, ...)
{
  va_list ap;

  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);

  if (connected)
  {
    TMR_destroy(rp);
  }
  exit(exitval);
}

void
serialPrinter(bool tx, uint32_t dataLen, const uint8_t data[],
              uint32_t timeout, void *cookie)
{
  FILE *out;
  uint32_t i;

  out = cookie;

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
}

void stringPrinter(bool tx,uint32_t dataLen, const uint8_t data[],uint32_t timeout, void *cookie)
{
  FILE *out = cookie;

  fprintf(out, "%s", tx ? "Sending: " : "Received:");
  fprintf(out, "%s\n", data);
}

struct commandtable
{
  const char *name;
  void (*func)(int argc, char *argv[]);
  int minargs;
  const char *shortDoc, *usage, *doc;
};


struct commandtable commands[] =
{
  {"quit", closeout, 0,
   "close program",
   "quit [ret]",
   "ret -- exit code"
   "quit\n"
   "quit 1"},
   
  {"read", readcmd, 0,
   "Search for tags",
   "read [timeout]",
   "timeout -- Number of milliseconds to search.  Defaults to 1000\n\n"
   "read\n"
   "read 3000"},

  {"readinto", readintocmd, 0,
   "Search for tags",
   "read [timeout]",
   "timeout -- Number of milliseconds to search.  Defaults to 1000\n\n"
   "read\n"
   "read 3000"},

  {"writeepc", writeepccmd, 1,
   "Write tag EPC",
   "writeepc epc [target]",
   "epc -- EPC to write to tag\n"
   "target -- Tag to write\n\n"
   "writeepc EPC:0123456789ABCDEF01234567\n"
   "writeepc EPC:4B4F6C095977225FC81B6A0F EPC:0123456789ABCDEF01234567"},

  {"readmemwords", readmemwordscmd, 3,
   "Read tag memory wordwise",
   "readmemwords bank addr len [target]",
   "bank -- Memory bank number\n"
   "addr -- Word address within memory bank\n"
   "len -- Number of words to read\n"
   "target -- Tag whose memory to read\n\n"
   "readmemwords 1 2 6\n"
   "readmemwords 1 2 6 EPC:E2003411B802011083257015"},

  {"readmembytes", readmembytescmd, 3,
   "Read tag memory bytewise",
   "readmemwords bank addr len [target]",
   "bank -- Memory bank number\n"
   "addr -- Byte address within memory bank\n"
   "len -- Number of bytes to read\n"
   "target -- Tag whose memory to read\n\n"
   "readmemwords 1 4 12\n"
   "readmemwords 1 4 12 EPC:E2003411B802011083257015"},

  {"writememwords", writememwordscmd, 3,
   "Write tag memory wordwise",
   "writememwords bank addr data [target]",
   "bank -- Memory bank number\n"
   "addr -- Word address within memory bank\n"
   "data -- Words to write\n"
   "target -- Tag whose memory to write\n\n"
   "writememwords 1 2 words:1234\n"
   "writememwords 1 2 words:12345678 EPC:E2003411B802011083257015"},

  {"writemembytes", writemembytescmd, 3,
   "Write tag memory bytewise",
   "writemembytes bank addr data [target]",
   "bank -- Memory bank number\n"
   "addr -- Byte address within memory bank\n"
   "data -- Bytes to write\n"
   "target -- Tag whose memory to write\n\n"
   "writemembytes 1 4 bytes:1234\n"
   "writemembytes 1 4 bytes:12345678 EPC:E2003411B802011083257015"},

  {"kill", killcmd, 1,
   "Kill tag",
   "kill password [target]",
   "password -- Kill password\n"
   "target - Tag to kill\n\n"
   "kill 0x12345678\n"
   "kill 0x12345678 EPC:044D00000000040007010062"},

  {"lock", lockcmd, 1,
   "Lock (or unlock) tag memory",
   "lock actionlist [target]",
   "actionlist -- List of lock actions to apply to tag\n"
   "target - Tag to act on\n\n"
   "lock Gen2.Lock:EPC_LOCK,EPC_PERMALOCK,USER_UNLOCK\n"
   "lock Gen2.Lock:EPC_LOCK,EPC_PERMALOCK,USER_UNLOCK EPC:044D00000000040007010062"},

  {"get-gpio", gpigetcmd, 0,
   "Read input pins",
   "get-gpio"},

  {"set-gpio", gposetcmd, 2,
   "Write output pin(s)",
   "set-gpio pin value [pin value]...",
   "pin -- Pin number\n"
   "value -- Pin state: True or 1 to set pin high, False or 0 to set pin low\n\n"
   "set-gpio 1 1\n"
   "set-gpio 1 1 2 1\n"
   "set-gpio 1 false 2 false\n"},

  {"loadfw", loadfwcmd, 1,
   "Reload reader firmware",
   "loadfw filename",
   "filename -- Name of firmware file to load\n\n"
   "loadfw M5eApp_1.0.37.85-20081001-0412.sim"},

  {"get", getcmd, 0,
   "Get reader parameter",
   "get [name...]",
   "With no arguments, gets all reader parameters\n"
   "With one or more arguments, gets each named parameter\n"
   "\n"
   "get\n"
   "get model\n"
   "get softwareVersion supportedProtocols\n"},

  {"set", setcmd, 2,
   "Set reader parameter",
   "set name value\n",
   "Examples:\n"
   "set gen2Session 1\n"
   "set singulation EPC:1234567890ABCDEF1234567\n"
   "set singulation null\n"
   "set readPlan [1,2]\n"
   "set readPlan AutoReadPlan:"},

  {"list", listcmd, 0,
   "List all reader parameters",
   "list",
   "\n"
   "list\n"},

  /* Demo-program user interface commands, no reader interaction */
  {"help", helpcmd, 0,
   "Print program usage instructions",
   "help [command]",
   "With no arguments, prints a list of available commands.\n"
   "With a command name, prints detailed help for that command."},

  {"trace", tracecmd, 1,
   "Enable or disable communication tracing",
   "trace on|off", 
   ""},

  {"echo", echocmd, 0,
   "Echo text",
   "echo [text]",
   "\n"
   "echo hello"},

  {"sleep", sleepcmd, 1,
   "Do nothing for a certain number of milliseconds",
   "sleep [timeout]",
   "timeout -- Number of milliseconds to wait\n\n"
   "sleep 3000"},

   {"testregcmds",regulatoryTestCmds, 0, "Hard-coded test of regulatory commands"},
   {"testsavedconfig",savedConfig, 0, "Hard-coded test of user profile configurations"},
   {"testblockwrite",blockWriteCmds, 0, "Hard-coded test for Block Write"},
   {"testblockpermalock",blockPermalockCmds, 0, "Hard-coded test for Block Permalock"},
   {"tagmetadata",tagMetaData, 0, "printing phase and gpio state of tag metadata"},
   {"testgpiodirection",gpiodirection, 0, "verifying gpio directionality"},
};

int numcommands = numberof(commands);

void
runcmd(int argc, char *argv[])
{
  int i;

  for (i = 0; i < numcommands; i++)
  {
    if (0 == strcmp(argv[0], commands[i].name))
    {
      if (argc - 1 < commands[i].minargs)
      {
        printf("Not enough arguments to command %s\n",
               commands[i].name);
      }
      else
      {
        commands[i].func(argc-1, argv+1);
      }
      break;
    }      
  }
  if (i == numcommands)
  {
    printf("No command '%s'. Try 'help'.\n", argv[0]);
  }
}

/* close program correctly */
void
closeout()
{
    TMR_destroy(rp);
    exit(0);
}

void
readcmd(int argc, char *argv[])
{
  int timeout;
  TMR_Status ret;

    // set timeout value
  timeout = 1000;
  if (argc > 0)
  {
    timeout = atoi(argv[0]);
  }

  ret = TMR_read(rp, timeout, NULL);
  if (TMR_SUCCESS != ret)
  {
    printf("Error reading tags: %s\n", TMR_strerr(rp, ret));
    return;
  }
  
  while (TMR_SUCCESS == TMR_hasMoreTags(rp))
  {
    TMR_TagReadData trd;
    char tmpStr[128];
    uint8_t data[500];

    trd.data.max = 500;
    trd.data.list = data;

    ret = TMR_getNextTag(rp, &trd);
    if (TMR_SUCCESS != ret)
    {
      printf("Error fetching tag: %s\n", TMR_strerr(rp, ret));
      return;
    }

    TMR_bytesToHex(trd.tag.epc, trd.tag.epcByteCount, tmpStr);
    printf("EPC:%s ant:%d count:%"PRIu32, tmpStr, trd.antenna, trd.readCount);
    
    if ((trd.metadataFlags & TMR_TRD_METADATA_FLAG_DATA)
        && trd.data.len > 0)
    {
      TMR_bytesToHex(data, trd.data.len, tmpStr);
      printf(" data:%s", tmpStr);
  }
    printf("\n");
  }

}

void
tagMetaData(int argc, char *argv[])
{
  TMR_Status ret;
  ret = TMR_read(rp, 1000, NULL);
  if (TMR_SUCCESS != ret)
  {
    printf("Error reading tags: %s\n", TMR_strerr(rp, ret));
    return;
  }
  
  while (TMR_SUCCESS == TMR_hasMoreTags(rp))
  {
    TMR_TagReadData trd;
    uint8_t data[32];

    trd.data.max = 32;
    trd.data.list = data;

    ret = TMR_getNextTag(rp, &trd);
    if (TMR_SUCCESS != ret)
    {
      printf("Error fetching tag: %s\n", TMR_strerr(rp, ret));
      return;
    }
    printf("Phase:%d",trd.phase);
    printf("  GPIO state:[");
    {
      int i;
      for (i=0; i<trd.gpioCount; i++)
      {
	TMR_GpioPin* pin = &trd.gpio[i];
	printf("%s%d:%s",
	       (i>0)?",":"",
	       pin->id,
	       (pin->high)?"H":"L");
      }
    }
    printf("]");

    printf("\n");
  }
}


void
readintocmd(int argc, char *argv[])
{
  int timeout;
  int32_t tagCount, i;
  TMR_TagReadData* result;
  TMR_Status ret;

  timeout = 1000;
  if (argc > 0)
  {
    timeout = atoi(argv[0]);
  }


  ret = TMR_readIntoArray(rp, timeout, &tagCount, &result);
  if (TMR_SUCCESS != ret)
  {
    printf("Error reading tags: %s\n", TMR_strerr(rp, ret));
    return;
  }
  
  for (i=0; i<tagCount; i++)
  {
    TMR_TagReadData* ptrd;
    char tmpStr[128];
    uint8_t data[32];

    ptrd = &result[i];
    TMR_bytesToHex(ptrd->tag.epc, ptrd->tag.epcByteCount, tmpStr);
    printf("EPC:%s ant:%d count:%"PRIu32, tmpStr, ptrd->antenna, ptrd->readCount);
    if ((ptrd->metadataFlags & TMR_TRD_METADATA_FLAG_DATA)
        && ptrd->data.len > 0)
    {
      TMR_bytesToHex(data, ptrd->data.len, tmpStr);
      printf(" data:%s", tmpStr);
    }
    printf("\n");
  }
}

void
writeepccmd(int argc, char *argv[])
{
  TMR_Status ret;
  TMR_TagData newepc;
  TMR_TagFilter *filter;

  filter = NULL;

  if (-1 == parseEpc(argv[0], &newepc, NULL))
  {
    printf("Can't parse '%s' as a tag EPC.\n", argv[0]);
    return;
  }

  if (argc > 1)
  {
    if (-1 == parseFilter(argv[1], &filter, NULL))
    {
      printf("Can't parse '%s' as a tag filter.\n", argv[1]);
      return;
    }
  }

  ret = TMR_writeTag(rp, filter, &newepc);
  free(filter);
  if (TMR_SUCCESS != ret)
  {
    printf("Error writing tag EPC: %s\n", TMR_strerr(rp, ret));
  }
}

void
readmemwordscmd(int argc, char *argv[])
{
  TMR_Status ret;
  TMR_TagFilter *filter;
  uint32_t bank, address, count;
  uint16_t *buf;

  filter = NULL;
  bank = atoi(argv[0]);
  address = atoi(argv[1]);
  count = atoi(argv[2]);

  if (argc > 3)
  {
    if (-1 == parseFilter(argv[3], &filter, NULL))
    {
      printf("Can't parse '%s' as tag filter\n", argv[3]);
      return;
    }
  }

  buf = malloc(count * sizeof(*buf));

  ret = TMR_readTagMemWords(rp, filter, bank, address, (uint16_t)count, buf);
  free(filter);
  if (TMR_SUCCESS != ret)
  {
    printf("Error reading tag memory: %s\n", TMR_strerr(rp, ret));
  }
  else
  {
    uint32_t i;
    printf("words:");
    for (i = 0 ; i < count; i++)
    {
      printf("%04X", buf[i]);
    }
    printf("\n");
  }
  free(buf);
}

void
readmembytescmd(int argc, char *argv[])
{
  TMR_Status ret;
  TMR_TagFilter *filter;
  uint32_t bank, address, count;
  uint8_t *buf;

  filter = NULL;
  bank = atoi(argv[0]);
  address = atoi(argv[1]);
  count = atoi(argv[2]);

  if (argc > 3)
  {
    if (-1 == parseFilter(argv[3], &filter, NULL))
    {
      printf("Can't parse '%s' as tag filter\n", argv[3]);
      return;
    }
  }

  buf = malloc(count * sizeof(*buf));

  ret = TMR_readTagMemBytes(rp, filter, bank, address, (uint16_t)count, buf);
  free(filter);
  if (TMR_SUCCESS != ret)
  {
    printf("Error reading tag memory: %s\n", TMR_strerr(rp, ret));
  }
  else
  {
    uint32_t i;
    printf("bytes:");
    for (i = 0 ; i < count; i++)
    {
      printf("%02X", buf[i]);
    }
    printf("\n");
  }
  free(buf);
}

void
writememwordscmd(int argc, char *argv[])
{
  TMR_Status ret;
  TMR_TagFilter *filter;
  uint32_t bank, address;
  TMR_uint16List words;
  
  filter = NULL;
  bank = atoi(argv[0]);
  address = atoi(argv[1]);

  if (-1 == parseWords(argv[2], &words))
  {
    printf("Can't parse '%s' as word data\n", argv[2]);
    return;
  }

  if (argc > 3)
  {
    if (-1 == parseFilter(argv[3], &filter, NULL))
    {
      printf("Can't parse '%s' as tag filter\n", argv[3]);
      goto out;
    }
  }

  ret = TMR_writeTagMemWords(rp, filter, bank, address, 
                             words.len, words.list);
  free(filter);
  if (TMR_SUCCESS != ret)
  {
    printf("Error writing tag memory: %s\n", TMR_strerr(rp, ret));
  }

out:
  free(words.list);
}

void
writemembytescmd(int argc, char *argv[])
{
  TMR_Status ret;
  TMR_TagFilter *filter;
  uint32_t bank, address;
  TMR_uint8List bytes;
  
  filter = NULL;
  bank = atoi(argv[0]);
  address = atoi(argv[1]);

  if (-1 == parseBytes(argv[2], &bytes))
  {
    printf("Can't parse '%s' as byte data\n", argv[2]);
    return;
  }

  if (argc > 3)
  {
    if (-1 == parseFilter(argv[3], &filter, NULL))
    {
      printf("Can't parse '%s' as tag filter\n", argv[3]);
      goto out;
    }
  }

  ret = TMR_writeTagMemBytes(rp, filter, bank, address, 
                             bytes.len, bytes.list);
  free(filter);
  if (TMR_SUCCESS != ret)
  {
    printf("Error writing tag memory: %s\n", TMR_strerr(rp, ret));
  }

out:
  free(bytes.list);
}


void
lockcmd(int argc, char *argv[])
{
  TMR_Status ret;
  TMR_TagLockAction la;
  TMR_TagFilter *filter = NULL;
  
  if (-1 == parseLockAction(argv[0], &la))
  {
    printf("Can't parse '%s' as a tag lock action\n", argv[0]);
  }

  if (argc > 1)
  {
    if (-1 == parseFilter(argv[1], &filter, NULL))
    {
      printf("Can't parse '%s' as tag filter\n", argv[3]);
      return;
    }
  }

  ret = TMR_lockTag(rp, filter, &la);
  if (TMR_SUCCESS != ret)
  {
    printf("Error locking tag: %s\n", TMR_strerr(rp, ret));
  }
  free(filter);
}


void
killcmd(int argc, char *argv[])
{
  TMR_Status ret;
  TMR_TagFilter *filter = NULL;
  TMR_TagAuthentication auth;

  if (1 != sscanf(argv[0], "%"SCNx32, &auth.u.gen2Password))
  {
    printf("Can't parse '%s' as a password (32-bit hex value)\n",
           argv[0]);
    return;
  }
  auth.type = TMR_AUTH_TYPE_GEN2_PASSWORD;

  if (argc > 1)
  {
    if (-1 == parseFilter(argv[3], &filter, NULL))
    {
      printf("Can't parse '%s' as tag filter\n", argv[3]);
      return;
    }
  }
  
  ret = TMR_killTag(rp, filter, &auth);
  if (TMR_SUCCESS != ret)
  {
    printf("Error killing tag: %s\n", TMR_strerr(rp, ret));
  }
  free(filter);
}


void
gpigetcmd(int argc, char *argv[])
{
  TMR_Status ret;
  TMR_GpioPin state[16];
  uint8_t i, stateCount = numberof(state);

  ret = TMR_gpiGet(rp, &stateCount, state);
  if (TMR_SUCCESS != ret)
  {
    printf("Error reading GPIO pins: %s\n", TMR_strerr(rp, ret));
    return;
  }
  printf("stateCount: %d\n", stateCount);
  for (i = 0 ; i < stateCount ; i++)
  {
    printf("Pin %d: %s\n", state[i].id, state[i].high ? "High" : "Low"); 
  }
}


void
gposetcmd(int argc, char *argv[])
{
  TMR_Status ret;
  TMR_GpioPin *state;
  uint8_t stateCount;
  int i, j;

  if (argc & 1)
  {
    printf("Invalid number of arguments - need an even number of items\n");
    return;
  }

  stateCount = argc / 2;
  state = malloc(stateCount * sizeof(*state));
  for (i = 0, j = 0 ; i < argc ; i += 2, j++)
  {
    printf("argv[i]='%s' argv[i+1]='%s'\n", argv[i],argv[i+1]);
    state[j].id = atoi(argv[i]);
    if (0 == strcasecmp("true", argv[i+1]) ||
        0 == strcasecmp("high", argv[i+1]) ||
        0 == strcasecmp("1", argv[i+1]))
    {
      state[j].high = true;
    }
    else if(0 == strcasecmp("false", argv[i+1]) ||
            0 == strcasecmp("low", argv[i+1]) ||
            0 == strcasecmp("0", argv[i+1]))
    {
      state[j].high = false;
    }
    else
    {
      printf("Can't parse '%s' as GPO state\n", argv[i+1]);
      return;
    }
    printf("state[j].id=%d state[j].high=%s\n",state[j].id,
           state[j].high ? "true" : "false");
  }
  
  ret = TMR_gpoSet(rp, stateCount, state);
  if (TMR_SUCCESS != ret)
  {
    printf("Error setting GPIO pins: %s\n", TMR_strerr(rp, ret));
  }
  
  free(state);
}

void
loadfwcmd(int argc, char *argv[])
{
  TMR_Status ret;
  FILE *f;

  f = fopen(argv[0], "rb");
  if (NULL == f)
  {
    printf("Error opening file '%s': %s\n", argv[0], strerror(errno));
    return;
  }

  ret = TMR_firmwareLoad(rp, f, TMR_fileProvider);
  if (TMR_SUCCESS != ret)
  {
    printf("Error loading firmware: %s\n", TMR_strerr(rp, ret));
  }

  fclose(f);
}

void
printU8List(TMR_uint8List *list)
{
  int i;

  putchar('[');
  for (i = 0; i < list->len && i < list->max; i++)
  {
    printf("%u%s", list->list[i],
           ((i + 1) == list->len) ? "" : ",");
  }
  if (list->len > list->max)
  {
    printf("...");
  }
  putchar(']');
}


void
printU32List(TMR_uint32List *list)
{
  int i;

  putchar('[');
  for (i = 0; i < list->len && i < list->max; i++)
  {
    printf("%"PRIu32"%s", list->list[i],
           ((i + 1) == list->len) ? "" : ",");
  }
  if (list->len > list->max)
  {
    printf("...");
  }
  putchar(']');
}



void
printPortValueList(TMR_PortValueList *list)
{
  int i;

  putchar('[');
  for (i = 0; i < list->len && i < list->max; i++)
  {
    printf("[%u,%u]%s", list->list[i].port, list->list[i].value,
           ((i + 1) == list->len) ? "" : ",");
  }
  if (list->len > list->max)
  {
    printf("...");
  }
  putchar(']');
}

static const char *hexChars = "0123456789abcdefABCDEF";
static const char *decimalChars = "0123456789";

static const char *regions[] = {"UNSPEC", "NA", "EU", "KR", "IN", "JP", "PRC",
                                "EU2", "EU3", "KR2", "PRC2", "AU", "NZ", "REDUCED_FCC"};
static const char *powerModes[] = {"FULL", "MINSAVE", "MEDSAVE", "MAXSAVE", "SLEEP"};
static const char *userModes[] = {"NONE", "PRINTER", NULL, "PORTAL"};

static const char *tagEncodingNames[] = {"FM0", "M2", "M4", "M8"};
static const char *sessionNames[] = {"S0", "S1", "S2", "S3"};
static const char *targetNames[] = {"A", "B", "AB", "BA"};
static const char *gen2LinkFrequencyNames[] = {"250kHz", "300kHz", "320kHz", "40kHz", "640KHz"};
static const char *tariNames[] = {"25us", "12.5us", "6.25us"};

static const char *iso180006bLinkFrequencyNames[] = {"40kHz", "160kHz"};

static const char *iso180006bModulationDepthNames[] = {"99 percent", "11 percent"};

static const char *iso180006bDelimiterNames[] = {"", "Delimiter1", "", "", "Delimiter4"};
static const char *protocolNames[] = {NULL, NULL, NULL, "ISO180006B",
                                      NULL, "GEN2", "UCODE", "IPX64", "IPX256"};

static const char *bankNames[] = {"Reserved", "EPC", "TID", "User"};
static const char *selectOptionNames[] = {"EQ", "NE", "GT", "LT"};

static const char *tagopNames[] = {"Gen2.Read","Gen2.Write","Gen2.Lock",
                                   "Gen2.Kill"};

const char *listname(const char *list[], int listlen, unsigned int id)
{
  if ((id < (unsigned)listlen) && list[id] != NULL)
  {
    return (list[id]);
  }
  return ("Unknown");
}

bool
strcasecmpcount(const char *a, const char *b, int *matches)
{

  int i = 0;

  while (*a && *b
         && (tolower((unsigned char)*a) == tolower((unsigned char)*b)))
  {
    i++;
    a++;
    b++;
  }

  if (matches)
    *matches = i;

  return (*a == *b);
}

int listid(const char *list[], int listlen, const char *name)
{
  int i, len, match, bestmatch, bestmatchindex;

  bestmatch = 0;
  bestmatchindex = -1;
  len = (int)strlen(name);
  for (i = 0; i < listlen; i++)
  {
    if (NULL != list[i])
    {
      if (strcasecmpcount(name, list[i], &match))
      {
        return i; /* Exact match - return immediately */
    }
      if (match == len)
      {
        if (bestmatch == 0)
        {
          /* Prefix match - return if nothing else conflicts */
          bestmatch = match;
          bestmatchindex = i;
  }
        else
        {
          /* More than one prefix match of the same length - ambiguous */
          bestmatchindex = -1;
        }
      }
    }
  }

  return bestmatchindex;
}

static const char *regionName(TMR_Region region)
{

  if (region == TMR_REGION_OPEN)
  {
    return "OPEN";
  }
  return listname(regions, numberof(regions), region);
}

static TMR_Region regionID(const char *name)
{

  if (0 == strcasecmp(name, "OPEN"))
  {
    return TMR_REGION_OPEN;
  }

  return listid(regions, numberof(regions), name);
}


void
printReadPlan(TMR_ReadPlan *plan)
{

  printf("[");
  if (TMR_READ_PLAN_TYPE_MULTI == plan->type)
  {
    int i;

    for (i = 0 ; i < plan->u.multi.planCount; i++)
    {
      printReadPlan(plan->u.multi.plans[i]);
      if (i + 1 !=  plan->u.multi.planCount)
      {
        printf(",");
      }
    }
  }
  else
  {
    printf("%s,", listname(protocolNames, numberof(protocolNames), 
                           plan->u.simple.protocol));
    printU8List(&plan->u.simple.antennas);
    printf(",");
    printFilter(plan->u.simple.filter);
    printf(",");
    printTagop(plan->u.simple.tagop);
  }
  printf(",%"PRIu32"]", plan->weight);
}

void
printFilter(TMR_TagFilter *filter)
{
  char tmpStr[128];

  if (NULL == filter)
  {
    printf("NULL");
  }
  else if (TMR_FILTER_TYPE_TAG_DATA == filter->type)
  {
    TMR_bytesToHex(filter->u.tagData.epc, filter->u.tagData.epcByteCount,
                   tmpStr);
    printf("EPC:%s", tmpStr);
  }
  else if (TMR_FILTER_TYPE_GEN2_SELECT == filter->type)
  {
    TMR_bytesToHex(filter->u.gen2Select.mask,
                   (filter->u.gen2Select.maskBitLength + 7) / 8,
                   tmpStr);
    printf("Gen2.Select:%s%s,%"PRIu32",%d,%s",
           filter->u.gen2Select.invert ? "~" : "",
           listname(bankNames, numberof(bankNames), filter->u.gen2Select.bank),
           filter->u.gen2Select.bitPointer,
           filter->u.gen2Select.maskBitLength,
           tmpStr);
  }
  else
    printf("Unknown");
}

void
printTagop(TMR_TagOp *tagop)
{

  if (NULL == tagop)
  {
    printf("NULL");
  }
  else
  {
    switch (tagop->type)
    {
    case TMR_TAGOP_GEN2_READDATA:
      printf("Gen2.Read:%s,%d,%d",
             listname(bankNames, numberof(bankNames),
                      tagop->u.gen2.u.readData.bank),
             tagop->u.gen2.u.readData.wordAddress,
             tagop->u.gen2.u.readData.len);
      break;
    case TMR_TAGOP_GEN2_WRITEDATA:
    case TMR_TAGOP_GEN2_LOCK:
    case TMR_TAGOP_GEN2_KILL:
    case TMR_TAGOP_LIST:
      default:
      printf("Unknown");
    }
  }
}

void
getoneparam(const char *paramName)
{
  TMR_Status ret;
  TMR_Param param;

  ret = TMR_SUCCESS;

  printf("%s: ", paramName);
  param = TMR_paramID(paramName);
  if (TMR_PARAM_NONE == param)
    printf("No such parameter\n");
  else if (TMR_PARAM_BAUDRATE == param
           || TMR_PARAM_COMMANDTIMEOUT == param
           || TMR_PARAM_TRANSPORTTIMEOUT == param
           || TMR_PARAM_READ_ASYNCOFFTIME == param
           || TMR_PARAM_READ_ASYNCONTIME == param
           || TMR_PARAM_REGION_HOPTIME == param
           || TMR_PARAM_TAGREADDATA_READFILTERTIMEOUT == param)
  {
    uint32_t value;
    ret = TMR_paramGet(rp, param, &value);
    if (TMR_SUCCESS != ret)
    {
      goto out;
    }
    printf("%"PRIu32"\n", value);
  }
  else if (TMR_PARAM_GEN2_ACCESSPASSWORD == param)
  {
    TMR_GEN2_Password value;
    ret = TMR_paramGet(rp, param, &value);
    if (TMR_SUCCESS != ret)
    {
      goto out;
    }
    printf("0x%"PRIx32"\n", value);
  }
  else if(TMR_PARAM_RADIO_READPOWER == param
          || TMR_PARAM_RADIO_WRITEPOWER == param)
  {
    int32_t value;
    ret = TMR_paramGet(rp, param, &value);
    if (TMR_SUCCESS != ret)
    {
      goto out;
    }
    printf("%d\n", value);
  }
  else if (TMR_PARAM_RADIO_POWERMAX == param
           || TMR_PARAM_RADIO_POWERMIN == param          
           || TMR_PARAM_TAGREADATA_TAGOPSUCCESSCOUNT == param
           || TMR_PARAM_TAGREADATA_TAGOPFAILURECOUNT == param
           || TMR_PARAM_PRODUCT_GROUP_ID == param)
  {
    uint16_t value;
    ret = TMR_paramGet(rp, param, &value);
    if (TMR_SUCCESS != ret)
    {
      goto out;
    }
    printf("%u\n", value);
  }
  else if (TMR_PARAM_RADIO_TEMPERATURE == param
           || TMR_PARAM_TAGOP_ANTENNA == param)
  {
    uint8_t value;
    ret = TMR_paramGet(rp, param, &value);
    if (TMR_SUCCESS != ret)
    {
      goto out;
    }
    printf("%u\n", value);
  }
  else if (TMR_PARAM_ANTENNA_CHECKPORT == param
	         || TMR_PARAM_RADIO_ENABLEPOWERSAVE == param
           || TMR_PARAM_RADIO_ENABLESJC == param
           || TMR_PARAM_EXTENDEDEPC == param
           || TMR_PARAM_TAGREADDATA_RECORDHIGHESTRSSI == param
           || TMR_PARAM_TAGREADDATA_REPORTRSSIINDBM == param
           || TMR_PARAM_TAGREADDATA_UNIQUEBYANTENNA == param
           || TMR_PARAM_TAGREADDATA_UNIQUEBYDATA == param
           || TMR_PARAM_REGION_LBT_ENABLE == param
           || TMR_PARAM_TAGREADDATA_ENABLEREADFILTER == param)
  {
    bool value;
    ret = TMR_paramGet(rp, param, &value);
    if (TMR_SUCCESS != ret)
    {
      goto out;
    }
    printf("%s\n", value ? "true" : "false");
  }
  else if (TMR_PARAM_ANTENNA_PORTLIST == param
           || TMR_PARAM_ANTENNA_CONNECTEDPORTLIST == param
           || TMR_PARAM_ANTENNA_PORTSWITCHGPOS == param
           || TMR_PARAM_GPIO_INPUTLIST == param
           || TMR_PARAM_GPIO_OUTPUTLIST == param)
  {
    TMR_uint8List value;
    uint8_t valueList[64];

    value.max = numberof(valueList);
    value.list = valueList;
    
    ret = TMR_paramGet(rp, param, &value);
    if (TMR_SUCCESS != ret)
    {
      goto out;
    }
    printU8List(&value);
    putchar('\n');
  }
  else if (TMR_PARAM_REGION_HOPTABLE == param)
  {
    TMR_uint32List value;
    uint32_t valueList[64];

    value.max = numberof(valueList);
    value.list = valueList;
    
    ret = TMR_paramGet(rp, param, &value);
    if (TMR_SUCCESS != ret)
    {
      goto out;
    }
    printU32List(&value);
    putchar('\n');
  }
  else if (TMR_PARAM_ANTENNA_TXRXMAP == param)
  {
    TMR_AntennaMapList value;
    TMR_AntennaMap valueList[64];
    int i;

    value.max = numberof(valueList);
    value.list = valueList;

    ret = TMR_paramGet(rp, param, &value);
    if (TMR_SUCCESS != ret)
    {
      goto out;
    }

    putchar('[');
    for (i = 0; i < value.len && i < value.max; i++)
    {
      printf("[%d,%d,%d]%s", 
             value.list[i].antenna,
             value.list[i].txPort,
             value.list[i].rxPort,
             ((i + 1) == value.len) ? "" : ",");
    }
    if (value.len > value.max)
    {
      printf("...");
    }
    printf("]\n");
  }
  else if (TMR_PARAM_ANTENNA_SETTLINGTIMELIST == param
           || TMR_PARAM_RADIO_PORTREADPOWERLIST == param
           || TMR_PARAM_RADIO_PORTWRITEPOWERLIST == param)
  {
    TMR_PortValueList value;
    TMR_PortValue valueList[64];

    value.max = numberof(valueList);
    value.list = valueList;

    ret = TMR_paramGet(rp, param, &value);
    if (TMR_SUCCESS != ret)
    {
      goto out;
    }
    printPortValueList(&value);
    printf("\n");
  }
  else if (TMR_PARAM_VERSION_HARDWARE == param
           || TMR_PARAM_VERSION_SERIAL == param
           || TMR_PARAM_VERSION_MODEL == param
           || TMR_PARAM_VERSION_SOFTWARE == param
           || TMR_PARAM_URI == param
           || TMR_PARAM_PRODUCT_GROUP == param)
  {
    TMR_String value;
    char stringValue[256];

    value.max = sizeof(stringValue);
    value.value = stringValue;

    ret = TMR_paramGet(rp, param, &value);
    if (TMR_SUCCESS != ret)
    {
      goto out;
    }
    printf("%s\n", value.value);
  }
  else if (TMR_PARAM_REGION_ID == param)
  {
    TMR_Region region;

    ret = TMR_paramGet(rp, param, &region);
    if (TMR_SUCCESS != ret)
    {
      goto out;
    }
    printf("%s\n", regionName(region));
  }
  else if (TMR_PARAM_USERMODE == param)
  {
    TMR_SR_UserMode mode;

    ret = TMR_paramGet(rp, param, &mode);
    if (TMR_SUCCESS != ret)
    {
      goto out;
    }

    printf("%s\n", listname(userModes, numberof(userModes), mode));
  }
  else if (TMR_PARAM_POWERMODE == param)
  {
    TMR_SR_PowerMode mode;

    ret = TMR_paramGet(rp, param, &mode);
    if (TMR_SUCCESS != ret)
    {
      goto out;
    }
    printf("%s\n", listname(powerModes, numberof(powerModes), mode));
  }
  else if (TMR_PARAM_GEN2_TAGENCODING == param)
  {
    TMR_GEN2_TagEncoding value;

    ret = TMR_paramGet(rp, param, &value);
    if (TMR_SUCCESS != ret)
    {
      goto out;
    }
    printf("%s\n", listname(tagEncodingNames, numberof(tagEncodingNames), value));
  }
  else if (TMR_PARAM_GEN2_SESSION == param)
  {
    TMR_GEN2_Session value;

    ret = TMR_paramGet(rp, param, &value);
    if (TMR_SUCCESS != ret)
    {
      goto out;
    }
    printf("%s\n", listname(sessionNames, numberof(sessionNames), value));
  }
  else if (TMR_PARAM_GEN2_TARGET == param)
  {
    TMR_GEN2_Target value;

    ret = TMR_paramGet(rp, param, &value);
    if (TMR_SUCCESS != ret)
    {
      goto out;
    }
    printf("%s\n", listname(targetNames, numberof(targetNames), value));
  }
  else if (TMR_PARAM_GEN2_Q == param)
  {
    TMR_SR_GEN2_Q value;

    ret = TMR_paramGet(rp, param, &value);
    if (TMR_SUCCESS != ret)
    {
      goto out;
    }
    if (value.type == TMR_SR_GEN2_Q_DYNAMIC)
    {
      printf("DynamicQ\n");
    }
    else if (value.type == TMR_SR_GEN2_Q_STATIC)
    {
      printf("StaticQ(%d)\n", value.u.staticQ.initialQ);
    }
  }
  else if (TMR_PARAM_GEN2_BLF == param)
  {
    TMR_GEN2_LinkFrequency value;

    ret = TMR_paramGet(rp, param, &value);
    if (TMR_SUCCESS != ret)
    {
      goto out;
    }
#ifndef WIN32
    {
      int temp = value;
      switch (temp)
      {
        case 250:
          value = 0;
          break;
        case 640:
          value = 4;
          break;
        case 320:
          value = 2;
          break;
        default:;
      }
    }
#endif
    printf("%s\n", listname(gen2LinkFrequencyNames,
                            numberof(gen2LinkFrequencyNames),
                            value));
  }
  else if (TMR_PARAM_GEN2_TARI == param)
  {
    TMR_GEN2_Tari value;

    ret = TMR_paramGet(rp, param, &value);
    if (TMR_SUCCESS != ret)
    {
      goto out;
    }
    printf("%s\n", listname(tariNames,
                            numberof(tariNames),
                            value));
  }
  else if (TMR_PARAM_REGION_SUPPORTEDREGIONS == param)
  {
    TMR_RegionList value;
    TMR_Region valueList[32];
    int i;

    value.max = numberof(valueList);
    value.list = valueList;

    ret = TMR_paramGet(rp, param, &value);
    if (TMR_SUCCESS != ret)
    {
      goto out;
    }

    printf("[");
    for (i = 0; i < value.len && i < value.max; i++)
    {
      printf("%s%s", regionName(value.list[i]),
             ((i + 1) == value.len) ? "" : ",");
    }
    if (value.len > value.max)
    {
      printf("...");
    }
    printf("]\n");
  }
  else if (TMR_PARAM_VERSION_SUPPORTEDPROTOCOLS == param)
  {
    TMR_TagProtocolList value;
    TMR_TagProtocol valueList[32];
    int i;

    value.max = numberof(valueList);
    value.list = valueList;

    ret = TMR_paramGet(rp, param, &value);
    if (TMR_SUCCESS != ret)
    {
      goto out;
    }

    printf("[");
    for (i = 0; i < value.len && i < value.max; i++)
    {
      printf("%s%s", listname(protocolNames, numberof(protocolNames), 
                              value.list[i]),
             ((i + 1) == value.len) ? "" : ",");
    }
    if (value.len > value.max)
    {
      printf("...");
    }
    printf("]\n");
  }
  else if (TMR_PARAM_TAGOP_PROTOCOL == param)
  {
    TMR_TagProtocol value;

    ret = TMR_paramGet(rp, param, &value);
    if (TMR_SUCCESS != ret)
    {
      goto out;
    }
    printf("%s\n", listname(protocolNames, numberof(protocolNames), value));
  }
  else if (TMR_PARAM_READ_PLAN == param)
  {
    TMR_ReadPlan value;

    ret = TMR_paramGet(rp, param, &value);
    if (TMR_SUCCESS != ret)
    {
      goto out;
    }

    printReadPlan(&value);
    printf("\n");
  }
  else if (TMR_PARAM_ISO180006B_BLF == param)
  {
    TMR_ISO180006B_LinkFrequency lf;

    ret = TMR_paramGet(rp, param, &lf);
    if (TMR_SUCCESS != ret)
    {
      goto out;
    }
#ifdef TMR_ENABLE_LLRP_READER
    {
      int temp = lf;
      switch (temp)
      {
        case 40: lf = 0;
                 break;
        case 160: lf = 1;
                  break;
        default:;
      }
    }
#endif
    printf("%s\n", listname(iso180006bLinkFrequencyNames,
                            numberof(iso180006bLinkFrequencyNames),
                            lf));
  }
  else if (TMR_PARAM_LICENSE_KEY == param || TMR_PARAM_USER_CONFIG == param)
  {
    printf("Set only parameter\n");
  }
  else if (TMR_PARAM_READER_STATS == param)
  {
    TMR_Reader_StatsValues stats;
    //uint8_t i;

    TMR_STATS_init(&stats);
    ret = TMR_paramGet(rp, param, &stats);
    if (TMR_SUCCESS != ret)
    {
      goto out;
    }
    /**
     * TODO :need to add proper printing format as per the newly added flags
     */ 
    /*printf("\nAntenna | RF On time | Noise Floor | Noise Floor with TX On |\n");
    for (i = 0; i < stats.numPorts; i++)
    {
      printf("%6d | %10d | %10d | %10d |\n", i+1, stats.rfOnTime[i], stats.noiseFloor[i], stats.noiseFloorTxOn[i]);
    }*/
  }
  else if(TMR_PARAM_GEN2_WRITEMODE == param)
  {
    TMR_GEN2_WriteMode mode;
    
    ret = TMR_paramGet(rp, param, &mode);
    if (TMR_SUCCESS != ret)
    {
      goto out;
    }

    switch(mode)
    {
      case TMR_GEN2_WORD_ONLY:
        printf(" WORD ONLY\n");
        break;
      case TMR_GEN2_BLOCK_ONLY:
        printf(" BLOCK ONLY\n");
        break;
      case TMR_GEN2_BLOCK_FALLBACK:
        printf("BLOCK FALLBACK\n");
        break;     
    }
  }
  else if( TMR_PARAM_READER_WRITE_REPLY_TIMEOUT == param)
  {
    uint16_t value;

    ret = TMR_paramGet(rp, param, &value);
    if (TMR_SUCCESS != ret)
    {
      goto out;
    }

    printf("%d\n", value);

  }
  else if(TMR_PARAM_READER_WRITE_EARLY_EXIT == param)
  {
    bool value;

    ret = TMR_paramGet(rp, param, &value);
    if (TMR_SUCCESS != ret)
    {
      goto out;
    }

    printf("%s\n", value ? "true" : "false");

  }
#ifdef TMR_ENABLE_LLRP_READER
    else if(TMR_PARAM_READER_DESCRIPTION == param)
    {
      TMR_String value;
      char stringValue[256];

      value.max = sizeof(stringValue);
      value.value = stringValue;

      ret = TMR_paramGet(rp, param, &value);
      if (TMR_SUCCESS != ret)
      {
        goto out;
      }
      printf("%s\n", value.value);
    }
  else if(TMR_PARAM_CURRENTTIME == param)
  {
    char stringValue[256];
    struct tm  currentTime;

    ret = TMR_paramGet(rp, param, &currentTime);
    if (TMR_SUCCESS != ret)
    {
      goto out;
    }

    strftime(stringValue, sizeof(stringValue), "%FT%H:%M:%S", &currentTime);
    printf("%s\n", stringValue);
  }
#endif

#ifdef TMR_ENABLE_ISO180006B
  else if(TMR_PARAM_ISO180006B_MODULATION_DEPTH == param)
  {
    TMR_ISO180006B_ModulationDepth depth;

    ret = TMR_paramGet(rp, param, &depth);
    if (TMR_SUCCESS != ret)
    {
      goto out;
    }

    printf("%s\n", listname(iso180006bModulationDepthNames,
          numberof(iso180006bModulationDepthNames),
          depth));
  }
  else if(TMR_PARAM_ISO180006B_DELIMITER == param)
  {
    TMR_ISO180006B_Delimiter delimiter;

    ret = TMR_paramGet(rp, param, &delimiter);
    if (TMR_SUCCESS != ret)
    {
      goto out;
    }

    printf("%s\n", listname(iso180006bDelimiterNames,
          numberof(iso180006bDelimiterNames),
          delimiter));
  }
#endif
  else
  {
    printf("Don't know how to display value\n");
  }

out:
  if (TMR_SUCCESS != ret)
  {
    printf("Error retrieving value: %s\n", TMR_strerr(rp, ret));
  }
}

void
listcmd(int argc, char *argv[])
{
  TMR_Param keys[TMR_PARAM_MAX];
  uint32_t i, len;

  len = numberof(keys);
  TMR_paramList(rp, keys, &len);
  for (i = 0; i < len ; i++)
  {
    printf("%s\n", TMR_paramName(keys[i]));
  }
}

void
getcmd(int argc, char *argv[])
{

  if (argc == 0)
  {
    /* Get all parameters */
    TMR_Param keys[TMR_PARAM_MAX];
    uint32_t i, len;

    len = numberof(keys);
    TMR_paramList(rp, keys, &len);

    for (i = 0; i < len; i++)
    {
      getoneparam(TMR_paramName(keys[i]));
    }
  }
  else 
  {
    while (argc)
    {
      getoneparam(*argv);
      argc--;
      argv++;
    }
  }
}

/* Parse strings of the form [1,2,3,4] into a TMR_uint32List */
int
parseU32List(const char *arg, TMR_uint32List *list, int *nchars)
{
  int value;
  int chars;
  const char *origarg;


  origarg = arg;

  if ('[' != arg[0])
  {
    return -1;
  }

  arg++;

  list->len = 0;
  list->max = 4;
  list->list = malloc(sizeof(list->list[0]) * list->max);

  while (1 == sscanf(arg, "%i%n", &value, &chars))
  {
    if (list->len+1 > list->max)
    {
      list->list = realloc(list->list, 2 * list->max * sizeof(list->list[0]));
      list->max = 2 * list->max;
    }
    list->list[list->len] = value;
    list->len++;
    arg += chars;
    if (',' == *arg)
    {
      arg++;
    }
  }

  if (']' == *arg)
  {
    arg++;
  }
  else
  {
    free(list->list);
    return -1;
  }

  if (nchars)
  {
    *nchars = (int)(arg - origarg);
  }

  return 0;
}

/* Parse strings of the form [1,2,3,4] into a TMR_uint8List */
int
parseU8List(const char *arg, TMR_uint8List *list, int *nchars)
{
  TMR_uint32List u32list;
  int i, ret;

  ret = 0;

  if (-1 == parseU32List(arg, &u32list, nchars))
  {
    return -1;
  }

  list->len = u32list.len;
  list->max = u32list.len;
  list->list = malloc(sizeof(list->list[0]) * list->max);
  for (i = 0; i < list->len; i++)
  {
    if (u32list.list[i] > UINT8_MAX)
    {
      ret = -1;
      goto out;
    }
    list->list[i] = (uint8_t)u32list.list[i];
  }

out:
  free(u32list.list);
  return ret;
}

/* Parse strings of the form words:1234ABCD. Allocates data array and places
 * it in list.
 */
int
parseWords(const char *arg, TMR_uint16List *words)
{
  int i, ret, len;
  static char wordPrefix[] = "words:";
  static int wordPrefixLen = sizeof(wordPrefix) - 1;

  if (0 != strncasecmp(arg, wordPrefix, wordPrefixLen))
  {
    return -1;
  }

  arg += wordPrefixLen;

  len = (int)strlen(arg);
  if (0 != len % 4)
  {
    return -1;
  }
  
  len /= 4;
  words->max = words->len = len;
  words->list = malloc(words->max * sizeof(words->list[0]));

  for (i = 0; i < len ; i++)
  {
    ret = sscanf(arg, "%4"SCNx16, &words->list[i]);
    if (1 != ret)
    {
      free(words->list);
      return -1;
    }
    arg += 4;
  }

  return 0;
}


/* Parse strings of the form bytes:1234ABCD. Allocates data array and places
 * it in list.
 */
int
parseBytes(const char *arg, TMR_uint8List *bytes)
{
  int i, ret, len;
  static char bytePrefix[] = "bytes:";
  static int bytePrefixLen = sizeof(bytePrefix) - 1;

  if (0 != strncasecmp(arg, bytePrefix, bytePrefixLen))
  {
    return -1;
  }

  arg += bytePrefixLen;

  len = (int)strlen(arg);
  if (0 != len % 2)
  {
    return -1;
  }
  
  len /= 2;
  bytes->max = bytes->len = len;
  bytes->list = malloc(bytes->max * sizeof(bytes->list[0]));

  for (i = 0; i < len ; i++)
  {
    /* scanf into a temporary variable to prevent memory overruns.
     * On Windows, %hhx was observed to write a full 32 bits
     * when pointed directly at the byte array. */
#ifdef WIN32
    unsigned int value;
#else
    uint8_t value;
#endif
    ret = sscanf(arg, "%2"SCNx8, &value);
    if (1 != ret)
    {
      free(bytes->list);
      return -1;
    }
    bytes->list[i] = (uint8_t)(value & 0xFF);
    arg += 2;
  }

  return 0;
}

/* Parse strings of the form [[1,100],[2,200]] into a
 * TMR_PortValueList structure.
 */
int
parsePortValueList(const char *arg, TMR_PortValueList *list, int *nchars)
{
  int port, value;
  int chars;
  const char *origarg = arg;

  if ('[' != arg[0]
      || ']' != arg[strlen(arg)-1])
  {
    return -1;
  }

  arg++;

  list->len = 0;
  list->max = 4;
  list->list = malloc(sizeof(list->list[0]) * list->max);

  while (2 == sscanf(arg, "[%i,%i]%*[],]%n", &port, &value, &chars))
  {
    if (list->len+1 > list->max)
    {
      list->list = realloc(list->list, 2 * list->max * sizeof(list->list[0]));
      list->max = 2 * list->max;
    }
    if (port > UINT8_MAX || value > UINT16_MAX)
    {
      return -1;
    }
    list->list[list->len].port = port;
    list->list[list->len].value = value;
    list->len++;
    arg += chars;
  }

  if (nchars)
  {
    *nchars = (int)(arg - origarg);
  }

  return 0;
}

/*
 * Simple read plan: [PROTOCOL,[antennalist],filter,weight]
 * all but protocol  are optional - [], NULL and 1 will be assumed)
 *   [GEN2]
 *   [GEN2,[]]
 *   [GEN2,[],NULL]
 *   [GEN2,[],NULL,1]
 *   [GEN2,[2],NULL,20]
 *   [GEN2,[2],EPC:1234,20]
 *   [GEN2,[2],Gen2.Select:~EPC,16,32,DEADBEEF,20]
 * 
 * Multiple read plans [[plan1],[plan2],[plan3],weight]
 *  weight is optional - 1 will be assumed
 *   [[GEN2],[IPX64],[IPX256],[ISO180006B]]
 *   [[GEN2,[2],NULL,20],[GEN2,[1],NULL,10]]
 *   [[GEN2,[2],NULL,20],[GEN2,[1],NULL,10],10]
 *   [[[GEN2,[2],NULL,20],[GEN2,[1],NULL,10],10],[GEN2,[1,2],NULL,20]]
 *
 */

int
parseReadPlan(const char *arg, TMR_ReadPlan *plan, int *nchars)
{
  int charcount, ret;
  const char *substring;

  if ('[' != arg[0])
  {
    return -1;
  }

  plan->weight = 1; /* Default value */

  substring = arg + 1;
  if ('[' == substring[0])
  {
    /* multi-plan */
    int i, nplans;
    TMR_ReadPlan *plans;
    TMR_ReadPlan **planlist;

    nplans = 2;
    plans = malloc(nplans * sizeof(plans[0]));

    i = 0;

    while ('[' == substring[0])
    {
      if (i == nplans)
      {
        nplans *= 2;
        plans = realloc(plans, nplans * sizeof(plans[0]));
      }
      ret = parseReadPlan(substring, &plans[i], &charcount);
      if (ret == -1 || ',' != *(substring + charcount))
      {
        free(plans);
        return -1;
      }
      substring += charcount + 1;
      i++;
    }
    nplans = i;
    planlist = malloc(nplans * sizeof(planlist[0]));
    for (i = 0; i < nplans ; i++)
      planlist[i] = &plans[i];
    substring--;
    plan->type = TMR_READ_PLAN_TYPE_MULTI;
    plan->u.multi.planCount = nplans;
    plan->u.multi.plans = planlist;
  }
  else
  {
    /* simple plan */
    static TMR_uint8List emptyList = {NULL, 0, 0};
    char protocolName[32];
    int tmp;

    plan->type = TMR_READ_PLAN_TYPE_SIMPLE;

    if (1 != sscanf(substring, "%[^],]%n", protocolName, &charcount))
    {
      return -1;
    }
    tmp = listid(protocolNames, numberof(protocolNames), protocolName);
    if (-1 == tmp)
    {
      return -1;
    }
    plan->u.simple.protocol = tmp;
    substring += charcount;

    /* Default values */
    plan->u.simple.antennas = emptyList;
    plan->u.simple.filter = NULL;
    plan->u.simple.tagop = NULL;

    if (']' == *substring)
    {
      substring++;
      goto out;
    }

    if (',' != *substring++
        || -1 == parseU8List(substring, &plan->u.simple.antennas, &charcount))
    {
      return -1;
    }
    substring += charcount;

    if (']' == *substring)
    {
      substring++;
      goto out;
    }

    if (',' != *substring++
        || -1 == parseFilter(substring, &plan->u.simple.filter, &charcount))
    {
      free(plan->u.simple.antennas.list);
      return -1;
    }
    plan->u.simple.tagop = NULL;
    substring += charcount;

    if (']' == *substring)
    {
      substring++;
      goto out;
    }

    if (',' == *substring
        && -1 != parseTagop(substring+1, &plan->u.simple.tagop, &charcount))
    {
      substring += charcount+1;
    }
  }

  if (']' == *substring)
  {
    charcount = 0;
  }
  else if (1 != sscanf(substring, ",%"SCNi32"%n", &plan->weight, &charcount))
  {
    if (TMR_READ_PLAN_TYPE_MULTI == plan->type)
    {
      /* XXX antenna lists in individual plans? */
      free(plan->u.multi.plans);
    }
    return -1;
  }
  substring += charcount;

  if (']' != *substring)
  {
    if (TMR_READ_PLAN_TYPE_MULTI == plan->type)
    {
      /* XXX antenna lists in individual plans? */
      free(plan->u.multi.plans);
    }
    return -1;
  }
  substring++;

out:
  if (nchars)
  {
    *nchars = (int)(substring - arg);
  }
  return 0;
}

int 
parseEpc(const char *arg, TMR_TagData *tag, int *nchars)
{
  static const char epcPrefix[] = "EPC:";
  static int epcPrefixLen = sizeof(epcPrefix) - 1;
  TMR_Status ret;
  int len;

  if (0 != strncasecmp(arg, epcPrefix, epcPrefixLen))
  {
    return -1;
  }

  arg += epcPrefixLen;
  len = (int)strspn(arg, hexChars) / 2;
  ret = TMR_hexToBytes(arg, tag->epc, len, NULL);
  if (TMR_SUCCESS != ret)
  {
    return -1;
  }
  tag->protocol = TMR_TAG_PROTOCOL_NONE;
  tag->epcByteCount = len;

  if (nchars)
  {
    *nchars = epcPrefixLen + 2 * len;
  }

  return 0;
}

/*
 * Tag operations (for embedding)
 *  Read:   gen2.read:EPC,2,2 (word address and count)
 *  Write:  gen2.write:EPC,2,2,DEADBEEF (word address and count, 16-bit multiple of data)
 *  Lock:   gen2.lock:EPC_LOCK,USER_UNLOCK (see parseLockAction())
 *  Kill:   gen2.kill:DEADBEEF (exactly 32-bit password)
 */
int
parseTagop(const char *arg, TMR_TagOp **tagopp, int *nchars)
{
  TMR_TagOp *tagop;
  const char *orig, *next;
  int id;
  char buf[16];

  if (0 == strncasecmp(arg, "NULL", 4))
  {
    *tagopp = NULL;
    if (nchars)
    {
      *nchars = 4;
    }
    return 0;
  }

  orig = arg;

  next = strchr(arg, ':');
  if (NULL == next || (next - arg) > 15)
    return -1;
  strncpy(buf, arg, next - arg);
  buf[next - arg] = '\0';

  tagop = malloc(sizeof(*tagop));

  id = listid(tagopNames, numberof(tagopNames), buf);
  if (-1 == id)
  {
    goto error;
  }
  arg = next + 1;

  tagop->type = id;
  switch (tagop->type)
  {
  case TMR_TAGOP_GEN2_READDATA:
  {
    TMR_GEN2_Bank bank;
    int address, len, wordLen;

    next = strchr(arg, ',');
    if (NULL == next)
    {
      goto error;
    }
    strncpy(buf, arg, next - arg);
    buf[next - arg] = '\0';
    id = listid(bankNames, numberof(bankNames), buf);
    if (-1 == id)
    {
      goto error;
    }
    bank = id;
    arg = next + 1;

    next = strchr(arg, ',');
    if (NULL == next)
    {
      goto error;
    }
    strncpy(buf, arg, next - arg);
    buf[next - arg] = '\0';
    address = atol(buf);
    arg = next + 1;

    len = (int)strspn(arg, decimalChars);
    if (len == 0 || len > 15)
    {
      goto error;
    }
    strncpy(buf, arg, len);
    buf[len] = '\0';
    wordLen = atol(buf);
    arg += len;

    TMR_TagOp_init_GEN2_ReadData(tagop, bank, address, wordLen);

    break;
  }
  case TMR_TAGOP_GEN2_WRITEDATA:

  case TMR_TAGOP_GEN2_LOCK:

  case TMR_TAGOP_GEN2_KILL:

  case TMR_TAGOP_LIST:

  default:
    goto error;
  }

  if (NULL != nchars)
    *nchars = (int)(arg - orig);

  *tagopp = tagop;
  return 0;

error:
  free(tagop);
  return -1;
}

/*
 * Tag filters:
 *  Tag ID:      EPC:000011112222333344445555
 *  Gen2 Select: Gen2.Select:~EPC,16,32,DEADBEEF
 *  ISO 18k6B:   Iso18k6b.Select:~EQ,1,ff,001122334455667788
 *               (EQ,NE,GT,LT),address,bytemask,data
 */
int
parseFilter(const char *arg, TMR_TagFilter **filterp, int *nchars)
{
  TMR_Status ret;
  TMR_TagFilter *filter;
  static const char gen2Prefix[] = "Gen2.Select:";
  static int gen2PrefixLen = sizeof(gen2Prefix) - 1;
  static const char iso18k6bPrefix[] = "Iso18k6b.Select:";
  static int iso18k6bPrefixLen = sizeof(iso18k6bPrefix) - 1;

  int len;

  if (0 == strncasecmp(arg, "NULL", 4))
  {
    *filterp = NULL;
    if (nchars)
    {
      *nchars = 4;
    }
    return 0;
  }

  filter = malloc(sizeof(*filter));

  if (0 == parseEpc(arg, &filter->u.tagData, nchars))
  {
    filter->type = TMR_FILTER_TYPE_TAG_DATA;
  }
  else if (0 == strncasecmp(arg, gen2Prefix, gen2PrefixLen))
  {
    bool invert;
    int bank, pointer, bitlen;
    uint8_t *mask;
    char numbuf[11];
    const char *orig;
    char *next;

    orig = arg;
    arg += gen2PrefixLen;
    
    if ('~' == *arg)
    {
      invert = true;
      arg++;
    }
    else
    {
      invert = false;
    }

    next = strchr(arg, ',');
    if (NULL == next)
    {
      goto error;
    }
    strncpy(numbuf, arg, next-arg);
    numbuf[next-arg] = '\0';
    bank = listid(bankNames, numberof(bankNames), numbuf);
    if (-1 == bank)
    {
      goto error;
    }
    arg = next + 1;

    next = strchr(arg, ',');
    if (NULL == next || next - arg > 10)
    {
      goto error;
    }
    strncpy(numbuf, arg, next-arg);
    numbuf[next-arg] = '\0';
    pointer = atol(numbuf);
    arg = next + 1;

    next = strchr(arg, ',');
    if (NULL == next || next - arg > 10)
    {
      goto error;
    }
    strncpy(numbuf, arg, next-arg);
    numbuf[next-arg] = '\0';
    bitlen = atol(numbuf);
    arg = next + 1;

    len = (int)(strspn(arg, hexChars) / 2);
    mask = malloc(len);
    ret = TMR_hexToBytes(arg, mask, len, NULL);
    if (TMR_SUCCESS != ret)
    {
      free(mask);
      goto error;
    }
    TMR_TF_init_gen2_select(filter, invert, bank, pointer, bitlen, mask);
    if (nchars)
    {
      *nchars = (int)((arg + 2 * len) - orig);
    }
  }
  else if (0 == strncasecmp(arg, iso18k6bPrefix, iso18k6bPrefixLen))
  {
    bool invert;
    int selectOp, address, mask;
    uint8_t bytes[8];
    char buf[16];
    const char *orig;
    char *next;

    orig = arg;
    arg += iso18k6bPrefixLen;

    if ('~' == *arg)
    {
      invert = true;
      arg++;
    }
  else
  {
      invert = false;
    }

    /* Parse group select option */
    next = strchr(arg, ',');
    if (NULL == next)
    {
    goto error;
  }
    strncpy(buf, arg, next-arg);
    buf[next-arg] = '\0';
    selectOp = listid(selectOptionNames, numberof(selectOptionNames), buf);
    if (-1 == selectOp)
    {
      goto error;
    }
    arg = next + 1;

    /* Parse byte address */
    next = strchr(arg, ',');
    if (NULL == next || next - arg > 16)
    {
      goto error;
    }
    strncpy(buf, arg, next-arg);
    buf[next-arg] = '\0';
    address = strtol(buf, NULL, 16);
    arg = next + 1;

    /* Parse byte mask */
    next = strchr(arg, ',');
    if (NULL == next || next - arg > 16)
    {
      goto error;
    }
    strncpy(buf, arg, next-arg);
    buf[next-arg] = '\0';
    mask = strtol(buf, NULL, 16);
    arg = next + 1;

    len = (int)strspn(arg, hexChars) / 2;
    if (8 != len)
    {
      goto error;
    }
    ret = TMR_hexToBytes(arg, bytes, len, NULL);
    if (TMR_SUCCESS != ret)
    {
      goto error;
    }
#ifdef TMR_ENABLE_ISO180006B    

    TMR_TF_init_ISO180006B_select(filter, invert, selectOp, address, mask,
                                  bytes);
#endif /* TMR_ENABLE_ISO180006B */

    if (nchars)
    {
      *nchars = (int)((arg + 2 * len) - orig);
    }
  }
  else
  {
    goto error;
  }

  *filterp = filter;
  return 0;

error:
  free(filter);
  return -1;
}

static const char *lockNames[] = 
{
  "USER_UNLOCK",
  "USER_LOCK",
  "USER_PERMAUNLOCK", 
  "USER_PERMALOCK",
  "TID_UNLOCK",
  "TID_LOCK",
  "TID_PERMAUNLOCK",
  "TID_PERMALOCK",
  "EPC_UNLOCK", 
  "EPC_LOCK",
  "EPC_PERMAUNLOCK", 
  "EPC_PERMALOCK",
  "ACCESS_UNLOCK", 
  "ACCESS_LOCK",
  "ACCESS_PERMAUNLOCK", 
  "ACCESS_PERMALOCK",
  "KILL_UNLOCK", 
  "KILL_LOCK",
  "KILL_PERMAUNLOCK", 
  "KILL_PERMALOCK"
};

static int lockMasks[] = 
{
  TMR_GEN2_LOCK_BITS_USER,
  TMR_GEN2_LOCK_BITS_USER,
  TMR_GEN2_LOCK_BITS_USER | TMR_GEN2_LOCK_BITS_USER_PERM,
  TMR_GEN2_LOCK_BITS_USER | TMR_GEN2_LOCK_BITS_USER_PERM,
  TMR_GEN2_LOCK_BITS_TID,
  TMR_GEN2_LOCK_BITS_TID,
  TMR_GEN2_LOCK_BITS_TID | TMR_GEN2_LOCK_BITS_TID_PERM,
  TMR_GEN2_LOCK_BITS_TID | TMR_GEN2_LOCK_BITS_TID_PERM,
  TMR_GEN2_LOCK_BITS_EPC,
  TMR_GEN2_LOCK_BITS_EPC,
  TMR_GEN2_LOCK_BITS_EPC | TMR_GEN2_LOCK_BITS_EPC_PERM,
  TMR_GEN2_LOCK_BITS_EPC | TMR_GEN2_LOCK_BITS_EPC_PERM,
  TMR_GEN2_LOCK_BITS_ACCESS,
  TMR_GEN2_LOCK_BITS_ACCESS,
  TMR_GEN2_LOCK_BITS_ACCESS | TMR_GEN2_LOCK_BITS_ACCESS_PERM,
  TMR_GEN2_LOCK_BITS_ACCESS | TMR_GEN2_LOCK_BITS_ACCESS_PERM,
  TMR_GEN2_LOCK_BITS_KILL,
  TMR_GEN2_LOCK_BITS_KILL,
  TMR_GEN2_LOCK_BITS_KILL | TMR_GEN2_LOCK_BITS_KILL_PERM,
  TMR_GEN2_LOCK_BITS_KILL | TMR_GEN2_LOCK_BITS_KILL_PERM
};

static int lockActions[] = 
{
  0,
  TMR_GEN2_LOCK_BITS_USER,
  TMR_GEN2_LOCK_BITS_USER_PERM,
  TMR_GEN2_LOCK_BITS_USER_PERM | TMR_GEN2_LOCK_BITS_USER,
  0,
  TMR_GEN2_LOCK_BITS_TID,
  TMR_GEN2_LOCK_BITS_TID_PERM,
  TMR_GEN2_LOCK_BITS_TID_PERM | TMR_GEN2_LOCK_BITS_TID,
  0,
  TMR_GEN2_LOCK_BITS_EPC,
  TMR_GEN2_LOCK_BITS_EPC_PERM,
  TMR_GEN2_LOCK_BITS_EPC_PERM | TMR_GEN2_LOCK_BITS_EPC,
  0,
  TMR_GEN2_LOCK_BITS_ACCESS,
  TMR_GEN2_LOCK_BITS_ACCESS_PERM,
  TMR_GEN2_LOCK_BITS_ACCESS_PERM | TMR_GEN2_LOCK_BITS_ACCESS,
  0,
  TMR_GEN2_LOCK_BITS_KILL,
  TMR_GEN2_LOCK_BITS_KILL_PERM,
  TMR_GEN2_LOCK_BITS_KILL_PERM | TMR_GEN2_LOCK_BITS_KILL
};
  


/*
 * Lock actions
 *
 * Gen2.Lock:         (no action)
 * Gen2.Lock:EPC_LOCK (lock EPC)
 * Gen2.Lock:EPC_LOCK,EPC_PERMALOCK,USER_UNLOCK (permalock EPC, unlock user)
 * Iso18k6b.Lock:100
 */
int
parseLockAction(const char *arg, TMR_TagLockAction *la)
{
  static const char gen2LockPrefix[] = "Gen2.LockAction:";
  static int gen2LockPrefixLen = sizeof(gen2LockPrefix) - 1;
  static const char isoLockPrefix[] = "Iso18k6b.LockAction:";
  static int isoLockPrefixLen = sizeof(isoLockPrefix) - 1;
  const char *next;
  char lockstr[32];
  int id, len;

  if (0 == strncasecmp(arg, gen2LockPrefix, gen2LockPrefixLen))
  {
    int mask, action;

    arg += gen2LockPrefixLen;
    mask = action = 0;
    
    while (*arg)
    {
      next = strchr(arg, ',');
      if (NULL == next)
      {
        len = (int)strlen(arg);
      }
      else
      {
        len = (int)(next - arg);
      }
      
      strncpy(lockstr, arg, len);
      lockstr[len] = '\0';

      id = listid(lockNames, numberof(lockNames), lockstr);
      if (-1 == id)
      {
        return -1;
      }
      mask |= lockMasks[id];
      action |= lockActions[id];

      arg += len;
      if (',' == *arg)
      {
        arg++;
      }
    }
    TMR_TLA_init_gen2(la, mask, action);
    return 0;
  }
  else if (0 == strncasecmp(arg, isoLockPrefix, isoLockPrefixLen))
  {
    int address;

    arg += isoLockPrefixLen;

    address = atoi(arg);
    printf("Lock address: %d\n", address);

#ifdef TMR_ENABLE_ISO180006B
    TMR_TLA_init_ISO180006B(la, address);
#endif /* TMR_ENABLE_ISO180006B */

    return 0;
  }
  else
  {
    return -1;
  }
}

void
setcmd(int argc, char *argv[])
{
  TMR_Status ret;
  int scans;
  TMR_Param param;
  char *paramName, *paramValue;

  ret = TMR_SUCCESS;

  paramName = argv[0];
  paramValue = argv[1];

  param = TMR_paramID(paramName);
  if (TMR_PARAM_NONE == param)
  {
    printf("%s: No such parameter\n", paramName);
  }
  else if (TMR_PARAM_BAUDRATE == param
           || TMR_PARAM_COMMANDTIMEOUT == param
           || TMR_PARAM_TRANSPORTTIMEOUT == param
           || TMR_PARAM_READ_ASYNCOFFTIME == param
           || TMR_PARAM_READ_ASYNCONTIME == param
           || TMR_PARAM_REGION_HOPTIME == param)
  {
    uint32_t value;

    scans = sscanf(paramValue, "%"SCNi32, &value);
    if (1 != scans)
    {
      printf("Can't parse '%s' as a 32-bit unsigned integer value\n",
             paramValue);
      return;
    }
    ret = TMR_paramSet(rp, param, &value);
  }
  else if (TMR_PARAM_GEN2_ACCESSPASSWORD == param)
  {
    TMR_GEN2_Password value;

    scans = sscanf(paramValue, "%"SCNx32, &value);
    if (1 != scans)
    {
      printf("Can't parse '%s' as a 32-bit hex value\n",
             paramValue);
      return;
    }
    ret = TMR_paramSet(rp, param, &value);
    
  }
  else if (TMR_PARAM_RADIO_POWERMAX == param
           || TMR_PARAM_RADIO_POWERMIN == param
           || TMR_PARAM_RADIO_READPOWER == param
           || TMR_PARAM_RADIO_WRITEPOWER == param)
  {
    uint16_t value;

    scans = sscanf(paramValue, "%"SCNi16, &value);
    if (1 != scans)
    {
      printf("Can't parse '%s' as a 16-bit unsigned integer value\n",
             paramValue);
      return;
    }
    ret = TMR_paramSet(rp, param, &value);
  }
  else if (TMR_PARAM_RADIO_TEMPERATURE == param
           || TMR_PARAM_TAGOP_ANTENNA == param)
  {
    uint8_t value;

    scans = sscanf(paramValue, "%"SCNi8, &value);
    if (1 != scans)
    {
      printf("Can't parse '%s' as an 8-bit unsigned integer value\n",
             paramValue);
      return;
    }
    ret = TMR_paramSet(rp, param, &value);
  }
  else if (TMR_PARAM_ANTENNA_CHECKPORT == param
           || TMR_PARAM_TAGREADDATA_RECORDHIGHESTRSSI == param
           || TMR_PARAM_TAGREADDATA_REPORTRSSIINDBM == param
           || TMR_PARAM_TAGREADDATA_UNIQUEBYANTENNA == param
           || TMR_PARAM_TAGREADDATA_UNIQUEBYDATA == param
           || TMR_PARAM_REGION_LBT_ENABLE == param
           || TMR_PARAM_READER_WRITE_EARLY_EXIT == param)
  {
    bool value;

    if (0 == strcasecmp(paramValue, "true"))
    {
      value = true;
    }
    else if (0 == strcasecmp(paramValue, "false"))
    {
      value = false;
    }
    else
    {
      printf("Can't parse '%s' as boolean value\n", paramValue);
      return;
    }
    ret = TMR_paramSet(rp, param, &value);
  }
  else if (TMR_PARAM_ANTENNA_PORTSWITCHGPOS == param
           || TMR_PARAM_GPIO_INPUTLIST == param
           || TMR_PARAM_GPIO_OUTPUTLIST == param)
  {
    TMR_uint8List value;

    if (-1 == parseU8List(paramValue, &value, NULL))
    {
      printf("Can't parse '%s' as list of 8-bit values\n", paramValue);
      return;
    }
    ret = TMR_paramSet(rp, param, &value);
    free(value.list);
  }
  else if (TMR_PARAM_REGION_HOPTABLE == param)
  {
    TMR_uint32List value;

    if (-1 == parseU32List(paramValue, &value, NULL))
    {
      printf("Can't parse '%s' as list of 32-bit values\n", paramValue);
      return;
    }
    ret = TMR_paramSet(rp, param, &value);
    free(value.list);
  }
  else if (TMR_PARAM_ANTENNA_TXRXMAP == param)
  {
    TMR_AntennaMapList list;
    char *arg;
    int chars, v1, v2, v3;

    arg = paramValue;
    if ('[' != arg[0]
        || ']' != arg[strlen(arg)-1])
    {
      printf("Can't parse '%s' as list of antenna map entries\n", paramValue);
      return;
    }
    arg++;
    list.len = 0;
    list.max = 4;
    list.list = malloc(sizeof(list.list[0]) * list.max);

    while (3 == sscanf(arg, "[%i,%i,%i]%*[],]%n", &v1, &v2, &v3,
                       &chars))
    {
      if (list.len + 1 > list.max)
      {
        list.list = realloc(list.list, 2 * list.max * sizeof(list.list[0]));
        list.max = 2 * list.max;
      }
      list.list[list.len].antenna = v1;
      list.list[list.len].txPort = v2;
      list.list[list.len].rxPort = v3;

      list.len++;
      arg += chars;
    }

    ret = TMR_paramSet(rp, param, &list);
    free(list.list);
  }
  else if (TMR_PARAM_ANTENNA_SETTLINGTIMELIST == param
           || TMR_PARAM_RADIO_PORTREADPOWERLIST == param
           || TMR_PARAM_RADIO_PORTWRITEPOWERLIST == param)
  {
    TMR_PortValueList value;

    if (-1 == parsePortValueList(paramValue, &value, NULL))
    {
      printf("Can't parse '%s' as a list of ports and values\n", paramValue);
      return;
    }
    ret = TMR_paramSet(rp, param, &value);
    free(value.list);
  }
  else if (TMR_PARAM_REGION_ID == param)
  {
    TMR_Region region;

    region = regionID(paramValue);
    if (TMR_REGION_NONE == region)
    {
      printf("Can't parse '%s' as a region name\n", paramValue);
      return;
    }
    ret = TMR_paramSet(rp, param, &region);
  }
  else if (TMR_PARAM_USERMODE == param)
  {
    TMR_SR_UserMode mode;
    int i;

    i = listid(userModes, numberof(userModes), paramValue);
    if (i == -1)
    {
      printf("Can't parse '%s' as a user mode\n", paramValue);
      return;
    }
    mode = i;
    ret = TMR_paramSet(rp, param, &mode);
  }
  else if (TMR_PARAM_POWERMODE == param)
  {
    TMR_SR_PowerMode mode;
    int i;

    i = listid(powerModes, numberof(powerModes), paramValue);
    if (i == -1)
    {
      printf("Can't parse '%s' as a power mode\n", paramValue);
      return;
    }
    mode = i;
    ret = TMR_paramSet(rp, param, &mode);
  }
  else if (TMR_PARAM_GEN2_Q == param)
  {
    TMR_SR_GEN2_Q value;
    int initialQ;

    if (0 == strcasecmp("DynamicQ", paramValue))
    {
      value.type = TMR_SR_GEN2_Q_DYNAMIC;
    }
    else if (0 == strncasecmp("StaticQ(", paramValue, 8) 
             && 1 == sscanf(paramValue+8, "%i", &initialQ)
             && initialQ >= 0
             && initialQ <= UINT8_MAX)
    {
      value.type = TMR_SR_GEN2_Q_STATIC;
      value.u.staticQ.initialQ = initialQ;
    }
    else  
    {
      printf("Can't parse '%s' as a Q algorithm\n", paramValue);
      return;
    }
    ret = TMR_paramSet(rp, param, &value);
  }
  else if (TMR_PARAM_GEN2_TAGENCODING == param)
  {
    TMR_GEN2_TagEncoding value;
    int i;

    i = listid(tagEncodingNames, numberof(tagEncodingNames), paramValue);
    if (i == -1)
    {
      printf("Can't parse '%s' as a Gen2 Miller M value\n", paramValue);
      return;
    }
    value = i;
    ret = TMR_paramSet(rp, param, &value);
  }
  else if (TMR_PARAM_GEN2_SESSION == param)
  {
    TMR_GEN2_Session value;
    int i;

    i = listid(sessionNames, numberof(sessionNames), paramValue);
    if (i == -1)
    {
      printf("Can't parse '%s' as a Gen2 session name\n", paramValue);
      return;
    }
    value = i;
    ret = TMR_paramSet(rp, param, &value);
  }
  else if (TMR_PARAM_GEN2_TARGET == param)
  {
    TMR_GEN2_Target value;
    int i;

    i = listid(targetNames, numberof(targetNames), paramValue);
    if (i == -1)
    {
      printf("Can't parse '%s' as a Gen2 target name\n", paramValue);
      return;
    }
    value = i;
    ret = TMR_paramSet(rp, param, &value);
  }
  else if (TMR_PARAM_GEN2_BLF == param)
  {
    TMR_GEN2_LinkFrequency lf;
    TMR_String model;
    char str[64];
    model.value = str;
    model.max = 64;

    lf = listid(gen2LinkFrequencyNames, 
                numberof(gen2LinkFrequencyNames),
                paramValue);
    TMR_paramGet(rp, TMR_PARAM_VERSION_MODEL, &model);
    if ((0 == strcmp("Mercury6", model.value))
        || (0 == strcmp("Astra-EX", model.value)))
    {
      int temp = lf;
      switch (temp)
      {
        case 0: lf = 250;
                break;
        case 2: lf = 320;
                break;
        case 4: lf = 640;
                break;
        default:;
      }
    }

    if (lf == -1)
    {
      printf("Can't parse '%s' as a Gen2 link frequency\n", paramValue);
      return;
    }
    ret = TMR_paramSet(rp, param, &lf);
  }
  else if (TMR_PARAM_GEN2_TARI == param)
  {
    TMR_GEN2_Tari value;
    int i;

    i = listid(tariNames, numberof(tariNames), paramValue);
    if (i == -1)
    {
      printf("Can't parse '%s' as a Gen2 Tari value\n", paramValue);
      return;
    }
    value = i;
    ret = TMR_paramSet(rp, param, &value);
  }
 
  else if (TMR_PARAM_TAGOP_PROTOCOL == param)
  {
    TMR_TagProtocol value;
    int i;

    i = listid(protocolNames, numberof(protocolNames), paramValue);
    if (i == -1)
    {
      printf("Can't parse '%s' as a protocol name\n", paramValue);
      return;
    }
    value = i;
    ret = TMR_paramSet(rp, param, &value);
  }
  else if (TMR_PARAM_READ_PLAN == param)
  {
    TMR_ReadPlan value;

    if (-1 == parseReadPlan(paramValue, &value, NULL))
    {
      printf("Can't parse '%s' as a read plan\n", paramValue);
      return;
    }
    ret = TMR_paramSet(rp, param, &value);
  }
  else if (TMR_PARAM_ISO180006B_BLF == param)
  {
    TMR_ISO180006B_LinkFrequency lf;
    TMR_String model;
    char str[64];
    model.value = str;
    model.max = 64;

    lf = listid(iso180006bLinkFrequencyNames, 
                numberof(iso180006bLinkFrequencyNames),
                paramValue);

    TMR_paramGet(rp, TMR_PARAM_VERSION_MODEL, &model);
    if ((0 == strcmp("Mercury6", model.value))
        || (0 == strcmp("Astra-EX", model.value)))
    {
      int temp = lf;
      switch (temp)
      {
        case 0: lf = 40;
                break;
        case 1: lf = 160;
                break;
        default:;
      }
    }


    if (lf == -1)
    {
      printf("Can't parse '%s' as an ISO180006B link frequency\n", paramValue);
      return;
    }
    ret = TMR_paramSet(rp, param, &lf);
  }
#ifdef TMR_ENABLE_ISO180006B
  else if(TMR_PARAM_ISO180006B_MODULATION_DEPTH == param)
  {
    TMR_ISO180006B_ModulationDepth depth;

    depth = listid(iso180006bModulationDepthNames, 
        numberof(iso180006bModulationDepthNames),
        paramValue);
    if (depth == -1)
    {
      printf("Can't parse '%s' as an ISO180006B Modulation Depath\n", paramValue);
      return;
    }
    ret = TMR_paramSet(rp, param, &depth);
  }
  else if(TMR_PARAM_ISO180006B_DELIMITER == param)
  {
    TMR_ISO180006B_Delimiter delimiter;

    delimiter = listid(iso180006bDelimiterNames, 
        numberof(iso180006bDelimiterNames),
        paramValue);
    if (delimiter == -1)
    {
      printf("Can't parse '%s' as an ISO180006B  Delimiter\n", paramValue);
      return;
    }
    ret = TMR_paramSet(rp, param, &delimiter);
  }
#endif
  else if(TMR_PARAM_READER_WRITE_REPLY_TIMEOUT == param)
  {
    uint16_t value;

    scans = sscanf(paramValue, "%"SCNu16, &value);
    if (1 != scans)
    {
      printf("Can't parse '%s' as an 16-bit unsigned integer value\n",
          paramValue);
      return;
    }
    ret = TMR_paramSet(rp, param, &value);
  }
  else if ( TMR_PARAM_URI == param
            ||TMR_PARAM_CURRENTTIME == param)
  {
    ret = TMR_ERROR_READONLY; 
  }
  else
  {
    printf("%s: Don't know how to parse value\n", paramName);
  }

  if (TMR_SUCCESS != ret)
  {
    printf("Error setting value: %s\n", TMR_strerr(rp, ret));
  }
}


void
helpcmd(int argc, char *argv[])
{
  int i;

  if (0 == argc)
  {
    for (i = 0; i < numcommands; i++)
    {
      printf("%s -- %s\n", commands[i].name, commands[i].shortDoc);
    }
  }
  else
  {
    for (i = 0; i < numcommands; i++)
    {
      if (0 == strcmp(argv[0], commands[i].name))
      {
        printf("%s\n%s\n%s\n",
               commands[i].shortDoc, commands[i].usage, commands[i].doc);
      break;
      }
    }
    if (i == numcommands)
    {
      printf("No command '%s'. Try 'help'.\n", argv[0]);
    }
  }
}


void
echocmd(int argc, char *argv[])
{
  int i;

  for (i = 0; i < argc ; i++)
  {
    printf("%s%s", argv[i], 
           (i+1 == argc) ? "" : " ");
  }
  printf("\n");
}


void
sleepcmd(int argc, char *argv[])
{
#ifndef WIN32
  int timeout;
  struct timespec ts;

  timeout = atoi(argv[0]);

  ts.tv_sec = timeout / 1000;
  ts.tv_nsec = (timeout % 1000) * 1000000L;

  nanosleep(&ts, NULL);
#endif /* not WIN32 */
} 


void
tracecmd(int argc, char *argv[])
{

  if (0 == strcmp(argv[0], "on"))
  {
    if (listener == NULL)
    {
      listener = &tb;
      TMR_addTransportListener(rp, listener);
    }
  }
  else
  {
    if (listener != NULL)
    {
      TMR_removeTransportListener(rp, listener);
      listener = NULL;
    }
  }
      
}

#ifdef TMR_ENABLE_READLINE
char **
demo_completion(const char *text, int start, int end)
{
  /* At the beginning of the line, complete commands */

  if (start == 0)
  {
    return rl_completion_matches(text, command_generator);
  }
  
  if (strncmp(text, "loadfw", 6) == 0)
  {
    const char *filename;
    int offset;

    offset = strcspn(text + 6, " ");
    filename = text + 6 + offset;
    return rl_completion_matches(filename, rl_filename_completion_function);
  }
  

  return NULL;
}


char *
command_generator(const char *text, int state)
{
  static int list_index, len;
  const char *name;

  if (state == 0)
  {
    list_index = 0;
    len = strlen(text);
  }

  while (list_index < numberof(commands))
  {
    name = commands[list_index].name;
    list_index++;

    if (strncmp(name, text, len) == 0)
      return (strdup(name));
  }
  
  return NULL;
}
#endif /* TMR_ENABLE_READLINE */

void
regulatoryTestCmds(int argc, char *argv[])
{
#ifdef TMR_ENABLE_SERIAL_READER

  TMR_Status ret;
  ret=TMR_SR_cmdTestSetFrequency(rp,915250);
  if (TMR_SUCCESS != ret)
  {
    errx(1, "Error setting frequency: %s\n", TMR_strerr(rp, ret));
  }
  else
    printf("Set frequency succeeded \n");

  ret=TMR_SR_cmdTestSendPrbs(rp,1000);
  if (TMR_SUCCESS != ret)
  {
    errx(1, "Error sending prbs: %s\n", TMR_strerr(rp, ret));
  }
  else
    printf("SendPrbs succeeded \n");


  ret=TMR_SR_cmdTestSendCw(rp,true);
  if (TMR_SUCCESS != ret)
  {
    errx(1, "Error sending Cw: %s\n", TMR_strerr(rp, ret));
  }
  else
    printf("SendCw succeeded when on \n");


  ret=TMR_SR_cmdTestSendCw(rp,false);
  if (TMR_SUCCESS != ret)
  {
    errx(1, "Error sending Cw: %s\n", TMR_strerr(rp, ret));
  }
  else
    printf("SendCw succeeded when off \n");
  
#endif /* TMR_ENABLE_SERIAL_READER */ 
}

void savedConfig(int argc, char *argv[])
{
#ifdef TMR_ENABLE_SERIAL_READER

  /* Testing 
  * TMR_SR_cmdSetUserProfile function
  */
  TMR_Status ret;
 /*{
  int timeout=15000;
  ret = TMR_paramSet(rp, TMR_PARAM_COMMANDTIMEOUT, &timeout);
  if (TMR_SUCCESS != ret)
  {
    errx(1, "Error setting timeout: %s\n", TMR_strerr(rp, ret));
  }
  } */
  ret = TMR_SR_cmdSetProtocol(rp,TMR_TAG_PROTOCOL_GEN2);   // This to set the protocol
  if (TMR_SUCCESS != ret)
  {
    printf("Error reading tags: %s\n", TMR_strerr(rp, ret));
    return;
  }
  ret=TMR_SR_cmdSetUserProfile(rp,0x01,0x01,0x01);  //Save all the configurations 
  if (TMR_SUCCESS != ret)
  {
    errx(1, "Error setting user profile option save all configuration: %s\n", TMR_strerr(rp, ret));
  }
  else
    printf("User profile set option:save all configuration\n");

  ret=TMR_SR_cmdSetUserProfile(rp,0x02,0x01,0x01);  //Restore all saved configuration parameters 
  if (TMR_SUCCESS != ret)
  {
    errx(1, "Error setting user profile option restore all saved configuration params: %s\n", TMR_strerr(rp, ret));
  }
  else
    printf("User profile set option:restore all saved configuration params\n");

  
  
  ret=TMR_SR_cmdSetUserProfile(rp,0x03,0x01,0x01);  //verify all configuration parameters 
  if (TMR_SUCCESS != ret)
  {
    errx(1, "Error setting user profile option verify all configuration parameters: %s\n", TMR_strerr(rp, ret));
  }
  else
    printf("User profile set option:verify all configuration parameters\n");

  
  /* Testing 
  * TMR_SR_cmdGetUserProfile function
  */
  {
  uint8_t data[]={0x67};
  uint8_t response[10],i,j=0;
  ret=TMR_SR_cmdGetUserProfile(rp,data,1,response,&j);  
  if (TMR_SUCCESS != ret)
  {
    errx(1, "Error getting user profile option region: %s\n", TMR_strerr(rp, ret));
  }
  else
  {
    printf(" Get user profile success option:region\n");
    for(i=0;i<j;i++)
    {
      printf(" %02x ",response[i]);
    }
    printf("\n");
  }
  }

  {
  uint8_t data[]={0x63};
  uint8_t response[10],i,j=0;
  ret=TMR_SR_cmdGetUserProfile(rp,data,1,response,&j);   
  if (TMR_SUCCESS != ret)
  {
    errx(1, "Error getting user profile option region: %s\n", TMR_strerr(rp, ret));
  }
  else
  {
    printf(" Get user profile success option:Protocol");
    for(i=0;i<j;i++)
    {
      printf(" %02x ",response[i]);
    }
    printf("\n");
  }
  }

  {
  uint8_t data[]={0x06};
  uint8_t response[10],i,j=0;
  ret=TMR_SR_cmdGetUserProfile(rp,data,1,response,&j);  
  if (TMR_SUCCESS != ret)
  {
    errx(1, "Error getting user profile option region: %s n", TMR_strerr(rp, ret));
  }
  else
  {
    printf(" Get user profile success option:baudrate");
    for(i=0;i<j;i++)
    {
      printf(" %02x ",response[i]);
    }
    printf("\n");
  }
  }
  ret=TMR_SR_cmdSetUserProfile(rp,0x04,0x01,0x01);  //reset all configuration parameters 
  if (TMR_SUCCESS != ret)
  {
    errx(1, "Error setting user profile option reset all configuration parameters: %s\n", TMR_strerr(rp, ret));
  }
  else
    printf("User profile set option:reset all configuration parameters\n");

  ret = TMR_SR_cmdSetProtocol(rp,TMR_TAG_PROTOCOL_GEN2);   // This to set the protocol
  if (TMR_SUCCESS != ret)
  {
    printf("Error reading tags: %s\n", TMR_strerr(rp, ret));
    return;
  }

  ret=TMR_SR_cmdSetUserProfile(rp,0x01,0x01,0x01);  //Save all the configurations 
  if (TMR_SUCCESS != ret)
  {
    errx(1, "Error setting user profile option save all configuration: %s\n", TMR_strerr(rp, ret));
  }
  else
    printf("User profile set option:save all configuration\n");

  ret=TMR_SR_cmdSetUserProfile(rp,0x02,0x01,0x00);  //restore firmware default configuration parameters
  if (TMR_SUCCESS != ret)
  {
    errx(1, "Error setting user profile option restore firmware default configuration parameters: %s\n", TMR_strerr(rp, ret));
  }
  else
    printf("User profile set option:restore firmware default configuration parameters\n");
#endif /* TMR_ENABLE_SERIAL_READER */
}


void
blockWriteCmds(int argc, char *argv[])
{
#ifdef TMR_ENABLE_SERIAL_READER
  TMR_Status ret;
  TMR_TagFilter filter;
  //TMR_GEN2_WriteMode mode = TMR_GEN2_BLOCK_FALLBACK;
  TMR_TF_init_gen2_select(&filter,false,TMR_GEN2_BANK_EPC,0,0,NULL);

  ret = TMR_SR_cmdSetProtocol(rp,TMR_TAG_PROTOCOL_GEN2);   // This to set the protocol
  if (TMR_SUCCESS != ret)
  {
    printf("Error reading tags: %s\n", TMR_strerr(rp, ret));
    return;
  }

  //TMR_paramSet(rp, TMR_PARAM_GEN2_WRITEMODE, &mode);
  {
    //uint8_t data[]={0x01,0x02,0x03,0X04};
    //TMR_SR_writeTagMemBytes(rp, &filter, TMR_GEN2_BANK_RESERVED, 0x00, 4, data);
  }

  //

  printf("ERROR: Block Write tests disabled.  Needs porting to Level 2 tagops.\n");
#if 0
  ret = TMR_SR_cmdBlockWrite(rp,1000,TMR_GEN2_BANK_USER , 0x00, 2,data, 0x00, &filter);
  if (TMR_SUCCESS != ret)
  {
    errx(1, "Error in L3 block write cmd : %s\n", TMR_strerr(rp, ret));
  }
  else
    printf("cmdBlockWrite succeeded\n");

  rp->u.serialReader.useStreaming = false;
  TMR_SR_cmdClearTagBuffer(rp);
  
  {
  TMR_TagOp tagop;
  TMR_TagOp *op = &tagop;
  uint8_t data1[]= {1,2,3,4};

  op->type=TMR_TAGOP_GEN2_BLOCKWRITE;
  op->u.gen2_blockWrite.accessPassword=0x0000;
  op->u.gen2_blockWrite.bank= TMR_GEN2_BANK_USER;
  op->u.gen2_blockWrite.data= data1;
  op->u.gen2_blockWrite.wordCount= 2;
  op->u.gen2_blockWrite.wordPtr= 0;

  ret= TMR_SR_executeTagOp(rp,op, NULL, NULL);
  if (TMR_SUCCESS != ret)
  {
    errx(1, "Error in block write cmd : %s\n", TMR_strerr(rp, ret));
  }
  else
    printf("BlockWrite succeeded \n");
  }
#endif
#endif /* TMR_ENABLE_SERIAL_READER */
}

void
blockPermalockCmds(int argc, char *argv[])
{
#ifdef TMR_ENABLE_SERIAL_READER  
  TMR_Status ret;
  TMR_TagFilter filter;
#if 0
  uint16_t returnData[256];
#endif
  TMR_TF_init_gen2_select(&filter,false,TMR_GEN2_BANK_EPC,0,0,NULL);
  ret = TMR_SR_cmdSetProtocol(rp,TMR_TAG_PROTOCOL_GEN2);   // This to set the protocol
  if (TMR_SUCCESS != ret)
  {
    printf("Error reading tags: %s\n", TMR_strerr(rp, ret));
    return;
  }

  printf("ERROR: Block Write tests disabled.  Needs porting to Level 2 tagops.\n");
#if 0
  {
    uint16_t mask[]={0x0000};
    ret=TMR_SR_cmdBlockPermaLock(rp,100,0x00, TMR_GEN2_BANK_USER,0x00, 0x01, mask,0x00, &filter,returnData);
  }

  printf("%4x\n",returnData[0]);


  if (TMR_SUCCESS != ret)
  {
    errx(1, "Error in L3 block permalock cmd: %s\n", TMR_strerr(rp, ret));
  }
  else
    printf("cmdBlockPermalock succeeded\n");

  rp->u.serialReader.useStreaming = false;
  TMR_SR_cmdClearTagBuffer(rp);
  
  {
  TMR_TagOp tagop;
  TMR_TagOp *op = &tagop;
  op->type = TMR_TAGOP_GEN2_BLOCKPERMALOCK;
  op->u.gen2_blockPermaLock.accessPassword=0;
  op->u.gen2_blockPermaLock.bank=TMR_GEN2_BANK_USER;
  op->u.gen2_blockPermaLock.blockPtr=0;
  op->u.gen2_blockPermaLock.blockRange=1;
  op->u.gen2_blockPermaLock.mask=NULL;
  op->u.gen2_blockPermaLock.readLock=0;

  ret = TMR_SR_executeTagOp(rp, op, NULL, returnData);
  if (TMR_SUCCESS != ret)
  {
    errx(1, "Error permalock: %s\n", TMR_strerr(rp, ret));
  }
  else
    printf("BlockPermalock succeeded\n");
  }
#endif
#endif /* TMR_ENABLE_SERIAL_READER */  
}


void
gpiodirection(int argc, char *argv[])
{
  TMR_Param param;
  TMR_Status ret;
  uint8_t in[]={1,2};
  uint8_t out[]={3,4};
  int i;
  uint8_t in_list[10],out_list[10];
  TMR_uint8List in_key,out_key;
  TMR_uint8List in_val,out_val;
  in_key.list=in;
  in_key.len=2;
  out_key.list=out;
  out_key.len=2;
  in_val.list=in_list;
  out_val.list=out_list;
  out_val.max=10;
  param=TMR_PARAM_GPIO_INPUTLIST;
  ret=TMR_paramSet(rp,param , &in_key);
  if (TMR_SUCCESS != ret)
  {
    errx(1, "Error setting gpio input list: %s\n", TMR_strerr(rp, ret));
  }
  else
    printf("Input list set\n");
  param=TMR_PARAM_GPIO_OUTPUTLIST;
  ret=TMR_paramSet(rp,param,&out_key);
  if (TMR_SUCCESS != ret)
  {
    errx(1, "Error setting gpio output list: %s\n", TMR_strerr(rp, ret));
  }
  else
    printf("Output list set\n");
  param=TMR_PARAM_GPIO_INPUTLIST;
  ret=TMR_paramGet(rp,param, &in_val);
  if (TMR_SUCCESS != ret)
  {
    errx(1, "Error getting gpio input list: %s\n", TMR_strerr(rp, ret));
  }
  else 
  {
    for(i=0;i<in_val.len;i++)
    {
      printf("input list=%d \n",in_val.list[i]);
    }
  }
  param= TMR_PARAM_GPIO_OUTPUTLIST;
  ret=TMR_paramGet(rp,param, &out_val);
  if (TMR_SUCCESS != ret)
  {
    errx(1, "Error getting gpio output list: %s\n", TMR_strerr(rp, ret));
  }
  else
  {
    for(i=0;i<out_val.len;i++)
    {
      printf("output list=%d\n",out_val.list[i]);
    }
  }
}
