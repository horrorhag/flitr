#include <flitr/modules/cpu_shader_passes/cpu_find_discreet_objects_pass.h>

#include <algorithm> 
#include <limits>

using namespace flitr;

namespace
{
	unsigned char * temp;

	int delta[8][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1},{1, 1}, {-1, -1}, {1, -1}, {-1, 1}};
	
	std::vector<int> used(10000);
	std::map< int, std::vector< std::pair<int, int> > > borders;
	std::vector< CPUFindDiscreetObjectsPass::Rect > rectangles;
}

void FollowBorder(int idx, int width, int height, int w, int h);
bool Intersect(CPUFindDiscreetObjectsPass::Rect r1, CPUFindDiscreetObjectsPass::Rect r2);

CPUFindDiscreetObjectsPass::CPUFindDiscreetObjectsPass(osg::Image* image) 
	: Image_(image)
	, expandRects_(0.0f)
	, minArea_(std::numeric_limits<int>::max())
	, maxArea_(0)
	, minWidth_(std::numeric_limits<int>::max())
	, minHeight_(std::numeric_limits<int>::max())
{
	temp = 0;
}
	
CPUFindDiscreetObjectsPass::~CPUFindDiscreetObjectsPass()
{
}

void CPUFindDiscreetObjectsPass::operator()(osg::RenderInfo& renderInfo) const
{
	used.clear();
	borders.clear();
	rectangles.clear();

    const unsigned long width=Image_->s();
    const unsigned long height=Image_->t();

    unsigned char * const data=(unsigned char *)Image_->data();
	if (!temp)
		temp = new unsigned char[width*height];

	// find edges
	int i;
	for (unsigned long w=0; w<width; w++)
	{
		for (unsigned long h=0; h<height; h++)
		{
			i = (h*width) + w;
			if (data[i] == 0)
			{
				temp[i] = 0;
				continue;
			}
			if (data[(h*width) + (w+1)] == 0 || data[(h*width) + (w-1)] == 0 || data[((h+1)*width) + w] == 0 || data[((h-1)*width) + w] == 0)
			{
				temp[i] = 255;
			}
			else
			{
				temp[i] = 0;
			}
		}
	}

	int idx = 0;
	for (unsigned long w=0; w<width; w++)
	{
		for (unsigned long h=0; h<height; h++)
		{
			i = (h*width) + w;
			if (temp[i] == 0 || (std::find(used.begin(), used.end(), i)!=used.end()) )
				continue;
			FollowBorder(idx, width, height, w, h);
			idx++;
		}
	}

	// bounding rectangles
	for (i=0; i<(int)borders.size(); i++)
	{
		int left = width, top = height, bottom = 0, right = 0;
		std::vector< std::pair<int, int> > border = borders[i];
		for(int j=0; j < (int)border.size(); j++)
		{
			int w = border[j].first;
			int h = border[j].second;
			if (w < left) left = w;
			if (w > right) right = w;
			if (h < top) top = h;
			if (h > bottom) bottom = h;
		}
		if (border.size()>0)
		{
			int growV = (right - left) * expandRects_;
			int growH = (bottom - top) * expandRects_;
			
			left-=growV;
			right+=growV;
			top-=growV;
			bottom+=growH;

			if (left < 0) left = 0;
			if (right >= width) right = width-1;
			if (top < 0) top = 0;
			if (bottom >= height) bottom = height-1;

			rectangles.push_back(Rect(left, right, top, bottom));
		}
	}

	// intersections
	bool intersect = true;
	
	Rect r1,r2;
	while (intersect)
	{
		if (rectangles.size() < 2) intersect = false;
		for (i = 0; i < ((int)rectangles.size())-1; i++)
		{
			for (int j = i+1; j < (int)rectangles.size(); j++)
			{
				intersect = (Intersect(rectangles[i], rectangles[j]) || Intersect(rectangles[j], rectangles[i]));
				if (intersect)
				{
					int left, top, bottom, right;
				
					r1 = rectangles[i];
					r2 = rectangles[j];

					if (r1.left < r2.left) 
						left = r1.left;
					else
						left = r2.left;
					if (r1.right > r2.right) 
						right = r1.right;
					else
						right = r2.right;
					if (r1.top < r2.top)
						top = r1.top;
					else
						top = r2.top;
					if (r1.bottom > r2.bottom)
						bottom = r1.bottom;
					else
						bottom = r2.bottom;

					rectangles.push_back(Rect(left, right, top, bottom));
					rectangles.erase(rectangles.begin()+i);
					rectangles.erase(rectangles.begin()+j);
					break;
				}
				if (intersect)
					break;
			}
		}
	}

	// Min and Max checks
	Rect r;
	int area, w, h;
	for (i = rectangles.size()-1; i >= 0 ; i--)
	{
		r = rectangles[i];
		w = r.right-r.left;
		h = r.bottom-r.top;
		area = w * h;

		if (area < minArea_ || w < minWidth_ || h < minHeight_ || area > maxArea_)
		{
			rectangles.erase(rectangles.begin()+i);
		}
	}
	

	// draw edges
	for (i=0; i < (int)(width*height); i++)
	{
		data[i] = temp[i];
	}

    Image_->dirty();
}

void CPUFindDiscreetObjectsPass::GetBoundingRects(std::vector<Rect>& rects)
{
	rects.swap(rectangles); 
}

void FollowBorder(int idx, int width, int height, int w, int h)
{
	int size = width*height;
	int i, dh, dw;
	for (int l = 0; l < 8; l++)
	{
		dh = h+delta[l][0];
		dw = w+delta[l][1];
		i = (dh*width) + dw;
		if (i >= size)
			continue;
		if (temp[i] == 0 || (std::find(used.begin(), used.end(), i)!=used.end()) )
			continue;
		used.push_back(i);
		borders[idx].push_back(std::pair<int,int>(dw,dh));
		FollowBorder(idx, width, height, dw, dh);
	}
}

bool Intersect(CPUFindDiscreetObjectsPass::Rect r1, CPUFindDiscreetObjectsPass::Rect r2)
{
	return ( (r1.left >= r2.left && r1.left <= r2.right) && (r1.top >= r2.top && r1.top <= r2.bottom)
		|| (r1.right >= r2.left && r1.right <= r2.right) && (r1.top >= r2.top && r1.top <= r2.bottom)
		|| (r1.left >= r2.left && r1.left <= r2.right) && (r1.bottom >= r2.top && r1.bottom <= r2.bottom)
		|| (r1.right >= r2.left && r1.right <= r2.right) && (r1.bottom >= r2.top && r1.bottom <= r2.bottom)
		);
}