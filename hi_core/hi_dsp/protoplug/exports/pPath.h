/*
  ==============================================================================

    exPath.h
    Created: 25 Feb 2014 7:37:52pm
    Author:  pac

  ==============================================================================
*/

#pragma once

#include "typedefs.h"


PROTO_API pPath Path_new()
{
	pPath p = {new Path()};
	return p;
}

PROTO_API void Path_delete(pPath p)
{
	delete p.p;
}


PROTO_API exRectangle_float Path_getBounds(pPath self)
{
	return exRectangle_float(self.p->getBounds());
}

PROTO_API exRectangle_float Path_getBoundsTransformed (pPath self, exAffineTransform transform)
{
	return exRectangle_float(self.p->getBoundsTransformed (transform.toJuceAff()));
}

PROTO_API bool Path_contains (pPath self, float x, float y,
                   float tolerance = 1.0f)
{
	return self.p->contains (x, y,
                   tolerance);
}

PROTO_API bool Path_contains2 (pPath self, exPoint_float point,
                   float tolerance = 1.0f)
{
	return self.p->contains (point.toJucePoint(),
                   tolerance);
}

PROTO_API bool Path_intersectsLine (pPath self, exLine_float line,
                         float tolerance = 1.0f)
{
	return self.p->intersectsLine (line.toJuceLine(),
                         tolerance);
}

PROTO_API exLine_float Path_getClippedLine (pPath self, exLine_float line, bool keepSectionOutsidePath)
{
	return exLine_float(self.p->getClippedLine (line.toJuceLine(), keepSectionOutsidePath));
}

PROTO_API float Path_getLength (pPath self, exAffineTransform transform)
{
	return self.p->getLength (transform.toJuceAff());
}

PROTO_API exPoint_float Path_getPointAlongPath (pPath self, float distanceFromStart,
                                    exAffineTransform transform)
{
	return exPoint_float(self.p->getPointAlongPath (distanceFromStart,
                                    transform.toJuceAff()));
}

PROTO_API float Path_getNearestPoint (pPath self, exPoint_float targetPoint,
                           exPoint_float &pointOnPath,
                           exAffineTransform transform)
{
	Point<float> p;
	float f = self.p->getNearestPoint (targetPoint.toJucePoint(), p,
                           transform.toJuceAff());
	pointOnPath.x = p.x;
	pointOnPath.y = p.y;
	return f;
}

PROTO_API void Path_clear(pPath self)
{
	self.p->clear();
}

PROTO_API void Path_startNewSubPath (pPath self, float startX, float startY)
{
	self.p->startNewSubPath (startX, startY);
}

PROTO_API void Path_startNewSubPath2 (pPath self, exPoint_float start)
{
	self.p->startNewSubPath (start.toJucePoint());
}

PROTO_API void Path_closeSubPath(pPath self)
{
	self.p->closeSubPath();
}

PROTO_API void Path_lineTo (pPath self, float endX, float endY)
{
	self.p->lineTo (endX, endY);
}

PROTO_API void Path_lineTo2 (pPath self, exPoint_float end)
{
	self.p->lineTo (end.toJucePoint());
}

PROTO_API void Path_quadraticTo (pPath self, float controlPointX,
                      float controlPointY,
                      float endPointX,
                      float endPointY)
{
	self.p->quadraticTo (controlPointX,
                      controlPointY,
                      endPointX,
                      endPointY);
}

PROTO_API void Path_quadraticTo2 (pPath self, exPoint_float controlPoint,
                      exPoint_float endPoint)
{
	self.p->quadraticTo (controlPoint.toJucePoint(),
                      endPoint.toJucePoint());
}

PROTO_API void Path_cubicTo (pPath self, float controlPoint1X,
                  float controlPoint1Y,
                  float controlPoint2X,
                  float controlPoint2Y,
                  float endPointX,
                  float endPointY)
{
	self.p->cubicTo (controlPoint1X,
                  controlPoint1Y,
                  controlPoint2X,
                  controlPoint2Y,
                  endPointX,
                  endPointY);
}

