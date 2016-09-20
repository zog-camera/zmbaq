#include "streamreader.h"
#include "../src/avcpp/packet.h"

namespace ZMBEntities {

StreamReader::StreamReader(const std::string& name)
{
    memset(&pkt, 0, sizeof(pkt));
    av_init_packet(&pkt);
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
    av_packet_unref(&pkt);
    av_init_packet(&pkt);
}

void StreamReader::stop()
{
    should_run = false;
}

bool StreamReader::rwSync()
{
  int res = ff.read_frame(&pkt);
  av::Rational tb(*ff.get_timebase());
  if (res >= 0)
    {
      /*copy contents*/
      auto avpkt = std::make_shared<av::Packet> (&pkt);
      avpkt->setTimeBase(tb);
      assert(avpkt->getSize() > 0);
      file_pkt_q->writeSync(avpkt);
    }
  return res >= 0;
}

bool StreamReader::rwAsync()
{
  int res = ff.read_frame(&pkt);
  av::Rational tb(*ff.get_timebase());
  if (res >= 0)
    {
      /*copy contents*/
      auto avpkt = std::make_shared<av::Packet> ();
      avpkt->deep_copy(&pkt);
      avpkt->setTimeBase(tb);
      assert(avpkt->getSize() > 0);
      file_pkt_q->writeAsync(avpkt);
    }
  return res >= 0;
}

}//ZMBEntities
