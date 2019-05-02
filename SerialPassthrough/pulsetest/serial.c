/*
 * This part of this code is mainly based on the work from karl,
 * found on:  github https://github.com/karlchen86/SDS011
 */


#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

#include "serial.h"

#include <linux/usbdevice_fs.h>

struct termios tty_back;
bool restore = false;

void configure_interface(int fd, int speed)
{
    struct termios tty;

    if (tcgetattr(fd, &tty_back) < 0) {
        perror("tcgetattr");
        exit(1);
    }

    if (tcgetattr(fd, &tty) < 0) {
        perror("tcgetattr");
        exit(1);
    }

    cfsetospeed(&tty, (speed_t)speed);
    cfsetispeed(&tty, (speed_t)speed);

    tty.c_cflag |= (CLOCAL | CREAD);    /* ignore modem controls */
    tty.c_cflag &= ~CSIZE;
    tty.c_cflag |= CS8;         /* 8-bit characters */
    tty.c_cflag &= ~PARENB;     /* no parity bit */
    tty.c_cflag &= ~CSTOPB;     /* only need 1 stop bit */
    tty.c_cflag &= ~CRTSCTS;    /* no hardware flowcontrol */

    /* setup for non-canonical mode */
    tty.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL | IXON);
    tty.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
    tty.c_oflag &= ~OPOST;

    /* fetch bytes as they become available */
    tty.c_cc[VMIN] = 1;
    tty.c_cc[VTIME] = 1;

    if (tcsetattr(fd, TCSANOW, &tty) != 0) {
        perror("tcsetattr");
        exit(1);
    }

    restore=true;
}

void set_blocking(int fd, int mcount)
{
    struct termios tty;

    if (tcgetattr(fd, &tty) < 0) {
        perror("tcgetattr");
        exit(1);
    }

    tty.c_cc[VMIN] = mcount ? 1 : 0;
    tty.c_cc[VTIME] = 5;        /* half second timer */

    if (tcsetattr(fd, TCSANOW, &tty) < 0) {
        perror("tcsetattr");
        exit(1);
    }
}

// paulvha : added to restore
void restore_ser(int fd)
{
    if (restore)
    {
        if (tcsetattr(fd, TCSANOW, &tty_back) < 0) {
            perror("reset tcsetattr");
            exit(1);
        }
    }
}


