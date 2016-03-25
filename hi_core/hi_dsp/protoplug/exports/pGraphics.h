/*
  ==============================================================================

    exGraphics.h
    Created: 25 Feb 2014 6:10:20pm
    Author:  pac

  ==============================================================================
*/

#pragma once

#include "typedefs.h"



PROTO_API     pGraphics Graphics_new(pImage imageToDrawOnto) 
{
	pGraphics g = { new Graphics(*imageToDrawOnto.i) };
	return g;
}

PROTO_API void Graphics_delete(pGraphics g)
{ delete g.g; }


PROTO_API     void Graphics_setColour(pGraphics self, exColour newColour) 
{	self.g->setColour(Colour(newColour.c)); }


PROTO_API     void Graphics_setOpacity(pGraphics self, float newOpacity) 
{	self.g->setOpacity(newOpacity); }


PROTO_API     void Graphics_setGradientFill(pGraphics self, const pColourGradient gradient) 
{	self.g->setGradientFill(*gradient.c); }


PROTO_API     void Graphics_setTiledImageFill(pGraphics self, const pImage imageToUse,
                            int anchorX, int anchorY,
                            float opacity) 
{	self.g->setTiledImageFill(*imageToUse.i, anchorX, anchorY, opacity); }


PROTO_API     void Graphics_setFillType(pGraphics self, const pFillType newFill) 
{	self.g->setFillType(*newFill.f); }


PROTO_API     void Graphics_setFont(pGraphics self, const pFont newFont) 
{	self.g->setFont(*newFont.f); }


PROTO_API     void Graphics_setFont2(pGraphics self, float newFontHeight) 
{	self.g->setFont(newFontHeight); }


PROTO_API     pFont Graphics_getCurrentFont(pGraphics self) 
{
	pFont f = {new Font(self.g->getCurrentFont())};
	return f;
}


PROTO_API     void Graphics_drawSingleLineText(pGraphics self, char * text,
                             int startX, int baselineY,
                             int justification) 
{	self.g->drawSingleLineText(text, startX, baselineY, Justification(justification)); }


PROTO_API     void Graphics_drawMultiLineText(pGraphics self, char * text,
                            int startX, int baselineY,
                            int maximumLineWidth) 
{	self.g->drawMultiLineText(text, startX, baselineY, maximumLineWidth); }


PROTO_API     void Graphics_drawText(pGraphics self, char * text,
                   int x, int y, int width, int height,
                   int justificationType,
                   bool useEllipsesIfTooBig) 
{	self.g->drawText(text, x, y, width, height, Justification(justificationType), useEllipsesIfTooBig); }


PROTO_API     void Graphics_drawText2(pGraphics self, char * text,
                   exRectangle_int area,
                   int justificationType,
                   bool useEllipsesIfTooBig) 
{	self.g->drawText(text, area.toJuceRect(), Justification(justificationType), useEllipsesIfTooBig); }


PROTO_API     void Graphics_drawFittedText(pGraphics self, char * text,
                         int x, int y, int width, int height,
                         int justificationFlags,
                         int maximumNumberOfLines,
                         float minimumHorizontalScale = 0.7f) 
{	
	self.g->drawFittedText(text, x, y, width, height, Justification(justificationFlags), 
		maximumNumberOfLines, minimumHorizontalScale);
}


PROTO_API     void Graphics_drawFittedText2(pGraphics self, char * text,
                         exRectangle_int area,
                         int justificationFlags,
                         int maximumNumberOfLines,
                         float minimumHorizontalScale = 0.7f) 
{
	self.g->drawFittedText(text, area.toJuceRect(), Justification(justificationFlags), 
		maximumNumberOfLines, minimumHorizontalScale);
}


PROTO_API     void Graphics_fillAll(pGraphics self) 
{	self.g->fillAll(); }


PROTO_API     void Graphics_fillAll2(pGraphics self, exColour colourToUse) 
{	self.g->fillAll(Colour(colourToUse.c)); }


PROTO_API     void Graphics_fillRect(pGraphics self, exRectangle_int rectangle) 
{	self.g->fillRect(rectangle.toJuceRect()); }


PROTO_API     void Graphics_fillRect2(pGraphics self, exRectangle_float rectangle) 
{	self.g->fillRect(rectangle.toJuceRect()); }


PROTO_API     void Graphics_fillRect3(pGraphics self, int x, int y, int width, int height) 
{	self.g->fillRect(x, y, width, height); }


PROTO_API     void Graphics_fillRect4(pGraphics self, float x, float y, float width, float height) 
{	self.g->fillRect(x, y, width, height); }

/*
PROTO_API     void Graphics_fillRectList(pGraphics self, const RectangleList<float>& rectangles) 
{	self.g->fillRectList(rectangles); }*/


