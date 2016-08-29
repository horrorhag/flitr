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

#ifndef FLITR_THREAD_H
#define FLITR_THREAD_H 1

#include <flitr/flitr_export.h>

#include <thread>
#include <chrono>

class FLITR_EXPORT FThread
{
public:
    FThread();
    virtual ~FThread();
    
    void startThread();
    
    virtual void run() = 0;
    
    void join()
    {
        _thread.join();
    }
    
    static void microSleep(const uint32_t us)
    {
        std::this_thread::sleep_for(std::chrono::microseconds(us));
    }
    
private:
    std::thread _thread;
};

#endif // FLITR_THREAD_H
