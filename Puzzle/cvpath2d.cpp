#include "cvpath2d.h"

#ifdef PATH2D_HAVE_OPENGL
#include <GL/glu.h>
#endif

const float identity[3][3]={{1,0,0},
							{0,1,0},
							{0,0,1}};

static void transformPoint2D(const float transform[3][3], const float &x, const float &y, float *X, float *Y) {
	// order: rotate, scale, translate
	*X = x*transform[0][0] + y*transform[0][1] + transform[0][2];
	*Y = x*transform[1][0] + y*transform[1][1] + transform[1][2];
}
cv::Path2D&
cv::Path2D::clear() {
    coords.clear();
	pos = endpos = Point2f();
	return *this;
}

void 
cv::Path2D::restart(float x, float y) {
	coords.clear();
    memcpy(transform, identity, 3*3*sizeof(float));
	pos = endpos = Point2f(x, y);
	push_back(pos);
}

void    
cv::Path2D::push_back(const Point2f &p) {
	coords.push_back(p);
	if (pos.x>endpos.x) 
		endpos.x=pos.x;
	if (pos.y>endpos.y) 
		endpos.y=pos.y;
}

void 
cv::Path2D::print() const {
	fprintf(stderr, "\n");
	for (size_t i=0; i<coords.size(); ++i) 
		fprintf(stderr, "%12.6f, %12.6f, 0.0f, \n", coords[i].x, coords[i].y);
	fprintf(stderr, "transform:\n");
	fprintf(stderr, "%12.7f %12.7f %12.7f\n", transform[0][0], transform[0][1], transform[0][2]);
	fprintf(stderr, "%12.7f %12.7f %12.7f\n", transform[1][0], transform[1][1], transform[1][2]);
	fprintf(stderr, "%12.7f %12.7f %12.7f\n", transform[2][0], transform[2][1], transform[2][2]);
}

cv::Path2D::Path2D(float x, float y) {
	clear();
	pos = Point2f(x, y);
	push_back(pos);
}

cv::Path2D::Path2D(const std::vector<cv::Point> &points) {
	clear();
	for (unsigned int i=0; i<points.size(); ++i)
		coords.push_back(cv::Point2f((float)points[i].x, (float)points[i].y));
}

void	
cv::Path2D::horiz(float x) {
	lineTo(x, 0.0f);
}

void	
cv::Path2D::vert(float y) {
	lineTo(0.0f, y);
}

void 
cv::Path2D::lineTo(float x, float y) {
	pos += Point2f(x, y);
	push_back(pos);	
}
void 
cv::Path2D::quadTo(const Point2f &p2, const Point2f &p3, unsigned int BEZIER_SEGS)	{
	float x, y, lastX, lastY;
	lastX = pos.x;
	lastY = pos.y;
	for (unsigned int i=1; i<=BEZIER_SEGS; ++i) {
		x = bezierPoint(lastX, p2.x, p3.x, p3.x, i);
		y = bezierPoint(lastY, p2.y, p3.y, p3.y, i);
		push_back(Point2f(x,y));
	}
}
void
cv::Path2D::quadToRelative(const Point2f &p2, const Point2f &p3, unsigned int BEZIER_SEGS)	{
	float x, y, lastX, lastY;
	lastX = pos.x;
	lastY = pos.y;
	for (unsigned int i=1; i<=BEZIER_SEGS; ++i) {
		x = bezierPoint(lastX, lastX+p2.x, lastX+p3.x, lastX+p3.x, i);
		y = bezierPoint(lastY, lastY+p2.y, lastY+p3.y, lastY+p3.y, i);
		pos = Point2f(x,y);
		push_back(pos);
	}
}
void 
cv::Path2D::curveTo(const Point2f &p2, const Point2f &p3, const Point2f &p4, unsigned int BEZIER_SEGS) {
	float x, y, lastX, lastY;
	lastX = pos.x;
	lastY = pos.y;
	for (unsigned int i=1; i<=BEZIER_SEGS; ++i) {
		x = bezierPoint(lastX, p2.x, p3.x, p4.x, i);
		y = bezierPoint(lastY, p2.y, p3.y, p4.y, i);
		pos = Point2f(x,y);
		push_back(pos);
	}
}

