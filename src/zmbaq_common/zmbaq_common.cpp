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

#include <tbb/scalable_allocator.h>
#include "char_allocator_singleton.h"
#include <cstdlib>
#include <assert.h>
#include "jsoncpp/json/value.h"


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
std::pair<Json::Value*, SHP(ZMB::GenericLocker)> Settings::get_all()
{
    return std::pair<Json::Value*, SHP(ZMB::GenericLocker)> (&all, get_locker());
}

std::shared_ptr<GenericLocker> Settings::get_locker() const
{
  return ZMB::GenericLocker::make_item((void*)this, lock_fn_ptr, unlock_fn_ptr);
}

Settings::Settings()
{
    lock_fn_ptr = [](void* Settings_cast_ptr)
    {
        Settings* s = (Settings*)Settings_cast_ptr;
        s->m_mutex.lock();
    };

    unlock_fn_ptr = [](void* Settings_cast_ptr)
    {
        Settings* s = (Settings*)Settings_cast_ptr;
        s->m_mutex.unlock();
    };


    set(ZMKW_LOGPORT, ZCSTR("59000"));
    set(ZMKW_LOGSERVER, ZCSTR("127.0.0.1"));
    set(ZMKW_LOGPROTO, ZCSTR("ZMG_PUB"));
    ZConstString home_str((const char*)secure_getenv("HOME"));
    set(ZMB::ZMKW_USER_HOME, home_str);

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
        set(key, ZConstString(origin, end));
    };
    __set_value(ZCSTR("/zmbaq_config.json"), ZMKW_CFG_LOCATION);

    /** TODO: platform-dependent temporary storage. **/
    __set_value(ZCSTR("/tmp/video_temp"), ZMKW_FS_TEMP_LOCATION);
    __set_value(ZCSTR("/tmp/video_perm"), ZMKW_FS_PERM_LOCATION);

    __set_value(ZCSTR("/tmp/test_db"), ZMKW_TESTDB_LOCATION);
    __set_value(ZCSTR("/tmp/test_blob_db"), ZMKW_TESTDBBLOB_LOCATION);
}

void Settings::set(ZConstString key, ZConstString value)
{
    all[key.begin()] = Json::Value(value.begin(), value.end());
}

ZConstString Settings::get_string(ZConstString key) const
{
    auto jp = get_jvalue(key);
    bool ok = nullptr != jp && jp->isString();
    const char* b = 0, *e = 0;
    assert(ok);
    if (ok)
    {
        jp->getString(&b, &e);
    }
    return ZConstString(b, e);
}

const Json::Value* Settings::get_jvalue(ZConstString key) const
{
    return all.find(key.begin(), key.end());
}

void Settings::s_set(ZConstString key, ZConstString value, SHP(GenericLocker) locker)
{
   return set(key, value);
}

ZConstString Settings::s_get_string(ZConstString key, SHP(GenericLocker) locker) const
{
   return get_string(key);
}

const Json::Value* Settings::s_get_jvalue(ZConstString key, SHP(GenericLocker) locker) const
{
    return get_jvalue(key);
}



//double-check singleton:
std::atomic<Settings*> Settings::m_instance;
std::mutex Settings::m_mutex;

Settings* Settings::instance()
{
    Settings* ptr = m_instance.load();
    if (nullptr != ptr)
        return ptr;
    std::lock_guard<std::mutex> lock(m_mutex);
    ptr = m_instance.load();

    if (nullptr == ptr)
    {
        ptr = new Settings;
        m_instance.store(ptr);
    }
    return ptr;
}
//-----------------------------------------------------------


}//namespace ZMB


