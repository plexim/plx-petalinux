#include "ToFileHandler.h"
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
   int aWriteDevice, bool aUseDouble, RPCReceiver* aParent)
 : QObject(aParent),
   mParent(aParent),
   mFileName(aFileName.simplified()),
   mModelName(aModelName),
   mWidth(aWidth),
   mNumSamples(aNumSamples),
   mBufferOffset(aBufferOffset),
   mFileType(aFileType),
   mDevice(aWriteDevice),
   mFile(nullptr),
   mWriter(nullptr),
   mFileCounter(0),
   mDataTypeSize(aUseDouble ? sizeof(double) : sizeof(float))
{
   if (!onSSD())
   {
      QProcess remount;
      remount.start("mount", QStringList() << "-o" << "remount,rw" << "/dev" + devicePath() << "/run/media" + devicePath());
      remount.waitForFinished();
      remount.close();
   }
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
      stop(false);
   }
}

void ToFileHandler::writeToFileBuffer(int aCurrentReadBuffer, int aBufferLength)
{
   if (!mWriter || !mFile)
      return;
   if (checkFileSize() >= 2000000000)  // Set max File size to 2GB
   {
      rotateFile();
   }
   int index = mBufferOffset + aCurrentReadBuffer * mWidth * mNumSamples;
   QByteArray buf((const char*)mParent->getToFileBuffer() + index * mDataTypeSize,
                  aBufferLength*mDataTypeSize);
   emit writeToFileBufferRequest(buf);
   if (mFile->error() != QFile::NoError)
   {
      mParent->reportErrorMessage(QString("Error while writing file %1: %2").arg(mFile->fileName()).arg(mFile->errorString()));
      stop(true);
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
      stop(false);
   }
}


bool ToFileHandler::initToFileHandler()
{
   return true;
}

void ToFileHandler::stop(bool aDueToError)
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
   QProcess remount;
   if (onSSD())
   {
      remount.start("sync");
      remount.waitForFinished();
      remount.close();
   }
   else
   {
      QString localPath = "/run/media" + devicePath();
      QDir dir(localPath);
      if (dir.exists())
      {
         remount.start("mount", QStringList() << "-o" << "remount,ro" << "/dev" + devicePath() << "/run/media" + devicePath());
         remount.waitForFinished();
         remount.close();
      }
   }
   if (aDueToError)
   {
      QProcess umount;
      umount.start("umount", QStringList() << "/run/media" + devicePath());
      umount.waitForFinished();
      umount.close();
      QDir dir("/run/media");
      dir.rmdir("sdb1");
   }
}

const QString& ToFileHandler::devicePath()
{
   static const QStringList pathList = QStringList() << "/sda1" << "/sdb1" << "/mmcblk1p1";
   if (mDevice > 1)
      return pathList[mDevice-1];
   else
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
   QString localPath = "/run/media" + devicePath() + "/";
   if (onSSD())
      localPath += "ToFileTargetBlock/";
   QDir dir(localPath);
   if (dir.exists())
   {
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
      mWorkingDir = localPath + subdir;
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
      // delay error message on display until initialization is complete
      QEventLoop loop;
      QTimer::singleShot(1000, &loop, &QEventLoop::quit);
      loop.exec();
      mParent->reportErrorMessage("No USB Flash Drive detected!\n");
      stop(false);
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
      mParent->reportErrorMessage(
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

