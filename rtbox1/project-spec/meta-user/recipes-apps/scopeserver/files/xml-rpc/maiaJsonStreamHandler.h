//
//  maiaJsonStreamHandler.h
//  applicationutils
//
//  Created by Simon Geisseler on 05.03.21.
//  Copyright © 2021 Plexim GmbH. All rights reserved.
//

#ifndef MAIAJSONSTREAMHANDLER_H
#define MAIAJSONSTREAMHANDLER_H

#include "maiaStreamHandler.h"

class MaiaJsonStreamHandler final : public MaiaStreamHandler
{
public:
   MaiaJsonStreamHandler(QIODevice& aStream);

   QVariant parseRequest(const QByteArray& aRequest, QString& aMethodOut, QList<QVariant>& aArgsOut) override;
   void sendError(const MaiaFault& aError) override;
   void sendResponse(const QVariant& aRespone) override;

protected:
   QByteArray getContentType() const override { return "application/json"; }

private:
   void writeJson(const QVariant& aValue);
   inline void writeDouble(const double aValue);

   QIODevice& mStream;
   QVariant mId;
};

#endif /* MAIAJSONSTREAMHANDLER_H */
