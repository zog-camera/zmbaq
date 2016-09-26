#include "streamreader.h"
#include "../src/avcpp/packet.h"

namespace ZMBEntities {

StreamReader::StreamReader(const std::string& name)
{
    should_run = true;
}

bool StreamReader::open(ZConstString url)
{
    int res = ff.open(url);
    if (0 != res)
      return false;
    res = ff.get_video_stream_and_codecs();
    if (0 != res)
      return false;
    ff.read_play();
    should_run = true;
    return true;
}

StreamReader::~StreamReader()
{
}

void StreamReader::stop()
{
    should_run = false;
}

bool StreamReader::rwSync()
{
  AVPacket pkt;
  av_init_packet(&pkt);

  int res = ff.read_frame(&pkt);
  if (res >= 0)
    {
      /*copy contents*/
      auto avpkt = std::make_shared<av::Packet> (&pkt);
      av::Rational tb(*ff.get_timebase());
      avpkt->setTimeBase(tb);
      assert(avpkt->getSize() > 0);
      file_pkt_q->writeSync(avpkt);
    }
  return res >= 0;
}

bool StreamReader::readSyncWriteAsync()
{
  AVPacket pkt;
  av_init_packet(&pkt);

  int res = ff.read_frame(&pkt);
  if (res >= 0)
    {
      /*copy contents*/
      auto avpkt = std::make_shared<av::Packet> ();
      avpkt->deep_copy(&pkt);
      av::Rational tb(*ff.get_timebase());
      avpkt->setTimeBase(tb);
      assert(avpkt->getSize() > 0);
      file_pkt_q->writeAsync(avpkt);
    }
  return res >= 0;
}

bool StreamReader::rwAsync()
{
  if (nullptr == pool)
    {
      pool = std::make_shared<ZMBCommon::ThreadsPool>(2);
      file_pkt_q->imbue(pool);
    }
  ZMBCommon::CallableDoubleFunc readTask;
  readTask.functor = [=]()
  {
      AVPacket pkt;
      ::memset(&pkt, 0x00, sizeof(AVPacket));
      av_init_packet(&pkt);
      int res = ff.read_frame(&pkt);
      if (res < 0)
        {
          return;
        }
      //dispatch the packet for writing:
      auto avpkt = std::make_shared<av::Packet> ();
      avpkt->deep_copy(&pkt);
      av::Rational tb(*ff.get_timebase());
      avpkt->setTimeBase(tb);
      assert(avpkt->getSize() > 0);

      ZMBCommon::CallableDoubleFunc writeTask;
      writeTask.functor = [=]()
      {
          file_pkt_q->writeAsync(avpkt);
      };
      pool->submit(writeTask);

    };
  pool->submit(readTask);
}

void StreamReader::imbue(std::shared_ptr<ZMBCommon::ThreadsPool> neuPool)
{
  if (nullptr != pool)
    {
      pool->close();
      pool->joinAll();
    }
  pool = neuPool;
  file_pkt_q->imbue(pool);
}

}//ZMBEntities
