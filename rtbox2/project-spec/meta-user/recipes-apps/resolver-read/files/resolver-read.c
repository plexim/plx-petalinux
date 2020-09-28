/*
 * Usage:
 * 
 * echo 442 > export
 * echo out > gpio442/direction 
 * echo 0 > gpio442/value; echo 1 > gpio442/value; resolver-read 

*/

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <linux/spi/spidev.h>
#include <sys/ioctl.h>

static void dumpstat(const char *name, int fd)
{
	__u8	lsb, bits;
	__u32	mode, speed;

	if (ioctl(fd, SPI_IOC_RD_MODE32, &mode) < 0) {
		perror("SPI rd_mode");
		return;
	}
	if (ioctl(fd, SPI_IOC_RD_LSB_FIRST, &lsb) < 0) {
		perror("SPI rd_lsb_fist");
		return;
	}
	if (ioctl(fd, SPI_IOC_RD_BITS_PER_WORD, &bits) < 0) {
		perror("SPI bits_per_word");
		return;
	}
	if (ioctl(fd, SPI_IOC_RD_MAX_SPEED_HZ, &speed) < 0) {
		perror("SPI max_speed_hz");
		return;
	}

	printf("%s: spi mode 0x%x, %d bits %sper word, %d Hz max\n",
		name, mode, bits, lsb ? "(lsb first) " : "", speed);
}

static unsigned char do_message(int aFd, unsigned char aAddr)
{
	struct spi_ioc_transfer xfer;
	unsigned char buf;
	int status;

	memset(&xfer, 0, sizeof(xfer));

	buf = aAddr;

	xfer.tx_buf = (unsigned long) &aAddr;
	xfer.rx_buf = (unsigned long) &buf;
	xfer.len = 1;

	status = ioctl(aFd, SPI_IOC_MESSAGE(1), &xfer);
	if (status < 0)
	{
		perror("SPI_IOC_MESSAGE");
		return 0;
	}
	return buf;
}

int openFile(const char* filename)
{
  int fh = open(filename, O_RDWR);
  if (fh < 0) {
    printf("Cannot open %s\n", filename);
    /* ERROR HANDLING; you can check errno to see what went wrong */
    exit(1);
  } 
  __u32 mode = SPI_CPHA;
  ioctl(fh, SPI_IOC_WR_MODE32, &mode);
  return fh;
}



int main(int argc, char **argv)
{
  int fh;
  static const char* filename_dev1 = "/dev/spidev1.0";
  static const char* filename_dev2 = "/dev/spidev1.1";
  const char* filename;

  if (argc > 3 || argc < 2)
  {
    printf("USAGE: %s resolver_num\n", argv[0]);
    exit(1);
  }
  int devNum = atoi(argv[1]);
  if (devNum == 1)
     filename = filename_dev2;
  else
     filename = filename_dev1;
  fh = openFile(filename);
  if (argc == 3)
  {
    dumpstat(filename, fh);
  }
  unsigned int pos;
  do_message(fh, 0x80);
  pos = do_message(fh, 0x81) * 256;
  pos |= do_message(fh, 0xff);
  unsigned int errCode = do_message(fh, 0x80);  
  close(fh);
  printf("Read 0x%04x, 0x%02x\n", pos, errCode); 
  return 0;
}
