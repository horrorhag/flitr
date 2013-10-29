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
	, minArea_(0)
	, maxArea_(std::numeric_limits<int>::max())
	, minWidth_(0)
	, minHeight_(0)
	, maxWidth_(std::numeric_limits<int>::max())
	, maxHeight_(std::numeric_limits<int>::max())
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
	int size = width*height;

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
			if (i >= size || i < 0 || temp[i] == 0 || (std::find(used.begin(), used.end(), i)!=used.end()) )
				continue;
			FollowBorder(idx, width, height, w, h);
			idx++;
		}
	}

	// bounding rectangles
	for (i=0; i<(int)borders.size(); i++)
	{
		int left = width, bottom = 0, right = 0, top = height;
		std::vector< std::pair<int, int> > border = borders[i];
		for(int j=0; j < (int)border.size(); j++)
		{
			int w = border[j].first;
			int h = border[j].second;
			if (w < left) left = w;
			if (w > right) right = w;
			if (h > bottom) bottom = h;
			if (h < top) top = h;
		}
		if (border.size()>0)
		{
			int growV = ((float)(right - left) * expandRects_) + 0.5f;
			int growH = ((float)(bottom - top) * expandRects_) + 0.5f;
			
			left-=growV;
			right+=growV;
			bottom+=growH;
			top-=growH;

			if (left < 0) left = 0;
			if (right >= (int)width) right = width-1;
			if (top < 0) top = 0;
			if (bottom >= (int)height) bottom = height-1;

			rectangles.push_back(Rect(left, right, top, bottom));
		}
	}

	// intersections
	bool intersect = true;
	
	Rect r1,r2;
	int left, top, bottom, right;
	while (intersect)
	{
		if (rectangles.size() < 2) intersect = false;
		for (i = 0; i < ((int)rectangles.size())-1; i++)
		{
			for (int j = i+1; j < (int)rectangles.size(); j++)
			{
				r1 = rectangles[i];
				r2 = rectangles[j];
				intersect = (Intersect(r1, r2));
				if (intersect)
				{
					if (r1.left < r2.left) 
						left = r1.left;
					else
						left = r2.left;
					if (r1.right > r2.right) 
						right = r1.right;
					else
						right = r2.right;
					if (r1.bottom > r2.bottom)
						bottom = r1.bottom;
					else
						bottom = r2.bottom;
					if (r1.top < r2.top)
						top = r1.top;
					else
						top = r2.top;

					rectangles.push_back(Rect(left, right, top, bottom));
					rectangles.erase(rectangles.begin()+j);
					rectangles.erase(rectangles.begin()+i);
					break;
				}
				if (intersect)
					break;
			}
			if (intersect)
				break;
		}
	}

	// Min and Max checks
	Rect r;
	int area;
	for (i = rectangles.size()-1; i >= 0 ; i--)
	{
		r = rectangles[i];
		area = r.width * r.height;

		if (area < minArea_ || r.width < minWidth_ || r.height < minHeight_ || area > maxArea_ || r.width > maxWidth_ || r.height > maxHeight_)
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
		if (i >= size || i < 0 || temp[i] == 0 || (std::find(used.begin(), used.end(), i)!=used.end()) )
			continue;
		used.push_back(i);
		borders[idx].push_back(std::pair<int,int>(dw,dh));
		FollowBorder(idx, width, height, dw, dh);
	}
}

bool Intersect(CPUFindDiscreetObjectsPass::Rect r1, CPUFindDiscreetObjectsPass::Rect r2)
{
	return ( (abs(r1.centerX - r2.centerX) <= (r1.width/2 + r2.width/2) )
		&& ( abs(r1.centerY - r2.centerY) <= (r1.height/2 + r2.height/2)) );

}