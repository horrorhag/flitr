#include <flitr/modules/cpu_shader_passes/cpu_find_discreet_objects_pass.h>

#include <algorithm> 

using namespace flitr;

namespace
{
	unsigned char * temp;

	int delta[8][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1},{1, 1}, {-1, -1}, {1, -1}, {-1, 1}};
	
	std::vector<int> used(10000);
	std::map< int, std::vector< std::pair<int, int> > > borders;
	std::vector< CPUFindDiscreetObjectsPass::Rect > rectangles;
}

void FollowBorder(int idx, int width, int w, int h);
bool Intersect(CPUFindDiscreetObjectsPass::Rect r1, CPUFindDiscreetObjectsPass::Rect r2);

CPUFindDiscreetObjectsPass::CPUFindDiscreetObjectsPass(osg::Image* image) 
	: Image_(image)
	, expandRects_(0.0f)
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
			FollowBorder(idx, width, w, h);
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
			rectangles.push_back(Rect(left-growV, right+growV, top-growH, bottom+growH));
		}
	}

	// intersections
	bool intersect = true;
	
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
					
					if (rectangles[i].left < rectangles[j].left) 
						left = rectangles[i].left;
					else
						left = rectangles[j].left;
					if (rectangles[i].right > rectangles[j].right) 
						right = rectangles[i].right;
					else
						right = rectangles[j].right;
					if (rectangles[i].top < rectangles[j].top)
						top = rectangles[i].top;
					else
						top = rectangles[j].top;
					if (rectangles[i].bottom > rectangles[j].bottom)
						bottom = rectangles[i].bottom;
					else
						bottom = rectangles[j].bottom;

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

void FollowBorder(int idx, int width, int w, int h)
{
	int i, dh, dw;
	for (int l = 0; l < 8; l++)
	{
		dh = h+delta[l][0];
		dw = w+delta[l][1];
		i = (dh*width) + dw;
		if (temp[i] == 0 || (std::find(used.begin(), used.end(), i)!=used.end()) )
			continue;
		used.push_back(i);
		borders[idx].push_back(std::pair<int,int>(dw,dh));
		FollowBorder(idx, width, dw, dh);
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