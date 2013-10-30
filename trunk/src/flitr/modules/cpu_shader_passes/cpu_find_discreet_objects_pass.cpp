#include <flitr/modules/cpu_shader_passes/cpu_find_discreet_objects_pass.h>

#include <algorithm> 
#include <limits>

using namespace flitr;

namespace
{
	unsigned char * temp;

	int delta[8][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1},{1, 1}, {-1, -1}, {1, -1}, {-1, 1}};
	
    std::vector<int> used;
    std::vector<int> edgelist;
    int ucounter;
	std::vector< CPUFindDiscreetObjectsPass::Rect > rectangles;
}

void FollowBorder(int idx, int width, int size, int w, int h, int &left, int &right, int &bottom, int &top);
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
    ucounter = 0;
}
	
CPUFindDiscreetObjectsPass::~CPUFindDiscreetObjectsPass()
{
}

void CPUFindDiscreetObjectsPass::operator()(osg::RenderInfo& renderInfo) const
{
    rectangles.resize(0);
    edgelist.resize(0);

    const unsigned long width=Image_->s();
    const unsigned long height=Image_->t();
	int size = width*height;
    if (used.size()!=size)
    {
        used.resize(size, 0);
    }
    ucounter++;

    unsigned char * const data=(unsigned char *)Image_->data();
	if (!temp)
		temp = new unsigned char[width*height];

	// find edges
    int i = 0;
    for (i=0;i<size;i++)
	{
        if (data[i] == 0)
        {
            temp[i] = 0;
            continue;
        }
        if (data[i+1] == 0 || data[i-1] == 0 || data[i+width] == 0 || data[i-width] == 0)
        {
            edgelist.push_back(i);
            temp[i] = 255;
        }
        else
        {
            temp[i] = 0;
        }
	}

	int idx = 0;
    for (size_t j=0;j<edgelist.size();j++)
    {
        i = edgelist[j];
        if (used[i]==ucounter)
        {
            continue;
        }
        int left = i%width, bottom = i/width, right = left, top = bottom;
        FollowBorder(idx, width, size, left, bottom, left, right, bottom, top);

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

        idx++;
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
            redo:
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
                    std::swap(rectangles[0], rectangles[rectangles.size()-1]);
                    i = 0;
                    goto redo;
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
    memcpy(data, temp, size);

    Image_->dirty();
}

void CPUFindDiscreetObjectsPass::GetBoundingRects(std::vector<Rect>& rects)
{
	rects.swap(rectangles); 
}

void FollowBorder(int idx, int width, int size, int w, int h, int &left, int &right, int &bottom, int &top)
{
	int i, dh, dw;
	for (int l = 0; l < 8; l++)
	{
		dh = h+delta[l][0];
		dw = w+delta[l][1];
		i = (dh*width) + dw;
        if (i >= size || i < 0 || temp[i] == 0 || used[i]==ucounter )
            continue;
        used[i] = ucounter;
        left = std::min(left, dw);
        right = std::max(right, dw);
        top = std::min(top, dh);
        bottom = std::max(bottom, dh);
        FollowBorder(idx, width, size, dw, dh, left, right, bottom, top);
	}
}

bool Intersect(CPUFindDiscreetObjectsPass::Rect r1, CPUFindDiscreetObjectsPass::Rect r2)
{
	return ( (abs(r1.centerX - r2.centerX) <= (r1.width/2 + r2.width/2) )
		&& ( abs(r1.centerY - r2.centerY) <= (r1.height/2 + r2.height/2)) );

}
