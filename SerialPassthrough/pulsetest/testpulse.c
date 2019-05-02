/** 
 * Test program to show the impact on the bootloader
 * of an arduino with sending data early.
 * 
 * *******************************************************************
 * This is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this software. If not, see <http://www.gnu.org/licenses/>. 
 * *******************************************************************
 * version alpha April 2019
 * paul van Haastrecht
 */

#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include "serial.h"

char port[20] = "/dev/ttyUSB0";

/*********************************************************************
 * @brief : main program start
 *********************************************************************/
int main(int argc, char *argv[])
{
    int fd = 0xff;                   // filepointer
    char packet[5];
    
    /* simulate a version request */
    packet[0] = 0xff;
    packet[1] = 0x00;
    packet[2] = 0x03;
    packet[3] = 0x1d;
    packet[4] = 0x0c;
    
    /* you need the driver to be loaded before opening /dev/ttyUSBx
     * otherwise it will hang. 
     * One can change the system setup so this is done automatically
     * when reboot and then you can remove the call below. */
    system("modprobe usbserial");
    system("modprobe ch341");

    fd = open(port, O_RDWR | O_NOCTTY | O_SYNC);

    if (fd < 0) {
        printf("could not open %s\n", port);
        exit(EXIT_FAILURE);
    }
/*    
    // toggle DTR to sumulate 2 double DTR
    // the URA is generating a double DTR signal
    // but during testing it did not make a difference
    // left for reference only
    
    int DTR_FLAG;
    DTR_FLAG = TIOCM_DTR;
    ioctl(fd,TIOCMBIC,&DTR_FLAG);
    usleep(200);
    ioctl(fd,TIOCMBIS,&DTR_FLAG);
*/    
    configure_interface(fd, B115200);
    
    // 3 pulses with 100ms in between.
    // no error checks of printf() in order for critical timing to achieve
    write(fd, packet, 5);
    usleep(100000);         // if extended to 1000000 (adding zero) all works fine
    write(fd, packet, 5);
    usleep(100000);
    write(fd, packet, 5);
    
    close(fd);
    
    // wiating to prove that a load of the Arduino program after timeout
    // is not the result of this program closing (and maybe causing
    // a DTR noise signal)        
    printf("waiting before closing\n");
    sleep(15);
    printf("exit\n");

    exit(0);
}
