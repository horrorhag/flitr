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

#ifndef CPU_FIND_DISCREET_OBJECTS_PASS
#define CPU_FIND_DISCREET_OBJECTS_PASS 1

#include <flitr/flitr_export.h>
#include <flitr/modules/cpu_shader_passes/cpu_shader_pass.h>

namespace flitr 
{

class FLITR_EXPORT CPUFindDiscreetObjectsPass : public flitr::CPUShaderPass::CPUShader
{
public:
	struct Rect
	{
		int left;
		int right;
		int top;
		int bottom;

		Rect() {};
		Rect(int l, int r, int t, int b)
			: left(l), right(r), top(t), bottom(b)	
		{}
	};

    CPUFindDiscreetObjectsPass(osg::Image* image);
    ~CPUFindDiscreetObjectsPass();

    virtual void operator () (osg::RenderInfo& renderInfo) const;

	void GetRects(std::map<int, Rect>& rects);

private:
	void FindEdges();

private:
	osg::Image* Image_;
};

}
#endif //CPU_FIND_DISCREET_OBJECTS_PASS
