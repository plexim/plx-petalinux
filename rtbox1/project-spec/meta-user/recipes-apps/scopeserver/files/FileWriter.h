#ifndef FILEWRITER_H
#define FILEWRITER_H

#include <QtCore/QVector>
#include <QtCore/QFile>
#include <QtCore/QIODevice>
#include <QtCore/QTextStream>

class Writer : public QObject
{

public:
   Writer(bool aUseDouble);
   virtual ~Writer();
   virtual void writeTerminate() = 0;
   void rotateFile(QFile* aFile);

public slots:
   void writeToFileBuffer(const QByteArray& aData);

protected:
   QFile* mFile;
   int mWidth;
   virtual void initFile() = 0;
   virtual bool appendData(const char* aBuffer, qint64 aSize) = 0;
   void log(QString aString);
   void error(QString aString);

private:
   const bool mUseDouble;
};


class CsvFileWriter : public Writer
{
public:
   CsvFileWriter(QFile* aFile, int aWidth, bool aUseDouble);
   ~CsvFileWriter();
protected:
   void writeTerminate();
   bool checkFileStatus(QTextStream& aStream);

private:
   virtual void initFile() override;
   const uint32_t mDataTypeSize;
};


class CsvDoubleFileWriter : public CsvFileWriter
{
public:
   CsvDoubleFileWriter(QFile* aFile, int aWidth) : CsvFileWriter(aFile, aWidth, true) {}
   ~CsvDoubleFileWriter() {}

protected:
   bool getBuffer(QVector<double>& aBuffer, const char* aBufferAddress);

private:
   virtual bool appendData(const char *aBuffer, qint64 aSize) override;
};


class CsvFloatFileWriter : public CsvFileWriter
{
public:
   CsvFloatFileWriter(QFile* aFile, int aWidth) : CsvFileWriter(aFile, aWidth, false) {}
   ~CsvFloatFileWriter() {}

protected:
   bool getBuffer(QVector<float>& aBuffer, const char* aBufferAddress);

private:
   virtual bool appendData(const char *aBuffer, qint64 aSize) override;
};


class MatFileWriter : public Writer
{
public:
   MatFileWriter(QFile* aFile, int aWidth, bool aUseDouble);
   ~MatFileWriter();

private:
   bool appendData(const char *aBuffer, qint64 aSize);
   void initFile();
   enum MatDataType
   {
      miINT8 = 1,
      miUINT8,
      miINT16,
      miUINT16,
      miINT32,
      miUINT32,
      miSINGLE,
      miDOUBLE = 9,
      miINT64 = 12,
      miUINT64,
      miMATRIX,
      miCOMPRESSED,
      miUTF8,
      miUTF16,
      miUTF32
   };
   enum ArrayType
   {
      CELL_ARRAY = 1,
      STRUCTURE,
      OBJECT,
      CHAR_ARRAY,
      SPARSE_ARRAY,
      DOUBLE_ARRAY,
      SINGLE_ARRAY,
      INT8_ARRAY,
      UINT8_ARRAY,
      INT16_ARRAY,
      UINT16_ARRAY,
      INT32_ARRAY,
      UINT32_ARRAY
   };

   qint64 mTotalSize;
   void writeMatHeader();
   bool writeBuffer(const QByteArray& aBuffer);
   bool writeBuffer(const char* aBuffer, qint64 aSize);
   QByteArray writeDataElement();
   const bool mUseDouble;
   const uint32_t mDataTypeSize;
   template<typename T> QByteArray writeElement(const T* aData, int aDataSize, MatDataType aDataType)
   {
      unsigned int totalSize = sizeof(T) * aDataSize;
      return writeRawElement(aData, totalSize, aDataType);
   }
   QByteArray writeRawElement(const void* aData, unsigned int aDataSize, MatDataType aDataType);
   unsigned int alignDataSize(unsigned int aSize);
   void writeTerminate();
#pragma pack(push, 1)
   struct MatHeader
   {
      char text[116];
      char subsystemDataOffset[8];
      unsigned short version;
      unsigned short endianIndicator;
   };

   struct DataElementTag
   {
      unsigned int dataType;
      unsigned int numberOfBytes;
   };
#pragma pack(pop)

};

#endif // FILEWRITER_H
