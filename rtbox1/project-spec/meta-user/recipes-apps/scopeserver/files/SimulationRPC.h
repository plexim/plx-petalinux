/*
 * Copyright (c) 2015-2020 Plexim GmbH.
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

class ServerAsync;
class QVariant;
struct MsgHdr;
class RPCReceiver;
class FanControl;

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
      int mMaxCycleTime;
      int mRunningCycleTime;
   };

   struct SimulationStartMsg
   {
      int mMsg;
      int mMsgLength;
      int mStartOnFirstTrigger;
      uint64_t mStartSec;
      uint64_t mStartUSec;
   };

   SimulationRPC(ServerAsync& aServer);
   ~SimulationRPC();
   bool armScope(const struct ArmMessage& aMsg);
   bool querySimulation(double& aSampleTime, int& aNumScopeSignals, 
                        int& aNumTuneableParameters, struct VersionType& aLibraryVersion, 
                        QByteArray& aChecksum, QByteArray& aModelName, int& aAnalogOutVoltageRange,
                        int& aAnalogInVoltageRange, int& aDigitalOutVoltage);
   bool verifyAPIVersion(int& aVersion);
   bool getScopeBuffer(QVector<float> &aBuffer, int aBufferIndex, int aOffset);
   bool openConnection(bool aVerbose=true);
   void shutdownSimulation();
   bool readPerformanceCounter(QueryPerformanceCounterResponse& aResponse);
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
   inline SimulationStatus getStatus() const { return mSimulationStatus; }
   bool isRTBoxCE() const { return mRTBoxCE; }

signals:
   void initComplete();
   void sendRequest(QByteArray);
   void shutdownRequest();

public slots:
   void log(QString aMsg);
   void reportError(QString aMsg);

protected slots:
   void syncModelInfos();
   void receiveError(QString aError);
   void scopeArmResponse(QByteArray);
   void tuneParameterResponse(int);
   void simulationError();

protected:
   bool verifyAPIVersion(bool aVerbose);
   void initDigitalInputs(const QByteArray& aConfig);
   void poke(quint32 aAddress, quint32 aValue);
   bool doQuerySimulation();
   bool checkRunning();
   bool sendMsg(const struct MsgHdr* aMessage);
   bool closeConnection();
   bool syncDataCaptureInfo(int aInstance);
   bool syncDataBlockInfos();
   bool syncDigitalInputConfig();
   void setRunning(bool aRun);

private:
   struct QueryModelResponse {
      double mSampleTime;
      qint32 mNumScopeSignals;
      qint32 mNumTuneableParameters;
      quint32 mScopeBufferAddress;
      quint32 mScopeBufferSize;
      quint32 mTxBufferAddress;
      quint32 mRxBufferAddress;
      quint32 mRxTxBufferSize;
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
   QFile mSharedMemoryFile;
   QFile mTxBufferFile;
   volatile uchar* mSharedMemory;
   volatile void* mRxBuffer;
   void* mTxBuffer;
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
   FanControl* mFanControl;
   bool mWaitForFirstTrigger;
   SimulationStatus mSimulationStatus;
   std::chrono::steady_clock::time_point mLastScopeArmTime;
   bool mRTBoxCE;
};

#pragma pack(pop)

#endif
