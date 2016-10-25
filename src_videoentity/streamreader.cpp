#include "streamreader.h"

namespace ZMBEntities {

//--------------------------------------------------------------
bool SRPV::open(const std::string& Uri)
{
  uri = Uri;
  ictx.openInput(uri, ec);
  if (ec)
    {
      std::cerr << "Can't open input\n";
      return false;
    }

  std::cerr << "Streams: " << ictx.streamsCount() << std::endl;

  ictx.findStreamInfo(ec);
  if (ec)
    {
      std::cerr << "Can't find streams: " << ec << ", " << ec.message() << std::endl;
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

  std::cerr << videoStream << std::endl;

  if (vst.isNull())
    {
      std::cerr << "Video stream not found\n";
      return false;
    }

  if (vst.isValid())
    {
      vdec = av::VideoDecoderContext(vst);


      av::Codec codec = av::findDecodingCodec(vdec.raw()->codec_id);

      vdec.setCodec(codec);
      vdec.setRefCountedFrames(true);

      vdec.open({{"threads", "1"}}, av::Codec(), ec);
      //vdec.open(ec);
      if (ec) {
          std::cerr << "Can't open codec\n";
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
//      clog << "Packet reading error: " << ec << ", " << ec.message() << std::endl;
      return ec;
    }

  if (pkt.streamIndex() != videoStream)
    {
      if (nullptr != onOtherPacket)
        {
          onOtherPacket(pkt, shared_from_this());
        }
      return ec;
    }

  auto ts = pkt.ts();
//  clog << "Read packet: " << ts << " / " << ts.seconds() << " / " << pkt.timeBase() << " / st: " << pkt.streamIndex() << std::endl;
  if (nullptr != onVideoPacket)
    {
      onVideoPacket(pkt, shared_from_this());
    }

  if(!doesDecodePackets)
    {
      return ec;
    }

  av::VideoFrame frame = vdec.decode(pkt, ec);

  count++;
  if (ec)
    {
      std::cerr << "Error: " << ec << ", " << ec.message() << std::endl;
      return ec;
    } else if (!frame)
    {
      std::cerr << "Empty frame\n";
      //continue;
    }

  ts = frame.pts();
//  clog << "  Frame: " << frame.width() << "x" << frame.height() << ", size=" << frame.size() << ", ts=" << ts << ", tm: " << ts.seconds() << ", tb: " << frame.timeBase() << ", ref=" << frame.isReferenced() << ":" << frame.refCount() << std::endl;
  if (nullptr != onDecoded)
    {
      onDecoded(frame, shared_from_this());
    }

  return ec;
}
//--------------------------------------------------------------

StreamReader::StreamReader(const std::string& name)
{
  pv = std::make_shared<SRPV>();
  pv->objTag = name;
}

bool StreamReader::open(const std::string& url, bool with_decoding)
{
  pv->doesDecodePackets = with_decoding;
  bool res = pv->open(url);
  if (!res)
    {
      pv.reset();
    }
  if (nullptr == pv->pool)
    {
      imbue(std::make_shared<ZMBCommon::ThreadsPool>(1));
    }
  ZMBCommon::CallableDoubleFunc readTask;

  readTask.functor = [this, &readTask]()
  {
      pv->readPacket();
      ///TODO: fix this:--------------------------------
      pv->pool->submit(readTask);
  };
  pv->pool->submit(readTask);

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
  if (nullptr != pv->pool)
    {
      pv->pool->close();
      pv->pool->joinAll();
    }
  pv->pool = neuPool;
}

}//ZMBEntities
