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

#ifndef _IOHELPER_H_
#define _IOHELPER_H_

#include <QtCore/QIODevice>

class IOHelper
{
public:
   IOHelper(QIODevice &aIODevice, int aMsTimeout = -1);
   
   class IOError : public std::exception
   {
   public:
      IOError(const QString &aWhat) : errMsg(aWhat) {}
      virtual ~IOError() throw() {};
      QString errMsg;
      virtual const char* what() const throw() { return errMsg.toLocal8Bit().data(); }
  };
   
   /* Blocking interface */
   
   template <typename T>
      void read(T *aBuf, int aNumItems = 1);

   template <typename T>
      void write(const T *aBuf, int aNumItems = 1);
   
   /* Non-blocking interface */
   
   // Return true if the requested item(s) have been read
   // false otherwise
   
   template <typename T>
      bool readAsync(T *aBuf, int aNumItems = 1);

   template <typename T>
      void writeAsync(const T *aBuf, int aNumItems = 1);

   
private:
   
   bool waitForNBytes(QIODevice *aIODevice, int aNbytes, int aMsTimeout);
   bool doRead(QIODevice *aDevice, void *aBuf, int aNbytes, int aMsTimeout);
   bool doWrite(QIODevice *aDevice, const void *aBuf, int aNbytes, int aMsTimeout);
   
   QIODevice &mIODevice;
   int mMsTimeout;
};



template <typename T>
void IOHelper::read(T *aBuf, int aNumItems)
{
   if (!doRead(&mIODevice, aBuf, sizeof(T)*aNumItems, mMsTimeout))
   {
      throw IOError(mIODevice.errorString());
   }
}


template <typename T>
void IOHelper::write(const T *aBuf, int aNumItems)
{
   if (!doWrite(&mIODevice, aBuf, sizeof(T)*aNumItems, mMsTimeout))
   {
      throw IOError(mIODevice.errorString());
   }
}



template <typename T>
bool IOHelper::readAsync(T *aBuf, int aNumItems)
{
   if (aNumItems == 0)
      return true;
   if (aNumItems < 0)
      throw IOError(QString("Invalid number of items requested: %1").arg(aNumItems));
      
   int bytesToRead = sizeof(T)*aNumItems;

   if (mIODevice.bytesAvailable() < bytesToRead)
   {
      return false;
   }
      
   if (mIODevice.read((char *)aBuf, bytesToRead) <= 0)
   {
      throw IOError(mIODevice.errorString());
   }
      
   return true;
}


template <typename T>
void IOHelper::writeAsync(const T *aBuf, int aNumItems)
{
   if (aNumItems < 0)
      throw IOError(QString("Invalid number of items: %1").arg(aNumItems));
    
   int bytesToWrite = sizeof(T)*aNumItems;
   int numWritten = 0;
   int rval = 0;
   while (numWritten < bytesToWrite) 
   {
      rval = mIODevice.write((const char *)aBuf+numWritten, bytesToWrite-numWritten);
      if (rval < 0) break;
      numWritten += rval;
   }
   
   if (rval < 0)
   {
      throw IOError(mIODevice.errorString());
   }
}


#endif

