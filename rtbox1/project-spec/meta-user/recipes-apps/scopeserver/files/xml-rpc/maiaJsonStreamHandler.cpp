//
//  maiaJsonStreamHandler.cpp
//  applicationutils
//
//  Created by Simon Geisseler on 05.03.21.
//  Copyright © 2021 Plexim GmbH. All rights reserved.
//

#include "maiaJsonStreamHandler.h"

#include "maiaStreamHandler.h"
#include "maiaFault.h"

#include <QtCore/QDebug>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonArray>


MaiaJsonStreamHandler::MaiaJsonStreamHandler(QIODevice& aStream)
: MaiaStreamHandler(aStream),
  mStream(aStream)
{ }

QVariant MaiaJsonStreamHandler::parseRequest(const QByteArray& aRequest, QString& aMethodOut, QList<QVariant>& aArgsOut)
{
   QJsonDocument doc = QJsonDocument::fromJson(aRequest);

   if(doc.isNull() || doc.isEmpty()) // received invalid json
      return QVariant::fromValue(MaiaFault(-32700, "parse error: not well formed"));

   if (doc.isArray())
      return QVariant::fromValue(MaiaFault(-32000, "server error: batch requests are not supported."));

   if(!doc.isObject()) // this should probably not happen, but handle it just in case...
      return QVariant::fromValue(MaiaFault(-32700, "parse error: not well formed"));

   auto root = doc.object();
   auto methodElement = root.value("method");
   auto params = root.value("params");

   if (root.contains("jsonrpc"))
   {
      if (root.value("jsonrpc").toString() != "2.0")
         return QVariant::fromValue(MaiaFault(-32600, "server error: invalid json-rpc version."));

      if (!root.contains("id"))
         return QVariant::fromValue(MaiaFault(-32000, "server error: notifications are not supported."));

      mId = root.value("id").toVariant();

      if (params.isObject())
         return QVariant::fromValue(MaiaFault(-32000, "server error: named parameters are not supported."));
   }

   if(!methodElement.isString() || !(params.isUndefined() || params.isArray())) // invalid call
      return QVariant::fromValue(MaiaFault(-32600, "server error: invalid json-rpc. not conforming to spec"));

   aMethodOut = methodElement.toString();

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

void MaiaJsonStreamHandler::sendError(const MaiaFault& aError)
{
   sendHeader();

   if (mId.isValid())
   {
      mStream.write("{\"jsonrpc\":\"2.0\",\"id\":");
      auto byteArray = QJsonDocument(QJsonArray{QJsonValue::fromVariant(mId)}).toJson(QJsonDocument::Compact);
      mStream.write(byteArray.data() + 1, byteArray.size() - 2);
      mStream.write(",\"error\":");
   }
   else
   {
      mStream.write("{\"error\":");
   }

   QVariantMap errorInfo;
   errorInfo["code"] = aError.fault["faultCode"];
   errorInfo["message"] = aError.fault["faultString"];
   writeJson(errorInfo);

   mStream.write("}");
}

void MaiaJsonStreamHandler::sendResponse(const QVariant& aResponse)
{
   sendHeader();

   if (mId.isValid())
   {
      mStream.write("{\"jsonrpc\":\"2.0\",\"id\":");
      auto byteArray = QJsonDocument(QJsonArray{QJsonValue::fromVariant(mId)}).toJson(QJsonDocument::Compact);
      mStream.write(byteArray.data() + 1, byteArray.size() - 2);
      mStream.write(",\"result\":");
   }
   else
   {
      mStream.write("{\"result\":");
   }

   if (aResponse.isNull())
   {
      mStream.write("null");
   }
   else
   {
      writeJson(aResponse);
   }

   mStream.write("}");
}

void MaiaJsonStreamHandler::writeJson(const QVariant& aValue)
{
   switch(aValue.type())
   {
      case QVariant::String:
      {
         // Use the string escaping functionality from QJsonDocument. Create the string in a list,
         // and then omit the leading and trailing square brackets when writing it to the stream.
         auto byteArray = QJsonDocument(QJsonArray{QJsonValue::fromVariant(aValue)}).toJson(QJsonDocument::Compact);
         mStream.write(byteArray.data() + 1, byteArray.size() - 2);
         break;
      }
      case QVariant::Int:
         mStream.write(QByteArray::number(aValue.toInt()));
         break;
      case QVariant::Double:
         writeDouble(aValue.toDouble());
         break;
      case QVariant::Bool:
         mStream.write(aValue.toBool() ? "true" : "false");
         break;
      case QVariant::ByteArray:
      {
         mStream.write("\"");
         mStream.write(aValue.toByteArray().toBase64());
         mStream.write("\"");
         break;
      }
      case QVariant::List:
      {
         const QList<QVariant> list = aValue.toList();
         bool sep = false;
         mStream.write("[");
         for(auto& entry : qAsConst(list))
         {
            if (sep)
               mStream.write(",");
            sep = true;
            writeJson(entry);
         }
         mStream.write("]");
         break;
      }
      case QVariant::Map:
      {
         QMap<QString, QVariant> map = aValue.toMap();
         bool sep = false;
         mStream.write("{");
         for(auto it = map.constBegin(); it != map.constEnd(); ++it)
         {
            if (sep)
               mStream.write(",");
            sep = true;
            writeJson(it.key());
            mStream.write(":");
            writeJson(it.value());
         }
         mStream.write("}");
         break;
      }
      default:
      {
         if (aValue.canConvert<QVector<double>>())
         {
            auto list = aValue.value<QVector<double>>();
            bool sep = false;
            mStream.write("[");
            for(auto& entry : qAsConst(list))
            {
               if (sep)
                  mStream.write(",");
               sep = true;
               writeDouble(entry);
            }
            mStream.write("]");
         }
         else if (aValue.canConvert<QVector<QVector<double>>>())
         {
            auto list = aValue.value<QVector<QVector<double>>>();
            bool sep = false;
            mStream.write("[");
            for(auto& entry : qAsConst(list))
            {
               if (sep)
                  mStream.write(",");
               sep = true;
               writeJson(QVariant::fromValue(entry));
            }
            mStream.write("]");
         }
         else
         {
            qDebug() << "Failed to stream unknown variant type: " << aValue.type() << endl;
         }
      }
   }
}

void MaiaJsonStreamHandler::writeDouble(const double aValue)
{
   if (qIsFinite(aValue))
      mStream.write(QByteArray::number(aValue, 'g', QLocale::FloatingPointShortest));
   else
      mStream.write("null");
}

