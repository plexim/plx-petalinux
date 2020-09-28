#include "rpumsg.h"

#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <stdarg.h>

#include <stdint.h>
#include <metal/io.h>
#include <metal/device.h>

#define BUS_NAME "platform"

#define IPI_TRIG_OFFSET 0x00
#define IPI_OBS_OFFSET  0x04
#define IPI_ISR_OFFSET  0x10
#define IPI_IMR_OFFSET  0x14
#define IPI_IER_OFFSET  0x18
#define IPI_IDR_OFFSET  0x1C


static uint32_t _ipi_chn_masks[11] = {
    1UL << 0,
    1UL << 8,
    1UL << 9,
    0,
    0,
    0,
    0,
    1UL << 24,
    1UL << 25,
    1UL << 26,
    1UL << 27
};

static struct metal_device *_shm_device = 0;
static struct metal_io_region *_shm_io = 0;
static struct metal_device *_ipi_device = 0;
static struct metal_io_region *_ipi_io = 0;
static uint32_t _ipi_remote_mask = 0;

static uint32_t _head_offset = 0;
static uint32_t _tail_offset = 0;
static uint32_t _buffer_start_offset = 0;
static uint32_t _buffer_size = 0;

static int _is_initialized = 0;


#define logerr(format, ...) _logerr(__func__, format, ##__VA_ARGS__)

static void _logerr(const char *func, const char *format, ...)
{
    va_list args;

    if (func)
        fprintf(stderr, "%s: ", func);

    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
}


int rpumsg_initialize(struct rpumsg_params *par)
{
    int ret;

    ret = metal_device_open(BUS_NAME, par->shm_dev_name, &_shm_device);
    if (ret)
    {
        logerr("Failed to open device %s.\n", par->shm_dev_name);
        goto out;
    }

    _shm_io = metal_device_io_region(_shm_device, 0);
    if (!_shm_io) {
        logerr("Failed to get io region for %s.\n", _shm_device->name);
        ret = -ENODEV;
        goto out;
    }

    ret = metal_device_open(BUS_NAME, par->ipi_dev_name, &_ipi_device);
    if (ret)
    {
        logerr("Failed to open device %s.\n", par->ipi_dev_name);
        goto out;
    }

    _ipi_io = metal_device_io_region(_ipi_device, 0);
    if (!_ipi_io)
    {
        logerr("Failed to map io region for %s.\n", _ipi_device->name);
        ret = -ENODEV;
        goto out;
    }

    switch (par->ipi_chn_remote)
    {
    case 0:
    case 1:
    case 2:
    case 7:
    case 8:
    case 9:
    case 10:
        _ipi_remote_mask = _ipi_chn_masks[par->ipi_chn_remote];
        break;

    default:
        ret = -EINVAL;
        goto out;
    }

    _head_offset = par->shm_msgbuf_offset;
    _tail_offset = _head_offset + sizeof(uint32_t);
    _buffer_start_offset = _tail_offset + sizeof(uint32_t);
    _buffer_size = par->shm_msgbuf_size;

    if (_buffer_size % 4)
    {
        logerr("Buffer size must be a multiple of 4.\n");
        ret = -EINVAL;
        goto out;
    }

    if (_buffer_start_offset + _buffer_size > _shm_io->size)
    {
        logerr("Buffer does not fit in the shared memory region.\n");
        ret = -EINVAL;
        goto out;
    }

    metal_io_write32(_shm_io, _head_offset, 0);
    metal_io_write32(_shm_io, _tail_offset, 0);

    _is_initialized = 1;

    return 0;

out:
    if (_shm_device)
        metal_device_close(_shm_device);

    if (_ipi_device)
        metal_device_close(_ipi_device);

    return ret;
}


int rpumsg_terminate()
{
    if (_shm_device)
        metal_device_close(_shm_device);

    if (_ipi_device)
        metal_device_close(_ipi_device);

    _is_initialized = 0;

    return 0;
}


int rpumsg_is_initialized()
{
    return _is_initialized;
}


int rpumsg_notify_remote()
{
    metal_io_write32(_ipi_io, IPI_TRIG_OFFSET, _ipi_remote_mask);

    return 0;
}


static void mod_increment(uint32_t *val, uint32_t inc)
{
    *val = (*val + inc) % _buffer_size;
}


int rpumsg_post_message(void *message, uint32_t length)
{
    if (message == 0 || length == 0)
        return -EINVAL;

    uint32_t head = metal_io_read32(_shm_io, _head_offset);
    uint32_t tail = metal_io_read32(_shm_io, _tail_offset);

    uint32_t length_padded = (length + 3) & 0xFFFFFFFC;
    uint32_t avail;

    if (head >= tail)
    {
        avail = (_buffer_size - head + tail) - 4;

        if (avail < (4 + length_padded))
            return -ENOMEM;

        metal_io_write32(_shm_io, _buffer_start_offset + head, length);
        mod_increment(&head, 4);

        uint32_t len1 = _buffer_size - head;
        if (length < len1)
            len1 = length;

        metal_io_block_write(_shm_io, _buffer_start_offset + head, message, len1);

        length -= len1;

        if (length)
            metal_io_block_write(_shm_io, _buffer_start_offset + 0, ((char*) message) + len1, length);

        mod_increment(&head, length_padded);
    }
    else
    {
        avail = (tail - head) - 4;

        if (avail < (4 + length_padded))
            return -ENOMEM;

        metal_io_write32(_shm_io, _buffer_start_offset + head, length);
        mod_increment(&head, 4);

        if (length)
            metal_io_block_write(_shm_io, _buffer_start_offset + head, message, length);

        mod_increment(&head, length_padded);
    }


    metal_io_write32(_shm_io, _head_offset, head);

    return 0;
}


int rpumsg_shm_read(int ofs, void *dst, int size)
{
    return metal_io_block_read(_shm_io, ofs, dst, size);
}


int rpumsg_shm_write(int ofs, const void *src, int size)
{
    return metal_io_block_write(_shm_io, ofs, src, size);
}
