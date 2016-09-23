#ifndef MP4WRITER_H
#define MP4WRITER_H

#include <functional>
#include <memory>
#include <Poco/Task.h>
#include "../src/fshelper.h"
#include "../src/avcpp/packet.h"

#include "../src/ffilewriter.h"
#include "../src/zmbaq_common/thread_pool.h"



namespace ZMBEntities {

using namespace ZMBCommon;

struct _Mp4TaskPriv
{
  _Mp4TaskPriv() : av_ctx(nullptr) {
    defaultChunkSize.store(64 * 1024 * 1024);
  }

  ZMFS::FSHelper fs_helper;
  ZMFS::FSItem file;

  std::string camera_name;
  std::shared_ptr<ZMB::FFileWriterBase> file_writer;
  const AVFormatContext* av_ctx;
  SHP(av::Packet) packet;

  std::atomic_uint defaultChunkSize;

  std::mutex mu;

  std::string makeFileName() const;

  void write();
  ZMB::FFileWriter* fn_spawn_writer();
  void fn_make_file();
};

class Mp4WriterTask : public ZMB::noncopyable
{

public:
    static const size_t MiB = 1024 * 1024;
    static const size_t default_chunk_size = 64 * MiB;

    std::string tag;
    std::function<ZMB::FFileWriterBase*()> fn_spawn_writer;

    Mp4WriterTask();
    virtual ~Mp4WriterTask();

    bool open (const AVFormatContext *av_in_fmt_ctx,
               ZConstString cam_name,
               ZMFS::FSHelper fsh = ZMFS::FSHelper());

    void close();

    void writeSync(SHP(av::Packet) pkt);
    void writeAsync(SHP(av::Packet) pkt);

private:
    std::shared_ptr<ZMBCommon::ThreadsPool> pool;
    std::shared_ptr<_Mp4TaskPriv> task;

};


}//ZMBEntities



#endif // MP4WRITER_H
