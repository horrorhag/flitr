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

#define NSEC_PER_SEC 1000000000LL

inline uint64_t timespec_to_ns(const struct timespec *ts)
{
    return ((uint64_t) ts->tv_sec * NSEC_PER_SEC) + ts->tv_nsec;
}

/// Returns nanoseconds since epoch.
inline uint64_t currentTimeNanoSec()
{
    struct timespec timestamp_ts;
    //clock_gettime(CLOCK_MONOTONIC, &timestamp_ts);
    clock_gettime(CLOCK_REALTIME, &timestamp_ts);
    return timespec_to_ns(&timestamp_ts);
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
