
#include "mp4writertask.h"

#include <Poco/Thread.h>
#include <Poco/DateTime.h>
#include "../src/ffilewriter.h"
#include "external/avcpp/src/packet.h"

namespace ZMBEntities {

using namespace ZMBCommon;





struct FileWriterDeleter
{
  void operator()(ZMB::FFileWriter* p)
  {
    p->dumpPocketContent(p->prevPktTimestamp);
  }
};
//------------------------------------------------------------
struct _Mp4TaskPriv
{
  _Mp4TaskPriv() : av_ctx(nullptr) {
    defaultChunkSize.store(64 * 1024 * 1024);
  }

  ZMFS::FSHelper fs_helper;
  ZMFS::FSItem file;

  std::string camera_name;
  std::shared_ptr<ZMB::FFileWriterBase> fileWriterMp4;
  const AVFormatContext* av_ctx;

  std::atomic_uint defaultChunkSize;


  std::string makeFileName() const;

  void write(SHP(av::Packet) packet);
  void fn_make_file();

  std::unique_lock<std::mutex>&& getLocker() {return std::move(std::unique_lock<std::mutex>(mu));}
  std::mutex mu;
};
//------------------------------------------------------------
void _Mp4TaskPriv::fn_make_file()
{
  std::string abs_name;
  auto tasklk = getLocker(); (void)tasklk;

  std::string name (makeFileName());
  file = fs_helper.spawn_temp(ZMBCommon::bindZCTo(name));
  assert(!file.fname.empty());
  file.fslocation.absolute_path(abs_name, ZMBCommon::bindZCTo(file.fname));


  //close old and create new writer context:

  auto pfile = new ZMB::FFileWriter;
  auto lk = pfile->locker(); (void)lk;
  fileWriterMp4 = std::shared_ptr<ZMB::FFileWriter>(pfile, FileWriterDeleter());
  fileWriterMp4->open(av_ctx, ZMBCommon::bindZCTo(abs_name));
  std::cout << "Started writing file: " << abs_name << "\n";
}


void _Mp4TaskPriv::write(std::shared_ptr<av::Packet> packet)
{

  if (nullptr == packet)
    {
      return;
    }

  if (nullptr == fileWriterMp4) { fn_make_file(); }

  SHP(av::Packet) item = packet;
  size_t sz = 0;

  {//push the packet into the cache
    auto lk = fileWriterMp4->locker(); (void)lk;
    AVPacket pkt = item->makeRef();
    size_t sz = pkt.size;
    sz += (0 == sz)? pkt.buf->size: 0;

    fileWriterMp4->prevPktTimestamp = fileWriterMp4->pocket.push(packet);
    fileWriterMp4->pb_file_size.fetch_add(item->size());

    if (item->isKeyPacket())
    {//dump on key frames
      fileWriterMp4->dumpPocketContent(fileWriterMp4->prevPktTimestamp);
    }

    sz += fileWriterMp4->file_size();
  }

  if (sz > defaultChunkSize)
    {//create multiple chunks instead onf 1 big file:
      auto lk = fileWriterMp4->locker();
        fs_helper.store_permanently(&file);
      lk.unlock();

      //create new file:
      fn_make_file();
    }
  item.reset();
}



std::string _Mp4TaskPriv::makeFileName() const
{
  //restore initial values:
  std::string name;
  name.reserve(256);
  name += camera_name;

  std::array<char,48> tmp;
  //format the filename:
  Poco::DateTime dt; //<current time on construction
  tmp.fill(0x00);
  snprintf(tmp.data(), 13,"_%02d.%02d.%04d_", dt.day(), dt.month(), dt.year());
  name += tmp.data();
  tmp.fill(0x00);
  snprintf(tmp.data(), 18, "_%02d:%02d:%02d.%03d.flv",
           dt.hour(), dt.minute(),
           dt.second(), dt.millisecond());
  name += tmp.data();
  return name;
}
//============================================================
Mp4WriterTask::Mp4WriterTask()
{
}

bool Mp4WriterTask::open (const AVFormatContext *av_in_fmt_ctx,
                          ZConstString cam_name,
                          ZMFS::FSHelper fsh)
{
  task = std::make_shared<_Mp4TaskPriv>();
  task->av_ctx = av_in_fmt_ctx;
  task->fs_helper = fsh;
  task->camera_name = std::move(std::string(cam_name.begin(), cam_name.size()));
  return true;
}

Mp4WriterTask::~Mp4WriterTask()
{
  close();
}

void Mp4WriterTask::close()
{
  if(nullptr != pool)
    {
      pool->close();
      pool->terminateDetach();
    }
}

void Mp4WriterTask::writeSync(SHP(av::Packet) pkt)
{
  task->write(pkt);
}

void Mp4WriterTask::writeAsync(SHP(av::Packet) pkt)
{
  if (nullptr == pool)
    {
      pool = std::make_shared<ZMBCommon::ThreadsPool>(1);
    }
  ZMBCommon::CallableDoubleFunc dfunc;
  ::snprintf(dfunc.tag.data(), dfunc.tag.size(), "writeAsync()");
  dfunc.functor = [this,pkt](){ writeSync(pkt); };
  pool->submit(dfunc);
}

void Mp4WriterTask::imbue(std::shared_ptr<ZMBCommon::ThreadsPool> neuPool)
{
  if(nullptr != pool)
    {
      pool->close();
      pool->joinAll();
    }
  pool = neuPool;
}

}//ZMBEntities
