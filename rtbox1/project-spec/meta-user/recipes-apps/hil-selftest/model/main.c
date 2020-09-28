#include "plexim/HIL_Framework.h"
#include "plexim/ScopeBuffer.h"
#include <math.h>
#include "emptyCode.h"
#include "plexim/hw_wrapper.h"

#define MODEL_PREFIX emptyCode
#define MODEL_NAME "emptyCode"
#define CONCAT_(a, b) a ## b
#define CONCAT(a, b) CONCAT_(a, b)
#define MODEL_FLOAT_TYPE CONCAT(MODEL_PREFIX, _FloatType)
#define MODEL_SAMPLE_TIME CONCAT(MODEL_PREFIX, _sampleTime)
#define MODEL_STEP CONCAT(MODEL_PREFIX, _step)
#define MODEL_ERROR_STATUS CONCAT(MODEL_PREFIX, _errorStatus)
#define MODEL_CHECKSUM CONCAT(MODEL_PREFIX, _checksum)
#define MODEL_INITIALIZE CONCAT(MODEL_PREFIX, _initialize)
#define MODEL_TERMINATE CONCAT(MODEL_PREFIX, _terminate)

#define min(a, b) ((a) < (b)) ? a : b
#define likely(x)       __builtin_expect((x),1)
#define unlikely(x)     __builtin_expect((x),0)

#if defined(EXTERNAL_MODE) && EXTERNAL_MODE
#define MODEL_NUM_EXT_MODE_SIGNALS CONCAT(MODEL_PREFIX, _NumExtModeSignals)
#define MODEL_EXT_MODE_SIGNALS CONCAT(MODEL_PREFIX, _ExtModeSignals)
#define MODEL_NUM_TUNABLE_PARAMETERS CONCAT(MODEL_PREFIX, _NumTunableParameters)
#define MODEL_TUNABLE_PARAMETERS CONCAT(MODEL_PREFIX, _P)

#pragma pack(push, 4)

struct ArmResponse
{
   int msg;
   int mMsgLength;
   int mTransactionId;
   int mLength;
   float mSampleTime;
   int mBufferIndex;
   int mOffset;
   int mNumActiveSignals;
   unsigned short mActiveSignals[MODEL_NUM_EXT_MODE_SIGNALS];
};

struct TriggerInfo
{
   int mTransactionId;
   int mLength;
   int mEdge; 
   float mTriggerValue;
   int mTriggerChannel;
   int mDecimation;
   int mTriggerDelay;
   int mNumActiveSignals;
   unsigned short mActiveSignals[MODEL_NUM_EXT_MODE_SIGNALS];
   const MODEL_FLOAT_TYPE* mTriggerChannelValue;
   const MODEL_FLOAT_TYPE* mActiveSignalValues[MODEL_NUM_EXT_MODE_SIGNALS];
};

#pragma pack(pop)

static volatile struct TriggerInfo mTriggerInfo;

static void initializeScopeTrigger()
{
   if (mTriggerInfo.mTriggerChannel >= 0)
      mTriggerInfo.mTriggerChannelValue = MODEL_EXT_MODE_SIGNALS[mTriggerInfo.mTriggerChannel];
   else
      mTriggerInfo.mTriggerChannelValue = MODEL_EXT_MODE_SIGNALS[0];
   int i=0;
   for (i=0; i<mTriggerInfo.mNumActiveSignals; i++)
   {
      int signalIdx = min(mTriggerInfo.mActiveSignals[i], MODEL_NUM_EXT_MODE_SIGNALS-1);
      mTriggerInfo.mActiveSignalValues[i] = MODEL_EXT_MODE_SIGNALS[signalIdx];
   }
}

#else
void initializeScopeTrigger() {}
#endif /* defined(EXTERNAL_MODE) */


static void modelInitFunction()
{
   MODEL_INITIALIZE(0);
   postInitCode();
   // Run step function once to have code in cache
   if (!plxErrorFlag)
      MODEL_STEP();
   if (!plxErrorFlag)
   {
      MODEL_TERMINATE();
      MODEL_INITIALIZE(0);
      postInitCode();
   }
   xil_printf("SFP In:  0x%x\n", (int)SFPInput);
   xil_printf("SFP Out: 0x%x\n", (int)SFPOutput);
}

