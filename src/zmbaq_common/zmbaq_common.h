/*
A video surveillance software with support of H264 video sources.
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
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/


#ifndef ZMBAQ_COMMON_H
#define ZMBAQ_COMMON_H

#include <memory>
#include <new>
#include <cassert>
#include <mutex>

#include "zmb_str.h"
#include "mbytearray.h"

/** A short define for shared pointer*/
#ifndef SHP
    #define SHP(T) std::shared_ptr<T>
#endif //SHP


static const uint16_t ZMBAQ_ZMQ_PORT_ROUTER = 5570;
static const uint16_t ZMBAQ_ZMQ_PORT_PUB = 5571;
static const uint32_t ZMBAQ_ZMQ_IDENTITY_LEN = 16;

static const uint16_t ZMBAQ_LHOST_UDP_PORTS_RANGE[] = {10000, 16000};

static const uint32_t ZMBAQ_MINIMAL_MOVEMENT_SECONDS = 30;
static const uint32_t ZMBAQ_PACKETS_BUFFERING_SECONDS = 12;

/** Passed to the ffmpeg h264 encoder,
 *  it will emit the i-frame once per (N) frames.*/
static const uint32_t ZMBAQ_ENCODER_GOP_SIZE = 2 * 12;
static const uint32_t ZMBAQ_ENCODER_BITRATE = 2000 * 1000;
static const uint32_t ZMBAQ_ENCODER_DEFAULT_FPS = 12;
static const uint32_t ZMBAQ_MAX_ENTITIES_PER_OBJECT = 16;


//======================================================================
namespace ZMB {
static const uint32_t __test_endianness = 0xdeadbeef;
enum endianness { BIG, LITTLE };

static bool is_little_endian() { return *(const char *)&__test_endianness == 0xef; }
static bool is_big_endian() { return !is_little_endian();}
static endianness get_endiannes() {return (endianness)is_little_endian();}

template<typename T>
T max(T a, T b)
{return a < b? b : a;}

template<typename T>
T min(T a, T b)
{return a > b? b : a;}


//key words for accessing set/get methods of the Settings class.
ZM_DEF_STATIC_CONST_STRING(ZMKW_LOGSERVER, "log_server")
ZM_DEF_STATIC_CONST_STRING(ZMKW_LOGPROTO, "log_proto")
ZM_DEF_STATIC_CONST_STRING(ZMKW_LOGPORT, "log_port")
ZM_DEF_STATIC_CONST_STRING(ZMKW_CFG_LOCATION, "config_location")
ZM_DEF_STATIC_CONST_STRING(ZMKW_USER_HOME, "HOME")
ZM_DEF_STATIC_CONST_STRING(ZMKW_FS_TEMP_LOCATION, "fs_temp_location")
ZM_DEF_STATIC_CONST_STRING(ZMKW_FS_PERM_LOCATION, "fs_perm_location")
ZM_DEF_STATIC_CONST_STRING(ZMKW_TESTDB_LOCATION, "testdb_locaiton")
ZM_DEF_STATIC_CONST_STRING(ZMKW_TESTDBBLOB_LOCATION, "testdb_blob_location")


/** Singleton that lets access the key-value settings.
 * Method's name prefix "s_*" stands for "thread-safe" (with locking).
*/
class Settings
{
public:
    static Settings* instance();

    void set(ZConstString key, ZConstString value);
    void s_set(ZConstString key, ZConstString value, SHP(GenericLocker) locker);

    ZConstString get_string(ZConstString key) const;
    ZConstString s_get_string(ZConstString key, SHP(GenericLocker) locker) const;

    const Json::Value* get_jvalue(ZConstString key) const;
    const Json::Value* s_get_jvalue(ZConstString key, SHP(GenericLocker) locker) const;

    /** Get the whole JSON key-values map and block access
     * via mutex locking until returned value exists. */
    std::pair<Json::Value*, SHP(ZMB::GenericLocker)> get_all();

    std::shared_ptr<ZMB::GenericLocker> get_locker() const;

private:
    Settings();
    static std::atomic<Settings*> m_instance;
    static std::mutex m_mutex;

    ZMB::GenericLocker::lk_pfn_t lock_fn_ptr;
    ZMB::GenericLocker::lk_pfn_t unlock_fn_ptr;

    Json::Value all;
};
//======================================================================
//simple Fletcher's checksum (from wiki)
uint16_t fletcher16(const uint8_t* data, size_t bytes );

//simple 32-bit Fletcher's checksum (from wiki)
uint32_t fletcher32( uint16_t const *data, size_t words );

class noncopyable
{
protected:
    noncopyable() = default;
    ~noncopyable() = default;
    noncopyable( const noncopyable& ) = delete;
    noncopyable& operator=( const noncopyable& ) = delete;
};


}//namespace ZMB

#endif // ZMBAQ_COMMON_H