PROTO_API     void Graphics_fillRoundedRectangle(pGraphics self, float x, float y, float width, float height,
                               float cornerSize) 
{	self.g->fillRoundedRectangle(x, y, width, height, cornerSize); }


PROTO_API     void Graphics_fillRoundedRectangle2(pGraphics self, exRectangle_float rectangle,
                               float cornerSize) 
{	self.g->fillRoundedRectangle(rectangle.toJuceRect(), cornerSize); }


PROTO_API     void Graphics_fillCheckerBoard(pGraphics self, exRectangle_int area,
                           int checkWidth, int checkHeight,
                           exColour colour1, exColour colour2) 
{	self.g->fillCheckerBoard(area.toJuceRect(), checkWidth, checkHeight, Colour(colour1.c), Colour(colour2.c)); }


PROTO_API     void Graphics_drawRect(pGraphics self, int x, int y, int width, int height, int lineThickness = 1) 
{	self.g->drawRect(x, y, width, height, lineThickness); }


PROTO_API     void Graphics_drawRect2(pGraphics self, float x, float y, float width, float height, float lineThickness = 1.0f) 
{	self.g->drawRect(x, y, width, height, lineThickness); }


PROTO_API     void Graphics_drawRect3(pGraphics self, exRectangle_int rectangle, int lineThickness = 1) 
{	self.g->drawRect(rectangle.toJuceRect(), lineThickness); }


PROTO_API     void Graphics_drawRect4(pGraphics self, exRectangle_float rectangle, float lineThickness = 1.0f) 
{	self.g->drawRect(rectangle.toJuceRect(), lineThickness); }


PROTO_API     void Graphics_drawRoundedRectangle(pGraphics self, float x, float y, float width, float height,
                               float cornerSize, float lineThickness) 
{	self.g->drawRoundedRectangle(x, y, width, height, cornerSize, lineThickness); }


PROTO_API     void Graphics_drawRoundedRectangle2(pGraphics self, exRectangle_float rectangle,
                               float cornerSize, float lineThickness) 
{	self.g->drawRoundedRectangle(rectangle.toJuceRect(), cornerSize, lineThickness); }


PROTO_API     void Graphics_setPixel(pGraphics self, int x, int y) 
{	self.g->setPixel(x, y); }


PROTO_API     void Graphics_fillEllipse(pGraphics self, float x, float y, float width, float height) 
{	self.g->fillEllipse(x, y, width, height); }


PROTO_API     void Graphics_fillEllipse2(pGraphics self, exRectangle_float area) 
{	self.g->fillEllipse(area.toJuceRect()); }


PROTO_API     void Graphics_drawEllipse(pGraphics self, float x, float y, float width, float height,
                      float lineThickness) 
{	self.g->drawEllipse(x, y, width, height, lineThickness); }


PROTO_API     void Graphics_drawLine(pGraphics self, float startX, float startY, float endX, float endY) 
{	self.g->drawLine(startX, startY, endX, endY); }


PROTO_API     void Graphics_drawLine2(pGraphics self, float startX, float startY, float endX, float endY, float lineThickness) 
{	self.g->drawLine(startX, startY, endX, endY, lineThickness); }


PROTO_API     void Graphics_drawLine3(pGraphics self, exLine_float line) 
{	self.g->drawLine(line.toJuceLine()); }


PROTO_API     void Graphics_drawLine4(pGraphics self, exLine_float line, float lineThickness) 
{	self.g->drawLine(line.toJuceLine(), lineThickness); }


PROTO_API     void Graphics_drawDashedLine(pGraphics self, exLine_float line,
                         const float* dashLengths, int numDashLengths,
                         float lineThickness = 1.0f,
                         int dashIndexToStartFrom = 0) 
{	self.g->drawDashedLine(line.toJuceLine(), dashLengths, numDashLengths, lineThickness, dashIndexToStartFrom); }


PROTO_API     void Graphics_drawVerticalLine(pGraphics self, int x, float top, float bottom) 
{	self.g->drawVerticalLine(x, top, bottom); }


PROTO_API     void Graphics_drawHorizontalLine(pGraphics self, int y, float left, float right) 
{	self.g->drawHorizontalLine(y, left, right); }


PROTO_API     void Graphics_fillPath(pGraphics self, pPath path,
                   exAffineTransform transform) 
{	self.g->fillPath(*path.p, transform.toJuceAff()); }


PROTO_API     void Graphics_strokePath(pGraphics self, pPath path,
                     exPathStrokeType strokeType,
                     exAffineTransform transform) 
{	self.g->strokePath(*path.p, strokeType.toJuceType(), transform.toJuceAff()); }


PROTO_API     void Graphics_drawArrow(pGraphics self, exLine_float line,
                    float lineThickness,
                    float arrowheadWidth,
                    float arrowheadLength) 
{	self.g->drawArrow(line.toJuceLine(), lineThickness, arrowheadWidth, arrowheadLength); }