PROTO_API void Path_cubicTo2 (pPath self, exPoint_float controlPoint1,
                  exPoint_float controlPoint2,
                  exPoint_float endPoint)
{
	self.p->cubicTo (controlPoint1.toJucePoint(),
                  controlPoint2.toJucePoint(),
                  endPoint.toJucePoint());
}

PROTO_API exPoint_float Path_getCurrentPosition(pPath self)
{
	return exPoint_float(self.p->getCurrentPosition());
}

PROTO_API void Path_addRectangle (pPath self, float x, float y, float width, float height)
{
	self.p->addRectangle (x, y, width, height);
}

PROTO_API void Path_addRectangle2 (pPath self, exRectangle_float rectangle)
{
	self.p->addRectangle (rectangle.toJuceRect());
}

PROTO_API void Path_addRoundedRectangle2 (pPath self, float x, float y, float width, float height,
                              float cornerSize)
{
	self.p->addRoundedRectangle (x, y, width, height,
                              cornerSize);
}

PROTO_API void Path_addRoundedRectangle3 (pPath self, float x, float y, float width, float height,
                              float cornerSizeX,
                              float cornerSizeY)
{
	self.p->addRoundedRectangle (x, y, width, height,
                              cornerSizeX,
                              cornerSizeY);
}

PROTO_API void Path_addRoundedRectangle4 (pPath self, float x, float y, float width, float height,
                              float cornerSizeX, float cornerSizeY,
                              bool curveTopLeft, bool curveTopRight,
                              bool curveBottomLeft, bool curveBottomRight)
{
	self.p->addRoundedRectangle (x, y, width, height,
                              cornerSizeX, cornerSizeY,
                              curveTopLeft, curveTopRight,
                              curveBottomLeft, curveBottomRight);
}

PROTO_API void Path_addRoundedRectangle5 (pPath self, exRectangle_float rectangle, float cornerSizeX, float cornerSizeY)
{
	self.p->addRoundedRectangle (rectangle.toJuceRect(), cornerSizeX, cornerSizeY);
}

PROTO_API void Path_addRoundedRectangle6 (pPath self, exRectangle_float rectangle, float cornerSize)
{
	self.p->addRoundedRectangle (rectangle.toJuceRect(), cornerSize);
}

PROTO_API void Path_addTriangle (pPath self, float x1, float y1,
                      float x2, float y2,
                      float x3, float y3)
{
	self.p->addTriangle (x1, y1,
                      x2, y2,
                      x3, y3);
}

PROTO_API void Path_addQuadrilateral (pPath self, float x1, float y1,
                           float x2, float y2,
                           float x3, float y3,
                           float x4, float y4)
{
	self.p->addQuadrilateral (x1, y1,
                           x2, y2,
                           x3, y3,
                           x4, y4);
}

PROTO_API void Path_addEllipse (pPath self, float x, float y, float width, float height)
{
	self.p->addEllipse (x, y, width, height);
}

PROTO_API void Path_addArc (pPath self, float x, float y, float width, float height,
                 float fromRadians,
                 float toRadians,
                 bool startAsNewSubPath = false)
{
	self.p->addArc (x, y, width, height,
                 fromRadians,
                 toRadians,
                 startAsNewSubPath);
}

PROTO_API void Path_addCentredArc (pPath self, float centreX, float centreY,
                        float radiusX, float radiusY,
                        float rotationOfEllipse,
                        float fromRadians,
                        float toRadians,
                        bool startAsNewSubPath = false)
{
	self.p->addCentredArc (centreX, centreY,
                        radiusX, radiusY,
                        rotationOfEllipse,
                        fromRadians,
                        toRadians,
                        startAsNewSubPath);
}