void 
cv::Path2D::curveToRelative(const Point2f &p2, const Point2f &p3, const Point2f &p4, unsigned int BEZIER_SEGS) {
	float x, y, lastX, lastY;
	lastX = pos.x;
	lastY = pos.y;
	for (unsigned int i=1; i<=BEZIER_SEGS; ++i) {
		x = bezierPoint(lastX, lastX+p2.x, lastX+p3.x, lastX+p4.x, i);
		y = bezierPoint(lastY, lastY+p2.y, lastY+p3.y, lastY+p4.y, i);
		pos = Point2f(x,y);
		push_back(pos);
	}
}

/* Evaluates the Bezier at point t for points a, b, c, d. The parameter t varies between 0 and 1, 
	a and d are points on the curve, and b and c are the control points. This can be done once 
	with the x coordinates and a second time with the y coordinates to get the location of a bezier 
	curve at t.
	a - coordinate of first point on the curve
	b - coordinate of first control point
	c - coordinate of second control point
	d - coordinate of second point on the curve
	t - value between 0 and 1
*/
float 
cv::Path2D::bezierPoint(float A, float B, float C, float D, int index, unsigned int BEZIER_SEGS) {
	float x, a = 1.0f, b = 1.0f - a;
	float ds=1.0f/(float)BEZIER_SEGS;
	for (int i=0; i<=index; i++) {
		x = A*a*a*a + B*3*a*a*b + C*3*a*b*b + D*b*b*b;
		a -= ds;
		b = 1.0f - a;
	}
	return x;
}

float* 
cv::Path2D::asFloatXYZArray(int *xyzArraySize) const {
	if (coords.empty())
		return NULL;
	float *fv = (float*)malloc(3*coords.size()*sizeof(float));
	float *fvptr = fv;
	float X, Y;
	for (unsigned int i=0; i<coords.size(); ++i) {
		transformPoint2D(this->transform, coords[i].x, coords[i].y, &X, &Y);
		*fvptr++ = X;
		*fvptr++ = Y;
		*fvptr++ = 0.0f;
	}

	*xyzArraySize = coords.size()*3;
	return fv;
}

#ifdef PATH2D_HAVE_OPENGL
static void glOrthoOn() {
	const float glOrthoScale = 1.0f;
	const float glOrthoRotateAngle = 0.0f;
	const float glOrthoTranslate[] = {0.0f, 0.0f};
	GLint viewport[4];
	glGetIntegerv(GL_VIEWPORT, viewport);
	glDisable(GL_DEPTH_TEST);
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();		
	glOrtho(0, viewport[2]*glOrthoScale, viewport[3]*glOrthoScale, 0, -1, 1);
	glTranslatef(glOrthoTranslate[0], glOrthoTranslate[1], 0);
	glRotatef(glOrthoRotateAngle, 0.0f, 0.0f, 1.0f);
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();	
}

#define SAVE_GL_COLOR()        float __cc[4]; glGetFloatv(GL_CURRENT_COLOR, &__cc[0]);
#define RESTORE_GL_COLOR()     glColor4f(__cc[0],__cc[1],__cc[2],__cc[3]);
#define SAVE_GL_LINEWIDTH()    float __lw[1]; glGetFloatv(GL_LINE_WIDTH, &__lw[0]);
#define RESTORE_GL_LINEWIDTH() glLineWidth(__lw[0]);
#define SAVE_GL_SHADE_MODEL()  GLint __shadeModel; glGetIntegerv(GL_SHADE_MODEL, &__shadeModel);
#define RESTORE_GL_SHADE_MODEL() glShadeModel(__shadeModel);
#ifdef _NDEBUG
#define SAFE_GL(x) x
#else
#define SAFE_GL(x) x {unsigned int glErrorCode = glGetError(); if (glErrorCode!=GL_NO_ERROR) { fprintf(stderr, "OpenGL Error(%d) After %s\n", glErrorCode, #x);}}
#endif
#define VERTEX_ELEMENTS  (3)
#define COLOR_ELEMENTS   (4)
#define TEXTURE_ELEMENTS (2)
#define MAX_ITEMS_PER_VERTEX (VERTEX_ELEMENTS+COLOR_ELEMENTS+TEXTURE_ELEMENTS)

