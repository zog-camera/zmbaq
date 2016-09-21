
#include "mp4writertask.h"

#include <Poco/Thread.h>
#include <Poco/DateTime.h>
#include "../src/ffilewriter.h"
#include "../src/avcpp/packet.h"

namespace ZMBEntities {

using namespace ZMBCommon;

Mp4WriterTask::Mp4WriterTask()
{
    fn_spawn_writer = []()
    {return (ZMB::FFileWriterBase*)(new ZMB::FFileWriter);};
}

bool Mp4WriterTask::open (const AVFormatContext *av_in_fmt_ctx,
                          ZConstString cam_name,
                          std::shared_ptr<ZMFS::FSHelper> fsh)
{
  task.av_ctx = av_in_fmt_ctx;
  task.fs_helper = fsh;
  task.camera_name = std::move(std::string(cam_name.begin(), cam_name.size()));
  task.fn_spawn_writer = fn_spawn_writer;
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
  task.write();
}

void Mp4WriterTask::writeAsync(SHP(av::Packet) pkt)
{
  if (nullptr == pool)
    {
      pool = std::make_shared<ZMBCommon::ThreadsPool>(1);
    }
  assert(false);//DEBUG and fix it!
  task.packet = pkt;
  pool->submit([=](){ task.write(); });
}

void _Mp4TaskPriv::write()
{
  std::lock_guard<std::mutex>lk(mu); (void)lk;

  if (nullptr == packet)
    {
      return;
    }
  bool ok = false;

  std::array<char, 1024> _fname_buf;
  ZUnsafeBuf name(_fname_buf.data(), _fname_buf.size());
  std::string abs_name;
  std::array<char,48> tmp;

  ZMB::PacketsPocket::seq_key_t time_tracking;

  auto fn_gen_name = [&]() {
      //restore initial values:
      name.len = 256u;
      name.fill(0);

      //format the filename:
      char* end = 0;
      name.read(&end, ZMBCommon::bindZCTo(camera_name), end);
      Poco::DateTime dt; //<current time on construction
      tmp.fill(0x00);
      snprintf(tmp.data(), 13,"_%02d.%02d.%04d_", dt.day(), dt.month(), dt.year());
      name.read(&end, ZConstString(tmp.data(),12), end);
      snprintf(tmp.data(), 18, "_%02d:%02d:%02d.%03d.flv",
               dt.hour(), dt.minute(),
               dt.second(), dt.millisecond());
      name.read(&end, ZConstString(tmp.data(),17), end);
      //"finalize" the string
      name.len = end - name.begin();
    };

  auto fn_dump = [&]()
  {
      /** TODO: debug this2. */
      auto lk = file_writer->locker();
      file_writer->pocket->dump_packets(time_tracking, (void*)file_writer.get());
  };

  auto fn_make_file = [&] ()
  {
      if (nullptr != file)
        {
          fs_helper->store_permanently(file.get());
          auto lk = file_writer->locker();
          file_writer->close();
        }
      fn_gen_name();
      file = fs_helper->spawn_temp(name.to_const());
      file->fslocation->absolute_path(abs_name, ZMBCommon::bindZCTo(file->fname));


      //close old and create new writer context:
      file_writer.reset(fn_spawn_writer());
      auto lk = file_writer->locker();
      file_writer->fs_item = file;
      file_writer->open(av_ctx, ZMBCommon::bindZCTo(abs_name));
      std::cout << "Started writing file: " << abs_name << "\n";

      file->on_destruction = [&]() { fn_dump();};

      assert(nullptr != file);
    };

  auto fn_push = [](ZMB::FFileWriterBase* ff, SHP(av::Packet)& _item )
  {
    /** TODO: debug this. */
    auto lk = ff->locker();
    ff->pocket->push(_item, true);
    ff->pb_file_size.fetch_add(_item->getSize());

    //        ff->write(_item->getAVPacket());
  };


  SHP(av::Packet)& item (this->packet);
  if (nullptr == file) { fn_make_file(); }

  AVPacket* pkt = item->getAVPacket();
  size_t sz = pkt->size;
  if (sz == 0 && pkt->buf != 0 )
    {
      sz = pkt->buf->size;
    }

  assert(item->getPts() > 0);
  assert(sz != 0);

  fn_push(file_writer.get(), item);
  if (item->isKeyPacket())
    fn_dump();

  sz = file_writer->file_size();
  if (sz > defaultChunkSize)
    {
      std::cout << "Finished writing file: " << name.begin() << "\n";
      fn_dump();

      fs_helper->store_permanently(file_writer->fs_item.get());
      time_tracking = ZMB::PacketsPocket::seq_key_t();
      //create new file:
      fn_make_file();
    }
  item.reset();
  ok = false;
}


}//ZMBEntities
