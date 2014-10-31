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

#include <osg/ref_ptr>
#include <osg/Timer>

osg::Timer *gHighResTimer = osg::Timer::instance();

/// Returns nanoseconds since some reference time. Might not be since the epoch!
uint64_t currentTimeNanoSec()
{
    osg::Timer_t timer_t = gHighResTimer->tick();
    #define NSEC_PER_SEC 1000000000LL
    return (uint64_t)(timer_t * gHighResTimer->getSecondsPerTick() * NSEC_PER_SEC);

    //BD: Below is the std::chrono code to be used beyond VS 2013!
    //std::chrono::time_point<std::chrono::system_clock> tp=std::chrono::system_clock::now();
    //return std::chrono::duration_cast<std::chrono::nanoseconds>(tp.time_since_epoch()).count();
}

