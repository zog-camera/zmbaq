#ifndef VIDEOENTITY_PV_H
#define VIDEOENTITY_PV_H

#include "zmbaq_common.h"
#include "src/survcamparams.h"
#include "zmbaq_common/thread_pool.h"
#include "streamreader.h"
#include "mp4writertask.h"
#include "src/fshelper.h"

namespace ZMBEntities {

class VEPV : public ZMB::noncopyable
{
public:
    explicit VEPV();
    virtual ~VEPV();

    void clear();

    bool configure(const Json::Value* jobject, bool do_start,
                   SHP(ZMBCommon::ThreadsPool) p_pool = std::make_shared<ZMBCommon::ThreadsPool>());

    //----------------------------
    ZMB::SurvCamParams cam_param;
    SHP(ZMFS::FSHelper) fs_helper;

    ZMBEntities::Mp4WriterTask* file_dump;
    ZMBEntities::StreamReader* stream;

    SHP(ZMBCommon::ThreadsPool) pool;

};
}//ZMBEntities

#endif // VIDEOENTITY_PV_H
