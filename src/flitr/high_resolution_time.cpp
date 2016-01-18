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

#include <flitr/high_resolution_time.h>

#include <chrono>
#include <ctime>
#include <string>

uint64_t currentTimeNanoSec()
{
    std::chrono::time_point<std::chrono::system_clock> tp=std::chrono::system_clock::now();
    return std::chrono::duration_cast<std::chrono::nanoseconds>(tp.time_since_epoch()).count();
}

std::string nanoSecToCalenderDate(uint64_t timeNanoSec)
{
    time_t timestamp_t=timeNanoSec / 1000000000;

    struct tm timestamp_tm;
    localtime_s(&timestamp_tm, &timestamp_t);
    //localtime_r(&timestamp_t, &timestamp_tm);
    
    char timestr[512];
    sprintf(timestr, "%d-%02d-%02d_%02dh%02dm%02d.%03ds",
            timestamp_tm.tm_year+1900, timestamp_tm.tm_mon+1, timestamp_tm.tm_mday,
            timestamp_tm.tm_hour, timestamp_tm.tm_min, timestamp_tm.tm_sec, int((timeNanoSec % 1000000000)/1000000));
    
    std::string s(timestr);
    return s;
}

//!Seconds since midnight in local time.
double secondsSinceMidnightLT()
{
    uint64_t timeNS=currentTimeNanoSec();
    
    time_t timestamp_t=currentTimeNanoSec() / 1000000000;
    
    struct tm timestamp_tm;
    //localtime_r(&timestamp_t, &timestamp_tm);
    localtime_s(&timestamp_tm, &timestamp_t);
    
    return timestamp_tm.tm_hour*3600.0 + timestamp_tm.tm_min*60.0 + timestamp_tm.tm_sec + (timeNS%1000000000)*0.000000001;
}

//!Seconds since midnight in GMT.
double secondsSinceMidnightGMT()
{
    uint64_t timeNS=currentTimeNanoSec();
    
    time_t timestamp_t=currentTimeNanoSec() / 1000000000;
    
    struct tm timestamp_tm;
    //gmtime_r(&timestamp_t, &timestamp_tm);
    gmtime_s(&timestamp_tm, &timestamp_t);

    return timestamp_tm.tm_hour*3600.0 + timestamp_tm.tm_min*60.0 + timestamp_tm.tm_sec + (timeNS%1000000000)*0.000000001;
}
