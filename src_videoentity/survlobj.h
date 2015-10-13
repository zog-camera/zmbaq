#ifndef SURVLOBJ_H
#define SURVLOBJ_H

#include "src/survcamparams.h"
#include "src/zmbaq_common/cds_map.h"
#include "videoentity.h"
#include <array>


//-------------------------------------------------
namespace ZMBEntities {

ZM_DEF_STATIC_CONST_STRING(ZMQW_ROOT_URI, "/")
ZM_DEF_STATIC_CONST_STRING(ZMQW_OBJECT_URI, "/object")
ZM_DEF_STATIC_CONST_STRING(ZM_V4L_OBJ, "v4l")
ZM_DEF_STATIC_CONST_STRING(ZM_OBJ_KW_SETTINGS, "settings")
ZM_DEF_STATIC_CONST_STRING(ZM_OBJ_KW_OBJECTS, "objects")
ZM_DEF_STATIC_CONST_STRING(ZM_OBJ_KW_ID, "id")

Json::Value jvalue_from_file(bool& ok, const ZConstString& fname);


/** Represents a group of cameras with different sources
 * that relate to commn surveillance object (a client's context).
*/
class SurvlObj
{
public:

    typedef std::pair<ZMB::VideoEntityID, SHP(ZMBEntities::VideoEntity)> id_video_entity_pair_t;

    /** A pair <VideoEntityID, SHP(VideoEntity)> */
    class IDEntityPair : public id_video_entity_pair_t
    {
        public:
        IDEntityPair() : id_video_entity_pair_t() {}
        IDEntityPair(ZMB::VideoEntityID _id, SHP(ZMBEntities::VideoEntity) _ent)
            : id_video_entity_pair_t(_id, _ent) {}

        ZMB::VideoEntityID& id() {return first;}
        SHP(ZMBEntities::VideoEntity)& entity() {return second;}
        bool empty() const {return nullptr == second;}
        void clear() {first = ZMB::VideoEntityID(); second.reset();}
    };


    explicit SurvlObj(SHP(Poco::ThreadPool) pool = std::make_shared<Poco::ThreadPool>(),
                      ZMB::SurvlObjectID obj_uid = ZMB::SurvlObjectID());

    virtual ~SurvlObj();

    void clear();

    /** The configuration object must have following structure:
     *  {"settings":{object with some key-values like "id":"string"};
     *   "objects":[array of objects describing VideoEntity class]
     *  }
     *  Each item in "objects" array must have following keys for values:
     *  {"id" : "string",} */
    bool create(const Json::Value* config_object);

    /** Warning! May return (nullptr) if not okay.
     *  Each item in "objects" array must have following keys for values:
     *  {"id" : "string"; "name":"name_string";
     *   "entity":{..entity settings viable for SurvCamParams..}
     *   } */
    SHP(ZMBEntities::VideoEntity) spawn_entity(bool& is_ok, const Json::Value* jo, bool do_create);

    void bind_entity(SHP(ZMBEntities::VideoEntity) ve);

    /** ID issued for this object at it's creation.*/
    ZMB::SurvlObjectID id() const;

    SHP(Poco::ThreadPool) pool;

protected:
    ZMB::SurvlObjectID my_id;
    std::array<IDEntityPair, ZMBAQ_MAX_ENTITIES_PER_OBJECT> ve_array;
    u_int32_t array_items_cnt;

    Json::FastWriter wr;
    Json::Value settings;
    Json::Value entities_list;

    char tmp[32];
    char hex_buf[3 * sizeof(ZMB::VideoEntityID)];
};


}//ZMBEntities


#endif // SURVLOBJ_H
