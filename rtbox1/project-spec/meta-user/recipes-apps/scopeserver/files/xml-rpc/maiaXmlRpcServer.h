/*
 * libMaia - maiaXmlRpcServer.h
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

#ifndef MAIAXMLRPCSERVER_H
#define MAIAXMLRPCSERVER_H

#include <QtCore/QObject>
#include <QtNetwork/QTcpServer>
#include <QtCore/QVariant>
#include <QtCore/QHash>

class MaiaXmlRpcServer : public QObject {
	Q_OBJECT
	
public:
   MaiaXmlRpcServer(quint16 port, QObject* parent = 0);
   void addMethod(QString method, QObject *responseObject, const char* responseSlot, QString aMethodHelp = "");
   void removeMethod(QString method);
   QHostAddress getServerAddress();
   void getMethod(QString method, QObject **responseObject, const char** responseSlot);
   
signals:
   void disconnected();
   
private slots:
   void newConnection();
   QVariantList listMethods();
   QVariant methodHelp(const QString& aMethod);
   
private:
   QTcpServer server;
   QHash<QString, QObject*> objectMap;
   QHash<QString, const char*> slotMap;
   QHash<QString, QString> helpMap;
   
friend class maiaXmlRpcServerConnection;
   
};

#endif
