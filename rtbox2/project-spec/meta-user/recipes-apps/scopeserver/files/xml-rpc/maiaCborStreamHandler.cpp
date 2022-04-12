//
//  maiaCborStreamHandler.cpp
//  applicationutils
//
//  Created by Simon Geisseler on 08.03.21.
//  Copyright © 2021 Plexim GmbH. All rights reserved.
//

#include "maiaCborStreamHandler.h"

#include "maiaStreamHandler.h"
#include "maiaFault.h"

#include <QtCore/QDebug>
#include <QtCore/QCborValue>
#include <QtCore/QCborMap>
#include <QtCore/QCborArray>


MaiaCborStreamHandler::MaiaCborStreamHandler(QIODevice& aStream)
: MaiaStreamHandler(aStream),
  mStream(&aStream)
{ }

QVariant MaiaCborStreamHandler::parseRequest(const QByteArray& aRequest, QString& aMethodOut, QList<QVariant>& aArgsOut)
{
   QCborParserError error;
   QCborValue doc = QCborValue::fromCbor(aRequest, &error);

   if(error.error != QCborError::NoError || !doc.isMap()) // recieved invalid cbor
      return QVariant::fromValue(MaiaFault(-32700, "parse error: not well formed"));

   auto root = doc.toMap();
   auto methodNameElement = root.value("method");
   auto params = root.value("params");
   if(!methodNameElement.isString() || !(params.isUndefined() || params.isArray())) // invalid call
      return QVariant::fromValue(MaiaFault(-32600, "server error: invalid cbor-rpc. not conforming to spec"));

   if (root.contains(QString("id")))
   {
      auto idElement = root.value("id");
      if (!(idElement.isInteger() || idElement.isDouble() || idElement.isString() || idElement.isByteArray()))
         return QVariant::fromValue(MaiaFault(-32600, "server error: invalid cbor-rpc request id. only supporting integer, double, string or byte array"));
      mId = idElement.toVariant();
   }

   aMethodOut = methodNameElement.toString();

   if (params.isArray())
   {
      auto paramsArray = params.toArray();
      for (auto param : paramsArray)
      {
         aArgsOut << param.toVariant();
      }
   }

   return QVariant();
}

void MaiaCborStreamHandler::sendError(const MaiaFault& aError)
{
   sendHeader();

   startResponseMap();
   mStream.append("error");

   QVariantMap errorInfo;
   errorInfo["code"] = aError.fault["faultCode"];
   errorInfo["message"] = aError.fault["faultString"];
   writeCbor(errorInfo);

   mStream.endMap();
}

void MaiaCborStreamHandler::sendResponse(const QVariant& aResponse)
{
   sendHeader();

   startResponseMap();
   mStream.append("result");
   writeCbor(aResponse);
   mStream.endMap();
}

void MaiaCborStreamHandler::startResponseMap()
{
   if (mId.isValid())
   {
      mStream.startMap(2);
      mStream.append("id");
      writeCbor(mId);
   }
   else
   {
      mStream.startMap(1);
   }
}

void MaiaCborStreamHandler::writeCbor(const QVariant& aValue)
{
   switch(aValue.type())
   {
      case QVariant::String:
         mStream.append(aValue.toString());
         break;
      case QVariant::Int:
         mStream.append(aValue.toInt());
         break;
      case QVariant::LongLong:
         mStream.append(aValue.toLongLong());
         break;
      case QVariant::Double:
         mStream.append(aValue.toDouble());
         break;
      case QVariant::Bool:
         mStream.append(aValue.toBool());
         break;
      case QVariant::ByteArray:
      {
         auto byteArray = aValue.toByteArray();
         mStream.appendByteString(byteArray.data(), byteArray.size());
         break;
      }
      case QVariant::List:
      {
         const QList<QVariant> list = aValue.toList();
         mStream.startArray(list.size());
         for(auto& entry : qAsConst(list))
         {
            writeCbor(entry);
         }
         mStream.endArray();
         break;
      }
      case QVariant::Map:
      {
         QMap<QString, QVariant> map = aValue.toMap();
         mStream.startMap(map.size());
         for(auto it = map.constBegin(); it != map.constEnd(); ++it)
         {
            mStream.append(it.key());
            writeCbor(it.value());
         }
         mStream.endMap();
         break;
      }
      default:
      {
         if (aValue.canConvert<QVector<double>>())
         {
            auto list = aValue.value<QVector<double>>();
            mStream.startArray(list.size());
            for(auto& entry : qAsConst(list))
            {
               mStream.append(entry);
            }
            mStream.endArray();
         }
         else if (aValue.canConvert<QVector<QVector<double>>>())
         {
            auto list = aValue.value<QVector<QVector<double>>>();
            mStream.startArray(list.size());
            for(auto& entry : qAsConst(list))
            {
               writeCbor(QVariant::fromValue(entry));
            }
            mStream.endArray();
         }
         else
         {
            qDebug() << "Failed to stream unknown variant type: " << aValue.type() << endl;
         }
      }
   }
}
