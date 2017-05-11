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


#ifndef TIMINGUTILS_H
#define TIMINGUTILS_H

namespace TimingUtils {

    /** Get current time of day starting from 00:00 in seconds.*/
    double getTimeOfDay();

    class LapTimer
    {
    public:
        /** registers time of day in seconds when object spawned.*/
        LapTimer();

        /** Return elapsed time in seconds since last lapTime() call or since LapTimer
         *	object creation. Resets timer*/
        double lapTime();

        /** Get elapsed time since obejct creation or last lapTime() call.
         * Does not reset timer */
        double elapsed();

    private:
        double mLastTick;
    };
}

#endif // TIMINGUTILS_H
