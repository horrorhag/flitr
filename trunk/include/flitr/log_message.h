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

#ifndef LOG_MESSAGE_H
#define LOG_MESSAGE_H 1

#include <flitr/flitr_export.h>
#include <ostream>

#include <flitr/flitr_stdint.h>

namespace flitr {

/// Message categories.
typedef uint32_t LogMessageCategory;
const LogMessageCategory LOG_NONE = 0;
const LogMessageCategory LOG_INFO = 1 << 0;
const LogMessageCategory LOG_CRITICAL = 1 << 1;
const LogMessageCategory LOG_DEBUG = 1 << 2;
const LogMessageCategory LOG_ALL = 0xffffffff;

/// Set the combination of flags for messages to log.
void setLogMessageCategory(LogMessageCategory flags);

/// Add flags.
void addLogMessageCategory(LogMessageCategory flags);

/// Remove category/ies.
void delLogMessageCategory(LogMessageCategory flags);

/// Get the current flags.
uint32_t getLogMessageCategory();

/// Init the flags.
//extern bool initLogMessage();
bool initLogMessage();

/// Actually log.
FLITR_EXPORT extern std::ostream& logMessage(const LogMessageCategory category);

inline std::ostream& logMessage(void) { return logMessage(LOG_ALL); }

}

#endif // LOG_MESSAGE_H
