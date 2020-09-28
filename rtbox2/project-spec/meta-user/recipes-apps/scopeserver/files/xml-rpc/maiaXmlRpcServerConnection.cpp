/*
 * libMaia - maiaXmlRpcServerConnection.cpp
 * Copyright (c) 2007 Sebastian Wiedenroth <wiedi@frubar.net>
 *                and Karl Glatz
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "maiaXmlRpcServerConnection.h"
#include "maiaXmlRpcServer.h"

#include <QtCore/QByteArray>
#include <QtCore/QMetaMethod>
#include <QtCore/QDebug>
#include <QtCore/QRegularExpression>
#include <QtNetwork/QTcpSocket>
#include <QtXml/QDomDocument>
#include "maiaFault.h"
#include "maiaObject.h"
#include "RTBoxError.h"

MaiaXmlRpcServerConnection::MaiaXmlRpcServerConnection(QTcpSocket *connection, MaiaXmlRpcServer& aParent)
 : QObject(&aParent),
   mServer(aParent),
   mCallProcessing(false),
   mDisconnected(false),
   mHeaderComplete(false),
   mContentLength(-1)
{
   clientConnection = connection;
   connect(clientConnection, SIGNAL(readyRead()), this, SLOT(readFromSocket()));
   connect(clientConnection, SIGNAL(disconnected()), &aParent, SIGNAL(disconnected()));
   connect(clientConnection, SIGNAL(disconnected()), this, SLOT(disconnected()));
}

MaiaXmlRpcServerConnection::~MaiaXmlRpcServerConnection() {
   clientConnection->deleteLater();
}

void MaiaXmlRpcServerConnection::readFromSocket()
{
   QString lastLine;

   while(clientConnection->canReadLine() && !mHeaderComplete)
   {
      lastLine = clientConnection->readLine();
      if(lastLine == "\r\n") { /* http header end */
         mHeaderComplete = true;
         if (mHeaderString.isEmpty()) {
            qDebug() << "Empty header!";
            return;
         }
         if (mHeaderString[0].startsWith("OPTIONS")) {
            if (mHeaderString.indexOf(QRegularExpression("^Origin: http.*", QRegularExpression::DotMatchesEverythingOption)) >= 0) {
               QByteArray data("HTTP/1.0 200 Ok\r\n"
                               "Server: MaiaXmlRpc/0.1\r\n"
                               "Access-Control-Allow-Origin: *\r\n"
                               "Access-Control-Allow-Methods: POST\r\n"
                               "Access-Control-Allow-Headers: Content-Type\r\n"
                               "Cache-Control: public, max-age=31536000\r\n"
                               "Connection: close\r\n"
                               "\r\n");
               clientConnection->write(data);
               clientConnection->disconnectFromHost();
               return;
            } else {
               sendResponse("");
               return;
            }
         } else if(!mHeaderString[0].startsWith("POST")) {
            // return http error
            qDebug() << "No Post!";
            sendResponse("");
            return;
         } else if(mContentLength <= 0) {
            /* return fault */
            qDebug() << "No Content Length";
            return;
         }
      }
      else
      {
         mHeaderString << lastLine;
         if (lastLine.toLower().startsWith("content-length:"))
         {
            bool ok;
            mContentLength = lastLine.mid(15).toInt(&ok);
            if (!ok)
               mContentLength = -1;
         }
      }
   }
   
   if (mHeaderComplete)
   {
      if(mContentLength <= clientConnection->bytesAvailable())
      {
         /* all data complete */
         parseCall(clientConnection->readAll());
      }
   }
}

void MaiaXmlRpcServerConnection::sendResponse(QString content) {
   QByteArray data("HTTP/1.0 200 Ok\r\n"
                   "Server: MaiaXmlRpc/0.1\r\n"
                   "Access-Control-Allow-Origin: *\r\n"
                   "Content-Type: text/xml\r\n"
                   "Connection: close\r\n"
                   "\r\n");
   data += content.toUtf8();
   clientConnection->write(data);
   clientConnection->disconnectFromHost();   
}

