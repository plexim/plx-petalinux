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
#include "maiaXmlStreamHandler.h"
#include "maiaJsonStreamHandler.h"
// #include "maiaCborStreamHandler.h"

#include <QtCore/QByteArray>
#include <QtCore/QMetaMethod>
#include <QtCore/QRegularExpression>
#include <QtNetwork/QTcpSocket>
#include "maiaFault.h"
#include "RTBoxError.h"
#include <memory>

// make_unique does not exist in C++ 11, declare it here
namespace std {
template<typename T, typename... Args>
std::unique_ptr<T> make_unique(Args&&... args) {
    return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}
}

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

MaiaXmlRpcServerConnection::~MaiaXmlRpcServerConnection()
{
    if (!clientConnection.isNull())
        clientConnection->deleteLater();
}

void MaiaXmlRpcServerConnection::readFromSocket()
{
   bool expectContinue = false;

	while(clientConnection->canReadLine() && !mHeaderComplete)
   {
		const auto lastLine = clientConnection->readLine().trimmed();
		if(lastLine.isEmpty()) { /* http header end */
			mHeaderComplete = true;
         if (mHeaderString.isEmpty())
         {
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
               return;
            }
         } else if(!mHeaderString[0].startsWith("POST")) {
				/* return http error */
				qDebug() << "No Post!";
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
         else if (lastLine.toLower().startsWith("expect:"))
         {
            expectContinue = lastLine.mid(7).trimmed() == "100-continue";
         }
      }
	}
	
	if (mHeaderComplete)
   {
      if (expectContinue)
      {
         sendContinueResponse();
         // give the client some time to actually send the content
         clientConnection->waitForReadyRead(3000);         
      }
      if(mContentLength <= clientConnection->bytesAvailable())
      {
         /* all data complete */
         try {
            parseCall(clientConnection->readAll());
         }
         catch(std::bad_alloc& )
         {
            qWarning() << "caught std::bad_alloc";
         }
      }
   }
}

void MaiaXmlRpcServerConnection::parseCall(const QByteArray& call) {
   std::unique_ptr<MaiaStreamHandler> streamHandler;
   QString methodName;
   QList<QVariant> args;
   QVariant ret;

   if (call.startsWith("<"))
   {
      streamHandler = std::make_unique<MaiaXmlStreamHandler>(*clientConnection.data());
   }
   else if (call.startsWith("{") || call.startsWith("["))
   {
      streamHandler = std::make_unique<MaiaJsonStreamHandler>(*clientConnection.data());
   }
   // else if (call.size() > 0 && (call.front() & 0xe0) == 0xa0) // CBOR major type 5 (map)
   // {
   //    streamHandler = std::make_unique<MaiaCborStreamHandler>(*clientConnection.data());
   // }
   else
   {
      // Reply with an xml response if request content isn't clear
      streamHandler = std::make_unique<MaiaXmlStreamHandler>(*clientConnection.data());
      MaiaFault fault(-32700, "parse error: not well formed");
      streamHandler->sendError(fault);
      return;
   }

   Q_ASSERT(streamHandler);

   try {
      ret = streamHandler->parseRequest(call, methodName, args);
   }
   catch(std::bad_alloc)
   {
      MaiaFault fault(-32702, "out of memory: arguments too large");
      streamHandler->sendError(fault);
      return;
   }
   if (ret.canConvert<MaiaFault>())
   {
      streamHandler->sendError(ret.value<MaiaFault>());
      return;
   }

   Q_ASSERT(ret.isNull());

   // Check if the requeste method exists
   QObject *responseObject;
   const char *responseSlot;
   mServer.getMethod(methodName, &responseObject, &responseSlot);
   if (!responseObject) /* unknown method */
   {
      MaiaFault fault(-32601, "server error: requested method not found");
      streamHandler->sendError(fault);
      return;
   }

   // Execute the actual method
   try
   {
      mCallProcessing = true;
      if (!invokeMethodWithVariants(responseObject, responseSlot, args, &ret)) /* error invoking... */
      {
         MaiaFault fault(-32602, "server error: invalid method parameters");
         streamHandler->sendError(fault);
         return;
      }
   }
   catch(RTBoxError& e)
   {
      ret = QVariant::fromValue(MaiaFault(-32603, "method execution error: " + e.errMsg));
   }
   catch(std::bad_alloc& )
   {
      ret = QVariant::fromValue(MaiaFault(-32604, "out of memory: arguments too large"));
   }

   mCallProcessing = false;
   if (mDisconnected)
      deleteLater();
   else if (ret.canConvert<MaiaFault>())
      streamHandler->sendError(ret.value<MaiaFault>());
   else
      streamHandler->sendResponse(ret);
}


/*	taken from http://delta.affinix.com/2006/08/14/invokemethodwithvariants/
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
	if(metatype != 0) {
		retval = QVariant(metatype, (const void *)0);
		retarg = QGenericReturnArgument(retval.typeName(), retval.data());
	} else { /* QVariant */
		retarg = QGenericReturnArgument("QVariant", &retval);
	}

	if(retTypeName.isEmpty()) { /* void */
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

void MaiaXmlRpcServerConnection::sendContinueResponse() const
{
   static const QByteArray data("HTTP/1.1 100 Continue\r\n"
                                "Server: MaiaXmlRpc/0.1\r\n"
                                "\r\n");
   clientConnection->write(data);
   clientConnection->flush();
}
 


