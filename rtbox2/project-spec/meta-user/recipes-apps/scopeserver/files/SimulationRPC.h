/*
 * Copyright (c) 2020 Plexim GmbH.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 3, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef _SIMULATIONRPC_H_
#define _SIMULATIONRPC_H_

#include <QtCore/QFile>
#include <QtCore/QByteArray>
#include <QtCore/QVector>
#include <QtCore/QMap>
#include <QtCore/QPointer>
#include <memory>
#include <chrono>
#include "RPCReceiver.h"

class ServerAsync;
class QVariant;
struct MsgHdr;
class TemperatureControl;
class PerformanceCounter;

#pragma pack(push, 4)

class SimulationRPC : public QObject
{
   Q_OBJECT

public:

   enum MsgType {
      RSP_SCOPE_DATA_READY = 1,
      MSG_ARM_SCOPE,
      MSG_RESET_PERFORMANCE_COUNTER,
      MSG_QUERY_MODEL,
      RSP_QUERY_MODEL,
      MSG_QUERY_PERFORMANCE_COUNTER,
      RSP_PERFORMANCE_COUNTER,
      MSG_TUNE_PARAMETERS,
      RSP_TUNE_PARAMETERS,
      MSG_QUERY_API_VERSION, // 10
      RSP_QUERY_API_VERSION,
      MSG_GET_DATA_CAPTURE_DATA,
      RSP_GET_DATA_CAPTURE_DATA,
      MSG_GET_DATA_CAPTURE_BUFFER_FULL_COUNT,
      RSP_GET_DATA_CAPTURE_BUFFER_FULL_COUNT,
      MSG_GET_NUM_DATA_BLOCKS,
      RSP_GET_NUM_DATA_BLOCKS,
      MSG_GET_DATA_CAPTURE_INFO,
      RSP_GET_DATA_CAPTURE_INFO,
      NOTIFICATION_INIT_COMPLETE, // 20
      MSG_GET_PROGRAMMABLE_VALUE_INFO,
      RSP_GET_PROGRAMMABLE_VALUE_INFO,
      MSG_SET_PROGRAMMABLE_VALUE_DATA,
      RSP_SET_PROGRAMMABLE_VALUE_DATA,
      MSG_RAW,
      RSP_RAW,
      CAN_INIT,
      CAN_TRANSMIT,
      CAN_REQUEST_ID,
      CAN_RECEIVE, // 30
      UDP_SEND,
      UDP_REQUEST_PORTS,
      UDP_RECEIVE,
      MSG_ERROR,
      MSG_START_SIMULATION,
      MSG_QUERY_DI_CONFIG,
      RSP_QUERY_DI_CONFIG,
      MSG_LOG,
      TO_FILE_BUFFER_FULL,
      MSG_GET_TO_FILE_INFO, // 40
      RSP_GET_TO_FILE_INFO,
      MSG_XCP_CMD,
      RSP_XCP_SEND_DTO,
      RSP_XCP_SEND_CTO,
      NOTIFICATION_INIT_ETHERCAT
   };

   enum class SimulationStatus {
      STOPPED,
      RUNNING,
      ERROR,
   };

   struct VersionType {
      int mVersionMajor;
      int mVersionMinor;
      int mVersionPatch;
   };

   struct ArmMessage {
      int mMsg;
      int mMsgLength;
      qint32 mTransactionId;
      int mLength;
      int mTriggerEdge;
      float mTriggerValue;
      int mTriggerChannel;
      int mDecimation;
      int mTriggerDelay;
      int mNumActiveSignals;
      quint16 mRequestedSignals; // placeholder for array of length mNumActiveSignals
   };

   struct ArmResponse {
      qint32 mTransactionId;
      qint32 mLength;
      float mSampleTime;
      int mBufferIndex;
      int mOffset;
      qint32 mNumActiveSignals;
      quint16 mActiveSignals; // placeholder for array of length mNumActiveSignals
   };

   struct CanSetupMsg {
      uint8_t  mModuleId;
      uint8_t  reserverd1;
      uint8_t  reserverd2;
      uint8_t  reserverd3;
      uint32_t mBaudRate;
      int32_t  mTermination;
      uint32_t mRecoveryTimeout;
   };
 
   struct CanTransmitMsg {
      uint8_t  mModuleId;
      uint8_t  reserverd1;
      uint8_t  reserverd2;
      uint8_t  reserverd3;
      uint32_t mCanId;
      uint8_t  mDataLength;
      uint8_t  reserverd4;
      uint8_t  reserverd5;
      uint8_t  reserverd6;
      uint8_t mData[8];
   };

   struct CanRequestIdMsg {
      uint8_t  mModuleId;
      uint8_t  reserverd1;
      uint8_t  reserverd2;
      uint8_t  reserverd3;
      uint32_t mNumCanIds;
      uint32_t mCanId;
   };

   struct UdpSendMsg {
      uint32_t mAddress;
      uint16_t mPort;
      int32_t mSize;
      char mData[1]; // placeholder dummy array of length 1
   };

   struct UdpRequestPortsMsg {
      uint32_t mNumPorts;
      uint16_t mPorts[1]; // placeholder dummy array of length 1
   };

   struct UdpReceiveMsg
   {
      uint16_t mPort;
      uint16_t mDataLength;
      const char mData[1]; // dummy placeholder array of length 1
   };

   struct QueryPerformanceCounterResponse
   {
      int mMaxCycleTime1;
      int mRunningCycleTime1;
      int mMaxCycleTime2;
      int mRunningCycleTime2;
      int mMaxCycleTime3;
      int mRunningCycleTime3;
   };

   struct ToFileBufferFullMsg
   {
      uint8_t mInstance;
      int mCurrentReadBuffer;
   };

   struct ToFileInfo
   {
      int mWidth;
      int mNumSamples;
      int mBufferOffset;
   };

   struct SimulationStartMsg
   {
      int mMsg;
      int mMsgLength;
      int mStartOnFirstTrigger;
      uint64_t mStartSec;
      uint64_t mStartUSec;
      uint16_t mHwVersionMajor;
      uint16_t mHwVersionMinor;
   };

   struct XcpCommandMsg
   {
      int mMsg;
      int mMsgLength;
      char mData[1]; // dummy placeholder array of length 1
   };

   SimulationRPC(ServerAsync& aServer);
   ~SimulationRPC();
   bool armScope(const struct ArmMessage& aMsg);
   bool querySimulation(double& aSampleTime, int& aNumScopeSignals, 
                        int& aNumTuneableParameters, struct VersionType& aLibraryVersion, 
                        QByteArray& aChecksum, QByteArray& aModelName, int& aAnalogOutVoltageRange,
                        int& aAnalogInVoltageRange, int& aDigitalOutVoltage);
   bool verifyAPIVersion(int& aVersion);
   bool getScopeBuffer(uint8_t* aData, int aLength, int aBufferIndex, int aOffset);
   bool openConnection();
   bool checkConnection(bool aVerbose=true);
   void shutdownSimulation();
   void resetPerformanceCounter();
   bool tuneParameters(QByteArray& aParameters);
   QString startSimulation(bool aWaitForFirstTrigger);
   inline int getModelTimeStamp() const { return mModelTimeStamp; }
   quint32 peek(quint32 aAddress);
   inline quint32 getScopeBufferSize() { return mLastReceivedModelResponse.mScopeBufferSize / 2; }
   int getDataCaptureTriggerCount(const QString& aDataCapturePath);
   void getDataCaptureData(const QString& aDataCapturePath, QVariantList& aData, int& aTriggerCount, double& aSampleTime);
   inline double getSampleTime() const { return mLastReceivedModelResponse.mSampleTime; }
   void setProgrammableValueData(const QString& aDataCapturePath, const QVariant& aData);
   bool sendRawMessage(const QByteArray& aMessage);
   QStringList getDataCaptureBlocks() const;
   QStringList getProgrammableValueBlocks() const;
   bool isRtBox3() const { return mRtBox3; }
   const QByteArray& getMessageBuffer() { return mMessageBuffer; }
   inline const QPointer<PerformanceCounter>& getPerformanceCounter() const { return mPerformanceCounter; }
   inline SimulationStatus getStatus() const { return mSimulationStatus; }

signals:
   void sendRequest(QByteArray);
   void shutdownRequest();
   void initCompleteDone();
   void simulationStartRequested();
   void simulationStarted();
   void simulationAboutToStop();
   void logMessage(int, QByteArray);
   void initToFileHandler(QString aFileName, QByteArray aModelName, int aWidth, int aNumSamples, int aBufferOffset, int aFileType, int aWriteDevice);

public slots:
   void log(QString aMsg);
   void reportError(QString aMsg);

protected slots:
   void initComplete();
   void initEthercat(int);
   void receiveError(QString aError);
   void scopeArmResponse(QByteArray);
   void tuneParameterResponse(int);
   void logMessageForWeb(int, QByteArray);
   void simulationError();

protected:
   bool verifyAPIVersion(bool aVerbose);
   void poke(quint32 aAddress, quint32 aValue);
   bool doQuerySimulation();
   bool checkRunning();
   bool sendMsg(const struct MsgHdr* aMessage);
   bool closeConnection();
   bool syncDataCaptureInfo(int aInstance);
   bool syncDataBlockInfos();
   bool setDigitalOut(int aGpio, bool aValue);

private:
   struct QueryModelResponse {
      double mSampleTime;
      uint32_t mNumScopeSignals;
      uint32_t mNumTuneableParameters;
      uint32_t mCorePeriodTicks[3];
      size_t mScopeBufferAddress;
      size_t mScopeBufferSize;
      size_t mToFileBufferSize;
      size_t mTxBufferAddress;
      size_t mRxBufferAddress;
      size_t mRxTxBufferSize;
      qint32 mDacSpan;
      qint32 mAnalogInputVoltageRange;
      qint32 mDigitalOutVoltage;
      qint32 mChecksumLength;
   };

   struct DataCaptureInfo
   {
      int mInstance;
      int mWidth;
      int mNumSamples;
      double mSampleTime;
   };

   struct ProgrammableValueConstInfo
   {
      int mInstance;
      int mWidth;
   };

   ServerAsync& mServer;
   const quint32 mPageSize;
   struct QueryModelResponse mLastReceivedModelResponse;
   QByteArray mLastReceivedModelChecksum;
   QByteArray mLastReceivedModelName;
   struct VersionType mLastReceivedLibraryVersion;
   struct VersionType mFirmwareVersion;
   int mModelTimeStamp;
   QMap<QString, DataCaptureInfo> mDataCaptureMap;
   QMap<QString, ProgrammableValueConstInfo> mProgrammableValueConstMap;
   QPointer<RPCReceiver> mReceiver;
   bool mRtBox3;
   TemperatureControl* mTemperatureControl;
   QByteArray mMessageBuffer;
   bool mInitComplete;
   QPointer<PerformanceCounter> mPerformanceCounter;
   bool mWaitForFirstTrigger;
   SimulationStatus mSimulationStatus;
   std::chrono::steady_clock::time_point mLastScopeArmTime;
   uint16_t mHwVersionMajor;
   uint16_t mHwVersionMinor;
   uint8_t mHwVersionRevision;
};

#pragma pack(pop)

#endif
