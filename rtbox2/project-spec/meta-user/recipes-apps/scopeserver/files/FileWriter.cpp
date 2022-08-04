#include "FileWriter.h"
#include <QtCore/QDataStream>
#include <QtCore/QDateTime>
#include "ToFileHandler.h"
#include <QDebug>
#include <QElapsedTimer>

Writer::Writer(){}

Writer::~Writer(){}

void Writer::log(QString aString)
{

}

void Writer::error(QString aString)
{

}

void Writer::rotateFile(QFile* aFile)
{
   mFile = aFile;
   initFile();
}

void Writer::writeToFileBuffer(const QByteArray& aData)
{
   appendData(aData.data(), aData.size()/sizeof(float));
}

bool Writer::getBuffer(QVector<float>& aBuffer, const char *aBufferAddress)
{
   size_t len = aBuffer.size() * sizeof(float);
   const char* buf = aBufferAddress;
   memcpy(aBuffer.data(), buf, len);
   return true;
}


MatFileWriter::MatFileWriter(QFile* aFile, int aWidth)
 : mTotalSize(0)
{
   mFile = aFile;
   mWidth= aWidth;
   if (mFile->isSequential())
   {
      error(QString("Cannot stream data to file %1: Not a random access file.").arg(aFile->fileName()));
   }
   MatFileWriter::initFile();
}

MatFileWriter::~MatFileWriter()
{
   writeTerminate();
}

bool MatFileWriter::appendData(const char *aBuffer, qint64 aSize)
{
      if (mFile->size() + aSize * sizeof(float) > std::numeric_limits<unsigned int>::max())
      {
         error(QString("Data file exeeds maximum size (%1 bytes).").arg(std::numeric_limits<unsigned int>::max()));
         return false;
      }
      bool success = false;
      success = writeBuffer(aBuffer, aSize * sizeof(float));
      mTotalSize += aSize;
      return success;
}

void MatFileWriter::writeMatHeader()
{
   QByteArray headerBuffer;
   headerBuffer.resize(128);
   MatHeader* header = (MatHeader*)(headerBuffer.data());
   QString txt = "MATLAB 5.0 MAT-file, written by PLECS RT Box, " +
      QDateTime::currentDateTime().toUTC().toString(Qt::ISODate) + " UTC";
   strncpy(header->text, txt.toLatin1().data(), 116);
   for (int i=0; i<8; i++)
   {
      header->subsystemDataOffset[i] = 0;
   }
   header->version = 0x0100;
   header->endianIndicator = ('M' << 8) + 'I';
   writeBuffer(headerBuffer);
}

bool MatFileWriter::writeBuffer(const QByteArray& aBuffer)
{
   qint64 toWrite = aBuffer.size();
   qint64 offset = 0;
   while(toWrite)
   {
      qint64 writtenBytes = mFile->write(aBuffer.data() + offset, toWrite);
      if (writtenBytes < 0)
      {
         error(QString( "Error during write: %1").arg(mFile->errorString()));
         return false;
      }
      if (!writtenBytes)
      {
         error(QString("Unexpected error during write."));
         return false;
      }
      toWrite -= writtenBytes;
      offset += writtenBytes;
   }
   return true;
}

bool MatFileWriter::writeBuffer(const char* aBuffer, qint64 aSize)
{
   qint64 toWrite = aSize;
   qint64 offset = 0;
   while(toWrite)
   {
      qint64 writtenBytes = mFile->write(aBuffer + offset, toWrite);
      if (writtenBytes < 0)
      {
         error(QString( "Error during write: %1").arg(mFile->errorString()));
         return false;
      }
      if (!writtenBytes)
      {
         error(QString("Unexpected error during write."));
         return false;
      }
      toWrite -= writtenBytes;
      offset += writtenBytes;
   }
   return true;
}

