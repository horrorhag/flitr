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

#ifndef STATS_COLLECTOR_H
#define STATS_COLLECTOR_H 1

#include <flitr/flitr_stdint.h>
#include <flitr/high_resolution_time.h>
#include <flitr/log_message.h>
#include <flitr/flitr_export.h>
#include <flitr/flitr_config.h>

#include <string>

namespace flitr {

#ifdef FLITR_PROFILE

class FLITR_EXPORT StatsCollector {
  public:
    StatsCollector(std::string ID) :
        ID_(ID),
        ttick_(0),
        tock_count_(0),
        ttock_(0),
        min_(18446744073709551615ULL), //UINT64_MAX
        max_(0),
        tock_count_at_max_(0),
        sum_(0)
    {
    }
    ~StatsCollector() 
    {
        logMessage(LOG_INFO) << ID_ << 
            " - tick() count       : " << tock_count_ << "\n";
        if (tock_count_ != 0) {
            logMessage(LOG_INFO) << ID_ << 
                " - tick() count at max: " << tock_count_at_max_ << "\n";
            logMessage(LOG_INFO) << ID_ << 
                " - min                : " << min_ << " ns\n";
            logMessage(LOG_INFO) << ID_ << 
                " - avg                : " << (uint64_t)(sum_ / tock_count_) << " ns\n";
            logMessage(LOG_INFO) << ID_ << 
                " - max                : " << max_ << " ns\n";
        }
    }
    inline void tick()
    {
        ttick_ = currentTimeNanoSec();
    }
    inline void tock()
    {
        ttock_ = currentTimeNanoSec();
        tock_count_++;

        uint64_t tdiff = ttock_ - ttick_;
        sum_ += tdiff;
        if (tdiff < min_) min_ = tdiff;
        if (tdiff > max_) {
            max_ = tdiff;
            tock_count_at_max_ = tock_count_;
        }
    }

    void setID(const std::string &ID)
    {
        ID_=ID;
    }
    
    
  private:
    /// Identifier string to use when printing info
    std::string ID_;
    /// Time at tick
    uint64_t ttick_;
    /// Times tick called
    uint64_t tock_count_;
    /// Time at tock;
    uint64_t ttock_;
    /// Min tock-tick
    uint64_t min_;
    /// Max tock-tick
    uint64_t max_;
    /// Tick that triggered max
    uint64_t tock_count_at_max_;
    /// Sum tock-tick
    uint64_t sum_;
};

#else

class FLITR_EXPORT StatsCollector {
  public:
    StatsCollector(std::string ID) {}
    ~StatsCollector() {}
    void tick() {};
    void tock() {};
};

#endif // FLITR_PROFILE

}

#endif // STATS_COLLECTOR_H
