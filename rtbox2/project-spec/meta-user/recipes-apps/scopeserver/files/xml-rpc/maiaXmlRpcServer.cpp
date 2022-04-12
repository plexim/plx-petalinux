/*
 * libMaia - maiaXmlRpcServer.cpp
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

#include "maiaXmlRpcServer.h"
#include "maiaXmlRpcServerConnection.h"
#include "maiaFault.h"

#include <QtNetwork/QTcpSocket>
#include <QtCore/QStringList>
#include <QtCore/QDebug>

MaiaXmlRpcServer::MaiaXmlRpcServer(quint16 port, QObject* aParent) : QObject(aParent) {
   connect(&mServer, SIGNAL(newConnection()), this, SLOT(newConnection()));
   addMethod("system.listMethods", this, "listMethods",
             "This method lists all the methods that the RPC server knows how to dispatch");
   addMethod("system.methodHelp", this, "methodHelp",
             "Returns help text if defined for the method passed, otherwise returns an empty string");
   if (!mServer.listen(QHostAddress::Any, port))
      qCritical() << tr("Faild to open port for RPC: %1").arg(mServer.errorString()).toUtf8().data();
}

void MaiaXmlRpcServer::addMethod(QString method, QObject* responseObject, 
                                 const char* responseSlot, QString aMethodHelp) 
{
   objectMap[method] = responseObject;
   slotMap[method] = responseSlot;
   helpMap[method] = aMethodHelp;
}

void MaiaXmlRpcServer::removeMethod(QString method) {
   objectMap.remove(method);
   slotMap.remove(method);
   helpMap.remove(method);
}

void MaiaXmlRpcServer::getMethod(QString method, QObject **responseObject, const char **responseSlot) {
   if(!objectMap.contains(method)) {
      *responseObject = NULL;
      *responseSlot = NULL;
      return;
   }
   *responseObject = objectMap[method];
   *responseSlot = slotMap[method];
}

void MaiaXmlRpcServer::newConnection() {
   QTcpSocket *connection = mServer.nextPendingConnection();
   new MaiaXmlRpcServerConnection(connection, *this);
}

QHostAddress MaiaXmlRpcServer::getServerAddress() {
   return mServer.serverAddress();
}

QVariantList MaiaXmlRpcServer::listMethods()
{
   QVariantList ret;
   QStringList methods = objectMap.keys();
   for (QStringList::const_iterator it = methods.begin(); it != methods.end(); ++it)
      ret << *it;
   return ret;
}

QVariant MaiaXmlRpcServer::methodHelp(const QString& aMethod)
{
   if (!helpMap.contains(aMethod))
   {
      MaiaFault fault(-32603, QString("method '%s' not found.").arg(aMethod));
      return QVariant::fromValue<MaiaFault>(fault);
   }
   return helpMap[aMethod];
}
