#ifndef IMAGE_N_PYRAMID_H
#define IMAGE_N_PYRAMID_H 1

/*
ToDo: Implement a RESET for the stabaliser!
*/
#include <osg/Program>
#include <osg/Shader>
#include <osg/Uniform>
#include <osg/TexEnv>
#include <osg/TextureRectangle>
#include <osg/Group>
#include <osg/Geometry>
#include <osg/Geode>
#include <osg/Switch>
#include <osg/AlphaFunc>
#include <sstream>

#include <flitr/modules/lucas_kanade/postRenderCallback.h>
#include <flitr/flitr_export.h>
#include <flitr/modules/parameters/parameters.h>
#include <flitr/texture.h>


namespace flitr {

    class ImageNPyramid {
	public:
        static double logbase(const double a, const double base);

		class PyramidRebuilt_CameraPostDrawCallback : public osg::Camera::DrawCallback
		{//Used if the CPU needs to know when the Pyramid is rebuilt/readback.
		public:
			PyramidRebuilt_CameraPostDrawCallback() :
			  m_pPostPyramidRebuiltCallback(0)
			{}

			virtual void operator () (const osg::Camera& camera) const
			{
				m_pPostPyramidRebuiltCallback->callback();
			}
		    
			PostRenderCallback *m_pPostPyramidRebuiltCallback;
		};


        ImageNPyramid(const flitr::TextureRectangle *i_pInputTexture,
            std::vector< std::pair<int,int> > &i_ROIVec,
			unsigned long i_ulROIWidth, unsigned long i_ulROIHeight,
			bool i_bIndicateROI, bool i_bUseGPU, bool i_bReadOutputBackToCPU);


		bool init(osg::Group *root_node, PostRenderCallback *i_pPostPyramidRebuiltCallback);//Allocate pyramid levels

		inline const osg::Image* getLevel(unsigned long i_ulPyramidNum, unsigned long i_ulLevel) const
		{
			return m_imageGausPyramid[i_ulPyramidNum][i_ulLevel].get();
		}

        inline const osg::Image* getLevelDeriv(unsigned long i_ulPyramidNum, unsigned long i_ulLevel) const
		{
			return m_derivImagePyramid[i_ulPyramidNum][i_ulLevel].get();
		}

		inline int getLevelWidth(unsigned long i_ulLevel) const
		{
			return m_imagePyramidWidth_[i_ulLevel];
		}

        inline int getLevelHeight(unsigned long i_ulLevel) const
		{
			return m_imagePyramidHeight_[i_ulLevel];
		}

		inline unsigned long getNumLevels() const
		{
            return m_ulNumPyramidLevels;
		}

		inline void setRebuildSwitchOn()
		{
			m_rpRebuildSwitch->setAllChildrenOn();
		}

        inline void setRebuildSwitchOff()
		{
			m_rpRebuildSwitch->setAllChildrenOff();
		}


        //Rebuild the image pyramid on the CPU.
        void rebuildNPyramidCPU();


	//private:
	public:
        const flitr::TextureRectangle *m_pInputTexture;

        unsigned long m_ulNumPyramidLevels;
		osg::ref_ptr<osg::Image> **m_imageGausPyramid;//R32F Image: Gaussian kernel result.
        osg::ref_ptr<flitr::TextureRectangle> **m_textureGausPyramid;

		osg::ref_ptr<osg::Image> **m_imageGausXPyramid;//R32F Image: Gaussian kernel result.
        osg::ref_ptr<flitr::TextureRectangle> **m_textureGausXPyramid;

		osg::ref_ptr<osg::Image> **m_derivImagePyramid;//R32F Image: Gaussian kernel result.
        osg::ref_ptr<flitr::TextureRectangle> **m_derivTexturePyramid;

		osg::ref_ptr<osg::Image> m_gausDistSqImage;
        osg::ref_ptr<flitr::TextureRectangle> m_gausDistSqTexture;
		osg::ref_ptr<osg::Image> m_gausDistSqH2LImage;
        osg::ref_ptr<flitr::TextureRectangle> m_gausDistSqH2LTexture;

		int *m_imagePyramidWidth_;
		int *m_imagePyramidHeight_;

        std::vector< std::pair<int,int> > m_ROIVec;
        int m_numPyramids_;

		const unsigned long m_ulROIWidth;
		const unsigned long m_ulROIHeight;

		const bool m_bIndicateROI;
		const bool m_bUseGPU;
		const bool m_bReadOutputBackToCPU;

		osg::ref_ptr<osg::Switch>			 m_rpRebuildSwitch;

		PyramidRebuilt_CameraPostDrawCallback m_pyramidRebuiltCallback;

