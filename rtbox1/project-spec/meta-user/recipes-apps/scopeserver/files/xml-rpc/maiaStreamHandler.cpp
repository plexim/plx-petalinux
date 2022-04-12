//
//  maiaStreamHandler.cpp
//  applicationutils
//
//  Created by Simon Geisseler on 05.03.21.
//  Copyright © 2021 Plexim GmbH. All rights reserved.
//

#include "maiaStreamHandler.h"
#include <QtNetwork/QTcpSocket>


MaiaStreamHandler::~MaiaStreamHandler()
{
   if (auto* socket = dynamic_cast<QTcpSocket*>(&mStream))
      socket->disconnectFromHost();
   else
      mStream.close();
}

void MaiaStreamHandler::sendHeader() const
{
   QByteArray data("HTTP/1.0 200 Ok\r\n"
                   "Server: MaiaXmlRpc/0.1\r\n"
                   "Access-Control-Allow-Origin: *\r\n"
                   "Content-Type: ");
   data.append(getContentType());
   data.append("\r\n"
               "Connection: close\r\n"
               "\r\n");

   mStream.write(data);
}
