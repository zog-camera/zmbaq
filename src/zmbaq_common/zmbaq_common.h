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
#include <cassert>
#include <mutex>
#include <array>

#include "noncopyable.hpp"


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
T g_max(T a, T b)
{return a < b? b : a;}

template<typename T>
T g_min(T a, T b)
{return a > b? b : a;}

enum class ZMB_SETTINGS_KW
{
  LOGSERVER, LOGPROTO, LOGPORT, CONFIG_LOCATION, USER_HOME, FS_TEMP_LOCATION, FS_PERMANENT_LOCATION, TESTDB_LOCATION, TESTDBBLOB_LOCATION, LAST_ENUM_SIZE_ITEM
};
 
typedef std::pair<ZMB_SETTINGS_KW, std::string> ZmbSettingsPair;
typedef std::array<std::pair<ZMB_SETTINGS_KW, std::string>,
  (int)ZMB_SETTINGS_KW::LAST_ENUM_SIZE_ITEM> ZmbSettingsWords;
 
//get key words for accessing set/get methods of the Settings class.
ZmbSettingsWords GetSettinsWords()
{
  return {
    ZmbSettingsPair(ZMB_SETTINGS_KW::LOGSERVER, "log_server"),
      ZmbSettingsPair(ZMB_SETTINGS_KW::LOGPROTOCOL, "log_protocol"),
      ZmbSettingsPair(ZMB_SETTINGS_KW::LOGPORT, "log_port"),
      ZmbSettingsPair(ZMB_SETTINGS_KW::CONFIG_LOCATION, "config_location"),
      ZmbSettingsPair(ZMB_SETTINGS_KW::USER_HOME, "HOME"),
      ZmbSettingsPair(ZMB_SETTINGS_KW::FS_TEMP_LOCATION, "fs_temp_location"),
      ZmbSettingsPair(ZMB_SETTINGS_KW::FS_TEMP_LOCATION, "fs_perm_location"),
      ZmbSettingsPair(ZMB_SETTINGS_KW::TESTDB_LOCATION, "testdb_location")
      };
}

using namespace ZMBCommon;

/** Singleton that lets access the key-value settings.
 * Method's name prefix "s_*" stands for "thread-safe" (with locking).
*/
class Settings
{
public:
  typedef std::unique_lock<std::mutex> Locker_t;

  /** Constructing local object will not make singleton instance.*/
  Settings();

  std::mutex accessMutex;
  Locker_t&& getLocker() {return std::move(Locker_t(accessMutex));}

  //setters:
  void set(ZConstString key, const Json::Value& val, Locker_t& lk);
  void set(ZConstString key, ZConstString value, Locker_t& lk);

  //getters:
  ZConstString string(ZConstString key, Locker_t& lk) const;

  const Json::Value* jvalue(ZConstString key, Locker_t& lk) const;

  /** Get the whole JSON key-values map and block access
     * via mutex locking until returned value exists. */
  Json::Value* getAll(Locker_t& lk);


private:
  Json::Value all;
};
//======================================================================
//simple Fletcher's checksum (from wiki)
uint16_t fletcher16(const uint8_t* data, size_t bytes );

//simple 32-bit Fletcher's checksum (from wiki)
uint32_t fletcher32( uint16_t const *data, size_t words );

}//namespace ZMB

#endif // ZMBAQ_COMMON_H

