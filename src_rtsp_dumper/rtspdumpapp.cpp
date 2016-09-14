/*
A video surveillance software with support of H264 video sources.
Copyright (C) 2015 Bogdan Maslowsky, Alexander Sorvilov. 

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU Affero General Public License as published
by the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Affero General Public License for more details.

You should have received a copy of the GNU Affero General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/


#include "rtspdumpapp.h"

namespace StreamDumper {
//-------------------------------------------------
std::atomic<RtspDumpApp*> RtspDumpApp::m_instance;
std::mutex RtspDumpApp::m_mutex;
//-------------------------------------------------

RtspDumpApp::RtspDumpApp()
{
    memset(&action, 0, sizeof(struct sigaction));
    sigaction(SIGTERM, &action, NULL);
    sigaction(SIGINT, &action, NULL);
    action.sa_handler = &app_terminate;

    argc = 0;
    argv = nullptr;
}

RtspDumpApp::RtspDumpApp(int _argc, char** _argv)
    : RtspDumpApp()
{
    argc = _argc;
    argv = _argv;
}

void RtspDumpApp::clear()
{
    suo.clear();
}

RtspDumpApp::~RtspDumpApp()
{
    clear();
}

RtspDumpApp* RtspDumpApp::instance(int _argc, char** _argv)
{
    RtspDumpApp* tmp = m_instance.load();
    if (tmp == nullptr) {
        std::lock_guard<std::mutex> lock(m_mutex);
        tmp = m_instance.load();

        if (tmp == nullptr) {
            //call constructor on existing memory:
            tmp = new RtspDumpApp(_argc, _argv);
            m_instance.store(tmp);
        }
    }
    return tmp;
}

void RtspDumpApp::app_terminate(int signum)
{
    RtspDumpApp* prog = RtspDumpApp::instance();
    prog->stop_condition.broadcast();
}

bool RtspDumpApp::start_with_args()
{
    if (argc == 1)
    {
        std::cerr << "Must have 1 argumet with config file.\n";
        return false;
    }
    bool ok = false;
    Json::Value cfg = ZMBEntities::jvalue_from_file(ok, ZMBCommon::ZConstString(argv[1]));

    if (!ok)
    {
        std::cerr << "Failed to open/parse JSON config file\n";
        return false;
    }

    suo = ZMBEntities::SurvlObj();
    ok = suo.create((const Json::Value*)&cfg);

    if (!ok)
    {
        std::cerr << "Failed to open/parse JSON config file\n";
        return false;
    }
    return true;
}

int RtspDumpApp::app_exec()
{
    if (!start_with_args())
        return -1;

    //wait until program exit signal wakes us.
    stop_cond_mutex.lock();
    stop_condition.wait<Poco::FastMutex>(stop_cond_mutex);
    clear();
    return 0;
}

}//namespace StreamDumper


