#ifndef CH_CTX_WIN32_H
#define CH_CTX_WIN32_H

#include <string>
#include <array>
#include <QObject>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QNetworkRequest>
#include <condition_variable>

namespace WebGrep {

class Scheme6 : public std::array<char,6>
{
public:
  Scheme6() { this->fill(0x00); }
};

class ClientCtx : public QObject
{
  Q_OBJECT
public:
  explicit ClientCtx(QObject *parent = 0);

  //@return TRUE if scheme is "https"
  bool isHttps() const;

  //Send a GET request
  //@return managed pointer
  std::shared_ptr<QNetworkReply> makeGet(const QNetworkRequest& req);

  std::string response;
  Scheme6 scheme;// "http\0\0" or "https\0"
  uint16_t port;
  std::string host_and_port;

  std::mutex mu;//locked in issueRequest(), also used for condition variable
  /** The callser must wait (cond) if he wants to sync. with QNetworkAccessManager*/
  std::condition_variable cond;

  std::shared_ptr<QNetworkReply> reply;
  QNetworkAccessManager* mgr;

signals:

public slots:
  //will invoke cond.notify_all(); takes ownership on pointer.
  void replyFinished(QNetworkReply*);

  //looks up (this->reply) pointer for data
  void onFinished();

  void onRedirected(const QUrl &url);


  void onSslErrors(const QList<QSslError> &errors);
  void onNetError(QNetworkReply::NetworkError);


};

//ref.count holding structure
struct IssuedRequest
{
  QNetworkRequest req;
  std::shared_ptr<ClientCtx> ctx; //holds reference of a context
};

}//namespace WebGrep

#endif // CH_CTX_WIN32_H
