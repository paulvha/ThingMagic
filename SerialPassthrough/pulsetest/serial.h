/*
 * This part of this code is mainly based on the work from karl,
 * found on:  github https://github.com/karlchen86/SDS011
 */

#include <sys/ioctl.h>

#ifndef _SERIAL_H
#define _SERIAL_H

// Speed: B115200, B230400, B9600, B19200, B38400, B57600, B1200, B2400, B4800
void configure_interface(int fd, int speed);

void set_blocking(int fd, int should_block);

// paulvha : added to restore settings
void restore_ser(int fd);

#endif /* _SERIAL_H */