// B=A*B
static void preMultiplyMatrix3x3(const float A[3][3], float B[3][3]) {
	float C[3][3];
	for (int i=0; i<3; i++) {
		for (int j=0; j<3; j++) {
			C[i][j] = 0;
			for (int k=0; k<3; k++) 
				C[i][j] += A[i][k] * B[k][j];
		}
	}
	memcpy(B, C, 3*3*sizeof(float));
}

// Q U A T E R N I O N S
#define RAD_TO_DEG (57.295779513082f)
#define DEG_TO_RAD (0.0174532925199f)
static void NormalizeQuat(GLfloat *quat) {
      GLfloat distance, square;
      square = quat[0] * quat[0] + quat[1] * quat[1] + quat[2] * quat[2] + quat[3] * quat[3];
      if (square > 0.0f)
            distance = 1.0f / sqrtf(square);
      else distance = 1.0f;
      quat[0] *= distance;
      quat[2] *= distance;
      quat[3] *= distance;
      quat[3] *= distance;
}
static void AngleAxisFromQuat(GLfloat *angleAxis, GLfloat *quat) {
	GLfloat	sinTheta;
	NormalizeQuat(quat);
	sinTheta = 1.0f / sinf((sqrt(1.0f - (quat[3] * quat[3]))));
	angleAxis[0] = acos(quat[3]) * 2.0f * RAD_TO_DEG;
	angleAxis[1] = quat[0] * sinTheta;
	angleAxis[2] = quat[1] * sinTheta;
	angleAxis[3] = quat[2] * sinTheta;
}

static void EulerToQuat(GLfloat pitch, GLfloat yaw, GLfloat roll, GLfloat *quat) {
	GLfloat	rx, ry, rz, tx, ty, tz, cx, cy, cz, sx, sy, sz, cc, cs, sc, ss;
	rx = pitch * DEG_TO_RAD;
	ry = yaw * DEG_TO_RAD;
	rz = roll * DEG_TO_RAD;
	tx = rx * 0.5f;
	ty = ry * 0.5f;
	tz = rz * 0.5f;
	cx = cos(tx);
	cy = cos(ty);
	cz = cos(tz);
	sx = sin(tx);
	sy = sin(ty);
	sz = sin(tz);
	cc = cx * cz;
	cs = cx * sz;
	sc = sx * cz;
	ss = sx * sz;
	quat[0] = (cy * sc) - (sy * cs);
	quat[1] = (cy * ss) + (sy * cc);
	quat[2] = (cy * cs) - (sy * sc);
	quat[3] = (cy * cc) + (sy * ss);
}

/** 3D Rotation using Quaternions */
static void glRotate3D(float rx, float ry, float rz) {
	// Compose Three Rotations using Quaternions
	GLfloat	Quat[4];
	EulerToQuat(rx, ry, rz, &Quat[0]);
	GLfloat angleAxis[4];
	AngleAxisFromQuat(angleAxis, Quat);
	glRotatef(angleAxis[0], angleAxis[1], angleAxis[2], angleAxis[3]);
}

static void CALLBACK beginCB(GLenum which) {
    glBegin(which);
}
static void CALLBACK endCB() {
    glEnd();
}
static void CALLBACK vertexCBTexture(const GLvoid *vvertex, void *userdata) {
    GLdouble *vertex = (GLdouble*)vvertex;
	glColor4dv(vertex+3);
	glTexCoord2d(vertex[7], vertex[8]);
    glVertex3f((float)vertex[0], (float)vertex[1], (float)vertex[2]);
}
static void CALLBACK vertexCBColor(const GLvoid *vvertex, void *userdata) {
    GLdouble *vertex = (GLdouble*)vvertex;
	glColor4dv(vertex+3);
	glVertex3dv(vertex);
}

