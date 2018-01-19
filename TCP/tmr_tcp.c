/**
 * Copyright (c) 2017 Paul van Haastrecht.
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
 * paulvh : November 2017 : version 1.0
 * 
 * This program will allow to connect over TCP to an TMR reader connected to 
 * the PC. 
 * 
 * Parts in the TCP connection code is taken from https://github.com/a1368017681/TCP
 * Part in tmr_open() is taken from THingMagic Mercury API 1.29.4.34.
 * 
 * 
 * to compile :
 *  cc -o tmr_tcp  tmr_tcp.c
 * 
 * to run :
 *  ./tmr_tcp /dev/ttyUSB0  (assuming this is where TMR is connected)
 * 
 *  ./tmr_tcp -v /dev/ttyUSB0 : will show all bytes exchanged
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <error.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/select.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <signal.h>
#include <getopt.h>
#include <termios.h>
#include <sys/ioctl.h>

#define VERSION "version 1.0 / November 2017 /paulvha "
#define PORT 3333       // TCP port use
#define BACK_LOG 10     // maximum length to which the queue of pending connections
#define MAXLEN 512      // buffer size to use

// handles
int listenfd = 0 ,connectfd = 0,tmr_handle =0;

/*********************************************************************
** close out program correctly 
**********************************************************************/
void closeout(int ret)
{
	if(listenfd)	close(listenfd);
	if(tmr_handle)	close(tmr_handle);
	if (connectfd)  close(connectfd);
	exit(ret);
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
			printf("\nStopping TMR_tcp reader\n");
			closeout(0);
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

/*********************************************************************
** Opens TMR reader device and set correct serial settings
* device = input device to open ( aka /dev/ttyUSB0)
**********************************************************************/
void tmr_open(char *device)
{
	int ret;
	struct termios t;

	tmr_handle = open(device, O_RDWR);

	if (tmr_handle == -1)
	{
	  printf("can not open device %s\n",device);
	  closeout(EXIT_FAILURE);
	}

  /* this part is coming from the Mercury API
   * Set 8N1, disable high-bit stripping, soft flow control, and hard
   * flow control (modem lines).
   */
	ret = tcgetattr(tmr_handle, &t);
  
	if (-1 == ret)
	{
	  printf("can not obtain termio from device\n");
	  closeout(EXIT_FAILURE);
	}

	t.c_iflag &= ~(ICRNL | IGNCR | INLCR | INPCK | ISTRIP | IXANY 
				 | IXON | IXOFF | PARMRK);
	t.c_oflag &= ~OPOST;
	t.c_cflag &= ~(CRTSCTS | CSIZE | CSTOPB | PARENB);
	t.c_cflag |= CS8 | CLOCAL | CREAD | HUPCL;
	t.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
	t.c_cc[VMIN] = 0;
	t.c_cc[VTIME] = 1;
	ret = tcsetattr(tmr_handle, TCSANOW, &t);
	
	if (-1 == ret)
	{
	  printf("can not set termio on device\n");
	  closeout(EXIT_FAILURE);
	}
}

/****************************************************************** 
 * Display bytes in verbose
 * buff = input message to display
 * n    = input length of message
 * dir  = either receiving or sending
 * 
 ******************************************************************/

void disp_verbose(char *buff, int n, char * dir)
{
	int i;
	
	printf("%s ", dir);
	for (i = 0 ; i < n; i++) printf("%02x", ( buff[i] & 0xff)); 
	
	printf("\n");
}

/****************************************************************** 
 * write bytes to the TCP remote client
 * buff = input message to write
 * n    = input length of message
 * 
 * Return
 * 0 = error
 * 1 = OK
 ******************************************************************/
int tcp_write (char *buff, int n)
{
	int ret, length;
	
	length = n;
	
	do
	{
		ret = write(connectfd,buff,length);
	
		if (ret == -1)
		{
			printf("Error during writing to TCP socket\n");
			return(0);
		}
	
		length -= ret;
		buff += ret;
	
	} while (length > 0);
	
		
	return(1);
}

/****************************************************************** 
 * write bytes to the TMR reader
 * buff = input message to write
 * n    = input length of message
 * 
 * Return
 * 0 = error
 * 1 = OK
 ******************************************************************/
int tmr_write(char *buff, int n)
{
	int ret, length;
	
	length = n;
	
    do
	{
		ret = write(tmr_handle,buff,length);
	
		if (ret == -1)
		{
			printf("Error during writing to TMR reader\n");
			return(0);
		}

		length -= ret;
		buff += ret;
	
	} while (length > 0);

	return(1);
}

/************************************************************** 
 *  read bytes from the TCP connection
 * *buff = ouptut message to received
 * *len  = output length of message received
 * 
 * The needed length is determined by the first byte following
 * the header (0xff)
 * 
 * Return
 * 0 = error
 * 1 = OK
 ***************************************************************/
int tcp_read(char *buff, int *len)
{
	int ret;
	int count=1;
    int det_hdr = 0;
    
    do
    {
        ret = read(connectfd, buff, MAXLEN);
	
        if (ret == -1)
        {
            printf("Error during reading from TCP\n");
            return(0);
        }
    
        // if first part of message
        if (det_hdr == 0  && (buff[0] &0xff) == 0xff && ret > 2)
        {
            // expect length = datalength > buff[1] + 5 (hdr,length,opcode,crc (2))
            *len = count = buff[1] + 5;
            det_hdr = 1;
        }
        
        count -= ret;
        buff += ret;
    
    } while (count > 0);
    
	return(1);
}

/************************************************************** 
 *  read bytes from the TMR reader
 * *buff = ouptut message to received
 * *len  = output length of message received
 * 
 * The needed length is determined by the first byte following
 * the header (0xff)
 * 
 * Return
 * 0 = error
 * 1 = OK
 ***************************************************************/
int tmr_read(char *buff, int *len)
{
	int ret;
	int count = 1;
    int det_hdr = 0;

    do
    {
        ret = read(tmr_handle, buff, MAXLEN);
	
        if (ret == -1)
        {
            printf("Error during reading from TMR reader\n");
            return(0);
        }
 
        // if first part of message
        if (det_hdr == 0  && (buff[0] & 0xff) == 0xff && ret != 0)
        {
            // expect length = datalength > buff[1] + 7 (hdr,length,opcode, statusword (2) , crc (2))
            *len = count = buff[1] + 7;

            det_hdr = 1;
        }

        count -= ret;
        buff += ret;
    
    } while (count > 0);

	return(1);
}

/**************************************************************
 * Main communication loop between TMR and TCP
 * 
 * connectfd = socket connection to remote client
 * verbose   = if set will display the bytes exchanged
 * ************************************************************/
void tmr_comm(int connectfd, int verbose)
{
	char buff[MAXLEN];
	int len = 0;
    int loop = 1;

	printf("ready to communicate\n");

    while (loop)
    {
		len = 0;
        
        // get command from TCP / client
		tcp_read(buff, &len);
		
		if (len == EOF || len == 0 )
		{
			loop  = 0;
			break;
		}
		
		if (verbose) disp_verbose(buff, len, "Sending:");
			
		//write command to TMR reader
		loop = tmr_write(buff,len);
		
		len = 0;
		//read TMR (wait for response)
		if (loop) loop = tmr_read(buff, &len);
	
		if (verbose && loop) disp_verbose(buff, len, "Receiving:");

		// write response to TCP / client
		if (loop && len > 0) loop = tcp_write(buff,len);
	}
    
    printf("ending connection\n");
    
    closeout(EXIT_SUCCESS);
}

/****************************************
 *  display usage and exit with failure 
 ****************************************/
 
void usage()
{
	printf("Usage: tmr_tcp  [-v] device\n"
	       "           device: 'COM1' or '/dev/ttyUSB0' "
           "or '/dev/ttyS0'\n"
           "       -v verbose: shows bytes exchanged\n\n"
           "%s\n\n",VERSION);
	
    exit(EXIT_FAILURE);
}

int
main(int argc, char *argv[])
{
    struct sockaddr_in server;
    struct sockaddr_in client;
    pid_t childpid;
    socklen_t addrlen;
    int option, opt, verbose = 0 ;

	while ((opt = getopt(argc, argv, "v")) != -1)
	{
		switch (opt) 
        {
            case 'v':
              verbose = 1;
              break;
            default: /* '?' */
                usage();
		}
	}
  
	argc -= optind;
	argv += optind;
    
    if (argc < 1) usage();

    // catch signals
	set_signals();
    
    // open connection to TMR reader
    tmr_open(argv[0]);
  
	// open network connection
    listenfd = socket(AF_INET,SOCK_STREAM,0);
    
    if(listenfd == -1)
    {
        perror("TCP socket created failed");
        closeout(0);
    }

	option = SO_REUSEADDR;
	setsockopt(listenfd,SOL_SOCKET,option,&option,sizeof(option));
	
	bzero(&server,sizeof(server));
	server.sin_family = AF_INET;
	server.sin_port = htons(PORT);
	server.sin_addr.s_addr = htonl(INADDR_ANY);
	
	// bind to port
	if(bind(listenfd,(struct sockaddr *)&server,sizeof(server)) == -1)
    {
		perror("TCP Bind error!");
		closeout(EXIT_FAILURE);
	}
	
	// start listening
	if(listen(listenfd,BACK_LOG) == -1)
    {
		perror("TCP listening error");
		closeout(EXIT_FAILURE);
	}
    
    printf("waiting for client's request.....\n");
    
    while(1)
    {
		addrlen = sizeof(client);
		connectfd = accept(listenfd,(struct sockaddr*)&client,&addrlen);
		
		if(connectfd == -1)
        {
			perror("TCP accept error");
			closeout(EXIT_FAILURE);
		}
		else
			printf("TCP client connected\n");

		// create child proces
		if((childpid = fork()) == 0)
        {	
			close(listenfd);
			listenfd = 0;
		
        	printf("client from %s\n",inet_ntoa(client.sin_addr));
            
            // sent as fast a possible
			option = 1;
			setsockopt(connectfd,IPPROTO_TCP,TCP_NODELAY,&option,sizeof(option));	

			tmr_comm(connectfd, verbose);
		}
		else if(childpid < 0)
			printf("fork error: %s\n",strerror(errno));
		
		// close connectfd in parent
		close(connectfd);
		connectfd = 0;
	}

    closeout(EXIT_SUCCESS);
}
