#include "videoentity.h"
#include <iostream>
#include "videoentity_pv.h"

namespace ZMBEntities {


void VideoEntity::cleanAndClear()
{
  if (cleanupMethod)
    {
      cleanupMethod();
      cleanupMethod = nullptr;
    }
}

/** Check if the config is acceptable to create the object.*/
bool VideoEntity::is_cfg_viable(const Json::Value* jval)
{
    if (!jval->isObject()) return false;
    auto jp = jval->find(ZMB::SURV_CAM_KW_ENTITY.begin(), ZMB::SURV_CAM_KW_ENTITY.end());
    if (nullptr == jp)
        jp = jval;
    ZMBCommon::ZConstString val(ZMB::SURV_CAM_PARAMS_PATH, jp);
    if (val.empty()) return false;
    return true;
}

}//ZMBEntities
