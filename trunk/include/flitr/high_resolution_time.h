/* Framework for Live Image Transformation (FLITr) 
 * Copyright (c) 2010 CSIR
 * 
 * This file is part of FLITr.
 *
 * FLITr is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 * 
 * FLITr is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with FLITr. If not, see
 * <http://www.gnu.org/licenses/>.
 */

#ifndef HIGHRES_TIME_H
#define HIGHRES_TIME_H 1 

#include <flitr/flitr_export.h>
#include <cstdint>

#include <flitr/flitr_stdint.h>
#include <string>
#include <time.h>


/**
   \brief Get the date and time of the nanosec since epoch as a string.
   \return String representing current PC time of the form "2007-02-26_17h35m20.001s".
*/
inline std::string nanoSecToCalenderDate(uint64_t timeNanoSec)
{
    time_t timestamp_t=timeNanoSec / 1000000000;
    struct tm *timestamp_tm=localtime(&timestamp_t);

    char timestr[512];
    sprintf(timestr, "%d-%02d-%02d_%02dh%02dm%02d.%03ds",
        timestamp_tm->tm_year+1900, timestamp_tm->tm_mon+1, timestamp_tm->tm_mday,
        timestamp_tm->tm_hour, timestamp_tm->tm_min, timestamp_tm->tm_sec, int((timeNanoSec % 1000000000)/1000000));

    std::string s(timestr);
    return s;
}

/// Returns nanoseconds since the epoch!
extern FLITR_EXPORT uint64_t currentTimeNanoSec();

//!Return the second since midnight local time.
extern FLITR_EXPORT double secondsSinceMidnightLT();

//!Return the second since midnight GMT.
extern FLITR_EXPORT double secondsSinceMidnightGMT();


#endif // HIGHRES_TIME_H
