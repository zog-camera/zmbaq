#ifndef VIDEOENTITY_H
#define VIDEOENTITY_H
#include "../src/zmbaq_common/zmbaq_common.h"
#include "../src/zmbaq_common/thread_pool.h"
#include "../src/survcamparams.h"
#include "../src/fshelper.h"
#include <Poco/TaskManager.h>


namespace ZMBEntities {


class VideoEntity
{
public:
  typedef std::shared_ptr<ZMBCommon::ThreadsPool> TPoolPtr;
  typedef std::pair<ZMB::SurvCamParams,Json::Value> ConfigPair_t;

  static bool is_cfg_viable(const Json::Value* jval);

  //must be called from visit() method of the visitor
  template<typename Visitor, typename ...TVarArgs>
  static void accept(VideoEntity* e, Visitor& v, TVarArgs...args)
  {
    e->cleanAndClear();
  }
  //use cleanupMethod() and set it nullptr
  void cleanAndClear();

  VideoEntity() = default;
  VideoEntity(const VideoEntity&) = default;

  ZMB::VideoEntityID entity_id;
  std::function<ConfigPair_t()> getConfigFunction;
  std::function<void()> cleanupMethod;
  TPoolPtr pool;
};

}//ZMBEntities


#endif // VIDEOENTITY_H
