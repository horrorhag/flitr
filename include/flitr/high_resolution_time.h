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

/// Sole purpose is to provide currentTimeNanoSec() for a platform

#include <flitr/flitr_stdint.h>

#if defined(linux) || defined(__linux) || defined(__linux__)

#include <time.h>
#include <boost/date_time.hpp>

#define NSEC_PER_SEC 1000000000LL
#define NSEC_PER_MSEC 1000000LL

inline uint64_t timespec_to_ns(const struct timespec *ts)
{
    return ((uint64_t) ts->tv_sec * NSEC_PER_SEC) + ts->tv_nsec;
}

inline struct timespec ns_to_timespec(const uint64_t ns)
{
    struct timespec timestamp_ts;
    timestamp_ts.tv_sec=ns / NSEC_PER_SEC;
    timestamp_ts.tv_nsec=ns % NSEC_PER_SEC;

    return timestamp_ts;
}

/// Returns nanoseconds since epoch.  UTC time_t epoch 1970-01-01 00:00:00.
inline uint64_t currentTimeNanoSec()
{
    struct timespec timestamp_ts;
    //clock_gettime(CLOCK_MONOTONIC, &timestamp_ts);
    clock_gettime(CLOCK_REALTIME, &timestamp_ts);
    return timespec_to_ns(&timestamp_ts);
}

/**
   \brief Get the date and time of the nanosec since epoch as a string.
   \return String representing current PC time of the form "2007-02-26_17h35m20.001s".
*/
inline std::string nanoSecToCalenderDate(uint64_t timeNanoSec)
{
    struct timespec timestamp_ts=ns_to_timespec(timeNanoSec);

    time_t timestamp_t=timestamp_ts.tv_sec;
    struct tm *timestamp_tm=localtime(&timestamp_t);

    char timestr[512];
    sprintf(timestr, "%d-%02d-%02d_%02dh%02dm%02d.%03ds",
        timestamp_tm->tm_year+1900, timestamp_tm->tm_mon+1, timestamp_tm->tm_mday,
        timestamp_tm->tm_hour, timestamp_tm->tm_min, timestamp_tm->tm_sec, (int)(timestamp_ts.tv_nsec/NSEC_PER_MSEC));

    std::string s(timestr);
    return s;
}

inline uint64_t clockResNanoSec()
{
    struct timespec timestamp_ts;
    //clock_getres(CLOCK_MONOTONIC, &timestamp_ts);
    clock_getres(CLOCK_REALTIME, &timestamp_ts);
    return timespec_to_ns(&timestamp_ts);
}

#if 0
int main(void)
{
    printf("%lld, %lld\n", currentTimeNanoSec(), clockResNanoSec());
    printf("%lld, %lld\n", currentTimeNanoSec(), clockResNanoSec());
}
#endif

#else // NOT LINUX

// fall back on OSG
// \todo how do we connect OSG time to real wall clock time?
#include <osg/ref_ptr>
#include <osg/Timer>
/// Returns nanoseconds since epoch.
extern uint64_t currentTimeNanoSec();

#endif

#endif // HIGHRES_TIME_H
