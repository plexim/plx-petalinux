//
//  maiaXmlStreamHandler.cpp
//  applicationutils
//
//  Created by Simon Geisseler on 05.03.21.
//  Copyright © 2021 Plexim GmbH. All rights reserved.
//

#include "maiaXmlStreamHandler.h"

#include "maiaStreamHandler.h"
#include "maiaFault.h"
#include "maiaObject.h"

#include <QtCore/QDebug>
#include <QtCore/QXmlStreamWriter>
#include <QtXml/QDomDocument>


MaiaXmlStreamHandler::MaiaXmlStreamHandler(QIODevice& aStream)
: MaiaStreamHandler(aStream),
  mStream(aStream)
{ }

QVariant MaiaXmlStreamHandler::parseRequest(const QByteArray& aRequest, QString& aMethodOut, QList<QVariant>& aArgsOut)
{
   QDomDocument doc;

   if(!doc.setContent(aRequest)) // recieved invalid xml
      return QVariant::fromValue(MaiaFault(-32700, "parse error: not well formed"));

   QDomElement methodNameElement = doc.documentElement().firstChildElement("methodName");
   QDomElement params = doc.documentElement().firstChildElement("params");
   if(methodNameElement.isNull()) // invalid call
      return QVariant::fromValue(MaiaFault(-32600, "server error: invalid xml-rpc. not conforming to spec"));

   aMethodOut = methodNameElement.text();

   QDomNode paramNode = params.firstChild();
   while(!paramNode.isNull())
   {
      aArgsOut << MaiaObject::fromXml( paramNode.firstChild().toElement());
      paramNode = paramNode.nextSibling();
   }

   return QVariant();
}

void MaiaXmlStreamHandler::sendError(const MaiaFault& aError)
{
   sendHeader();

   mStream.write("<?xml version=\"1.0\" encoding=\"UTF-8\"?>");
   mStream.write("<methodResponse>");
   mStream.write("<fault>");

   writeXml(aError.fault);

   mStream.write("</fault>");
   mStream.write("</methodResponse>");
}

void MaiaXmlStreamHandler::sendResponse(const QVariant& aRespone)
{
   sendHeader();

   mStream.write("<?xml version=\"1.0\" encoding=\"UTF-8\"?>");
   mStream.write("<methodResponse>");
   mStream.write("<params>");

   if (!aRespone.isNull())
   {
      mStream.write("<param>");
      writeXml(aRespone);
      mStream.write("</param>");
   }

   mStream.write("</params>");
   mStream.write("</methodResponse>");
}

void MaiaXmlStreamHandler::writeCall(const QString& aMethod, QList<QVariant>& aArgs)
{
   mStream.write("<?xml version=\"1.0\" encoding=\"UTF-8\"?>");
   mStream.write("<methodCall>");

   mStream.write("<methodName>");
   QXmlStreamWriter(&mStream).writeCharacters(aMethod);
   mStream.write("</methodName>");

   mStream.write("<params>");
   for(const auto& arg : qAsConst(aArgs))
   {
      mStream.write("<param>");
      writeXml(arg);
      mStream.write("</param>");
   }
   mStream.write("</params>");

   mStream.write("</methodCall>");
}

void MaiaXmlStreamHandler::writeXml(const QVariant& aValue)
{
   mStream.write("<value>");

   switch(aValue.type())
   {
      case QVariant::String:
         mStream.write("<string>");
         // Use QXmlStreamWriter for correct string escaping. Codec defaults to UTF-8.
         QXmlStreamWriter(&mStream).writeCharacters(aValue.toString());
         mStream.write("</string>");
         break;
      case QVariant::Int:
         mStream.write("<int>");
         mStream.write(QByteArray::number(aValue.toInt()));
         mStream.write("</int>");
         break;
      case QVariant::Double:
         mStream.write("<double>");
         writeDouble(aValue.toDouble());
         mStream.write("</double>");
         break;
      case QVariant::Bool:
         mStream.write("<boolean>");
         mStream.write(aValue.toBool() ? "1" : "0");
         mStream.write("</boolean>");
         break;
      case QVariant::ByteArray:
         mStream.write("<base64>");
         mStream.write(aValue.toByteArray().toBase64());
         mStream.write("</base64>");
         break;
      case QVariant::List:
      {
         mStream.write("<array><data>");

         const QList<QVariant> args = aValue.toList();
         for(auto& entry : qAsConst(args))
         {
            writeXml(entry);
         }

         mStream.write("</data></array>");
         break;
      }
      case QVariant::Map:
      {
         mStream.write("<struct>");

         QMap<QString, QVariant> map = aValue.toMap();
         for(auto it = map.constBegin(); it != map.constEnd(); ++it)
         {
            mStream.write("<member><name>");
            mStream.write(it.key().toUtf8());
            mStream.write("</name>");
            writeXml(it.value());

            mStream.write("</member>");
         }

         mStream.write("</struct>");
         break;
      }
      default:
      {
         if (aValue.canConvert<QVector<double>>())
         {
            mStream.write("<array><data>");
            auto list = aValue.value<QVector<double>>();
            for(auto& entry : qAsConst(list))
            {
               mStream.write("<value><double>");
               writeDouble(entry);
               mStream.write("</double></value>");
            }
            mStream.write("</data></array>");
         }
         else if (aValue.canConvert<QVector<QVector<double>>>())
         {
            mStream.write("<array><data>");
            auto list = aValue.value<QVector<QVector<double>>>();
            for(auto& entry : qAsConst(list))
            {
               writeXml(QVariant::fromValue(entry));
            }
            mStream.write("</data></array>");
         }
         else
         {
            qDebug() << "Failed to stream unknown variant type: " << aValue.type() << endl;
         }
      }
   }

   mStream.write("</value>");
}

void MaiaXmlStreamHandler::writeDouble(const double aValue)
{
   mStream.write(QByteArray::number(aValue, 'g', QLocale::FloatingPointShortest));
}
