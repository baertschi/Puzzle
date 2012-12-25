#ifndef PATH2D_H
#define PATH2D_H

/** - If you want to draw on OpenCV matrix, define PATH2D_HAVE_OPENCV and link against:
	{opencv_imgproc.lib, opencv_highgui.lib, opencv_core.lib and zlib.lib}
	- If you want to draw on OpenGL window, define PATH2D_HAVE_OPENGL and link against:
	{opengl32.lib and glu32.lib}
	- On Microsoft Windows platform, always link against {comctl32.lib}

	You can use it from C and C++ using 'cv:drawPath2D' or 
	cv::Path2D path();
	path.<operation you like>();
	path.drawOpenCV();
	
	or draw using opengl buffers:
	path.drawOpenGL();

	see example code under:
	cvpath2ddemo.cpp
*/
#include <vector>

#define BEZIER_RESOLUTION 40

#ifdef PATH2D_HAVE_OPENCV
#include <opencv/cv.h>
#endif

namespace cv {
#ifndef PATH2D_HAVE_OPENCV
    struct Point2f {
        Point2f						(float theX=0, float theY=0) : x(theX), y(theY) {}
        float x, y;
    };
#endif

	class Path2D {
		public:
#ifdef PATH2D_HAVE_OPENGL
			enum OpenGLDrawFlags    {	PATH2D_OPENGL_DRAWPOINTS_3D					= 1,  PATH2D_OPENGL_DRAWPOINTS_USE_LIGHTS		= 2  , PATH2D_OPENGL_POLYGON_TESSELATE			= 4, 
										PATH2D_OPENGL_POLYGON_WINDING_ABS_GEQ_TWO	= 8,  PATH2D_OPENGL_POLYGON_WINDING_NEGATIVE	= 16 , PATH2D_OPENGL_POLYGON_WINDING_NONZERO	= 32, 
										PATH2D_OPENGL_POLYGON_WINDING_ODD			= 64, PATH2D_OPENGL_POLYGON_WINDING_POSITIVE	= 128, PATH2D_OPENGL_POLYGON_VERTEX_TEXCOORD	= 256, 
										PATH2D_OPENGL_POLYGON_VERTEX_COLORS			= 512,PATH2D_OPENGL_FILLED                      = 1024};
#endif
#ifdef PATH2D_HAVE_OPENCV
			Path2D					(const std::vector<cv::Point> &points);         // construct using an opencv-friendly contour
#endif
			Path2D					(float x=0.0f, float y=0.0f);					// construct a new path with a start point
			Path2D&	clear			();												// clear all shape including transform
			void	restart			(float x=0.0f, float y=0.0f);					// clears all and restart from a new point
			void    push_back		(const Point2f &p);								// insert a new point to path
			void	lineTo			(float x, float y);								// make a relative-line to x,y
			void	horiz			(float x);										// make an horizntal line to x steps from this position
			void	vert			(float y);										// make a vertical line to y steps from this position
            void	quadTo			(const Point2f &p2, const Point2f &p3, unsigned int BEZIER_SEGS = BEZIER_RESOLUTION);
																					// draw a quad using this position, absolute (p2 and p3)
            void	curveTo			(const Point2f &p2, const Point2f &p3, const Point2f &p4, unsigned int BEZIER_SEGS = BEZIER_RESOLUTION);
																					// draw a curve using this position, absolute(p2, p3 and p4)
			void    close           ();												// close the shape back to first point
			void    makeIdentity    ();												// clear the transform of this shape
			void    rotate          (float angle=0.0f);								// compose a rotation for this shape transform
			void    scale           (float sx=1.0f, float sy=1.0f);					// compose a scale (x,y) for this shape transform
			void    translate       (float tx=0.0f, float ty=0.0f);					// compose a translation (x,y) for this shape transform
																					// p1 - active point in path p2 - coordinate of first control point p3 - 
																					// coordinate of second control point
																					// p4 - coordinate of second point on the curve
            void	curveToRelative	(const Point2f &p2, const Point2f &p3, const Point2f &p4, unsigned int BEZIER_SEGS = BEZIER_RESOLUTION);
            void	quadToRelative	(const Point2f &p2, const Point2f &p3, unsigned int BEZIER_SEGS = BEZIER_RESOLUTION);
																					// draw a quad using this position, absolute (p2 and p3)
			float*	asFloatXYZArray	(int *xyzArraySize) const;						// get a new array with all shape coordinates as x,y,z triplets. z=0.0f
			Point2f centerPoint     () const;										// the center-of-gravity point
			Point2f bottomLeftPoint () const;										// the bottom-left point of the AABB
			void	print			() const;										// print the data to stderr
			void    getTransform    (float out[3][3]) const;						// get the 3x3 transform matrix (row-major format)
			const std::vector<Point2f>&	getCoords() const;							// get a const handle to the coordinates

#ifdef PATH2D_HAVE_OPENCV
																					// thickness: -1=fills the shape, 0=fill and draw the shape, >=1 draw the shape
																					// Type of the line: 8 (or omitted) 8-connected line. 4 4-connected line. 
																					// CV_AA antialiased line.
			void    drawOpenCV      (cv::Mat &buffer, const Scalar &lineColor, const Scalar &fillColor, int thickness=1, int lineType=8) const;
#endif
#ifdef PATH2D_HAVE_OPENGL
			void    drawOpenGL      (const int		flags=0,						// PATH2D_OPENGL_XXX or'd together
									 const float	*translate=NULL,				// array size 3
									 const float	*rotate=NULL,					// array size 3
									 const float	*scale=NULL,					// array size 3
									 const float	thickness=1,					// line thickness in pixels
									 const float    *color=NULL						// array size 4: rgb and alpha
									) const;
#endif
	protected:
            float	bezierPoint		(float A, float B, float C, float D, int index, unsigned int BEZIER_SEGS=BEZIER_RESOLUTION);
			Point2f					pos, endpos;
			std::vector<Point2f>	coords;
			float					transform[3][3];
		};
#ifdef PATH2D_HAVE_OPENCV
	// thickness: -1=fills the shape, 0=fill and draw the shape, >=1 draw the shape
	void drawPath2D(cv::Mat &buffer, const Path2D &path2d, const Scalar &lineColor, const Scalar &fillColor, int thickness=1, int lineType=8);
#endif
}

#endif