void MaiaXmlRpcServerConnection::parseCall(QString call) {
   QDomDocument doc;
   QList<QVariant> args;
   QVariant ret;
   QString response;
   QObject *responseObject;
   const char *responseSlot;
   
   if(!doc.setContent(call)) { /* recieved invalid xml */
      MaiaFault fault(-32700, "parse error: not well formed");
      sendResponse(fault.toString());
      return;
   }
   
   QDomElement methodNameElement = doc.documentElement().firstChildElement("methodName");
   QDomElement params = doc.documentElement().firstChildElement("params");
   if(methodNameElement.isNull()) { /* invalid call */
      MaiaFault fault(-32600, "server error: invalid xml-rpc. not conforming to spec");
      sendResponse(fault.toString());
      return;
   }
   
   QString methodName = methodNameElement.text();
   
   mServer.getMethod(methodName, &responseObject, &responseSlot);
   if(!responseObject) { /* unknown method */
      MaiaFault fault(-32601, "server error: requested method not found");
      sendResponse(fault.toString());
      return;
   }
   
   QDomNode paramNode = params.firstChild();
   while(!paramNode.isNull()) {
      args << MaiaObject::fromXml( paramNode.firstChild().toElement());
      paramNode = paramNode.nextSibling();
   }
   
   try 
   {
      mCallProcessing = true;
      if(!invokeMethodWithVariants(responseObject, responseSlot, args, &ret)) { /* error invoking... */
         MaiaFault fault(-32602, "server error: invalid method parameters");
         sendResponse(fault.toString());
         return;
      }

      if(ret.canConvert<MaiaFault>()) {
         response = ret.value<MaiaFault>().toString();
      } else {
         response = MaiaObject::prepareResponse(ret);
      }
      
   } catch(RTBoxError& e)
   {
      MaiaFault fault(-32500, "method execution error: " + e.errMsg);
      response = fault.toString();
   }
   mCallProcessing = false;
   if (mDisconnected)
      deleteLater();
   else
      sendResponse(response);
}


/*   taken from http://delta.affinix.com/2006/08/14/invokemethodwithvariants/
   thanks to Justin Karneges once again :) */
bool MaiaXmlRpcServerConnection::invokeMethodWithVariants(QObject *obj,
         const QByteArray &method, const QVariantList &args,
         QVariant *ret, Qt::ConnectionType type) {

   // QMetaObject::invokeMethod() has a 10 argument maximum
   if(args.count() > 10)
      return false;

   QList<QByteArray> argTypes;
   for(int n = 0; n < args.count(); ++n)
      argTypes += args[n].typeName();

   // get return type
   int metatype = 0;
   QByteArray retTypeName = getReturnType(obj->metaObject(), method, argTypes);
   if(!retTypeName.isEmpty()  && retTypeName != "QVariant") {
      metatype = QMetaType::type(retTypeName.data());
      if(metatype == 0) // lookup failed
         return false;
   }

   QGenericArgument arg[10];
   for(int n = 0; n < args.count(); ++n)
      arg[n] = QGenericArgument(args[n].typeName(), args[n].constData());

   QGenericReturnArgument retarg;
   QVariant retval;
   if(metatype != 0 && retTypeName != "void") {
      retval = QVariant(metatype, (const void *)0);
      retarg = QGenericReturnArgument(retval.typeName(), retval.data());
   } else { /* QVariant */
      retarg = QGenericReturnArgument("QVariant", &retval);
   }

   if(retTypeName.isEmpty() || retTypeName == "void") { /* void */
      if(!QMetaObject::invokeMethod(obj, method.data(), type,
                  arg[0], arg[1], arg[2], arg[3], arg[4],
                  arg[5], arg[6], arg[7], arg[8], arg[9]))
         return false;
   } else {
      if(!QMetaObject::invokeMethod(obj, method.data(), type, retarg,
                  arg[0], arg[1], arg[2], arg[3], arg[4],
                  arg[5], arg[6], arg[7], arg[8], arg[9]))
         return false;
   }

   if(retval.isValid() && ret)
      *ret = retval;
   return true;
}

QByteArray MaiaXmlRpcServerConnection::getReturnType(const QMetaObject *obj,
         const QByteArray &method, const QList<QByteArray> argTypes) {
   for(int n = 0; n < obj->methodCount(); ++n) {
      QMetaMethod m = obj->method(n);
      QByteArray sig = m.methodSignature();
      int offset = sig.indexOf('(');
      if(offset == -1)
         continue;
      QByteArray name = sig.mid(0, offset);
      if(name != method)
         continue;
      if(m.parameterTypes() != argTypes)
         continue;

      return m.typeName();
   }
   return QByteArray();
}

void MaiaXmlRpcServerConnection::disconnected()
{
   if (mCallProcessing)
      mDisconnected = true;
   else 
      deleteLater();
}