        osg::ref_ptr<osg::Geode> createScreenAlignedQuad(unsigned long i_ulWidth, unsigned long i_ulHeight, double i_dBorderPixels);
		osg::Camera *createScreenAlignedCamera(unsigned long i_ulWidth, unsigned long i_ulHeight, double i_dBorderPixels);

		osg::Node* createDerivLevel(unsigned long i_ulLevel, unsigned long i_ulWidth, unsigned long i_ulHeight, bool i_bDoPostPyramidRebuiltCallback);

		osg::Node* createGausFiltLevel0(unsigned long i_ulWidth, unsigned long i_ulHeight, bool i_bDoPostPyramidRebuiltCallback);
		osg::Node* createGausFiltLevel1toN(unsigned long i_ulLevel, unsigned long i_ulWidth, unsigned long i_ulHeight, bool i_bDoPostPyramidRebuiltCallback);

		osg::Node* createGausXFiltLevel0(unsigned long i_ulWidth, unsigned long i_ulHeight, bool i_bDoPostPyramidRebuiltCallback);
		osg::Node* createGausXFiltLevel1toN(unsigned long i_ulLevel, unsigned long i_ulWidth, unsigned long i_ulHeight, bool i_bDoPostPyramidRebuiltCallback);

		osg::Node* createGausYFiltLevel0(unsigned long i_ulWidth, unsigned long i_ulHeight, bool i_bDoPostPyramidRebuiltCallback);
		osg::Node* createGausYFiltLevel1toN(unsigned long i_ulLevel, unsigned long i_ulWidth, unsigned long i_ulHeight, bool i_bDoPostPyramidRebuiltCallback);

		//Get gaussian filtered pixel from level i_ulLevel.
		// The filter kernel is centre in the four pixels that has (i_ulX, i_ulY) as the top-left corner.
//ToDo: Optimise execution by doing gaussian filter on entire texture instead of per pixel lookup.
		inline double gaussianLookupCPU(float *data, const long i_lX, const long i_lY, const unsigned long i_ulWidth, const unsigned long i_ulHeight) const
		{
			long startX=-2;//The centre is at 0.5
			long startY=-2;//The centre is at 0.5

			long endX=3;//The centre is at 0.5
			long endY=3;//The centre is at 0.5

			const float *gaussian=((float *)(m_gausDistSqH2LImage->data()));

			if ((startX+i_lX)<0) startX=-i_lX;
			if ((startY+i_lY)<0) startY=-i_lY;

			if ((endX+i_lX)>((long)i_ulWidth-1)) endX=(long)i_ulWidth-1-i_lX;
			if ((endY+i_lY)>((long)i_ulHeight-1)) endY=(long)i_ulHeight-1-i_lY;

			data+=((startX+i_lX)+(startY+i_lY)*i_ulWidth);

			double gSum=0.0;
			double gSumReference=0.0;
			for (long y=startY; y<=endY; y++)
			{
				const double ySqr=(y-0.5)*(y-0.5);

				for (long x=startX; x<=endX; x++)
				{
					const double radiusSq=(x-0.5)*(x-0.5)+ySqr;
                    const unsigned long index=std::min<unsigned long>(((unsigned long)radiusSq),7);
					gSum+=(*data)*gaussian[index<<2];
					data++;
					gSumReference+=gaussian[index<<2];
				}
				data+=(i_ulWidth-(endX-startX+1));
			}
			gSum/=gSumReference;

			return gSum;
		}

		inline double gaussianLookupSymCPU(float *data, const long i_lX, const long i_lY, const unsigned long i_ulWidth, const unsigned long i_ulHeight) const
		{
			long startX=-2;//The centre is at 0.0
			long startY=-2;//The centre is at 0.0

			long endX=2;//The centre is at 0.0
			long endY=2;//The centre is at 0.0

			const float *gaussian=((float *)(m_gausDistSqImage->data()));

			if ((startX+i_lX)<0) startX=-i_lX;
			if ((startY+i_lY)<0) startY=-i_lY;

			if ((endX+i_lX)>((long)i_ulWidth-1)) endX=(long)i_ulWidth-1-i_lX;
			if ((endY+i_lY)>((long)i_ulHeight-1)) endY=(long)i_ulHeight-1-i_lY;

			data+=((startX+i_lX)+(startY+i_lY)*i_ulWidth);

			double gSum=0.0;
			double gSumReference=0.0;
			for (long y=startY; y<=endY; y++)
			{
				const double ySqr=(y-0.0)*(y-0.0);

				for (long x=startX; x<=endX; x++)
				{
					const double radiusSq=(x-0.0)*(x-0.0)+ySqr;
                    const unsigned long index=std::min<unsigned long>(((unsigned long)radiusSq),5);
					gSum+=(*data)*gaussian[index<<2];
					data++;
					gSumReference+=gaussian[index<<2];
				}
				data+=(i_ulWidth-(endX-startX+1));
			}
			gSum/=gSumReference;

			return gSum;
		}


