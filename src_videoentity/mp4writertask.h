#ifndef MP4WRITER_H
#define MP4WRITER_H

#include <functional>
#include <memory>
#include <Poco/Task.h>
#include "../src/fshelper.h"
#include "external/avcpp/src/packet.h"

#include "../src/ffilewriter.h"
#include "../src/zmbaq_common/thread_pool.h"



namespace ZMBEntities {

using namespace ZMBCommon;


struct _Mp4TaskPriv;

class Mp4WriterTask : public ZMB::noncopyable
{

public:
    static const size_t MiB = 1024 * 1024;
    static const size_t default_chunk_size = 64 * MiB;

    std::string tag;

    Mp4WriterTask();
    virtual ~Mp4WriterTask();

    bool open (const AVFormatContext *av_in_fmt_ctx,
               ZConstString cam_name,
               ZMFS::FSHelper fsh = ZMFS::FSHelper());

    void close();

    void writeSync(SHP(av::Packet) pkt);
    void writeAsync(SHP(av::Packet) pkt);

    /** take the provided pool for the tasks
    and delete the internal one*/
    void imbue(std::shared_ptr<ZMBCommon::ThreadsPool> neuPool);


private:
    std::shared_ptr<ZMBCommon::ThreadsPool> pool;
    std::shared_ptr<_Mp4TaskPriv> task;

};


}//ZMBEntities



#endif // MP4WRITER_H
