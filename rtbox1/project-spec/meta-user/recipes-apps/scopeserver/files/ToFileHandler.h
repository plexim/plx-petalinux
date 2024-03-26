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
   explicit ToFileHandler(QString aModelName, QString aFileName, int aWidth, int aNumSamples, 
                          int aBufferOffset, int aFileType,bool aUseDouble, RPCReceiver* aParent);
   void stop();
   void writeToFileBuffer(int aCurrentReadBuffer, int aBufferLength);
   void rotateFile();
   void log(QString aString);
   void error(QString aString);
   static void staticInit() { mWorkingDir.clear(); }

signals:
   void writeToFileBufferRequest(const QByteArray& aData);

protected:
   bool initToFileHandler();
   void writeData(char* aBufferIndex, int aSize);
   void writeBuffer(char* aBuffer, int aSize);
   const QString& devicePath();
   bool createDir();
   bool createFile();
   qint64 checkFileSize() const;

private:
   RPCReceiver* mParent;
   QFile* mFile;
   const QString mFileName;
   const QString mModelName;
   const int mWidth;
   const int mNumSamples;
   const int mBufferOffset;
   const int mFileType;
   Writer* mWriter;
   static QString mWorkingDir;
   int mFileCounter;
   const int mDataTypeSize;
};

#endif // TOFILEHANDLER_H
