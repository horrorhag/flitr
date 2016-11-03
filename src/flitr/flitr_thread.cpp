/* Framework for Live Image Transformation (FLITr) 
 * Copyright (c) 2016 CSIR
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

#include <flitr/flitr_thread.h>

#ifndef WIN32
#include <pthread.h>
#endif /* WIN32 */

FThread::FThread()
{
}

void FThread::startThread(int32_t cpu_affinity)
{
    std::thread t(&FThread::run, this);
    _thread.swap(t);
    applyAffinity(_thread, cpu_affinity);
}

bool FThread::applyAffinity(std::thread& thread, int32_t cpu_affinity)
{
#ifdef WIN32
    return false;
#endif
    
#ifdef __APPLE__
    return false;
#endif
    
#ifdef __linux
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);

    if(cpu_affinity >= 0)
    {
        CPU_SET(cpu_affinity, &cpuset);
    }

    int status = pthread_setaffinity_np(thread.native_handle(), sizeof(cpu_set_t), &cpuset);
    if(status != 0)
    {
        return false;
    }
    return true;
#endif /* __GNUC__ */
}

FThread::~FThread()
{
    if (_thread.joinable()) _thread.join();
}
