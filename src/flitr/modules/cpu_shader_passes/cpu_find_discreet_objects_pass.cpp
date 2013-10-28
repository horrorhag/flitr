#include <flitr/modules/cpu_shader_passes/cpu_find_discreet_objects_pass.h>

#include <algorithm> 

using namespace flitr;

namespace
{
	unsigned char * temp;

	int delta[8][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1},{1, 1}, {-1, -1}, {1, -1}, {-1, 1}};
	
	std::vector<int> used(10000);
	std::map< int, std::vector< std::pair<int, int> > > borders;
	std::map< int, CPUFindDiscreetObjectsPass::Rect > rectangles;
}

void FollowBorder(int idx, int width, int w, int h);

CPUFindDiscreetObjectsPass::CPUFindDiscreetObjectsPass(osg::Image* image) :
    Image_(image)
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

	unsigned long i;
	// find edges
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

	for (i=0; i<borders.size(); i++)
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
			rectangles[i] = Rect(left, right, top, bottom);
		}
	}

	// draw edges
	for (i=0; i < (width*height); i++)
	{
		data[i] = temp[i];
	}

	// clear
	//for (i=0; i < (width*height); i++)
	//{
	//	data[i] = 0;
	//}

	//// draw borders
	//for (i=0; i<borders.size(); i++)
	//{
	//	std::vector< std::pair<int, int> > border = borders[i];
	//	for(int j=0; j < (int)border.size(); j++)
	//	{
	//		int w = border[j].first;
	//		int h = border[j].second;
	//		int idx = (h*width) + w;
	//		data[idx] = 255;
	//	}
	//}

	// draw rects
	//for (i=0; i < rectangles.size(); i++)
	//{
	//	int w = rectangles[i].left;
	//	int h = rectangles[i].top;
	//	int idx = (h*width) + w;
	//	data[idx] = 255;

	//	w = rectangles[i].left;
	//	h = rectangles[i].bottom;
	//	idx = (h*width) + w;
	//	data[idx] = 255;

	//	w = rectangles[i].right;
	//	h = rectangles[i].top;
	//	idx = (h*width) + w;
	//	data[idx] = 255;

	//	w = rectangles[i].right;
	//	h = rectangles[i].bottom;
	//	idx = (h*width) + w;
	//	data[idx] = 255;
	//}

    Image_->dirty();
}

void CPUFindDiscreetObjectsPass::GetRects(std::map<int, Rect>& rects)
{
	rects.insert(rectangles.begin(), rectangles.end());
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