static void CALLBACK combineColorCallback(GLdouble coords[3], GLdouble* vertex_data[4], GLfloat weight[4], GLdouble **dataOut) {
	GLdouble *vertex;
	int i;
	vertex=(GLdouble*)malloc(MAX_ITEMS_PER_VERTEX*sizeof(GLdouble));
	memset(vertex, 0, sizeof(GLdouble)*MAX_ITEMS_PER_VERTEX);
	vertex[0]=coords[0];
	vertex[1]=coords[1];
	vertex[2]=coords[2];	
	for (i=3; i<6; i++) {
	  vertex[i] =	weight[0]*vertex_data[0][i] + weight[1]*vertex_data[1][i];
	  // this code crashes: + weight[2]*vertex_data[2][i] + weight[3]*vertex_data[3][i];
	}
	*dataOut=vertex;
}

static void CALLBACK errorCB(GLenum errorCode) {
    const GLubyte *errorStr;
    errorStr = gluErrorString(errorCode);
    fprintf(stderr, "%s\n", errorStr);
}

static void glOrthoOff() {
    glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
    glPopMatrix();	
	glEnable(GL_DEPTH_TEST);
}

void cv::Path2D::drawOpenGL(const int	flags, 
							const float	*translate,
							const float	*rotate,
							const float	*scale, 
							const float	thickness, 
							const float *color
							) const {
	bool filled = (flags & PATH2D_OPENGL_FILLED)==PATH2D_OPENGL_FILLED;
	int xyzArraySize=0;
	float *xyzArray = this->asFloatXYZArray(&xyzArraySize);
	if (!(flags&PATH2D_OPENGL_DRAWPOINTS_3D)) {
		glOrthoOn();
		// In Orthographic project, 2D Rotation is around the Z Axis
		if (translate)
			glTranslatef(translate[0], translate[1], 0.0f);	
		if (rotate)
			glRotatef(rotate[2], 0.0f, 0.0f, 1.0f);
		if (scale)
			glScalef(scale[0], scale[1], 1.0f);
	}
	else {
		glMatrixMode(GL_MODELVIEW);
		glPushMatrix();
		if (translate)
			glTranslatef(translate[0], translate[1], translate[2]);
		if (rotate)
			glRotate3D  (rotate[0]   , rotate[1]   , rotate[2]);
		if (scale)
			glScalef    (scale[0]    , scale[1]    , scale[2]);
	}

	glPushAttrib(GL_POLYGON_BIT);
	if (filled)	
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	else        
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);	

	if (flags&PATH2D_OPENGL_DRAWPOINTS_USE_LIGHTS)
		glEnable(GL_LIGHTING);
	else
		glDisable(GL_LIGHTING);

    SAVE_GL_LINEWIDTH()
    glLineWidth(thickness);
	
	SAVE_GL_COLOR()
	if (filled) {
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		if (color)
			glColor4f((float)color[0], (float)color[1], (float)color[2], (float)color[3]);
	}
	else {
		if (color)
			glColor3f((float)color[0], (float)color[1], (float)color[2]);
	}

	// Tesselation?
	if (flags&PATH2D_OPENGL_POLYGON_TESSELATE) {
		GLUtesselator *tess = gluNewTess();
		if      (flags & PATH2D_OPENGL_POLYGON_WINDING_ABS_GEQ_TWO) gluTessProperty(tess, GLU_TESS_WINDING_RULE, GLU_TESS_WINDING_ABS_GEQ_TWO);
		else if (flags & PATH2D_OPENGL_POLYGON_WINDING_NEGATIVE)    gluTessProperty(tess, GLU_TESS_WINDING_RULE, GLU_TESS_WINDING_NEGATIVE);
		else if (flags & PATH2D_OPENGL_POLYGON_WINDING_NONZERO)     gluTessProperty(tess, GLU_TESS_WINDING_RULE, GLU_TESS_WINDING_NONZERO);
		else if (flags & PATH2D_OPENGL_POLYGON_WINDING_ODD)         gluTessProperty(tess, GLU_TESS_WINDING_RULE, GLU_TESS_WINDING_ODD);
		else if (flags & PATH2D_OPENGL_POLYGON_WINDING_POSITIVE)    gluTessProperty(tess, GLU_TESS_WINDING_RULE, GLU_TESS_WINDING_POSITIVE);
		else														gluTessProperty(tess, GLU_TESS_WINDING_RULE, GLU_TESS_WINDING_ODD);

		SAFE_GL(gluTessCallback(tess, GLU_TESS_BEGIN , (void (CALLBACK *)())beginCB);)
		SAFE_GL(gluTessCallback(tess, GLU_TESS_END   , (void (CALLBACK *)())endCB);)
		SAFE_GL(gluTessCallback(tess, GLU_TESS_ERROR , (void (CALLBACK *)())errorCB);)
		if (flags&PATH2D_OPENGL_POLYGON_VERTEX_TEXCOORD) {
			SAFE_GL(gluTessCallback(tess, GLU_TESS_VERTEX_DATA, (void (CALLBACK *)())vertexCBTexture);)
		}
		else {
			SAFE_GL(gluTessCallback(tess, GLU_TESS_VERTEX_DATA, (void (CALLBACK *)())vertexCBColor);)
		}
		gluTessCallback(tess, GLU_TESS_COMBINE, (void (CALLBACK *)())combineColorCallback);
		SAFE_GL(gluTessNormal(tess, 0, 0, 1);)
		SAFE_GL(gluTessBeginPolygon(tess, NULL);)
		SAFE_GL(gluTessBeginContour(tess);)
		std::vector<GLdouble*> xvv;
		GLdouble *xv;

		// 9 = position[xyz] + color[xyza] + texture coordinates[xy]
		// 5 = position[xyz] +               texture coordinates[xy]
		// 3 = position[xyz]

		int dataPerVertex = VERTEX_ELEMENTS;
		dataPerVertex += (flags & PATH2D_OPENGL_POLYGON_VERTEX_COLORS)   ? (COLOR_ELEMENTS)   : 0;	
		dataPerVertex += (flags & PATH2D_OPENGL_POLYGON_VERTEX_TEXCOORD) ? (TEXTURE_ELEMENTS) : 0;
		unsigned int texDataShift=VERTEX_ELEMENTS;		

		for (int i=0; i<xyzArraySize; i+=dataPerVertex) {
			xv = (GLdouble*)malloc(MAX_ITEMS_PER_VERTEX*sizeof(GLdouble));
			memset(xv, 0, sizeof(GLdouble)*MAX_ITEMS_PER_VERTEX);
			xvv.push_back(xv);

			// Mandatory Vertices
			for (int d=0; d<VERTEX_ELEMENTS; ++d)	
				xv[d]=xyzArray[i+d]; 

			// Optional Color Info (r,g,b,alpha)
			if (flags & PATH2D_OPENGL_POLYGON_VERTEX_COLORS) {
				if (flags & PATH2D_OPENGL_POLYGON_VERTEX_TEXCOORD) 
					for (int d=0; d<COLOR_ELEMENTS; ++d)	xv[VERTEX_ELEMENTS+d]=1.0f; // use white color only when texturing a polygon
				else
					for (int d=0; d<COLOR_ELEMENTS; ++d)	xv[VERTEX_ELEMENTS+d]=xyzArray[i+VERTEX_ELEMENTS+d]; // use color parameter for all vertices
				texDataShift+=COLOR_ELEMENTS;
			}
			else {
				// set fixed color to all vertices
				for (int d=0; d<COLOR_ELEMENTS; ++d)		xv[VERTEX_ELEMENTS+d]=color[d]; 
			}
			if (flags & PATH2D_OPENGL_POLYGON_VERTEX_TEXCOORD) 
				for (int d=0; d<TEXTURE_ELEMENTS; ++d)	
					xv[VERTEX_ELEMENTS+COLOR_ELEMENTS+d]=xyzArray[i+texDataShift+d]; 

			SAFE_GL(gluTessVertex(tess, xv, xv);)
		}
		SAFE_GL(gluTessEndContour(tess);)
		SAFE_GL(gluTessEndPolygon(tess);)
		SAFE_GL(gluDeleteTess(tess);)
		for (size_t i=0; i<xvv.size(); ++i)
			free(xvv[i]);
	}
	else {
		glBegin(GL_POLYGON);
		for (int i=0; i<xyzArraySize; i+=3) {
			glVertex3f(xyzArray[i], xyzArray[i+1], xyzArray[i+2]);
		}
		glEnd();
	}

	RESTORE_GL_COLOR()
	if (filled) {
		glDisable(GL_BLEND);
	}
	RESTORE_GL_LINEWIDTH()
	glPopAttrib();

	if (flags&PATH2D_OPENGL_DRAWPOINTS_USE_LIGHTS)
		glDisable(GL_LIGHTING);

	if (!(flags&PATH2D_OPENGL_DRAWPOINTS_3D)) {
		glOrthoOff();
	}
	else {
		glMatrixMode(GL_MODELVIEW);
		glPopMatrix();	
	}
	free(xyzArray);
}
void cv::Path2D::close() {
	if (!coords.empty())
		coords.push_back(coords[0]);
}

