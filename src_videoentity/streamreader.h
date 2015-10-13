#ifndef STREAMREADER_H
#define STREAMREADER_H
#include "zmbaq_common.h"
#include <Poco/Task.h>
#include <Poco/TaskManager.h>
#include "src/av_things.h"
#include <atomic>

#include "mp4writertask.h"


namespace ZMBEntities {

class StreamReader : public Poco::Task
{
public:
    av_things ff;

    /** Must be set before the task will go running.*/
    Mp4WriterTask* file_pkt_q;

    AVPacket pkt;

    bool should_run;


    StreamReader(const std::string& name);
    virtual ~StreamReader();

    bool open(ZConstString url);
    void stop();
    virtual void runTask();
};

}

#endif // STREAMREADER_H