QByteArray MatFileWriter::writeDataElement()
{
   QByteArray ret;
   ret.fill(0,56);
   *(unsigned int*)ret.data() = miMATRIX;
   *(unsigned int*)(ret.data() + 4) = 48;
   *(unsigned int*)(ret.data() + 8) = miUINT32;
   *(unsigned int*)(ret.data() + 12) = 8;
   *(unsigned char*)(ret.data() + 16) = 7;
   *(unsigned char*)(ret.data() + 18) = 0;
   *(unsigned char*)(ret.data() + 19) = 0;
   *(unsigned int*)(ret.data() + 24) = miINT32;
   *(unsigned int*)(ret.data() + 28) = 8;
   *(unsigned int*)(ret.data() + 32) = mWidth;
   *(unsigned short*)(ret.data() + 40) = miINT8;
   *(unsigned short*)(ret.data() + 42) = 4;
   *(unsigned char*)(ret.data() + 44) = 'd';
   *(unsigned char*)(ret.data() + 45) = 'a';
   *(unsigned char*)(ret.data() + 46) = 't';
   *(unsigned char*)(ret.data() + 47) = 'a';
   *(unsigned int*)(ret.data() + 48) = miSINGLE;
   return ret;
}

QByteArray MatFileWriter::writeRawElement(const void* aData, unsigned int aDataSize, MatDataType aDataType)
{
   QByteArray ret;
   if (aDataSize < 5 && aDataSize != 0)
   {
      // small element
      ret.fill(0, 8);
      *(unsigned short*)ret.data() = aDataType;
      *(unsigned short*)(ret.data() + 2) = aDataSize;
      memcpy(ret.data() + 4, aData, aDataSize);
   }
   else
   {
      ret.fill(0, alignDataSize(aDataSize) + 8);
      *(int*)ret.data() = aDataType;
      *(unsigned int*)(ret.data() + 4) = aDataSize;
      memcpy(ret.data() + 8, aData, aDataSize);
   }
   return ret;
}

unsigned int MatFileWriter::alignDataSize(unsigned int aSize)
{
   int ret = aSize;
   if (aSize % 8 != 0)
      ret = (aSize / 8 + 1) * 8;
   return ret;
}

void MatFileWriter::writeTerminate()
{
   if (mFile->error() == QFile::NoError && mFile->flush())
   {
      unsigned totalBytes = unsigned(mTotalSize * sizeof(float)) + 48;
      if (totalBytes % 8 != 0)
      {
         QByteArray paddingBytes;
         paddingBytes.fill(0, 4);
         writeBuffer(paddingBytes);
         totalBytes += 4;
      }
      mFile->seek(128 + 4);
      mFile->write(reinterpret_cast<const char*>(&totalBytes), 4);
      mFile->seek(128 + 32);
      mFile->write(reinterpret_cast<const char*>(&mWidth));
      unsigned cols = (unsigned)mTotalSize / (unsigned)mWidth;
      mFile->seek(128 + 36);
      mFile->write(reinterpret_cast<const char*>(&cols), 4);
      unsigned totalDataBytes = unsigned(mTotalSize * sizeof(float));
      mFile->seek(128 + 52);
      mFile->write(reinterpret_cast<const char*>(&totalDataBytes), 4);
      mTotalSize = 0;
   }
}

void MatFileWriter::initFile()
{
   writeMatHeader();
   writeBuffer(writeDataElement());
}


CsvFileWriter::CsvFileWriter(QFile* aFile, int aWidth)
{
   mFile = aFile;
   mWidth = aWidth;
   CsvFileWriter::initFile();
}

CsvFileWriter::~CsvFileWriter()
{
   writeTerminate();
}

bool CsvFileWriter::appendData(const char *aBuffer, qint64 aSize)
{
   QVector<float> buffer(aSize);
   if (!getBuffer(buffer, aBuffer))
   {
      return false;
   }
   QTextStream outStream(mFile);
   outStream.setRealNumberPrecision(8);
   int i = 0;
   for (int j=0; j < buffer.size(); j++)
   {
      outStream << buffer[j];
      i++;
      if (i == mWidth)
      {
         outStream << '\n';  //endl provokes every time a flush, leave this task to the operating system
         i = 0;
      }
      else
      {
          outStream << ",";
      }
   }
   return checkFileStatus(outStream);
}

bool CsvFileWriter::checkFileStatus(QTextStream& aStream)
{
   if (aStream.status() == QTextStream::WriteFailed)
   {
      error(QString("Error while writing to file %1: %2").arg(mFile->fileName()).arg(mFile->errorString()));
      return false;
   }
   return true;
}

void CsvFileWriter::writeTerminate()
{
   mFile->flush();
}

void CsvFileWriter::initFile()
{
   return;
}
