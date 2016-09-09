#include "ch_ctx_win32.h"
#include <cassert>

namespace WebGrep {


ClientCtx::ClientCtx(QObject *parent) : QObject(parent)
{
  mgr = new QNetworkAccessManager(this);
  QObject::connect(mgr, SIGNAL(finished(QNetworkReply*)),
                   this, SLOT(replyFinished(QNetworkReply*)), Qt::DirectConnection);
  port = 0;
  scheme.fill(0x00);
  assert(QSslSocket::supportsSsl());
}

bool ClientCtx::isHttps() const
{
  return (0 == ::memcmp(scheme.data(), "https", 5));
}

void ReplyToString(std::string& response, QNetworkReply* rep)
{
  response.clear();
  if(nullptr == rep || rep->size() <= 0)
    return;

  response.resize(std::max(rep->size(),(qint64)0));
  ::memset(&response[0], 0x00, response.size());

  //read out and copy to ctx->response
  size_t pos = 0;
  for(qint64 rd = 1; rd > 0 && pos < response.size(); pos += rd)
    {
      rd = rep->read(&(response[pos]), response.size() - pos);
      pos += rd;
    }
}

#include <QDebug>

std::shared_ptr<QNetworkReply> ClientCtx::makeGet(const QNetworkRequest& req)
{
  QNetworkReply* rep = mgr->get(req);
  connect(rep, SIGNAL(finished()), this,SLOT(onFinished()), Qt::DirectConnection);
  connect(rep, SIGNAL(redirected(QUrl)), this, SLOT(onRedirected(QUrl)), Qt::DirectConnection);
  connect(rep, SIGNAL(error(QNetworkReply::NetworkError)),
          this, SLOT(onNetError(QNetworkReply::NetworkError)), Qt::DirectConnection);
  connect(rep, SIGNAL(sslErrors(QList<QSslError>)),
          this, SLOT(onSslErrors(QList<QSslError>)), Qt::DirectConnection);

  reply.reset(rep);
  return reply;
}

void ClientCtx::onRedirected(const QUrl &url)
{
  QNetworkRequest req;
  req.setUrl(url);
  req.setRawHeader("User-Agent", "Qt5GET 1.0");
  makeGet(req);
}

void ClientCtx::replyFinished(QNetworkReply* rep)
{
  reply.reset(rep);
  ReplyToString(response, rep);
  cond.notify_all();
}

void ClientCtx::onFinished()
{
  ReplyToString(response, reply.get());
  cond.notify_all();
}

void ClientCtx::onSslErrors(const QList<QSslError> &errors)
{
  reply->ignoreSslErrors();
  (void)errors;
}
void ClientCtx::onNetError(QNetworkReply::NetworkError e)
{
  qDebug() << e;
}

}//WebGrep
