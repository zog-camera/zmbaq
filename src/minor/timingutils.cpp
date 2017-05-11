/*A video surveillance software with support of H264 video sources.
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
along with this program.  If not, see <http://www.gnu.org/licenses/>.*/


#include "timingutils.h"

namespace TimingUtils
{

/*----------------------------------------------------------------------------*/
#if defined(__unix) || defined(__APPLE__)
#include <sys/time.h>
#include <unistd.h>
#endif

#ifdef _WIN32
#include <sys/timeb.h>
#include <time.h>
#include <windows.h>
#define snprintf _snprintf_s
#endif

double getTimeOfDay()
{
#if defined(__unix) || defined(__APPLE__)
    struct timeval tv;
    struct timezone tz;
    gettimeofday(&tv, &tz);
    return tv.tv_sec + tv.tv_usec * 1e-6;
#elif defined(_WIN32)
    LARGE_INTEGER f;
    LARGE_INTEGER t;
    QueryPerformanceFrequency(&f);
    QueryPerformanceCounter(&t);
    return t.QuadPart/(double) f.QuadPart;
#else
    return 0;
#endif
}
/*----------------------------------------------------------------------------*/
LapTimer::LapTimer()
{
    mLastTick = getTimeOfDay();
}
//------------------------------------------------------------------------------
double LapTimer::lapTime()
{
   double t_old = mLastTick;
   mLastTick = getTimeOfDay();
   return mLastTick - t_old;
}

double LapTimer::elapsed()
{
   return getTimeOfDay()-mLastTick;
}

//------------------------------------------------------------------------------
} //namespace TimingUtils
/*----------------------------------------------------------------------------*/
