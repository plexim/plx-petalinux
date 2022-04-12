//
//  maiaXmlStreamHandler.h
//  applicationutils
//
//  Created by Simon Geisseler on 05.03.21.
//  Copyright © 2021 Plexim GmbH. All rights reserved.
//

#ifndef MAIAXMLSTREAMHANDLER_H
#define MAIAXMLSTREAMHANDLER_H

#include "maiaStreamHandler.h"

class MaiaXmlStreamHandler final : public MaiaStreamHandler
{
public:
   MaiaXmlStreamHandler(QIODevice& aStream);

   QVariant parseRequest(const QByteArray& aRequest, QString& aMethodOut, QList<QVariant>& aArgsOut) override;
   void sendError(const MaiaFault& aError) override;
   void sendResponse(const QVariant& aRespone) override;
   void writeCall(const QString& aMethod, QList<QVariant>& aArgs);

protected:
   QByteArray getContentType() const override { return "text/xml"; }

private:
   void writeXml(const QVariant& aValue);
   inline void writeDouble(const double aValue);

   QIODevice& mStream;
};

#endif /* MAIAXMLSTREAMHANDLER_H */
