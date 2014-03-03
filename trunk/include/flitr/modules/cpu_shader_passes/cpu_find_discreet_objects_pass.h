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

/*!
* Takes a binary input image and finds bounding rectangles for all discreet objects in the image. 
* Bounding rectangles can be expanded and combined to improve results.
*/
class FLITR_EXPORT CPUFindDiscreetObjectsPass : public flitr::CPUShaderPass::CPUShader
{
public:
	struct Rect
	{
		int left;
		int right;
		int top;
		int bottom;

		float centerX;
		float centerY;

		int width;
		int height;

		Rect() {};

		Rect(int l, int r, int t, int b)
			: left(l), right(r), top(t), bottom(b)	
		{
			width = right - left;
			height = bottom - top;
			centerX = left + width/2;
			centerY = top + height/2;
		}
	};

    CPUFindDiscreetObjectsPass(osg::Image* image);
    ~CPUFindDiscreetObjectsPass();

    virtual void operator () (osg::RenderInfo& renderInfo) const;

	//! Expand bounding rectangles by the given percentage, intersecting rectangles are combined [0,1]
	void ExpandRects(float percentage) { expandRects_ = percentage; }

	//! The minimum area considered as a object
	void SetMinArea(int area) { minArea_ = area; }

	//! The maximum area considered as a object
	void SetMaxArea(int area) { maxArea_ = area; }

	//! The minimum width considered as a object
	void SetMinWidth(int width) { minWidth_ = width; }

	//! The minimum height considered as a object
	void SetMinHeight(int height) { minHeight_ = height; }

	//! The maximum width considered as a object
	void SetMaxWidth(int width) { maxWidth_ = width; }

	//! The max height considered as a object
	void SetMaxHeight(int height) { maxHeight_ = height; }

    //! The Region of Interest (ROI) for locating objects
    void SetROI(int l, int r, int t, int b)
    {
        int width = Image_->s();
        int height = Image_->t();

        if (l < 0) l = 0;
        if (r >= width) r = width - 1 ;
        if (t < 0) t = 0;
        if (b >= height) b = height - 1;

        theROI_ = Rect(l,r,t,b);
    }

    //! The Region of Interest (ROI) for locating objects
    void SetROI(Rect roi)
    {
        int l = roi.left;
        int r = roi.right;
        int t = roi.top;
        int b = roi.bottom;

        SetROI(l,r,t,b);
    }

	//! Call each frame to retrieve the bounding rectangles of discreet objects
	void GetBoundingRects(std::vector<Rect>& rects);

    //! Retrieve the current ROI being processed
    Rect GetROI() { return theROI_; }

private:
	void FindEdges();

private:
	osg::Image* Image_;
	float expandRects_;
	int minArea_;
	int maxArea_;
	int minWidth_;
	int minHeight_;
	int maxWidth_;
	int maxHeight_;
    Rect theROI_;
};

}
#endif //CPU_FIND_DISCREET_OBJECTS_PASS
