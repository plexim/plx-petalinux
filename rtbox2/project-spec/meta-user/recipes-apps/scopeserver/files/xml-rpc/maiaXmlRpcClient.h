/*
 * libMaia - maiaXmlRpcClient.h
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

#ifndef MAIAXMLRPCCLIENT_H
#define MAIAXMLRPCCLIENT_H

#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkProxy>

#include "maiaObject.h"

class MaiaXmlRpcClient : public QObject {
   Q_OBJECT
   
public:
   MaiaXmlRpcClient(QObject* parent = 0);
   MaiaXmlRpcClient(QUrl url, QObject* parent = 0);
   MaiaXmlRpcClient(QUrl url, QString userAgent, QObject *parent = 0);
   void setUrl(QUrl url);
   void setUserAgent(QString userAgent);
   QNetworkReply* call(QString method, QList<QVariant> args,
                       QObject* responseObject, const char* responseSlot,
                       QObject* faultObject, const char* faultSlot);
   
#ifndef QT_NO_OPENSSL
   void setSslConfiguration(const QSslConfiguration &config);
   QSslConfiguration sslConfiguration () const;
#endif // QT_NO_OPENSSL
   
   void setProxy(const QNetworkProxy &aProxy);
   QNetworkProxy proxy() const;
   
signals:
   void sslErrors(QNetworkReply *reply, const QList<QSslError> &errors);
   
private slots:
   void replyFinished(QNetworkReply*);
   
private:
   void init();
   QNetworkAccessManager manager;
   QNetworkRequest request;
   QMap<QNetworkReply*, MaiaObject*> callmap;
};

#endif
