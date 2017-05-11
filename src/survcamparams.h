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


#ifndef SURVCAMPARAMS_H
#define SURVCAMPARAMS_H
#include "json/json.h"
#include "zmbaq_common/zmbaq_common.h"
#include "zmbaq_common/mbytearray.h"

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
  LOGSERVER, LOGPROTO, LOGPORT, CONFIG_LOCATION, USER_HOME, FS_TEMP_LOCATION, FS_PERMANENT_LOCATION, TESTDB_LOCATION, TESTDBBLOB_LOCATION, LAST_ENUM_SIZE
};
 
typedef std::pair<ZMB_SETTINGS_KW, std::string> ZmbSettingsPair;
typedef std::array<ZmbSettingsPair, (int)ZMB_SETTINGS_KW::LAST_ENUM_SIZE> ZmbSettingsWords;
 
//get key words for accessing set/get methods of the Settings class.
ZmbSettingsWords&& GetSettinsWords()
{
  return std::move({
    ZmbSettingsPair(ZMB_SETTINGS_KW::LOGSERVER, "log_server"),
      ZmbSettingsPair(ZMB_SETTINGS_KW::LOGPROTOCOL, "log_protocol"),
      ZmbSettingsPair(ZMB_SETTINGS_KW::LOGPORT, "log_port"),
      ZmbSettingsPair(ZMB_SETTINGS_KW::CONFIG_LOCATION, "config_location"),
      ZmbSettingsPair(ZMB_SETTINGS_KW::USER_HOME, "HOME"),
      ZmbSettingsPair(ZMB_SETTINGS_KW::FS_TEMP_LOCATION, "fs_temp_location"),
      ZmbSettingsPair(ZMB_SETTINGS_KW::FS_TEMP_LOCATION, "fs_perm_location"),
      ZmbSettingsPair(ZMB_SETTINGS_KW::TESTDB_LOCATION, "testdb_location")
      });
}

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

 
struct ClientID
{
    u_int32_t client_id = 0xffffffff;
};

struct SurvlObjectID
{
    ClientID  client_id;
    u_int32_t survlobj_id = 0xffffffff;

    ZUnsafeBuf mem_accessor() const {return ZUnsafeBuf((char*)this, sizeof(*this));}
};
bool operator < (const ZMB::SurvlObjectID& one, const ZMB::SurvlObjectID& two);

struct VideoEntityID
{
    VideoEntityID() {id = 0xffffffff;}
    bool empty() const {return 0xffffffff == id;}

    SurvlObjectID suo_id;
    u_int32_t id;

    ZUnsafeBuf mem_accessor() const {return ZUnsafeBuf((char*)this, sizeof(*this));}

    /** Print fields in hex numbers grouped by 4bytes.
     * @param str: must have length >= sizeof(*this) * 2.25f;
     *  the length value str->len will be modified.
     *
     * @return end of the string(next element after he last one). */
    char* make_entity_id(ZUnsafeBuf* str) const;

    /** Read values from hex format of type: 0xaabbccdd (represents u_int32_t)*/
    bool read_from_hex4b(const ZConstString& hex4b, char** p_end = 0);

    bool operator () (const VideoEntityID& one, const VideoEntityID& two) const;
};


/** Keywords for JSON settings.*/
ZM_DEF_STATIC_CONST_STRING(SURV_CAM_KW_ENTITY, "entity")
ZM_DEF_STATIC_CONST_STRING(SURV_CAM_PARAMS_ID, "id")
ZM_DEF_STATIC_CONST_STRING(SURV_CAM_PARAMS_SETTINGS, "settings")
ZM_DEF_STATIC_CONST_STRING(SURV_CAM_PARAMS_TYPE, "type")
ZM_DEF_STATIC_CONST_STRING(SURV_CAM_PARAMS_PATH, "path")

/** Path on file system to where we usually dump files (if there are any)*/
ZM_DEF_STATIC_CONST_STRING(SURV_CAM_PARAMS_FS, "fs")
ZM_DEF_STATIC_CONST_STRING(SURV_CAM_PARAMS_TMPFS, "tmpfs")

ZM_DEF_STATIC_CONST_STRING(SURV_CAM_PARAMS_NAME, "name")
ZM_DEF_STATIC_CONST_STRING(SURV_CAM_PARAMS_METHOD, "method")
ZM_DEF_STATIC_CONST_STRING(SURV_CAM_PARAMS_OPTIONS, "options")
ZM_DEF_STATIC_CONST_STRING(SURV_CAM_PARAMS_LABEL_X, "label_x")
ZM_DEF_STATIC_CONST_STRING(SURV_CAM_PARAMS_LABEL_Y, "label_y")

/** Possible values*/
ZM_DEF_STATIC_CONST_STRING(SURV_CAM_PARAMS_TYPE_USB, "usb")
ZM_DEF_STATIC_CONST_STRING(SURV_CAM_PARAMS_TYPE_RTSP, "rtsp")

//keywords for SurvlObj
ZM_DEF_STATIC_CONST_STRING(SURV_OBJECT_KW_ID, "object_id")
ZM_DEF_STATIC_CONST_STRING(SURV_OBJECT_KW_SETTINGS, "object_settings")
ZM_DEF_STATIC_CONST_STRING(SURV_OBJECT_KW_ENTITIES, "object_entities")

class SurvCamParams
{
public:

    enum ImgOrientation{Default, ROTATE_90, ROTATE_180, ROTATE_270};

    SurvCamParams() = default;
    SurvCamParams(const SurvCamParams&) = default;

    /**set RTP_settings that shall be exported as.
     * {"RTP":{"server":"127.0.0.1"; "port":"9002"}} */
    void set_rtp(const std::string& server, u_int16_t port);


    bool is_empty() const;
    const ZMB::VideoEntityID& get_id() const;

    /** Find string-type value in this->settings.*/
    ZConstString find(const ZConstString& key) const;
    Json::Value const* set(const ZConstString& key, const ZConstString& val);
    Json::Value const* set(const ZConstString& key, int val);

    /** Used only in runtime (no export),
     * updated within client/server negotiation. */
    Json::Value RTP_settings;

    /** Must be set externally.
     item keys for string-type values:
     @verbatim
        id type name path method
        options label_x label_y
     @endverbatim

     item keys for int-type values:
      @verbatim
        function enabled width height colours
        orientation brightness contrast hue colour
        deinterlacing track_motion cam_width cam_height
     @endverbatim  */
    Json::Value settings;

    /** Must be set externally. Identifies client + entity.
     * ZMB::SurvlObjectID parent_id = entity_id.suo_id; */
    ZMB::VideoEntityID entity_id;
};
}//ZMB

bool operator == (const ZMB::VideoEntityID& one, const ZMB::VideoEntityID& two);
bool operator < (const ZMB::VideoEntityID& one, const ZMB::VideoEntityID& two);


#endif // SURVCAMPARAMS_H
