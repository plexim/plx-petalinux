/*
 * Implementation file for: emptyCode
 * Generated with         : PLECS 4.4.2
 *                          PLECS RT Box 1 2.0
 * Generated on           : 19 Aug 2020 08:44:55
 */
#include "emptyCode.h"
#ifndef PLECS_HEADER_emptyCode_h_
#error The wrong header file "emptyCode.h" was included. Please check your
#error include path to see whether this file name conflicts with the name
#error of another header file.
#endif /* PLECS_HEADER_emptyCode_h_ */
#if defined(__GNUC__) && (__GNUC__ > 4)
#   define _ALIGNMENT 16
#   define _RESTRICT __restrict
#   define _ALIGN __attribute__((aligned(_ALIGNMENT)))
#   if defined(__clang__)
#      if __has_builtin(__builtin_assume_aligned)
#         define _ASSUME_ALIGNED(a) __builtin_assume_aligned(a, _ALIGNMENT)
#      else
#         define _ASSUME_ALIGNED(a) a
#      endif
#   else
#      define _ASSUME_ALIGNED(a) __builtin_assume_aligned(a, _ALIGNMENT)
#   endif
#else
#   ifndef _RESTRICT
#      define _RESTRICT
#   endif
#   ifndef _ALIGN
#      define _ALIGN
#   endif
#   ifndef _ASSUME_ALIGNED
#      define _ASSUME_ALIGNED(a) a
#   endif
#endif
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <math.h>
#include <string.h>
#include "plexim/AnalogOut.h"
#include "plexim/AnalogIn.h"
#include "plexim/DigitalOut.h"
#include "plexim/SFP.h"
#include "plexim/hw_wrapper.h"
#include "plexim/DigitalIn.h"
#include "plexim/HIL_Framework.h"
#include "plexim/DataCapture.h"
#include "plexim/ProgrammableValue.h"
#define PLECSRunTimeError(msg) emptyCode_errorStatus = msg
static const double emptyCode_UNCONNECTED = 0;
static uint32_t emptyCode_tickLo;
static int32_t emptyCode_tickHi;
const char * emptyCode_errorStatus;
const double emptyCode_sampleTime = 1.00000000000000008e-05;
const char * const emptyCode_checksum =
   "1f4347e749e2d74f9618dd369b0afade909dad78";
/* Target declarations */
struct PlxDataCaptureRegistry plxDataCaptureRegistry[1];
struct PlxProgrammableValueRegistry plxProgrammableValueRegistry[1];

