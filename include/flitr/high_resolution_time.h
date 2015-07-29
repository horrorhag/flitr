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
extern FLITR_EXPORT std::string nanoSecToCalenderDate(uint64_t timeNanoSec);

/// Returns nanoseconds since the epoch!
extern FLITR_EXPORT uint64_t currentTimeNanoSec();

//!Return the second since midnight local time.
extern FLITR_EXPORT double secondsSinceMidnightLT();

//!Return the second since midnight GMT.
extern FLITR_EXPORT double secondsSinceMidnightGMT();


#endif // HIGHRES_TIME_H
