#ifndef STREAMREADER_H
#define STREAMREADER_H
#include "../src/zmbaq_common/zmbaq_common.h"
#include <Poco/Task.h>
#include <Poco/TaskManager.h>
#include "../src/av_things.h"
#include <atomic>

#include "mp4writertask.h"


namespace ZMBEntities {

class StreamReader
{
public:
  ZMB::av_things ff;

  /** Must be set before the task will go running.*/
  std::shared_ptr<Mp4WriterTask> file_pkt_q;


  bool should_run;


  StreamReader(const std::string& name = "StreamReader0");
  virtual ~StreamReader();

  bool open(ZConstString url);
  void stop();

  //read one frame, write it syncronously
  bool rwSync();

  //read one frame, write it in another thread
  bool readSyncWriteAsync();

  //use tasks to read & write the frame
  bool rwAsync();

  /** take the provided pool for the tasks
    and delete the internal one*/
  void imbue(std::shared_ptr<ZMBCommon::ThreadsPool> neuPool);

  std::shared_ptr<ZMBCommon::ThreadsPool> pool;

};

}

#endif // STREAMREADER_H
