#ifndef VIDEOENTITY_PV_H
#define VIDEOENTITY_PV_H
#include "json/json.h"

namespace ZMBEntities {

class VEPV;
class VideoEntity;

/** Make a a stream reader + mp4 file writer for the given video source.
 * Will change VideoEnity::cleanupMethod() that will destruct resources.*/
class VEMp4WritingVisitor
{
public:

  template<class T>
  void visit(T entity, const Json::Value* jobject, bool separateThreadPool)
  {
    entity::accept<ZMBEntities::VideoEntity*, const Json::Value*, bool>(entity, this, jobject, separateThreadPool);
  }

  /** @param separateThreadPool: when TRUE the visitor will not submit tasks to VideoEntity::pool and will allocate it's own;
   * on FALSE it will use VideoEntity::pool */
  void visit(ZMBEntities::VideoEntity* entity, const Json::Value* jobject, bool separateThreadPool = true);

  std::shared_ptr<VEPV> data;
};

class VECleanupVisitor
{
public:
  template<class T>
  void visit(T entity)
  {
    entity::accept<ZMBEntities::VideoEntity*>(entity, this);
  }

  void visit(ZMBEntities::VideoEntity* entity);
};

}//ZMBEntities

#endif // VIDEOENTITY_PV_H
