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

#ifndef FLITR_PARAMETERS_H
#define FLITR_PARAMETERS_H 1

#include <string>

namespace flitr {

class Parameters
{
public:
    enum EParmType
    {
        PARM_INT,
        PARM_FLOAT,
        PARM_DOUBLE,
        PARM_BOOL,
	PARM_ENUM
    };

    virtual int getNumberOfParms() { return 0; }

    virtual EParmType getParmType(int id) {}
    virtual std::string getParmName(int id) {}

    virtual std::string getTitle() { return "Title"; }

    virtual int getEnum(int id) { return 0; }
    virtual std::string getEnumText(int id, int v) { return "Default"; }
    virtual int getInt(int id) { return 0; }
    virtual float getFloat(int id) { return 0.f; }
    virtual double getDouble(int id) { return 0.0; }
    virtual bool getBool(int id) { return false; }

    virtual bool getEnumRange(int id, int &low, int &high) { return false; }
    virtual bool getIntRange(int id, int &low, int &high) { return false; }
    virtual bool getFloatRange(int id, float &low, float &high) { return false; }
    virtual bool getDoubleRange(int id, double &low, double &high) { return false; }

    virtual bool setEnum(int id, int v) { return false; }
    virtual bool setInt(int id, int v) { return false; }
    virtual bool setFloat(int id, float v) { return false; }
    virtual bool setDouble(int id, double v) { return false; }
    virtual bool setBool(int id, bool v) { return false; }

    // TODO: these need to be moved to a image processing process interface class or something like that
    virtual void enable(bool state=true) {}
    virtual bool isEnabled() { return false; }
    virtual void preFrame() {}
    virtual void postFrame() {}


};

}

#endif //FLITR_PARAMETERS_H
