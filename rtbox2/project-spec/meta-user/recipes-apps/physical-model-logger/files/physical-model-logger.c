#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <string.h>

#define CTRL_UNIT_ADDR 0x80027000
#define CTRL_UNIT_SZ 1024

#define LOGGER_BUFFER_ADDR 0x80030000
#define LOGGER_BUFFER_SZ 16384


static int map_addr(off_t addr, size_t size, void **ptr, void **aligned_ptr, size_t *aligned_size)
{
    long page_size = sysconf(_SC_PAGE_SIZE);
    off_t aligned_addr = (addr / page_size) * page_size;
    off_t offset = addr - aligned_addr;
    *aligned_size = size + offset;

    int fd = open("/dev/mem", O_RDWR|O_SYNC);
    *aligned_ptr = mmap(NULL, *aligned_size, PROT_READ|PROT_WRITE, MAP_SHARED, fd, aligned_addr);
    close(fd);

    if (*aligned_ptr == MAP_FAILED)
        return -1;

    *ptr = (char*)(*aligned_ptr) + offset;

    return 0;
}


int main(int argc, char **argv)
{
    volatile unsigned *logbuf_aligned;
    volatile unsigned *logbuf;
    size_t logbuf_aligned_size;

    if (map_addr(LOGGER_BUFFER_ADDR, LOGGER_BUFFER_SZ*4, &logbuf, &logbuf_aligned, &logbuf_aligned_size))
    {
	perror("Cannot mmap logger buffer");
	return -1;
    }

    volatile unsigned *ctrl_unit_aligned;
    volatile unsigned *ctrl_unit;
    size_t ctrl_unit_aligned_size;

    if (map_addr(CTRL_UNIT_ADDR, CTRL_UNIT_SZ*4, &ctrl_unit, &ctrl_unit_aligned, &ctrl_unit_aligned_size))
    {
	perror("Cannot mmap control unit");
	return -1;
    }

    volatile unsigned *logctrl = (unsigned *)((char *)ctrl_unit + 0x0104);


    int samples_per_row = 1;
    int do_trigger = 0;
    int decimation = 1;
    int ch;
    while ((ch = getopt(argc, argv, "n:td:h")) != -1)
    {
	switch (ch)
	{
	case 'n':
	    samples_per_row = atoi(optarg);
	    break;

	case 't':
	    do_trigger = 1;
	    break;

	case 'd':
	    decimation = atoi(optarg);
	    break;

	case 'h':
	default:
	    fprintf(stderr, "Usage: %s [-n samples_per_row] [-t [-d decimation]]\n", argv[0]);
	    return -1;
	}
    }

    if (do_trigger)
    {
	*logctrl = decimation;
	while (*logctrl == 0) {}
    }

    int nsamp = (LOGGER_BUFFER_SZ/samples_per_row)*samples_per_row;
    int m = 0;
    for (int i = 0; i < nsamp; ++i)
    {
	float x;
	memcpy(&x, logbuf+m, 4);
	printf("%20.8e", x);

	m++;
	if ((m % samples_per_row) == 0)
	    printf("\n");
    }

    munmap(logbuf_aligned, logbuf_aligned_size);

    return 0;
}