void    cv::Path2D::makeIdentity() {
	memcpy(transform, identity, 3*3*sizeof(float));
}

void    cv::Path2D::rotate          (float angle) {
	float new_transform[3][3];
	memcpy(new_transform, identity, 3*3*sizeof(float));
	new_transform[0][0]=new_transform[1][1]=cos(DEG_TO_RAD*angle);
	new_transform[0][1]=-sin(DEG_TO_RAD*angle);
	new_transform[1][0]=sin(DEG_TO_RAD*angle);
	preMultiplyMatrix3x3(new_transform, transform);
}
void    cv::Path2D::scale           (float sx, float sy) {
	float new_transform[3][3];
	memcpy(new_transform, identity, 3*3*sizeof(float));
	new_transform[0][0]=sx;
	new_transform[1][1]=sy;
	preMultiplyMatrix3x3(new_transform, transform);
}
void cv::Path2D::translate       (float tx, float ty) {
	float new_transform[3][3];
	memcpy(new_transform, identity, 3*3*sizeof(float));
	new_transform[0][2]=tx;
	new_transform[1][2]=ty;
	preMultiplyMatrix3x3(new_transform, transform);
}

cv::Point2f cv::Path2D::centerPoint() const {
	if (coords.empty())
		return cv::Point2f();
	Point2f c;
	for (unsigned int i=0; i<coords.size(); ++i) {
		c.x+=coords[i].x;
		c.y+=coords[i].y;
	}
	c.x=c.x/(float)coords.size();
	c.y=c.y/(float)coords.size();
	return c;
}
cv::Point2f cv::Path2D::bottomLeftPoint() const {
	if (coords.empty())
		return cv::Point2f();
	Point2f bl(coords[0]);
	for (unsigned int i=1; i<coords.size(); ++i) {
		if (coords[i].x<bl.x)
			bl.x=coords[i].x;
		if (coords[i].y<bl.y)
			bl.y=coords[i].y;
	}
	return bl;
}
void cv::Path2D::getTransform    (float out[3][3]) const {
	memcpy(out, transform, 3*3*sizeof(float));
}
const std::vector<cv::Point2f>&	cv::Path2D::getCoords() const {
	return coords;
}