void emptyCode_initialize(double time)
{
   double remainder;
   size_t i;
   emptyCode_errorStatus = NULL;
   emptyCode_tickHi = floor(time/(4294967296.0*emptyCode_sampleTime));
   remainder = time - emptyCode_tickHi*4294967296.0*emptyCode_sampleTime;
   emptyCode_tickLo = floor(remainder/emptyCode_sampleTime + .5);
   remainder -= emptyCode_tickLo*emptyCode_sampleTime;
   if (fabs(remainder) > 1e-6*fabs(time))
   {
      emptyCode_errorStatus =
         "Start time must be an integer multiple of the base sample time.";
   }

   /* Target pre-initialization */
   setAnalogInputVoltage(0);
   setupDACs(3);
   setDigitalOutVoltage(0);
   plxInitDigitalOut();
   initPWMCapture();
   plxSetupAnalogSampling(1, 0, 1.00000000000000008e-05);
   plxSetMaxNumConsecutiveOverruns(5);
   setupSFPSyncMaster(0, 0, 0, 0);
   setupSFPSyncSlave(0, 0, 0);


   /* Initialization for Analog Out : 'emptyCode/Analog Out1' */
   setupAnalogOut(0, 1.000000000e+00f, 0.000000000e+00f, -inf, inf);
   setupAnalogOut(1, 1.000000000e+00f, 0.000000000e+00f, -inf, inf);
   setupAnalogOut(2, 1.000000000e+00f, 0.000000000e+00f, -inf, inf);
   setupAnalogOut(3, 1.000000000e+00f, 0.000000000e+00f, -inf, inf);
   setupAnalogOut(4, 1.000000000e+00f, 0.000000000e+00f, -inf, inf);
   setupAnalogOut(5, 1.000000000e+00f, 0.000000000e+00f, -inf, inf);
   setupAnalogOut(6, 1.000000000e+00f, 0.000000000e+00f, -inf, inf);
   setupAnalogOut(7, 1.000000000e+00f, 0.000000000e+00f, -inf, inf);
   setupAnalogOut(8, 1.000000000e+00f, 0.000000000e+00f, -inf, inf);
   setupAnalogOut(9, 1.000000000e+00f, 0.000000000e+00f, -inf, inf);
   setupAnalogOut(10, 1.000000000e+00f, 0.000000000e+00f, -inf, inf);
   setupAnalogOut(11, 1.000000000e+00f, 0.000000000e+00f, -inf, inf);
   setupAnalogOut(12, 1.000000000e+00f, 0.000000000e+00f, -inf, inf);
   setupAnalogOut(13, 1.000000000e+00f, 0.000000000e+00f, -inf, inf);
   setupAnalogOut(14, 1.000000000e+00f, 0.000000000e+00f, -inf, inf);
   setupAnalogOut(15, 1.000000000e+00f, 0.000000000e+00f, -inf, inf);

   /* Initialization for Analog In : 'emptyCode/Analog In1' */
   setupAnalogIn(0, 1.000000000e+00f, 0.000000000e+00f);
   setupAnalogIn(1, 1.000000000e+00f, 0.000000000e+00f);
   setupAnalogIn(2, 1.000000000e+00f, 0.000000000e+00f);
   setupAnalogIn(3, 1.000000000e+00f, 0.000000000e+00f);
   setupAnalogIn(4, 1.000000000e+00f, 0.000000000e+00f);
   setupAnalogIn(5, 1.000000000e+00f, 0.000000000e+00f);
   setupAnalogIn(6, 1.000000000e+00f, 0.000000000e+00f);
   setupAnalogIn(7, 1.000000000e+00f, 0.000000000e+00f);
   setupAnalogIn(8, 1.000000000e+00f, 0.000000000e+00f);
   setupAnalogIn(9, 1.000000000e+00f, 0.000000000e+00f);
   setupAnalogIn(10, 1.000000000e+00f, 0.000000000e+00f);
   setupAnalogIn(11, 1.000000000e+00f, 0.000000000e+00f);
   setupAnalogIn(12, 1.000000000e+00f, 0.000000000e+00f);
   setupAnalogIn(13, 1.000000000e+00f, 0.000000000e+00f);
   setupAnalogIn(14, 1.000000000e+00f, 0.000000000e+00f);
   setupAnalogIn(15, 1.000000000e+00f, 0.000000000e+00f);

   /* Initialization for Digital Out : 'emptyCode/Digital Out1' */
   setupDigitalOut(0, DO_GPIO, DO_NINV);
   setupDigitalOut(1, DO_GPIO, DO_NINV);
   setupDigitalOut(2, DO_GPIO, DO_NINV);
   setupDigitalOut(3, DO_GPIO, DO_NINV);
   setupDigitalOut(4, DO_GPIO, DO_NINV);
   setupDigitalOut(5, DO_GPIO, DO_NINV);
   setupDigitalOut(6, DO_GPIO, DO_NINV);
   setupDigitalOut(7, DO_GPIO, DO_NINV);
   setupDigitalOut(8, DO_GPIO, DO_NINV);
   setupDigitalOut(9, DO_GPIO, DO_NINV);
   setupDigitalOut(10, DO_GPIO, DO_NINV);
   setupDigitalOut(11, DO_GPIO, DO_NINV);
   setupDigitalOut(12, DO_GPIO, DO_NINV);
   setupDigitalOut(13, DO_GPIO, DO_NINV);
   setupDigitalOut(14, DO_GPIO, DO_NINV);
   setupDigitalOut(15, DO_GPIO, DO_NINV);
   setupDigitalOut(16, DO_GPIO, DO_NINV);
   setupDigitalOut(17, DO_GPIO, DO_NINV);
   setupDigitalOut(18, DO_GPIO, DO_NINV);
   setupDigitalOut(19, DO_GPIO, DO_NINV);
   setupDigitalOut(20, DO_GPIO, DO_NINV);
   setupDigitalOut(21, DO_GPIO, DO_NINV);
   setupDigitalOut(22, DO_GPIO, DO_NINV);
   setupDigitalOut(23, DO_GPIO, DO_NINV);
   setupDigitalOut(24, DO_GPIO, DO_NINV);
   setupDigitalOut(25, DO_GPIO, DO_NINV);
   setupDigitalOut(26, DO_GPIO, DO_NINV);
   setupDigitalOut(27, DO_GPIO, DO_NINV);
   setupDigitalOut(28, DO_GPIO, DO_NINV);
   setupDigitalOut(29, DO_GPIO, DO_NINV);
   setupDigitalOut(30, DO_GPIO, DO_NINV);
   setupDigitalOut(31, DO_GPIO, DO_NINV);

   /* Initialization for SFP In : 'emptyCode/SFP In' */
   {
      static float sfp0InitData[] = {
         0.000000000e+00f
      };
      setupSFP0In(1, sfp0InitData);
   }

   /* Initialization for SFP In : 'emptyCode/SFP In1' */
   {
      static float sfp1InitData[] = {
         0.000000000e+00f
      };
      setupSFP1In(1, sfp1InitData);
   }

   /* Initialization for SFP In : 'emptyCode/SFP In2' */
   {
      static float sfp2InitData[] = {
         0.000000000e+00f
      };
      setupSFP2In(1, sfp2InitData);
   }

   /* Initialization for SFP In : 'emptyCode/SFP In3' */
   {
      static float sfp3InitData[] = {
         0.000000000e+00f
      };
      setupSFP3In(1, sfp3InitData);
   }

   /* Initialization for SFP Out : 'emptyCode/SFP Out' */
   setupSFP0Out(1);

   /* Initialization for SFP Out : 'emptyCode/SFP Out1' */
   setupSFP1Out(1);

   /* Initialization for SFP Out : 'emptyCode/SFP Out2' */
   setupSFP2Out(1);

   /* Initialization for SFP Out : 'emptyCode/SFP Out3' */
   setupSFP3Out(1);
}

