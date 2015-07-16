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

uint64_t currentTimeNanoSec()
{
    std::chrono::time_point<std::chrono::system_clock> tp=std::chrono::system_clock::now();
    return std::chrono::duration_cast<std::chrono::nanoseconds>(tp.time_since_epoch()).count();
}

//!Seconds since midnight in local time.
double secondsSinceMidnightLT()
{
    uint64_t timeNS=currentTimeNanoSec();
    
    time_t timestamp_t=currentTimeNanoSec() / 1000000000;
    
    struct tm timestamp_tm;
    localtime_r(&timestamp_t, &timestamp_tm);
    
    return timestamp_tm.tm_hour*3600.0 + timestamp_tm.tm_min*60.0 + timestamp_tm.tm_sec + (timeNS%1000000000)*0.000000001;
}

//!Seconds since midnight in GMT.
double secondsSinceMidnightGMT()
{
    uint64_t timeNS=currentTimeNanoSec();
    
    time_t timestamp_t=currentTimeNanoSec() / 1000000000;
    
    struct tm timestamp_tm;
    gmtime_r(&timestamp_t, &timestamp_tm);

    return timestamp_tm.tm_hour*3600.0 + timestamp_tm.tm_min*60.0 + timestamp_tm.tm_sec + (timeNS%1000000000)*0.000000001;
}
