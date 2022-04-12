//
//  maiaCborStreamHandler.h
//  applicationutils
//
//  Created by Simon Geisseler on 08.03.21.
//  Copyright © 2021 Plexim GmbH. All rights reserved.
//

#ifndef MAIACBORSTREAMHANDLER_H
#define MAIACBORSTREAMHANDLER_H

#include "maiaStreamHandler.h"
#include <QtCore/QCborStreamWriter>

class MaiaCborStreamHandler final : public MaiaStreamHandler
{
public:
   MaiaCborStreamHandler(QIODevice& aStream);

   QVariant parseRequest(const QByteArray& aRequest, QString& aMethodOut, QList<QVariant>& aArgsOut) override;
   void sendError(const MaiaFault& aError) override;
   void sendResponse(const QVariant& aRespone) override;

protected:
   QByteArray getContentType() const override { return "application/cbor"; }

private:
   void startResponseMap();
   void writeCbor(const QVariant& aValue);

   QCborStreamWriter mStream;
   QVariant mId;
};

#endif /* MAIACBORSTREAMHANDLER_H */
