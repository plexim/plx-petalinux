/*
 * Header file for: emptyCode
 * Generated with : PLECS 4.4.2
 *                  PLECS RT Box 1 2.0
 * Generated on   : 19 Aug 2020 08:44:55
 */
#ifndef PLECS_HEADER_emptyCode_h_
#define PLECS_HEADER_emptyCode_h_

#include <stdbool.h>
#include <stdint.h>

/* Model floating point type */
typedef double emptyCode_FloatType;

/* Model checksum */
extern const char * const emptyCode_checksum;

/* Model error status */
extern const char * emptyCode_errorStatus;


/* Model sample time */
extern const double emptyCode_sampleTime;


/* Entry point functions */
void emptyCode_initialize(double time);
void emptyCode_step(void);
void emptyCode_terminate(void);

#endif /* PLECS_HEADER_emptyCode_h_ */
