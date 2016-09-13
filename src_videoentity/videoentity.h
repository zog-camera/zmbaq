#ifndef VIDEOENTITY_H
#define VIDEOENTITY_H
#include "zmbaq_common.h"
#include "zmbaq_common/thread_pool.h"
#include "src/survcamparams.h"
#include "src/fshelper.h"
#include <Poco/TaskManager.h>


namespace ZMBEntities {


class VideoEntity
{
public:
  typedef std::shared_ptr<ZMBCommon::ThreadsPool> TPoolPtr;
  typedef std::pair<ZMB::SurvCamParams,Json::Value> ConfigPair_t;

  static bool is_cfg_viable(const Json::Value* jval);

  template<typename This, typename Visitor, typename ...TVarArgs>
  static void accept(This t, Visitor v, TVarArgs...args)
  {
    if (t->cleanupMethod)
      {
        t->cleanupMethod();
        t->cleanupMethod = nullptr;
      }
    v->visit(t,args...);
  }

  ZMB::VideoEntityID entity_id;
  std::function<ConfigPair_t()> getConfigFunction;
  std::function<void()> cleanupMethod;
  TPoolPtr pool;
};

}//ZMBEntities


#endif // VIDEOENTITY_H
