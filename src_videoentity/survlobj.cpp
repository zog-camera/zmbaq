#include "survlobj.h"
#include "src/noteinfo.h"
#include "mbytearray.h"

#include <fstream>
#include <iostream>

//-------------------------------------------------
namespace ZMBEntities {

Json::Value jvalue_from_file(bool &ok, const ZConstString& fname)
{
    char _buf [1024];
    ZUnsafeBuf buf(_buf, sizeof(_buf));
    buf.fill(0);
    ZMBCommon::MByteArray text;
    std::ifstream in;
    in.open(fname.begin());

    for (size_t bytes_read = 0; !in.eof() && in.good();
         text += ZConstString(buf.begin(), bytes_read))
    {
        bytes_read = in.readsome(buf.begin(), buf.size());
        if (0 == bytes_read)
            break;
    }
    in.close();
    Json::Reader rd;
    Json::Value cfg;

    ok = rd.parse(text.data(), text.data() + text.size(), cfg);
    return cfg;
}

SurvlObj::SurvlObj(SHP(ZMBCommon::ThreadsPool) pool, ZMB::SurvlObjectID obj_uid)
{
    my_id = obj_uid;
    array_items_cnt = 0;
}

SurvlObj::~SurvlObj()
{
}
ZMB::SurvlObjectID SurvlObj::id() const
{
    return my_id;
}

void SurvlObj::clear()
{
    for (auto it = ve_array.begin(); it != ve_array.end(); ++it)
    {
        (*it).entity()->stop();
        (*it).clear();
    }
}


bool SurvlObj::create(const Json::Value* config_object)
{
    static auto fn_fallback_fs_settings = [](Json::Value* dest, const Json::Value* src)
            -> bool/*true if the permanent storage location was modified*/
    {
        bool modified = false;
        ZConstString fs_loc(ZMB::SURV_CAM_PARAMS_FS, dest);
        if (fs_loc.empty())
        {
            modified = true;
            fs_loc = ZConstString(ZMB::SURV_CAM_PARAMS_FS, src);
            assert(!fs_loc.empty());
            dest->operator [](ZMB::SURV_CAM_PARAMS_FS.begin()) = Json::Value(fs_loc.begin(), fs_loc.end());
        }
        fs_loc = ZConstString(ZMB::SURV_CAM_PARAMS_TMPFS, dest);
        if (fs_loc.empty())
        {
            fs_loc = ZConstString(ZMB::SURV_CAM_PARAMS_TMPFS, src);
            assert(!fs_loc.empty());
            dest->operator [](ZMB::SURV_CAM_PARAMS_TMPFS.begin()) = Json::Value(fs_loc.begin(), fs_loc.end());
        }
        return modified;
    };

    settings = config_object->operator [](ZM_OBJ_KW_SETTINGS.begin());
    {
      ZMB::Settings* S = ZMB::Settings::instance();
      auto lk = S->getLocker();
      Json::Value* all = S->getAll(lk);
      fn_fallback_fs_settings(&settings, all);
    }

    ZMB::SurvlObjectID& id (my_id);
    ZConstString id_str(ZM_OBJ_KW_ID, (const Json::Value*)&(settings));

    if (!id_str.empty())
    {//hex4b -> id
        ZUnsafeBuf data((char*)&id, sizeof(id));
        ZMB::from_texthex_4b(&data, id_str);
        assert(data.size() == sizeof(id));
    }
    else
    {//generate id:
        id.survlobj_id = rand() % 1024;
    }
    const Json::Value* entities = config_object->find(ZM_OBJ_KW_OBJECTS.begin(), ZM_OBJ_KW_OBJECTS.end());
    if (nullptr == entities)
    {
        return false;
    }
    entities_list = *entities;
    for (auto eit = entities->begin(); eit != entities->end(); ++eit)
    {
        auto entity_cfg = (Json::Value*)eit.operator ->();
        if ( !ZMBEntities::VideoEntity::is_cfg_viable(entity_cfg))
            continue;
        bool ok = false;
        bool was_modified = fn_fallback_fs_settings(entity_cfg, &settings);
        if (was_modified)
        {
            /**HINT: add subdirectory with video entity name if we are using
             * common SurvlObj setting of the permanent storage path.
             **/
            ZConstString fs_loc(ZMB::SURV_CAM_PARAMS_FS, entity_cfg);
            ZConstString item_name(ZMB::SURV_CAM_PARAMS_NAME, entity_cfg);
            ZMBCommon::MByteArray np = fs_loc;
            np += ZMFS::FSLocation::dir_path_sep;
            np += item_name;
            auto znp = np.get_const_str();
            //(*entity_cfg)["fs"] = path;
            entity_cfg->operator [](ZMB::SURV_CAM_PARAMS_FS.begin()) =
                    Json::Value(znp.begin(), znp.end());
        }
        auto ve_ptr = spawn_entity(ok, entity_cfg, true/*does invoke VideoEntity::create()*/);

        if (nullptr == ve_ptr)
            continue;
    }
    return true;

}

SHP(ZMBEntities::VideoEntity) SurvlObj::spawn_entity(bool& is_ok, const Json::Value* jo, bool do_create)
{
    SHP(ZMBEntities::VideoEntity) en;
    if (array_items_cnt == ZMBAQ_MAX_ENTITIES_PER_OBJECT)
    {
        is_ok = false;
        return en;
    }
    en = std::make_shared<ZMBEntities::VideoEntity>(pool);

    is_ok = true;
    if (do_create)
    {
        is_ok = en->configure(jo);
        if (!is_ok)
        {
            //                auto msg = make_mbytearray(__PRETTY_FUNCTION__);
            //                (*msg) += " Wrong configuration JSON!";
            //                emit sig_error(shared_from_this(), msg);
            return SHP(ZMBEntities::VideoEntity)(nullptr);
        }
        bind_entity(en);
        // start the processing threads of the entity:
        en->start();
    }
    return en;
}

void SurvlObj::bind_entity(SHP(ZMBEntities::VideoEntity) ve)
{
    //------------------------------------------------
    /** TODO: ASSIGN THE VIDEOENTITY ID FROM SAVED SETTINGS.**/
    auto veid = ve->id();
    veid.suo_id = this->id();
    veid.id = array_items_cnt;

    //copy VideoEntity id to params:
    ve->set_id(veid);
    ve_array.at(array_items_cnt) = IDEntityPair(veid, ve);
    ++array_items_cnt;
    //------------------------------------------------
}

}//ZMBEntities
