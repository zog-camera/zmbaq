#ifndef CH_CTX_CURL_H
#define CH_CTX_CURL_H

#include <string>
#include <array>
#include <mutex>
#include <memory>
#include <cstring>
#include <cassert>
#include "../noncopyable.hpp"

extern "C" {
        #include "curl/curl.h"
        #include "curl/easy.h"
}

namespace WebGrep {

class Scheme6 : public std::array<char,6>
{
public:
  Scheme6() { this->fill(0x00); }

  void copyFrom(const char* cstr) { copyFrom(cstr, ::strlen(cstr)); }
  void copyFrom(const char* other, size_t len)
  { assert(len <= size()); fill(0x00); ::memcpy(data(), other, len); }
  void writeTo(char* dest) {::memcpy(dest, data(), size());}
};

/** libneon ne_session holder*/
class ClientCtx : public ZMBCommon::noncopyable
{
public:
  ClientCtx();

  virtual ~ClientCtx();

  //@return TRUE if scheme is "https"
  bool isHttps() const;
  void disconnect();

  CURL* curl;
  CURLcode status;
  std::string url;

  uint16_t port;
  Scheme6 scheme;// "http\0\0" or "https\0"
  std::string response;
  std::string host_and_port;
  std::mutex mu;//locked in issueRequest()
};

struct IssuedRequest
{//ref.count holding structure
  IssuedRequest()
  { res = CURL_LAST; responseStringPtr = nullptr;}
  bool valid() const {return nullptr != ctx && nullptr != ctx->curl;}

  CURLcode res;
  WebGrep::Scheme6 method;//example: "POST\0\0"
  std::shared_ptr<ClientCtx> ctx;
  std::string* responseStringPtr;//valid while ctx is valid too
};


}//WebGrep
#endif // CH_CTX_CURL_H
