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

struct ClientID
{
    ClientID(u_int32_t val = 0xffffffff)
    {
        client_id = val;
    }
    u_int32_t client_id;
};

struct SurvlObjectID
{
    SurvlObjectID()
    {
        ZUnsafeBuf zb(mem_accessor()); 
        survlobj_id = 0xffffffff;
    }
    ClientID  client_id;
    u_int32_t survlobj_id;

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
