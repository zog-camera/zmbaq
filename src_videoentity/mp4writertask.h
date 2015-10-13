#ifndef MP4WRITER_H
#define MP4WRITER_H

#include <functional>
#include <memory>
#include <Poco/Task.h>
#include "src/fshelper.h"
#include "src/avcpp/packet.h"
#include "src/zmbaq_common/chan.h"

#include "src/ffilewriter.h"



namespace ZMBEntities {

class Mp4WriterTask : public Poco::Task
{

public:
    static const size_t MiB = 1024 * 1024;
    static const size_t default_chunk_size = 64 * MiB;

    std::function<ZMB::FFileWriterBase*()> fn_spawn_writer;

    SHP(ZMFS::FSHelper) fs_helper;
    SHP(ZMFS::FSItem) file;

    Chan< SHP(av::Packet), 2048 > pkt_chan;
    ZMBCommon::MByteArray camera_name;
    bool should_run;//< set to "false" to stop the loop.


    Mp4WriterTask(const std::string& name);
    virtual ~Mp4WriterTask();

    bool open (const AVFormatContext *av_in_fmt_ctx,
               ZConstString cam_name,
               std::shared_ptr<ZMFS::FSHelper> fsh,
               bool movement_detection = true,
               size_t max_file_size = default_chunk_size);

    void stop();

    virtual void runTask();

private:
    bool does_detection;
    size_t file_chunk_size;
    const AVFormatContext* av_ctx;
    std::shared_ptr<ZMB::FFileWriterBase> file_writer;

};


}//ZMBEntities



#endif // MP4WRITER_H
