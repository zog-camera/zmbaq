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
  Mp4WriterTask* file_pkt_q;

  AVPacket pkt;

  bool should_run;


  StreamReader(const std::string& name = "StreamReader0");
  virtual ~StreamReader();

  bool open(ZConstString url);
  void stop();

  //read one frame, write it syncronously
  void rwSync();

  //read one frame, write it in another thread
  void rwAsync();
};

}

#endif // STREAMREADER_H