		//calcSSD is by far the most expensive operation of the brute force windowed ssd minimise algorithm!!!
		inline double calcSSDCPU(const osg::Image *imageCurrent, const osg::Image *imagePrevious, double i_dOffsetFromPreviousX, double i_dOffsetFromPreviousY) const
		{
			double ssd=0.0;
			unsigned long numSamples=0;

			//=== Calculate offsetXFrac and offsetYFrac for bi-linear filtering ===//
				long offsetFromPreviousX=(long)i_dOffsetFromPreviousX;
				long offsetFromPreviousY=(long)i_dOffsetFromPreviousY;	
/*
				double offsetXFrac=i_dOffsetFromPreviousX-offsetFromPreviousX;
				if (offsetXFrac<0.0)
				{//This ensures that offsetXFrac for bilinear filtering is always positive!
					offsetXFrac+=1.0;
					offsetFromPreviousX--;
				}
				double offsetYFrac=i_dOffsetFromPreviousY-offsetFromPreviousY;
				if (offsetYFrac<0.0)
				{//This ensures that offsetYFrac for bilinear filtering is always positive!
					offsetYFrac+=1.0;
					offsetFromPreviousY--;
				}
*/
			//=====================================================================//

			long width=imageCurrent->s();
			long height=imageCurrent->t();

            long startY=std::max<long>(0, -offsetFromPreviousY);
            long startX=std::max<long>(0, -offsetFromPreviousX);

            long endY=std::min<long>(height, height-offsetFromPreviousY);
            long endX=std::min<long>(height, width-offsetFromPreviousX);

			for (long y=startY; y<endY; y++)
			{
				const float* data=(float *)(imageCurrent->data()+(startX+y*width)*4);
				const float* previousData=(float *)(imagePrevious->data()+(startX+offsetFromPreviousX+(y+offsetFromPreviousY)*width)*4);

				for (long x=startX; x<endX; x++)
				{
					double diff=((long) *data) - ((long) *previousData);
					data++;
					previousData++;

					ssd+=diff*diff;

					numSamples++;
				}
			}

			if (numSamples>0)
			{
				return ssd/numSamples;// 'NB: /numSamples' to compensate for larger shifts having smaller overlaps and therefore a smaller SSD.
			} else
			{
				return DBL_MAX;
			}
		}

		//Get bilinearly interpolated lookup.
		inline double bilinearLookupCPU(unsigned long i_ulPyramidNum, const unsigned long i_ulLevel, const osg::Vec2d &i_xy) const
		{
			const osg::Image *image=m_imageGausPyramid[i_ulPyramidNum][i_ulLevel].get();
			const float *iData=(float *)image->data();
			const unsigned long width=image->s();
			const unsigned long height=image->t();
			bool outOfBoundsX=false, outOfBoundsY=false;

			//=== Calculate offsetXFrac and offsetYFrac for bi-linear filtering ===//
				long offsetX=(long)i_xy.x();
				long offsetY=(long)i_xy.y();	

				double offsetXFrac=i_xy.x()-offsetX;
				if (offsetXFrac<0.0)
				{//This ensures that offsetXFrac for bilinear filtering is always positive!
					offsetXFrac+=1.0;
					offsetX--;
				}
				double offsetYFrac=i_xy.y()-offsetY;
				if (offsetYFrac<0.0)
				{//This ensures that offsetYFrac for bilinear filtering is always positive!
					offsetYFrac+=1.0;
					offsetY--;
				}

			//=====================================================================//
			

			//=== Do bounds check on image for correct result around image edges and clamped result outside of image ===//
			if (offsetX<0)
			{
				offsetX=0;
				outOfBoundsX=true;
			}
			if (offsetX>=((long)width-1))
			{
				offsetX=(width-1);
				outOfBoundsX=true;
			}

			if (offsetY<0)
			{
				offsetY=0;
				outOfBoundsY=true;
			}
			if (offsetY>=((long)height-1))
			{
				offsetY=(height-1);
				outOfBoundsY=true;
			}
			//==========================================================================================================//


			iData+=((offsetX+offsetY*width));
			const double dA=iData[0];
			const double dB=(outOfBoundsX) ? dA : iData[(1)];

			const double dC=(outOfBoundsY) ? dA : iData[(0+width)];
			const double dD=(outOfBoundsY) ? dB : ((outOfBoundsX) ? dC : iData[(1+width)]);

			return ((dB-dA)*offsetXFrac+dA)*(1.0-offsetYFrac)+((dD-dC)*offsetXFrac+dC)*offsetYFrac;
		}

		void bilinearResample(unsigned long i_ulPyramidNum, unsigned long i_ulLevelNum, osg::Vec2d i_pixelOffset, float *i_dpResampPreviousImg);
	};
}

#endif //IMAGE_N_PYRAMID_H
