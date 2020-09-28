/*
* Copyright (C) 2013 - 2016  Xilinx, Inc.  All rights reserved.
*
* Permission is hereby granted, free of charge, to any person
* obtaining a copy of this software and associated documentation
* files (the "Software"), to deal in the Software without restriction,
* including without limitation the rights to use, copy, modify, merge,
* publish, distribute, sublicense, and/or sell copies of the Software,
* and to permit persons to whom the Software is furnished to do so,
* subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included
* in all copies or substantial portions of the Software.
*
* Use of the Software is limited solely to applications:
* (a) running on a Xilinx device, or (b) that interact
* with a Xilinx device through a bus or interconnect.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
* IN NO EVENT SHALL XILINX  BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
* WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
* CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*
* Except as contained in this notice, the name of the Xilinx shall not be used
* in advertising or otherwise to promote the sale, use or other dealings in this
* Software without prior written authorization from Xilinx.
*
*/

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
//#include <i2c/smbus.h>

int main(int argc, char **argv)
{
  int file;
  int adapter_nr = 1; /* probably dynamically determined */
  char filename[20];

  if (argc != 2)
  {
    printf("USAGE: %s <value>\n", argv[0]);
    exit(1);
  }
  int val = atoi(argv[1]);
  if (val > 4095 || val < 0)
  {
    printf("Illegal input value (0-4095)\n");
    exit(1);
  }
  snprintf(filename, 19, "/dev/i2c-%d", adapter_nr);
  file = open(filename, O_RDWR);
  if (file < 0) {
    printf("Cannot open %s\n", filename);
    /* ERROR HANDLING; you can check errno to see what went wrong */
    exit(1);
  }
  int addr = 0x0c; /* The I2C address */

  if (ioctl(file, I2C_SLAVE, addr) < 0) {
    printf("Cannot connect to i2c device %d on %s\n", addr, filename);
    /* ERROR HANDLING; you can check errno to see what went wrong */
    close(file);
    exit(1);
  }
  unsigned char buf[2];
  buf[0] = (unsigned char)((val & 0x0f00) >> 8);
  buf[1] = (unsigned char)(val & 0xff);
  if (write(file, buf, 2) != 2)
  {
    printf("Cannot write to i2c device.");
    close(file);
    exit(1);
  }
  close(file);
  return 0;
}
