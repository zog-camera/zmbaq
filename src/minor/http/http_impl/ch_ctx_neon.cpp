#include "http_impl/ch_ctx_neon.h"
#include <cstring>

namespace WebGrep {

ClientCtx::~ClientCtx()
{
  if (nullptr != sess)
    ne_session_destroy(sess);
}

bool ClientCtx::isHttps() const
{
  return (0 == ::memcmp(scheme.data(), "https", 5));
}

}//WebGrep