PROTO_API void Path_addPieSegment (pPath self, float x, float y,
                        float width, float height,
                        float fromRadians,
                        float toRadians,
                        float innerCircleProportionalSize)
{
	self.p->addPieSegment (x, y,
                        width, height,
                        fromRadians,
                        toRadians,
                        innerCircleProportionalSize);
}

PROTO_API void Path_addLineSegment (pPath self, exLine_float line, float lineThickness)
{
	self.p->addLineSegment (line.toJuceLine(), lineThickness);
}

PROTO_API void Path_addArrow (pPath self, exLine_float line,
                   float lineThickness,
                   float arrowheadWidth,
                   float arrowheadLength)
{
	self.p->addArrow (line.toJuceLine(),
                   lineThickness,
                   arrowheadWidth,
                   arrowheadLength);
}

PROTO_API void Path_addPolygon (pPath self, exPoint_float centre,
                     int numberOfSides,
                     float radius,
                     float startAngle = 0.0f)
{
	self.p->addPolygon (centre.toJucePoint(),
                     numberOfSides,
                     radius,
                     startAngle);
}

PROTO_API void Path_addStar (pPath self, exPoint_float centre,
                  int numberOfPoints,
                  float innerRadius,
                  float outerRadius,
                  float startAngle = 0.0f)
{
	self.p->addStar (centre.toJucePoint(),
                  numberOfPoints,
                  innerRadius,
                  outerRadius,
                  startAngle);
}

PROTO_API void Path_addBubble (pPath self, exRectangle_float bodyArea,
                    exRectangle_float maximumArea,
                    exPoint_float arrowTipPosition,
                    const float cornerSize,
                    const float arrowBaseWidth)
{
	self.p->addBubble (bodyArea.toJuceRect(),
                    maximumArea.toJuceRect(),
                    arrowTipPosition.toJucePoint(),
                    cornerSize,
                    arrowBaseWidth);
}

PROTO_API void Path_addPath (pPath self, pPath pathToAppend)
{
	self.p->addPath (*pathToAppend.p);
}

PROTO_API void Path_addPath2 (pPath self, pPath pathToAppend,
                  exAffineTransform transformToApply)
{
	self.p->addPath (*pathToAppend.p,
                  transformToApply.toJuceAff());
}

PROTO_API void Path_applyTransform (pPath self, exAffineTransform transform)
{
	self.p->applyTransform (transform.toJuceAff());
}

PROTO_API void Path_scaleToFit (pPath self, float x, float y, float width, float height,
                     bool preserveProportions)
{
	self.p->scaleToFit (x, y, width, height,
                     preserveProportions);
}

PROTO_API exAffineTransform Path_getTransformToScaleToFit (pPath self, float x, float y, float width, float height,
                                              bool preserveProportions,
                                              int justificationType)
{
	return exAffineTransform(self.p->getTransformToScaleToFit (x, y, width, height,
                                              preserveProportions,
                                              justificationType));
}

PROTO_API  exAffineTransform Path_getTransformToScaleToFit2 (pPath self, exRectangle_float area,
                                              bool preserveProportions,
                                              int justificationType)
{
	return exAffineTransform(self.p->getTransformToScaleToFit (area.toJuceRect(),
                                              preserveProportions,
                                              justificationType));
}

PROTO_API pPath Path_createPathWithRoundedCorners (pPath self, float cornerRadius)
{
	pPath p = {new Path()};
	*p.p = self.p->createPathWithRoundedCorners (cornerRadius);
	return p;
}

PROTO_API void Path_setUsingNonZeroWinding (pPath self, bool isNonZeroWinding)
{
	self.p->setUsingNonZeroWinding (isNonZeroWinding);
}

PROTO_API bool Path_isUsingNonZeroWinding(pPath self)
{
	return self.p->isUsingNonZeroWinding();
}

PROTO_API void Path_toString(pPath self, char* dest, int bufSize)
{
	self.p->toString().copyToUTF8(dest, bufSize);
}

PROTO_API void Path_restoreFromString (pPath self, const char *src)
{
	self.p->restoreFromString (StringRef(src));
}