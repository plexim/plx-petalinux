/*
 * libMaia - maiaXmlRpcServerConnection.h
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

#ifndef MAIAXMLRPCSERVERCONNECTION_H
#define MAIAXMLRPCSERVERCONNECTION_H

class QTcpSocket;
class QHttpRequestHeader;
class QByteArray;
class MaiaXmlRpcServer;

#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QObject>
#include <QtCore/QVariant>

class MaiaXmlRpcServerConnection : public QObject {
	Q_OBJECT

public:
   MaiaXmlRpcServerConnection(QTcpSocket *connection, MaiaXmlRpcServer& aParent);
   ~MaiaXmlRpcServerConnection();
   
private slots:
   void readFromSocket();
   void disconnected();

private:
   void sendResponse(QString content);
   void parseCall(QString call);
   bool invokeMethodWithVariants(QObject *obj,
           const QByteArray &method, const QVariantList &args,
           QVariant *ret, Qt::ConnectionType type = Qt::AutoConnection);
   static QByteArray getReturnType(const QMetaObject *obj,
              const QByteArray &method, const QList<QByteArray> argTypes);
   
   MaiaXmlRpcServer& mServer;
   QTcpSocket *clientConnection;
   QStringList mHeaderString;
   bool mCallProcessing;
   bool mDisconnected;
   bool mHeaderComplete;
   int mContentLength;
};

#endif
