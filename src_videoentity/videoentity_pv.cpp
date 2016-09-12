#include "videoentity_pv.h"

namespace ZMBEntities {

VEPV::VEPV(SHP(ZMBCommon::ThreadsPool) p_pool) : pool(p_pool)
{
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
    file_dump->close();
    file_dump = 0;
    stream = 0;
}

bool VEPV::configure(const Json::Value* jobject, bool do_start, std::shared_ptr<ZMBCommon::ThreadsPool> p_pool)
{
  pool = p_pool;

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
        auto file_dump = new ZMBEntities::Mp4WriterTask();
        file_dump->tag = _s;
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