static void modelStepFunction()
{
   MODEL_STEP();
   
   if(unlikely((int)MODEL_ERROR_STATUS))
   {
      stopTimer();
      xil_printf("%s\n",MODEL_ERROR_STATUS);
      plxErrorFlag = 1;
      waitForIrqAck();
   }
   if (unlikely(plxErrorFlag))
      return;

#if defined(EXTERNAL_MODE) && EXTERNAL_MODE
   static float lastTriggerSignal = 0;
   static int decimationCounter = 1;

   if (scopeArmed)
   {
      float triggerSignal = *mTriggerInfo.mTriggerChannelValue;
      decimationCounter--;
      if (!decimationCounter)
      {
         struct ScopeBuffer* currentBufferPtr = &plxScopeBuffer[plxCurrentWriteBuffer];
         int i=0;
         for (; i < mTriggerInfo.mNumActiveSignals; i++)
         {
            addToScopeBuffer(currentBufferPtr, *mTriggerInfo.mActiveSignalValues[i]);
         }
         if (!scopeTriggered && getScopeBufferNumValidElements(currentBufferPtr) * mTriggerInfo.mDecimation > -mTriggerInfo.mTriggerDelay * mTriggerInfo.mNumActiveSignals )
         {
            if (mTriggerInfo.mEdge == 0 && lastTriggerSignal < mTriggerInfo.mTriggerValue && triggerSignal >= mTriggerInfo.mTriggerValue) scopeTriggered = TRUE;
            else if (mTriggerInfo.mEdge == 1 && lastTriggerSignal > mTriggerInfo.mTriggerValue && triggerSignal <= mTriggerInfo.mTriggerValue) scopeTriggered = TRUE;
            if (scopeTriggered)
               setScopeBufferNumValidElements(currentBufferPtr, (-mTriggerInfo.mTriggerDelay / mTriggerInfo.mDecimation + 1) * mTriggerInfo.mNumActiveSignals);
         }
         lastTriggerSignal = triggerSignal;
         if (scopeTriggered)
         {
            if (getScopeBufferNumValidElements(currentBufferPtr) == getScopeBufferSize(currentBufferPtr))
            {
               scopeArmed = FALSE;
               setScopeBufferFull(currentBufferPtr, TRUE);
               scopeTriggered = FALSE;
               plxCurrentWriteBuffer = !plxCurrentWriteBuffer;
            }
         }
         decimationCounter = mTriggerInfo.mDecimation;
      }
   }
   else
      lastTriggerSignal = NAN;
#endif /* defined(EXTERNAL_MODE) */
}

void plxSetBufferAdresses();
void clearIRQ();

void doStep()
{
   clearIRQ();
   plxSetBufferAdresses();
   waitForIrqAck();
}

int main(void)
{
   int useExternalMode = 0;
   int numExtModeSignals = 0;
   void* tunableParameterValues = NULL;
   int numTunableParameters = 0;
   struct PlxTriggerInfo* trigger = NULL;
   struct PlxArmResponse* armResponse = NULL;
   struct PlxModelFunctions modelFunctions = 
   {
      .init = &modelInitFunction,
      .step = &modelStepFunction,
      .terminate = &MODEL_TERMINATE,
      .initializeScopeTrigger = &initializeScopeTrigger
   };
#if defined(EXTERNAL_MODE) && EXTERNAL_MODE
   useExternalMode = 1;
   numExtModeSignals = MODEL_NUM_EXT_MODE_SIGNALS;
   trigger = (struct PlxTriggerInfo*)&mTriggerInfo;
   static struct ArmResponse armResponseInst;
   armResponse = (struct PlxArmResponse*)&armResponseInst;
#if (MODEL_NUM_TUNABLE_PARAMETERS > 0)
   numTunableParameters = MODEL_NUM_TUNABLE_PARAMETERS;
   tunableParameterValues = &MODEL_TUNABLE_PARAMETERS;
#endif
#endif
   plxRegisterBackgroundTask(plxStandaloneBackgroundTask);
   plxStartSimulationModel(MODEL_SAMPLE_TIME, sizeof(MODEL_FLOAT_TYPE),
                           useExternalMode, numExtModeSignals,
                           numTunableParameters, tunableParameterValues, 
                           trigger, armResponse, MODEL_CHECKSUM, MODEL_NAME,
                           &modelFunctions);
}
