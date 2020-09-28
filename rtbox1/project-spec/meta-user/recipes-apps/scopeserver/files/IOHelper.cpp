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

#include "IOHelper.h"


IOHelper::IOHelper(QIODevice &aIODevice, int aMsTimeout):
   mIODevice(aIODevice), mMsTimeout(aMsTimeout)
{

}


bool IOHelper::waitForNBytes(QIODevice *aIODevice, int aNbytes, int aMsTimeout)
{
   while (aIODevice->bytesAvailable() < aNbytes) {
      if (!aIODevice->waitForReadyRead(aMsTimeout)) {
         return false;
      }
   }
   return true;
}


bool IOHelper::doRead(QIODevice *aIODevice, void *aBuf, int aNbytes, int aMsTimeout)
{
   if (!waitForNBytes(aIODevice, aNbytes, aMsTimeout))
      return false;
   if (aIODevice->read((char *)aBuf, aNbytes) <= 0)
      return false;
   return true;
}


bool IOHelper::doWrite(QIODevice *aIODevice, const void *aBuf, int aNbytes, int aMsTimeout)
{
   int nwritten=0;
   int rval;
   do {
      rval = aIODevice->write((const char *)aBuf+nwritten, aNbytes-nwritten);
      if (rval < 0) break;
      nwritten += rval;
   } while (nwritten < aNbytes);
   
   if (rval < 0)
   {
      return false;
   }
   return aIODevice->waitForBytesWritten(aMsTimeout);
}





///* Specialized versions using Qt serialization */
//
//template <>
//void IOHelper::read(int32_t *aBuf, int aNumItems)
//{
//   QByteArray buffer(sizeof(int32_t)*aNumItems, 0);
//   QDataStream in(&buffer, QIODevice::ReadOnly);
//   in.setVersion(QDataStream::Qt_4_0);
//   
//   if (!doRead(&mIODevice, buffer.data(), buffer.size(), mMsTimeout))
//   {
//      throw IOError(mIODevice.errorString());
//   }
//   
//   for (int i=0; i<aNumItems; ++i)
//   {
//      in >> (qint32&)aBuf[i];
//   }
//}
//
//
//template <>
//void IOHelper::write(const int32_t *aBuf, int aNumItems)
//{
//   QByteArray buffer;
//   QDataStream out(&buffer, QIODevice::WriteOnly);
//   out.setVersion(QDataStream::Qt_4_0);
//
//   for (int i=0; i<aNumItems; ++i)
//   {
//      out << (qint32)aBuf[i];
//   }
//   
//   if (!doWrite(&mIODevice, buffer.data(), buffer.size(), mMsTimeout))
//   {
//      throw IOError(mIODevice.errorString());
//   }
//   
////   qDebug() << "IOHelper::write<int32_t>: size=" << buffer.size()/aNumItems;
//}
//
//
//
//template <>
//void IOHelper::read(float *aBuf, int aNumItems)
//{
//   QByteArray buffer(sizeof(float)*aNumItems, 0);
//   QDataStream in(&buffer, QIODevice::ReadOnly);
//   in.setVersion(QDataStream::Qt_4_0);
//   
//   if (!doRead(&mIODevice, buffer.data(), buffer.size(), mMsTimeout))
//   {
//      throw IOError(mIODevice.errorString());
//   }
//   
//   for (int i=0; i<aNumItems; ++i)
//   {
//      in >> aBuf[i];
//   }
//}
//
//
//
//template <>
//void IOHelper::write(const float *aBuf, int aNumItems)
//{
//   QByteArray buffer;
//   QDataStream out(&buffer, QIODevice::WriteOnly);
//   out.setVersion(QDataStream::Qt_4_0);
//   
//   for (int i=0; i<aNumItems; ++i)
//   {
//      out << aBuf[i];
//   }
//   
//   if (!doWrite(&mIODevice, buffer.data(), buffer.size(), mMsTimeout))
//   {
//      throw IOError(mIODevice.errorString());
//   }
//   
////   qDebug() << "IOHelper::write<float>: size=" << buffer.size()/aNumItems;
//}
//

