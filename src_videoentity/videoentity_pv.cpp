#include "survcamparams.h"

#include "zmbaq_common/zmbaq_common.h"
#include "zmbaq_common/thread_pool.h"
#include "streamreader.h"
#include "mp4writertask.h"
#include "fshelper.h"


#include "videoentity_pv.h"
#include "videoentity.h"

namespace ZMBEntities {

class VEPV : public ZMB::noncopyable
{
public:
    explicit VEPV();
    virtual ~VEPV();

    void clear();

    bool configure(const Json::Value* jobject,
                   SHP(ZMBCommon::ThreadsPool) p_pool = std::make_shared<ZMBCommon::ThreadsPool>(2));

    //----------------------------
    ZMB::VideoEntityID e_id;
    ZMB::SurvCamParams cam_param;
    Json::Value config;

    SHP(ZMFS::FSHelper) fs_helper;

    ZMBEntities::Mp4WriterTask* file_dump;
    ZMBEntities::StreamReader* stream;

    SHP(ZMBCommon::ThreadsPool) pool;
};


VEPV::VEPV()
{
    file_dump = 0;
    stream = 0;
}

VEPV::~VEPV()
{
    clear();
}

void VEPV::clear()
{
    if (0 == stream)
        return;

    stream->stop();
    file_dump->close();
    file_dump = 0;
    stream = 0;
}

bool VEPV::configure(const Json::Value* jobject, std::shared_ptr<ZMBCommon::ThreadsPool> p_pool)
{
  config = *jobject;
  pool = nullptr == p_pool? std::make_shared<ZMBCommon::ThreadsPool>(2) : p_pool;

  ZConstString name(ZMB::SURV_CAM_PARAMS_NAME, jobject);
  ZConstString path(ZMB::SURV_CAM_PARAMS_PATH, jobject);
  if (name.empty() || path.empty())
    {
      std::cerr << "VideoEntity failed to parse JSON object\n";
      return false;
    }

  fs_helper = std::make_shared<ZMFS::FSHelper>();
  ZConstString fs(ZMB::SURV_CAM_PARAMS_FS, jobject);
  ZConstString tmpfs(ZMB::SURV_CAM_PARAMS_TMPFS, jobject);
  fs_helper->set_dirs(fs, tmpfs);

  cam_param.settings = *jobject;

  std::string _s (path.begin(), path.size());
  auto _stream = new ZMBEntities::StreamReader(_s);
  bool ok = _stream->open(path);
  if (ok)
    {
      _s = name.begin();
      auto file_dump = new ZMBEntities::Mp4WriterTask();
      file_dump->tag = _s;
      file_dump->open(_stream->ff.get_format_ctx(), name, fs_helper);
      _stream->file_pkt_q = file_dump;
      file_dump = file_dump;
    }
  else
    {
      delete _stream;
      _stream = 0;
    }
  stream = _stream;
  return ok;

}
//------------------------------------------------------------------------
bool VEMp4WritingVisitor::visit(ZMBEntities::VideoEntity* entity, const Json::Value* jobject, bool separateThreadPool)
{
  VideoEntity::accept<VEMp4WritingVisitor> (entity, *this);

  data = std::make_shared<VEPV>();
  data->e_id = entity->entity_id;
  data->configure(jobject, separateThreadPool?
                    std::make_shared<ZMBCommon::ThreadsPool>(2) : entity->pool);

  entity->getConfigFunction = [=](){ return VideoEntity::ConfigPair_t(data->cam_param, data->config); };
  entity->cleanupMethod = [=](){ data->clear(); };
  return true;
}
//------------------------------------------------------------------------
bool VECleanupVisitor::visit(ZMBEntities::VideoEntity* entity)
{
  VideoEntity::accept<VECleanupVisitor> (entity, *this);

  //do nothing, it'll make sure previous setup was cleared
  return true;
}
//------------------------------------------------------------------------
}//ZMBEntities

