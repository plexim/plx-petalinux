//
//  maiaStreamHandler.h
//  applicationutils
//
//  Created by Simon Geisseler on 05.03.21.
//  Copyright © 2021 Plexim GmbH. All rights reserved.
//

#ifndef MAIASTREAMHANDLER_H
#define MAIASTREAMHANDLER_H

#include <QtCore/QList>
#include <QtCore/QVariant>

class QIODevice;
class MaiaFault;

class MaiaStreamHandler
{
public:
   virtual ~MaiaStreamHandler();

   virtual QVariant parseRequest(const QByteArray& aRequest, QString& aMethodOut, QList<QVariant>& aArgsOut) = 0;
   virtual void sendError(const MaiaFault& aError) = 0;
   virtual void sendResponse(const QVariant& aRespone) = 0;

protected:
   MaiaStreamHandler(QIODevice& aStream) : mStream(aStream) { }

   void sendHeader() const;
   virtual QByteArray getContentType() const = 0;

private:
   QIODevice& mStream;
};

#endif /* MAIASTREAMHANDLER_H */
