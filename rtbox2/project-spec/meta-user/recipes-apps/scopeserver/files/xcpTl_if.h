#ifndef __XCP_UTIL_H_ 
#define __XCP_UTIL_H_


#ifdef __cplusplus
extern "C" {
#endif

#include "xcptl_cfg.h"

int udpTlSendCrmPacket(void* aBuffer);
int udpTlSendDtoPacket(void* aBuffer);

#ifdef __cplusplus
}
#endif

#endif