void emptyCode_step()
{
   if (emptyCode_errorStatus)
   {
      return;
   }
   /* Analog Out : 'emptyCode/Analog Out1'
    * incorporates
    *  Subsystem : 'emptyCode'
    */
   setAnalogOut(0, emptyCode_UNCONNECTED);
   setAnalogOut(1, emptyCode_UNCONNECTED);
   setAnalogOut(2, emptyCode_UNCONNECTED);
   setAnalogOut(3, emptyCode_UNCONNECTED);
   setAnalogOut(4, emptyCode_UNCONNECTED);
   setAnalogOut(5, emptyCode_UNCONNECTED);
   setAnalogOut(6, emptyCode_UNCONNECTED);
   setAnalogOut(7, emptyCode_UNCONNECTED);
   setAnalogOut(8, emptyCode_UNCONNECTED);
   setAnalogOut(9, emptyCode_UNCONNECTED);
   setAnalogOut(10, emptyCode_UNCONNECTED);
   setAnalogOut(11, emptyCode_UNCONNECTED);
   setAnalogOut(12, emptyCode_UNCONNECTED);
   setAnalogOut(13, emptyCode_UNCONNECTED);
   setAnalogOut(14, emptyCode_UNCONNECTED);
   setAnalogOut(15, emptyCode_UNCONNECTED);

   /* Digital Out : 'emptyCode/Digital Out1'
    * incorporates
    *  Subsystem : 'emptyCode'
    */
   writeDigitalOut(0, emptyCode_UNCONNECTED);
   writeDigitalOut(1, emptyCode_UNCONNECTED);
   writeDigitalOut(2, emptyCode_UNCONNECTED);
   writeDigitalOut(3, emptyCode_UNCONNECTED);
   writeDigitalOut(4, emptyCode_UNCONNECTED);
   writeDigitalOut(5, emptyCode_UNCONNECTED);
   writeDigitalOut(6, emptyCode_UNCONNECTED);
   writeDigitalOut(7, emptyCode_UNCONNECTED);
   writeDigitalOut(8, emptyCode_UNCONNECTED);
   writeDigitalOut(9, emptyCode_UNCONNECTED);
   writeDigitalOut(10, emptyCode_UNCONNECTED);
   writeDigitalOut(11, emptyCode_UNCONNECTED);
   writeDigitalOut(12, emptyCode_UNCONNECTED);
   writeDigitalOut(13, emptyCode_UNCONNECTED);
   writeDigitalOut(14, emptyCode_UNCONNECTED);
   writeDigitalOut(15, emptyCode_UNCONNECTED);
   writeDigitalOut(16, emptyCode_UNCONNECTED);
   writeDigitalOut(17, emptyCode_UNCONNECTED);
   writeDigitalOut(18, emptyCode_UNCONNECTED);
   writeDigitalOut(19, emptyCode_UNCONNECTED);
   writeDigitalOut(20, emptyCode_UNCONNECTED);
   writeDigitalOut(21, emptyCode_UNCONNECTED);
   writeDigitalOut(22, emptyCode_UNCONNECTED);
   writeDigitalOut(23, emptyCode_UNCONNECTED);
   writeDigitalOut(24, emptyCode_UNCONNECTED);
   writeDigitalOut(25, emptyCode_UNCONNECTED);
   writeDigitalOut(26, emptyCode_UNCONNECTED);
   writeDigitalOut(27, emptyCode_UNCONNECTED);
   writeDigitalOut(28, emptyCode_UNCONNECTED);
   writeDigitalOut(29, emptyCode_UNCONNECTED);
   writeDigitalOut(30, emptyCode_UNCONNECTED);
   writeDigitalOut(31, emptyCode_UNCONNECTED);

   /* SFP Out : 'emptyCode/SFP Out'
    * incorporates
    *  Subsystem : 'emptyCode'
    */
   setSFP0Out(0, emptyCode_UNCONNECTED);

   /* SFP Out : 'emptyCode/SFP Out1'
    * incorporates
    *  Subsystem : 'emptyCode'
    */
   setSFP1Out(0, emptyCode_UNCONNECTED);

   /* SFP Out : 'emptyCode/SFP Out2'
    * incorporates
    *  Subsystem : 'emptyCode'
    */
   setSFP2Out(0, emptyCode_UNCONNECTED);

   /* SFP Out : 'emptyCode/SFP Out3'
    * incorporates
    *  Subsystem : 'emptyCode'
    */
   setSFP3Out(0, emptyCode_UNCONNECTED);
   if (emptyCode_errorStatus)
   {
      return;
   }
}
void emptyCode_terminate()
{
}
