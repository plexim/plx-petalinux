#ifndef TOFILEHANDLER_H
#define TOFILEHANDLER_H

#include <QtCore/QObject>
#include "SimulationRPC.h"
#include <QtCore/QVector>
#include "FileWriter.h"

class RPCReceiver;
class QFile;

class ToFileHandler : public QObject
{
   Q_OBJECT

public:
   explicit ToFileHandler(QString aModelName, QString aFileName, int aWidth, int aNumSamples, int aBufferOffset, int aFileType, int aWriteDevice, RPCReceiver* aParent);
   void stop(bool aDueToError);
   void writeToFileBuffer(int aCurrentReadBuffer);
   void log(QString aString);
   void error(QString aString);
   static void staticInit() { mWorkingDir.clear(); }

signals:
   void writeToFileBufferRequest(const QByteArray& aData);

public slots:

protected:
   bool initToFileHandler();
   void writeData(char* aBufferIndex, int aSize);
   void writeBuffer(char* aBuffer, int aSize);
   const QString& devicePath();
   bool createDir();
   bool createFile();
   qint64 checkFileSize() const;
   inline bool onSSD() const { return mDevice < 2; }

private:
   RPCReceiver* mParent;
   QFile* mFile;
   const QString mFileName;
   const QString mModelName;
   const int mWidth;
   const int mNumSamples;
   const int mBufferOffset;
   const int mFileType;
   const int mDevice;
   Writer* mWriter;
   static QString mWorkingDir;
   int mFileCounter;
};

#endif // TOFILEHANDLER_H
