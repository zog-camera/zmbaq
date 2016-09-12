#ifndef VIDEOENTITY_H
#define VIDEOENTITY_H
#include "zmbaq_common.h"
#include "zmbaq_common/thread_pool.h"
#include "src/survcamparams.h"
#include "src/fshelper.h"
#include <Poco/TaskManager.h>


namespace ZMBEntities {

class VEPV; //< private variables available via videoentity_pv.h

class Mp4WriterTask;
class StreamReader;

class VideoEntity
{
public:
    static bool is_cfg_viable(const Json::Value* jval);

    explicit VideoEntity(SHP(ZMBCommon::ThreadsPool) p_pool = std::make_shared<ZMBCommon::ThreadsPool>(2));
    virtual ~VideoEntity();
    bool configure(const Json::Value* jobject, bool do_start = false);
    bool start();
    void stop();
    bool reopen(bool do_start = false);//Re-open with previosly used config settings.

    const ZMB::SurvCamParams& params() const;

    //---------------------------------------------------------------
    void set_id(ZMB::VideoEntityID number);
    ZMB::VideoEntityID id() const;
    //---------------------------------------------------------------
protected:
    SHP(ZMBCommon::ThreadsPool) pool;
    SHP(VEPV) pv;
};

}//ZMBEntities


#endif // VIDEOENTITY_H
