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


#include "survcamparams.h"
#include "noteinfo.h"
#include <cstring>

namespace ZMB {
bool operator < (const ZMB::SurvlObjectID& one, const ZMB::SurvlObjectID& two)
{
   if (one.client_id.client_id < two.client_id.client_id)
       return true;
   return one.survlobj_id < two.survlobj_id;
}

char* VideoEntityID::make_entity_id(ZUnsafeBuf* str) const
{
    VideoEntityID id = *this;
    ZUnsafeBuf id_data((char*)&id, sizeof(id));
    id_data.htonl_self((u_int32_t*)id_data.begin(), 0);
    char* end = ZMB::to_texthex_4b(str, ZConstString(id_data.to_const()));
    str->len = end - str->begin();
    return end;
}

bool VideoEntityID::read_from_hex4b(const ZConstString& hex4b, char** p_end)
{
    ZUnsafeBuf dt((char*)this, sizeof(*this));
    char* end = ZMB::from_texthex_4b(&dt, hex4b);
    dt.ntohl_self((u_int32_t*)dt.begin());
    if (0 == end || (end - (char*)this) <= sizeof(*this))
        return false;
    if (nullptr != p_end)
        *p_end = end;
    return true;
}

bool VideoEntityID::operator () (const VideoEntityID& one, const VideoEntityID& two) const
{
    u_int64_t a = 0, b = 0;
    ::memcpy(&a, &one, 8);
    ::memcpy(&b, &two, 8);
    if (a < b)
        return true;
    return one.id < two.id;
}


void SurvCamParams::set_rtp(const ZMBCommon::MByteArray& server, u_int16_t port)
{
   RTP_settings["server"] = server.to_jvalue();
   RTP_settings["port"] = (int)port;
}

bool SurvCamParams::is_empty() const
{
    return settings.empty();
}

const ZMB::VideoEntityID& SurvCamParams::get_id() const
{
   assert(entity_id.empty());
   return entity_id;
}

ZConstString SurvCamParams::find(const ZConstString& key) const
{
   const char* b = 0, *e = 0;
   const Json::Value* j = settings.find(key.begin(), key.end());
   if (0 != j)
       j->getString(&b, &e);
   return ZConstString(b, e);
}

Json::Value const* SurvCamParams::set(const ZConstString& key, const ZConstString& val)
{
    settings[key.begin()] = Json::Value(val.begin(), val.end());
    auto jp = settings.find(key.begin(), key.end());
    return jp;
}

Json::Value const* SurvCamParams::set(const ZConstString& key, int val)
{
    settings[key.begin()] = val;
    auto jp = settings.find(key.begin(), key.end());
    return jp;
}
}

bool operator == (const ZMB::VideoEntityID& one, const ZMB::VideoEntityID& two)
{
   return 0 == memcmp(&one, &two, sizeof(one));
}

bool operator < (const ZMB::VideoEntityID& one, const ZMB::VideoEntityID& two)
{
    return one.operator ()(one, two);
}


//#ifndef __LOCAL_JSON_EXT__
//#define JSON_EXTR_INT(JOBJECT, KEY, DEFAULT_VAL) \
//    (JOBJECT).get((KEY), Json::Value((DEFAULT_VAL))).asInt()

//#define JSON_EXTR_STR(JOBJECT, KEY) \
//    (JOBJECT)[KEY].asString()
//#endif

//void SurvCamParams::jread(const Json::Value& obj)
//{
//    if (!obj.isMember("class"))
//        return;

//    if (!obj.isMember("params"))
//        return;

//    Json::Value jo = obj["params"];

//    Json::Value empty;

//    id = jo.get("id", Json::Value(-1)).asInt();

//    cam_width = ((orientation==ROTATE_90||orientation==ROTATE_270)?height:width);
//    cam_height = ((orientation==ROTATE_90||orientation==ROTATE_270)?width:height);


//    client_id = JSON_EXTR_STR(jo, "client");
//    path = JSON_EXTR_STR(jo, "path");
//    method = JSON_EXTR_STR(jo, "method");
//    type = JSON_EXTR_STR(jo, "type");
//    options = JSON_EXTR_STR(jo, "options");
//    name = JSON_EXTR_STR(jo, "name");
////    sock_video = jo.value("sock_video").toString();
////    sock_video_preview = jo.value("sock_video_preview").toString();

//    deinterlacing = JSON_EXTR_INT(jo, "deinterlacing", -1);
//    function = JSON_EXTR_INT(jo, "function", 3);/*3, MODECT*/
//    enabled = JSON_EXTR_INT(jo, "enabled", 1);
//    width = JSON_EXTR_INT(jo, "width", 640);
//    height = JSON_EXTR_INT(jo, "width", 480);
//    colours = JSON_EXTR_INT(jo, "colours", -1);
//    orientation = JSON_EXTR_INT(jo, "orientation", 0);
//    brightness = JSON_EXTR_INT(jo, "brightness", -1);
//    contrast = JSON_EXTR_INT(jo, "contrast", -1);
//    hue = JSON_EXTR_INT(jo, "hue", -1);

//    label_x = JSON_EXTR_STR(jo, "label_x");
//    label_y = JSON_EXTR_STR(jo, "label_x");

//    track_motion = JSON_EXTR_INT(jo, "trac_motion", 1);

////    image_buffer_count = jo.value("image_buffer_count").toInt(-1);
////    warmup_count = jo.value("warmup_count").toInt(-1);
////    pre_event_count = jo.value("pre_event_count").toInt(24);
////    post_event_count = jo.value("post_event_count").toInt(24);
////    stream_replay_buffer = jo.value("stream_replay_buffer").toInt(-1);
////    alarm_frame_count = jo.value("alarm_frame_count").toInt(24);
////    section_length = jo.value("section_length").toInt(-1);
////    frame_skip = jo.value("frame_skip").toInt(-1);
////    motion_frame_skip = jo.value("motion_frame_skip").toInt(-1);
////    capture_delay = jo.value("capture_delay").toInt(-1);
////    alarm_capture_delay = jo.value("alarm_capture_delay").toInt(-1);
////    fps_report_interval = jo.value("fps_report_interval").toInt(-1);
////    ref_blend_perc = jo.value("ref_blend_perc").toInt(-1);
////    alarm_ref_blend_perc = jo.value("alarm_ref_blend_perc").toInt(-1);


////    event_prefix = jo.value("event_prefix").toString().toUtf8();
////    label_format = jo.value("label_format").toString().toUtf8();


//}

//Json::Value SurvCamParams::tojson() const
//{
//    Json::Value all;
//    all["class"] = "camera";
//    all["name"] = name.to_jvalue();
//    if (name.empty())
//    {
//        all["name"] = path.to_jvalue();
//    }
//    all["type"] =  type.to_jvalue();
//    if (type == "local")
//        all["location"] = "/dev";

//    Json::Value jo;

//    jo["id"]     = id;
//    jo["path"]   = path.to_jvalue();
//    jo["method"] = method.to_jvalue();
//    jo["options"]= options.to_jvalue();
//    jo["client"] = client_id.to_jvalue();

////    jo.operator[]("sock_video", QJsonValue(sock_video));
////    jo.operator[]("sock_video_preview", QJsonValue(sock_video_preview));

//    if (type == "local")
//    {
//        jo["type"] = "local";
//    }

//    if (!RTP_settings.isNull())
//        jo["RTP"] = RTP_settings;

//    jo["width"]  = width;
//    jo["height"] = height;
//    jo["enabled"]= enabled;

//    all["params"]= jo;
//    return all;
//}

//--------------------------------------------------------
