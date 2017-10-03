#ifndef STREAMREADER_H
#define STREAMREADER_H
#include <iostream>
#include <set>
#include <map>
#include <memory>
#include <functional>

#include "../external/avcpp/src/av.h"
#include "../external/avcpp/src/ffmpeg.h"
#include "../external/avcpp/src/codec.h"
#include "../external/avcpp/src/packet.h"
#include "../external/avcpp/src/videorescaler.h"
#include "../external/avcpp/src/audioresampler.h"
#include "../external/avcpp/src/avutils.h"

// API2
#include "../external/avcpp/src/format.h"
#include "../external/avcpp/src/formatcontext.h"
#include "../external/avcpp/src/codec.h"
#include "../external/avcpp/src/codeccontext.h"

#include "../src/zmbaq_common/zmbaq_common.h"
#include "../src/zmbaq_common/thread_pool.h"
#include <atomic>
#include <iostream>


namespace ZMBEntities {
//--------------------------------------------------------------
class SRPV;

typedef std::shared_ptr<SRPV> SRPVPtr;
typedef std::function<void(av::Packet& pkt, SRPVPtr rdCtx)> OnPacketAction;
typedef std::function<void(av::VideoFrame& pkt, SRPVPtr rdCtx)> OnFrameAction;
//--------------------------------------------------------------

class SRPV : public std::enable_shared_from_this<SRPV>
{

public:
  SRPV()
  {
    videoStream = -1;
    doesDecodePackets = true;
    count = 0;
  }
  ~SRPV()
  {
    if (nullptr != pool)
      {
        pool->close();
        pool->terminateDetach();
      }
  }

  bool open(const std::string& Uri);
  std::error_code readPacket();

  std::string objTag;
  std::string uri;

  ssize_t videoStream;
  av::VideoDecoderContext vdec;
  av::Stream      vst;
  std::error_code   ec;
  av::FormatContext ictx;
  bool doesDecodePackets;
  size_t count;

  /** By default has 1 thread if imbue(neuPool) was not called.*/
  std::shared_ptr<ZMBCommon::ThreadsPool> pool;


  //should be set externally, triggered from a Callable task in a thread
  OnPacketAction onVideoPacket;
  OnPacketAction onOtherPacket;
  OnFrameAction onDecoded;
};
//--------------------------------------------------------------


class StreamReader
{
public:
  bool should_run;

  StreamReader(const std::string& name = "StreamReader0");
  virtual ~StreamReader();

  /** take the provided pool for the tasks and delete the internal one.
   * Must be invoked before open(). */
  void imbue(std::shared_ptr<ZMBCommon::ThreadsPool> neuPool);

  /** @return may be NULL; */
  std::shared_ptr<ZMBCommon::ThreadsPool> getPool() const {
    return pv->pool;
  }

  /** Open the stream and start reading in the thread.
   * onPacketAction() called iteratively. */
  bool open(const std::string& url, bool with_decoding = true);
  void stop();

  //should be set externally, triggered from a Callable task in a thread
  void setOnVideoPacket(OnPacketAction);
  void setOnOtherPacket(OnPacketAction);
  void setOnVideoFrame(OnFrameAction);

  std::shared_ptr<SRPV> pv;
};

}

#endif // STREAMREADER_H
