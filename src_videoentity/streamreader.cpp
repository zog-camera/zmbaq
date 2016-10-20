#include "streamreader.h"
#include "../src/avcpp/packet.h"

namespace ZMBEntities {

class SRPV
{
public:

  Mp4WriterTask file_pkt_q;

};

bool SRPV::open(const std::string& Uri)
{
  uri = Uri;
  ictx.openInput(uri, ec);
  if (ec)
    {
      cerr << "Can't open input\n";
      return false;
    }

  cerr << "Streams: " << ictx.streamsCount() << endl;

  ictx.findStreamInfo(ec);
  if (ec)
    {
      cerr << "Can't find streams: " << ec << ", " << ec.message() << endl;
      return false;
    }

  for (size_t i = 0; i < ictx.streamsCount(); ++i) {
      auto st = ictx.stream(i);
      if (st.mediaType() == AVMEDIA_TYPE_VIDEO) {
          videoStream = i;
          vst = st;
          break;
        }
    }

  cerr << videoStream << endl;

  if (vst.isNull())
    {
      cerr << "Video stream not found\n";
      return false;
    }

  if (vst.isValid())
    {
      vdec = VideoDecoderContext(vst);


      Codec codec = findDecodingCodec(vdec.raw()->codec_id);

      vdec.setCodec(codec);
      vdec.setRefCountedFrames(true);

      vdec.open({{"threads", "1"}}, Codec(), ec);
      //vdec.open(ec);
      if (ec) {
          cerr << "Can't open codec\n";
          return false;
        }
    }
  return true;
}

std::error_code SRPV::readPacket()
{
  av::Packet pkt = ictx.readPacket(ec);
  if (ec)
    {
      clog << "Packet reading error: " << ec << ", " << ec.message() << endl;
      return ec;
    }

  if (pkt.streamIndex() != videoStream)
    {
      //return for
      return ec;
    }

  auto ts = pkt.ts();
  clog << "Read packet: " << ts << " / " << ts.seconds() << " / " << pkt.timeBase() << " / st: " << pkt.streamIndex() << endl;

  if(!doesDecodePackets)
    {
      return ec;
    }

  av::VideoFrame frame = vdec.decode(pkt, ec);

  count++;
  if (ec)
    {
      cerr << "Error: " << ec << ", " << ec.message() << endl;
      return ec;
    } else if (!frame)
    {
      cerr << "Empty frame\n";
      //continue;
    }

  ts = frame.pts();
  clog << "  Frame: " << frame.width() << "x" << frame.height() << ", size=" << frame.size() << ", ts=" << ts << ", tm: " << ts.seconds() << ", tb: " << frame.timeBase() << ", ref=" << frame.isReferenced() << ":" << frame.refCount() << endl;
  if (nullptr != )

  return ec;
}

StreamReader::StreamReader(const std::string& name)
{
  pv = std::make_shared<SRPV>();
  pv->objTag = name;
}

bool StreamReader::open(const std::string& url, bool with_decoding)
{
  bool res = pv->open(url);
  if (!res)
    {
      pv.reset();
    }
  if (nullptr == pv->pool)
    {
      pv->pool = std::make_shared<ZMBCommon::ThreadsPool>(1);
    }
  ZMBCommon::CallableDoubleFunc readTask;
  readTask.functor = [=]()
  {



      ZMBCommon::CallableDoubleFunc writeTask;
      writeTask.functor = [=]()
      {
          pv->file_pkt_q.writeAsync(avpkt);
      };
      pool->submit(writeTask);
    };
  pool->submit(readTask);

  return res;
}

StreamReader::~StreamReader()
{
}

void StreamReader::setOnVideoPacket(OnPacketAction ftor)
{
  pv->onVideoPacket = ftor;
}

void StreamReader::setOnOtherPacket(OnPacketAction ftor)
{
  pv->onOtherPacket = ftor;
}

void StreamReader::setOnVideoFrame(OnFrameAction ftor)
{
  pv->onDecoded = ftor;
}

void StreamReader::stop()
{
}



void StreamReader::imbue(std::shared_ptr<ZMBCommon::ThreadsPool> neuPool)
{
  if (nullptr != pool)
    {
      pool->close();
      pool->joinAll();
    }
  pool = neuPool;
  pv->file_pkt_q.imbue(pool);
}

}//ZMBEntities