PROTO_API     void Graphics_setImageResamplingQuality(pGraphics self, const int newQuality) 
{	self.g->setImageResamplingQuality((Graphics::ResamplingQuality)newQuality); }


PROTO_API     void Graphics_drawImageAt(pGraphics self, pImage imageToDraw, int topLeftX, int topLeftY,
                      bool fillAlphaChannelWithCurrentBrush = false) 
{	self.g->drawImageAt(*imageToDraw.i, topLeftX, topLeftY, fillAlphaChannelWithCurrentBrush); }


PROTO_API     void Graphics_drawImage(pGraphics self, pImage imageToDraw,
                    int destX, int destY, int destWidth, int destHeight,
                    int sourceX, int sourceY, int sourceWidth, int sourceHeight,
                    bool fillAlphaChannelWithCurrentBrush = false) 
{	self.g->drawImage(*imageToDraw.i, destX, destY, destWidth, destHeight, sourceX, sourceY, sourceWidth, sourceHeight, fillAlphaChannelWithCurrentBrush); }


PROTO_API     void Graphics_drawImageTransformed(pGraphics self, pImage imageToDraw,
                               exAffineTransform transform,
                               bool fillAlphaChannelWithCurrentBrush = false) 
{	self.g->drawImageTransformed(*imageToDraw.i, transform.toJuceAff(), fillAlphaChannelWithCurrentBrush); }


PROTO_API     void Graphics_drawImageWithin(pGraphics self, pImage imageToDraw,
                          int destX, int destY, int destWidth, int destHeight,
                          int placementWithinTarget,
                          bool fillAlphaChannelWithCurrentBrush = false) 
{
	self.g->drawImageWithin(*imageToDraw.i, destX, destY, destWidth, destHeight, 
		RectanglePlacement(placementWithinTarget), fillAlphaChannelWithCurrentBrush); }


PROTO_API     exRectangle_int Graphics_getClipBounds(pGraphics self) 
{	return exRectangle_int(self.g->getClipBounds()); }


PROTO_API     bool Graphics_clipRegionIntersects(pGraphics self, exRectangle_int area) 
{	return self.g->clipRegionIntersects(area.toJuceRect()); }


PROTO_API     bool Graphics_reduceClipRegion(pGraphics self, int x, int y, int width, int height) 
{	return self.g->reduceClipRegion(x, y, width, height); }


PROTO_API     bool Graphics_reduceClipRegion2(pGraphics self, exRectangle_int area) 
{	return self.g->reduceClipRegion(area.toJuceRect()); }

/*
PROTO_API     bool Graphics_reduceClipRegion3(pGraphics self, const RectangleList<int>& clipRegion) 
{	return self.g->reduceClipRegion(clipRegion); }*/


PROTO_API     bool Graphics_reduceClipRegion4(pGraphics self, pPath path, exAffineTransform transform) 
{	return self.g->reduceClipRegion(*path.p, transform.toJuceAff()); }


PROTO_API     bool Graphics_reduceClipRegion5(pGraphics self, pImage image, exAffineTransform transform) 
{	return self.g->reduceClipRegion(*image.i, transform.toJuceAff()); }


PROTO_API     void Graphics_excludeClipRegion(pGraphics self, exRectangle_int rectangleToExclude) 
{	self.g->excludeClipRegion(rectangleToExclude.toJuceRect()); }


PROTO_API     bool Graphics_isClipEmpty(pGraphics self) 
{	return self.g->isClipEmpty(); }


PROTO_API     void Graphics_saveState(pGraphics self) 
{	self.g->saveState(); }


PROTO_API     void Graphics_restoreState(pGraphics self) 
{	self.g->restoreState(); }


PROTO_API     void Graphics_beginTransparencyLayer(pGraphics self, float layerOpacity) 
{	self.g->beginTransparencyLayer(layerOpacity); }


PROTO_API     void Graphics_endTransparencyLayer(pGraphics self) 
{	self.g->endTransparencyLayer(); }


PROTO_API     void Graphics_setOrigin(pGraphics self, exPoint_int newOrigin) 
{	self.g->setOrigin(newOrigin.toJucePoint()); }


PROTO_API     void Graphics_setOrigin2(pGraphics self, int newOriginX, int newOriginY) 
{	self.g->setOrigin(newOriginX, newOriginY); }


PROTO_API     void Graphics_addTransform(pGraphics self, exAffineTransform transform) 
{	self.g->addTransform(transform.toJuceAff()); }


PROTO_API     void Graphics_resetToDefaultState(pGraphics self) 
{	self.g->resetToDefaultState(); }


PROTO_API     bool Graphics_isVectorDevice(pGraphics self) 
{	return self.g->isVectorDevice(); }

/*
PROTO_API     LowLevelGraphicsContext& Graphics_getInternalContext(pGraphics self) 
{	return self.g->getInternalContext(); }*/
