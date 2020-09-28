#ifndef PLECSERROR_H_
#define PLECSERROR_H_


#include <QtCore/QString>
#include <exception>

class RTBoxError : public std::exception
{
public:
   RTBoxError(const QString& str) : errMsg(str), errMsgByteArray(errMsg.toLocal8Bit()) {}
   virtual ~RTBoxError() throw() {};
   QString errMsg;
   QByteArray errMsgByteArray;
   virtual const char* what() const throw() { return errMsgByteArray.constData(); }
};

#endif

