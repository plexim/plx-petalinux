#ifndef RPUMSG_H
#define RPUMSG_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct rpumsg_params {
    const char *shm_dev_name;
    uint32_t shm_msgbuf_offset;
    uint32_t shm_msgbuf_size;

    const char *ipi_dev_name;
    int ipi_chn_remote;
};

int rpumsg_initialize(struct rpumsg_params *params);
int rpumsg_is_initialized();
int rpumsg_terminate();
int rpumsg_post_message(void *message, uint32_t length);
int rpumsg_notify_remote();

int rpumsg_shm_read(int ofs, void *dst, int size);
int rpumsg_shm_write(int ofs, const void *src, int size);

#ifdef __cplusplus
}
#endif

#endif
