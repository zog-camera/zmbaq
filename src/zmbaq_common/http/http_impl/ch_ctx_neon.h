#ifndef CH_CTX_NIX_H
#define CH_CTX_NIX_H
#include <string>
#include <array>
#include <mutex>
#include <memory>
#include <cstring>
#include <cassert>
#include "../noncopyable.hpp"

extern "C" {
#include <neon/ne_session.h>
#include <neon/ne_request.h>
#include <neon/ne_utils.h>
#include <neon/ne_uri.h>
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
  ClientCtx() : sess(nullptr), port(0) {
    scheme.fill(0x00);
  }

  virtual ~ClientCtx();

  //@return TRUE if scheme is "https"
  bool isHttps() const;

  ne_session* sess;
  uint16_t port;
  Scheme6 scheme;// "http\0\0" or "https\0"
  std::string response;
  std::string host_and_port;
  std::mutex mu;//locked in issueRequest()
};

struct IssuedRequest
{//ref.count holding structure
  std::shared_ptr<ne_request> req;
  std::shared_ptr<ClientCtx> ctx; //holds reference of a context
};


}//WebGrep

#endif // CH_CTX_NIX_H
