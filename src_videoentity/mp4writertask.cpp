
#include "mp4writertask.h"

#include <Poco/Thread.h>
#include <Poco/DateTime.h>
#include "src/ffilewriter.h"
#include "src/avcpp/packet.h"

namespace ZMBEntities {

Mp4WriterTask::Mp4WriterTask(const std::string& name)
    : Poco::Task(name)
{
    does_detection = false;
    av_ctx = 0;
    file_writer = 0;
    file_chunk_size = default_chunk_size;

    fn_spawn_writer = []() {return (ZMB::FFileWriterBase*)(new ZMB::FFileWriter);};
}

bool Mp4WriterTask::open (const AVFormatContext *av_in_fmt_ctx,
                          ZConstString cam_name,
                          std::shared_ptr<ZMFS::FSHelper> fsh,
                          bool movement_detection,
                          size_t max_file_size)
{
    does_detection = movement_detection;
    file_chunk_size = max_file_size;
    av_ctx = av_in_fmt_ctx;
    fs_helper = fsh;
    camera_name = cam_name;
    should_run = true;

    return true;
}



Mp4WriterTask::~Mp4WriterTask()
{
}

void Mp4WriterTask::stop()
{
    should_run = false;
}

void Mp4WriterTask::runTask()
{
    bool ok = false;

    char _fname_buf[256];
    char _absname_buf[4096];
    ZUnsafeBuf name(_fname_buf, 256u);
    ZUnsafeBuf abs_name(_absname_buf, 4096u);
    char tmp[16];

    auto fn_gen_name = [&]() {
        //restore initial values:
        abs_name.len = 4096u;
        abs_name.str = _absname_buf;
        name.len = 256u;
        name.fill(0);

        //format the filename:
        char* end = 0;
        name.read(&end, camera_name.get_const_str(), end);
        Poco::DateTime dt; //<current time on construction
        memset(tmp, 0x00, sizeof(tmp));
        snprintf(tmp, 13,"_%02d.%02d.%04d_", dt.day(), dt.month(), dt.year());
        name.read(&end, ZConstString(tmp,12), end);
        snprintf(tmp, 18, "_%02d:%02d:%02d.%03d.flv",
                dt.hour(), dt.minute(),
                dt.second(), dt.millisecond());
        name.read(&end, ZConstString(tmp,17), end);
        //"finalize" the string
        name.len = end - name.begin();
    };
    auto fn_make_file = [&] ()
    {
        abs_name.len = 4096u;
        abs_name.str = _absname_buf;
        if (nullptr != file)
        {
            fs_helper->store_permanently(file.get());
            file_writer->close();
        }
        fn_gen_name();
        file = fs_helper->spawn_temp(name.to_const());
        abs_name.fill(0);
        file->fslocation->absolute_path(&abs_name, file->fname.to_const());

        //close old and create new writer context:
        file_writer.reset(fn_spawn_writer());
        file_writer->fs_item = file;
        file_writer->open(av_ctx, abs_name.to_const());
        std::cout << "Started writing file: " << abs_name.begin() << "\n";
        assert(nullptr != file);
    };

    ZMB::PacketsPocket::seq_key_t time_tracking;
    auto fn_push = [](std::shared_ptr<ZMB::FFileWriterBase>& ff,
            SHP(av::Packet)& _item )
    {
        /** TODO: debug this. */
        ff->pocket->push(_item, true);
        ff->pb_file_size += _item->getSize();

//        ff->write(_item->getAVPacket());
    };

    auto fn_dump = [&]()
    {
        /** TODO: debug this2. */
        file_writer->pocket->dump_packets(time_tracking, (void*)file_writer.get());
    };

    SHP(av::Packet) item;
    while (should_run)
    {
        pkt_chan.pop(item, ok);
        if (!ok)
        {
            Poco::Thread::sleep(1000 / 40);
            continue;
        }
        if (nullptr == file)
        {
            fn_make_file();
        }

        AVPacket* pkt = item->getAVPacket();
        size_t sz = pkt->size;
        if (sz == 0 && pkt->buf != 0 )
        {
           sz = pkt->buf->size;
        }

        assert(item->getPts() > 0);
        assert(sz != 0);

        fn_push(file_writer, item);
        if (item->isKeyPacket())
            fn_dump();

        sz = file_writer->file_size();
        if (sz > default_chunk_size)
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
    if (nullptr != file_writer)
        fn_dump();

}

}//ZMBEntities
