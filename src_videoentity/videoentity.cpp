#include "videoentity.h"
#include <iostream>
#include "videoentity_pv.h"

namespace ZMBEntities {

VideoEntity::VideoEntity(SHP(Poco::ThreadPool) p_pool)
    : pool(p_pool)
{
    pv = std::make_shared<VEPV>(this, pool);
}

VideoEntity::~VideoEntity()
{

}

void VideoEntity::set_id(ZMB::VideoEntityID number)
{
    pv->cam_param.entity_id = number;
}

ZMB::VideoEntityID VideoEntity::id() const
{
   return pv->cam_param.entity_id;
}

bool VideoEntity::configure(const Json::Value* jobject, bool do_start)
{
    bool ok = pv->configure(jobject, do_start);
    if (ok && do_start)
        start();
    return ok;
}

bool VideoEntity::start()
{
    if (0 != pv->stream)
    {
        pv->task_mgr->start(pv->file_dump);
        pv->task_mgr->start(pv->stream);
        return true;
    }
    return false;
}

void VideoEntity::stop()
{
    pv->clear();
}

const ZMB::SurvCamParams& VideoEntity::params() const
{
    return pv->cam_param;
}

bool VideoEntity::reopen(bool do_start)
{
    return configure(&pv->cam_param.settings, do_start);
}

/** Check if the config is acceptable to create the object.*/
bool VideoEntity::is_cfg_viable(const Json::Value* jval)
{
    if (!jval->isObject()) return false;
    auto jp = jval->find(ZMB::SURV_CAM_KW_ENTITY.begin(), ZMB::SURV_CAM_KW_ENTITY.end());
    if (nullptr == jp)
        jp = jval;
    ZConstString val(ZMB::SURV_CAM_PARAMS_PATH, jp);
    if (val.empty()) return false;
    return true;
}

}//ZMBEntities
