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

#include <string>
#include <iostream>
#include <fstream>
#include <stdlib.h>

#include <flitr/log_message.h>

using namespace std;
//using namespace flitr;

uint32_t g_LogMessageCategory = flitr::LOG_ALL;

void flitr::setLogMessageCategory(LogMessageCategory flags)
{
    initLogMessage();
    g_LogMessageCategory = flags;
}

void flitr::addLogMessageCategory(LogMessageCategory flags)
{
    initLogMessage();
    g_LogMessageCategory |= flags;
}

void flitr::delLogMessageCategory(LogMessageCategory flags)
{
    initLogMessage();
    g_LogMessageCategory &= ~flags;
}

uint32_t flitr::getLogMessageCategory()
{
    initLogMessage();
    return g_LogMessageCategory;
}

int setInEnvironment(const char *e)
{
    int value = 0;
    char* env_c_str = getenv(e);
    if(env_c_str) {
        std::string envstring(env_c_str);
		if (envstring.find("1")!=std::string::npos) {
			value = 1;
		}
    }
    return value;
}

bool flitr::initLogMessage()
{
    static bool s_LogMessageInit = false;

    if (s_LogMessageInit) return true;
    
    s_LogMessageInit = true;

    //g_LogMessageCategory = LOG_CRITICAL; // Default value
    g_LogMessageCategory = LOG_ALL; // Default value

    if (setInEnvironment("FLITR_LOG_INFO")) {
		addLogMessageCategory(LOG_INFO);
    }
    if (setInEnvironment("FLITR_LOG_DEBUG")) {
		addLogMessageCategory(LOG_DEBUG);
    }
    if (setInEnvironment("FLITR_LOG_ALL")) {
		addLogMessageCategory(LOG_ALL);
    }
    return true;
}

class NullStreamBuffer : public std::streambuf
{
  private:  
	virtual streamsize xsputn (const char_type*, streamsize n)
    {
        return n;
    }
};

std::ostream& flitr::logMessage(const LogMessageCategory category)
{
    // set up global notify null stream for inline notify
    static std::ostream s_LogMessageNullStream(new NullStreamBuffer());

    static bool initialized = false;
    if (!initialized) 
    {
        cerr << ""; // dummy op to force construction of cerr, before a reference is passed back to calling code.
        cout << ""; // dummy op to force construction of cout, before a reference is passed back to calling code.
        initialized = initLogMessage();
    }

    // check bitmask
    if (category & g_LogMessageCategory)
    {
        return std::cout;
		//return std::cerr;
    }
    return s_LogMessageNullStream;
}
