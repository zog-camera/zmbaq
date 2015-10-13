#include "videoentity_pv.h"

namespace ZMBEntities {

VEPV::VEPV(void* parent, SHP(Poco::ThreadPool) p_pool)
    : master(parent), pool(p_pool)
{
    task_mgr = 0;
    file_dump = 0;
    stream = 0;
}

VEPV::~VEPV()
{
    clear();
}

void VEPV::clear()
{
    if (0 == stream)
        return;

    stream->stop();
    file_dump->stop();
    task_mgr->cancelAll();
    task_mgr->joinAll();
    delete task_mgr;

    task_mgr = 0;
    file_dump = 0;
    stream = 0;

}

void VEPV::init()
{
    if (nullptr == pool)
    {
        pool = std::make_shared<Poco::ThreadPool>();
    }
    task_mgr = new Poco::TaskManager(*pool);
}

bool VEPV::configure(const Json::Value* jobject, bool do_start)
{
    init();

    ZConstString name(ZMB::SURV_CAM_PARAMS_NAME, jobject);
    ZConstString path(ZMB::SURV_CAM_PARAMS_PATH, jobject);
    if (name.empty() || path.empty())
    {
        std::cerr << "VideoEntity failed to parse JSON object\n";
        return false;
    }

    fs_helper = std::make_shared<ZMFS::FSHelper>();
    ZConstString fs(ZMB::SURV_CAM_PARAMS_FS, jobject);
    ZConstString tmpfs(ZMB::SURV_CAM_PARAMS_TMPFS, jobject);
    fs_helper->set_dirs(fs, tmpfs);

    auto cp_lock = cam_param.get_locker();
    cam_param.settings = *jobject;

    std::string _s (path.begin(), path.size());
    auto stream = new ZMBEntities::StreamReader(_s);
    bool ok = stream->open(path);
    if (ok)
    {
        _s = name.begin();
        auto file_dump = new ZMBEntities::Mp4WriterTask(_s);
        file_dump->open(stream->ff.get_format_ctx(), name, fs_helper);
        stream->file_pkt_q = file_dump;
        file_dump = file_dump;
    }
    else
    {
        delete stream;
        stream = 0;
    }
    stream = stream;
    return ok;

}

}//ZMBEntities