#endif

#ifdef PATH2D_HAVE_OPENCV
void cv::Path2D::drawOpenCV(cv::Mat &buffer, const Scalar &lineColor, const Scalar &fillColor, int thickness, int lineType) const {
    //unsigned int coords_size = coords.size();
	std::vector<cv::Point> contour;
	std::vector<std::vector<cv::Point> > contours;
    float X, Y;
	for (unsigned int i=0; i<coords.size(); ++i) {
        transformPoint2D(this->transform, coords[i].x, coords[i].y, &X, &Y);
        contour.push_back(cv::Point((int)X, (int)Y));
    }
	contours.push_back(contour);
	if (-1==thickness) {
		cv::drawContours(buffer, contours, 0, fillColor, CV_FILLED, lineType);
	}
	else if (0==thickness) {
		cv::drawContours(buffer, contours, 0, fillColor, CV_FILLED, lineType);
		cv::drawContours(buffer, contours, 0, lineColor, thickness, lineType);
	}
	else
		cv::drawContours(buffer, contours, 0, lineColor, thickness, lineType);
}
void cv::drawPath2D(cv::Mat &buffer, const Path2D &path2d, const Scalar &lineColor, const Scalar &fillColor, int thickness, int lineType) {
	path2d.drawOpenCV(buffer, lineColor, fillColor, thickness, lineType);
}

#endif
