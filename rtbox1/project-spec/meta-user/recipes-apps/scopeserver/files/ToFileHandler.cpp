#include "ToFileHandler.h"
#include "RPCReceiver.h"
#include <QtCore/QTextStream>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QDateTime>
#include <QtCore/QVector>
#include <QtCore/QProcess>
#include <QtCore/QTimer>
#include <QtCore/QEventLoop>
#include <QDebug>
#include <QtCore/QThread>

QString ToFileHandler::mWorkingDir;

ToFileHandler::ToFileHandler(QString aModelName, QString aFileName,
   int aWidth, int aNumSamples, int aBufferOffset, int aFileType,
   bool aUseDouble, RPCReceiver* aParent)
 : QObject(aParent),
   mParent(aParent),
   mFileName(aFileName.simplified()),
   mModelName(aModelName),
   mWidth(aWidth),
   mNumSamples(aNumSamples),
   mBufferOffset(aBufferOffset),
   mFileType(aFileType),
   mFile(nullptr),
   mWriter(nullptr),
   mFileCounter(0),
   mDataTypeSize(aUseDouble ? sizeof(double) : sizeof(float))
{
   QProcess remount;
   remount.start("mount", QStringList() << "-o" << "remount,rw" << "/dev" + devicePath() << "/run/media/sda1");
   remount.waitForFinished();
   remount.close();

   if (createDir() && createFile())
   {
      if (mFileType == 2)
      {
         mWriter = new MatFileWriter(mFile, mWidth, aUseDouble);
      }
      else
      {
         if (aUseDouble)
            mWriter = new CsvDoubleFileWriter(mFile, mWidth);
         else
            mWriter = new CsvFloatFileWriter(mFile, mWidth);
      }
   
      QThread* writerThread = new QThread(this);
      mWriter->moveToThread(writerThread);
      QEventLoop eventLoop;
      connect(writerThread, &QThread::started, &eventLoop, &QEventLoop::quit);
      connect(this, &ToFileHandler::writeToFileBufferRequest, mWriter, &Writer::writeToFileBuffer);
      writerThread->start(QThread::NormalPriority);
      eventLoop.exec();
   }
   else
   {
      stop();
   }
}

void ToFileHandler::writeToFileBuffer(int aCurrentReadBuffer, int aBufferLength)
{
   if (!mWriter || !mFile)
      return;
   if (checkFileSize() >= 2000000000) // Maximum file size is 2GB
   {
      rotateFile();
   }
   int index = mBufferOffset + aCurrentReadBuffer * mWidth * mNumSamples;
   QByteArray buf((const char*)mParent->getToFileBuffer() + index * mDataTypeSize,
                  aBufferLength*mDataTypeSize);
   emit writeToFileBufferRequest(buf);
   if (mFile->error() != QFile::NoError)
   {
      mParent->reportError(QString("Error while writing file %1: %2").arg(mFile->fileName()).arg(mFile->errorString()));
      stop();
   }
}

void ToFileHandler::rotateFile()
{
   mWriter->thread()->quit();
   mWriter->thread()->wait();
   mWriter->writeTerminate();
   mFile->close();
   delete mFile;
   mFile = nullptr;
   if (createFile())
   {
      mWriter->rotateFile(mFile);
      mWriter->thread()->start(QThread::NormalPriority);
   }
   else
   {
      stop();
   }
}

bool ToFileHandler::initToFileHandler()
{
   return true;
}

void ToFileHandler::stop()
{
   if (mWriter)
   {
      mWriter->thread()->quit();
      mWriter->thread()->wait();
      mWriter->thread()->deleteLater();
      delete mWriter;
      mWriter = nullptr;
   }
   if (mFile)
   {
      mFile->close();
      delete mFile;
      mFile = nullptr;
   }
   {
      QProcess remount;
      QString localPath = "/run/media/sda1";
      QDir dir(localPath);
      if (dir.exists())
      {
         remount.start("mount", QStringList() << "-o" << "remount,ro" << "/dev" + devicePath() << "/www/pages/dav/usb");
         remount.waitForFinished();
         remount.close();
      }
   }
}

const QString& ToFileHandler::devicePath()
{
   static const QStringList pathList = QStringList() << "/sda1";
   return pathList[0];
}

void ToFileHandler::log(QString aString)
{
   mParent->log(aString);
}

void ToFileHandler::error(QString aString)
{
   mParent->reportError(aString);
}

bool ToFileHandler::createDir()
{
   if (!mWorkingDir.isEmpty()) // mWorkingDir already set
      return true;
   QString prefix = mModelName;
   QString localPath = "/dev" + devicePath();
   if (QFileInfo::exists(localPath))
   {
      QString mountPoint = "/run/media/sda1/";
      QDir dir(mountPoint);
      const QStringList existingFiles = dir.entryList(QStringList() << (prefix + "_????*"));
      int max = 0;
      int from = prefix.length() + 1;
      for (const QString& name : existingFiles)
      {
         int num = name.midRef(from).toInt();
         if (max < num)
            max = num;
      }
      QString subdir = prefix + QString::asprintf("_%04d", max+1);
      mWorkingDir = mountPoint + subdir;
      dir.mkdir(subdir);
      QFile latest(dir.filePath("latest.txt"));
      if (latest.open(QIODevice::WriteOnly | QIODevice::Truncate))
      {
         latest.write(subdir.toUtf8());
         latest.close();
      }
   }
   else
   {
      mParent->reportError("No USB Flash Drive detected!\n");
      stop();
      return false;
   }
   return true;
}

bool ToFileHandler::createFile()
{
   bool success = true;
   mFile = new QFile;
   QString suffix;
   if (mFileCounter)
   {
      suffix = "_" + QString::number(mFileCounter);
   }
   mFileCounter++;
   QString fileName = mFileName + suffix;
   QDir dir(mWorkingDir);
   QIODevice::OpenMode mode;
   if (mFileType == 2)
   {
      fileName.append(".mat");
      mode = QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Unbuffered;
   }
   else
   {
      fileName.append(".csv");
      mode = QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text;
   }
   mFile->setFileName(dir.filePath(fileName));
   if (!mFile->open(mode))
   {
      mParent->reportError(
         QString("Error while opening file '%1' for writing: %2.\n")
            .arg(mFile->fileName())
            .arg(mFile->errorString())
      );
      mFile->close();
      success = false;
   }
   return success;
}

qint64 ToFileHandler::checkFileSize() const
{
   return mFile->size();
}
