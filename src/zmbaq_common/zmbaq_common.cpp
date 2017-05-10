/*A video surveillance software with support of H264 video sources.
Copyright (C) 2015 Bogdan Maslowsky, Alexander Sorvilov. 

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU Affero General Public License as published
by the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Affero General Public License for more details.

You should have received a copy of the GNU Affero General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.*/


#include "zmbaq_common.h"

#include <cstdlib>
#include <assert.h>
#include "json/value.h"


//----
namespace ZMB {


uint16_t fletcher16(const uint8_t* data, size_t bytes )
{
    const uint8_t* ptr = data;
    uint16_t sum1 = 0xff, sum2 = 0xff;

    while (bytes) {
        size_t tlen = bytes > 20 ? 20 : bytes;
        bytes -= tlen;
        do {
            sum2 += sum1 += *ptr++;
        } while (--tlen);
        sum1 = (sum1 & 0xff) + (sum1 >> 8);
        sum2 = (sum2 & 0xff) + (sum2 >> 8);
    }
    /* Second reduction step to reduce sums to 8 bits */
    sum1 = (sum1 & 0xff) + (sum1 >> 8);
    sum2 = (sum2 & 0xff) + (sum2 >> 8);
    return sum2 << 8 | sum1;
}

uint32_t fletcher32( uint16_t const *data, size_t words )
{
        uint32_t sum1 = 0xffff, sum2 = 0xffff;

        while (words) {
                unsigned tlen = words > 359 ? 359 : words;
                words -= tlen;
                do {
                        sum2 += sum1 += *data++;
                } while (--tlen);
                sum1 = (sum1 & 0xffff) + (sum1 >> 16);
                sum2 = (sum2 & 0xffff) + (sum2 >> 16);
        }
        /* Second reduction step to reduce sums to 16 bits */
        sum1 = (sum1 & 0xffff) + (sum1 >> 16);
        sum2 = (sum2 & 0xffff) + (sum2 >> 16);
        return sum2 << 16 | sum1;
}
//-----------------------------------------------------------


Settings::Settings()
{
  auto lk = getLocker();

  set(ZMKW_LOGPORT, std::string("59000"), lk);
  set(ZMKW_LOGSERVER, std::string("127.0.0.1"), lk);
  set(ZMKW_LOGPROTO, std::string("ZMG_PUB"), lk);
  ZConstString home_str((const char*)secure_getenv("HOME"));
  set(ZMB::ZMKW_USER_HOME, home_str, lk);

  char* temp = (char*)alloca(ZMB::max<unsigned>(1024u, (unsigned)(4 * home_str.len)));
  ZUnsafeBuf buf(temp, 1024); buf.fill(0);
  buf.read(0, home_str, 0);
  char* origin = buf.begin();

  buf.str += home_str.len;//advance to point after the user home location
  buf.len -= home_str.len;

  auto __set_value = [&](ZConstString str, ZConstString key) mutable
  {
    char* end = 0;
    buf.read(&end, str, 0);
    *end = '\0';
    set(key, ZConstString(origin, end), lk);
  };
  __set_value(std::string("/zmbaq_config.json"), ZMKW_CFG_LOCATION);

  /** TODO: platform-dependent temporary storage. **/
  __set_value(std::string("/tmp/video_temp"), ZMKW_FS_TEMP_LOCATION);
  __set_value(std::string("/tmp/video_perm"), ZMKW_FS_PERM_LOCATION);

  __set_value(std::string("/tmp/test_db"), ZMKW_TESTDB_LOCATION);
  __set_value(std::string("/tmp/test_blob_db"), ZMKW_TESTDBBLOB_LOCATION);
}

void Settings::set(ZConstString key, const Json::Value& val, Locker_t& lk)
{
  (void)lk;
  all[key.begin()] = val;
}

void Settings::set(ZConstString key, ZConstString value, Settings::Locker_t& lk)
{
  (void)lk;
  all[key.begin()] = Json::Value(value.begin(), value.end());
}

ZConstString Settings::string(ZConstString key, Settings::Locker_t& lk) const
{
  (void)lk;
  auto jp = jvalue(key, lk);
  bool ok = nullptr != jp && jp->isString();
  const char* b = 0, *e = 0;
  assert(ok);
  if (ok)
    {
      jp->getString(&b, &e);
    }
  return ZConstString(b, e);
}

const Json::Value* Settings::jvalue(ZConstString key, Settings::Locker_t& lk) const
{
  (void)lk;
  return all.find(key.begin(), key.end());
}

Json::Value* Settings::getAll(Locker_t& lk)
{
  (void)lk;
  return &all;
}


//-----------------------------------------------------------


}//namespace ZMB


