
#include "mp4writertask.h"

#include <Poco/Thread.h>
#include <Poco/DateTime.h>
#include "../src/ffilewriter.h"
#include "../src/avcpp/packet.h"

namespace ZMBEntities {

using namespace ZMBCommon;

Mp4WriterTask::Mp4WriterTask()
{
}

ZMB::FFileWriter* _Mp4TaskPriv::fn_spawn_writer()
{
  return new ZMB::FFileWriter;
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
  task->write();
}

void Mp4WriterTask::writeAsync(SHP(av::Packet) pkt)
{
  if (nullptr == pool)
    {
      pool = std::make_shared<ZMBCommon::ThreadsPool>(1);
    }
  assert(false);//DEBUG and fix it!
  task->packet = pkt;
  pool->submit([=](){ task->write(); });
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

struct FileWriterDeleter
{
  void operator()(ZMB::FFileWriter* p)
  {
    p->dumpPocketContent(p->prevPktTimestamp);
  }
};

void _Mp4TaskPriv::fn_make_file()
{
  std::lock_guard<std::mutex>tasklk(mu); (void)tasklk;

  std::string name = std::move(makeFileName());
  std::string abs_name;
  file = std::move(fs_helper.spawn_temp(ZMBCommon::bindZCTo(name)));
  file.fslocation.absolute_path(abs_name, ZMBCommon::bindZCTo(file.fname));


  //close old and create new writer context:

  auto pfile = fn_spawn_writer();
  auto lk = pfile->locker(); (void)lk;
  file_writer = std::shared_ptr<ZMB::FFileWriter>(pfile, FileWriterDeleter());
  file_writer->open(av_ctx, ZMBCommon::bindZCTo(abs_name));
  std::cout << "Started writing file: " << abs_name << "\n";
}


void _Mp4TaskPriv::write()
{

  if (nullptr == packet)
    {
      return;
    }

  if (nullptr == file_writer) { fn_make_file(); }

  SHP(av::Packet)& item (this->packet);
  AVPacket* pkt = item->getAVPacket();
  size_t sz = pkt->size;
  sz += (0 == sz)? pkt->buf->size: 0;

  {//push the packet into the cache
    auto lk = file_writer->locker(); (void)lk;
    file_writer->prevPktTimestamp = file_writer->pocket.push(packet);
    file_writer->pb_file_size.fetch_add(item->getSize());

    if (item->isKeyPacket())
    {//dump on key frames
      file_writer->dumpPocketContent(file_writer->prevPktTimestamp);
    }

    sz += file_writer->file_size();
  }

  if (sz > defaultChunkSize)
    {//create multiple chunks instead onf 1 big file:
      auto lk = file_writer->locker();
        fs_helper.store_permanently(&file);
      lk.unlock();

      //create new file:
      fn_make_file();
    }
  item.reset();
}


}//ZMBEntities
