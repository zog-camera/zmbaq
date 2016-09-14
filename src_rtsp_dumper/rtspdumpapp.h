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


#ifndef RTSPDUMPAPP_H
#define RTSPDUMPAPP_H

extern "C"
{
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
}

#include "json/value.h"
#include <fstream>
#include <iostream>


#include <Poco/ThreadPool.h>
#include <Poco/DateTime.h>
#include <Poco/Condition.h>
#include <Poco/Mutex.h>

//#include "src/noteinfo.h"
#include "src_videoentity/survlobj.h"

namespace StreamDumper {

class RtspDumpApp
{
public:
    RtspDumpApp();
    RtspDumpApp(int _argc, char** _argv);

    static RtspDumpApp* instance(int _argc = 0, char** _argv = 0);
    static void app_terminate(int signum);

    /** Read config files from argv[1] and
     *  run in a loop / or pthread_cond_wait.*/
    virtual int app_exec();
    void clear();

    virtual ~RtspDumpApp();

protected:
    /** Parse argc, argv and start operation streads.*/
    bool start_with_args();

    //-----------
    struct sigaction action;
    int argc;
    char** argv;
    //-------------------------------------------------
    static std::atomic<RtspDumpApp*> m_instance;
    static std::mutex m_mutex;
    //-------------------------------------------------
    //-----------
    ZMBEntities::SurvlObj suo;

    Poco::FastMutex stop_cond_mutex;
    Poco::Condition stop_condition;

};

}//StreamDumper



#endif // RTSPDUMPAPP_H
