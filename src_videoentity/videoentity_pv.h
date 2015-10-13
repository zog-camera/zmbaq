#ifndef VIDEOENTITY_PV_H
#define VIDEOENTITY_PV_H

#include "zmbaq_common.h"
#include "src/survcamparams.h"
#include "streamreader.h"
#include "mp4writertask.h"
#include "src/fshelper.h"

namespace ZMBEntities {

class VEPV : public ZMB::noncopyable
{
public:
    VEPV(void* parent, SHP(Poco::ThreadPool) p_pool);
    virtual ~VEPV();

    void clear();
    void init();

    bool configure(const Json::Value* jobject, bool do_start);

    void* master;

    ZMB::SurvCamParams cam_param;
    SHP(ZMFS::FSHelper) fs_helper;


    ZMBEntities::Mp4WriterTask* file_dump;
    ZMBEntities::StreamReader* stream;

    Poco::TaskManager* task_mgr;
    SHP(Poco::ThreadPool) pool;

};
}//ZMBEntities

#endif // VIDEOENTITY_PV_H
