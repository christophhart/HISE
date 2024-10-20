/*  ===========================================================================
*
*   This file is part of HISE.
*   Copyright 2016 Christoph Hart
*
*   HISE is free software: you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation, either version 3 of the License, or
*   (at your option) any later version.
*
*   HISE is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public License
*   along with HISE.  If not, see <http://www.gnu.org/licenses/>.
*
*   Commercial licenses for using HISE in an closed source project are
*   available on request. Please visit the project's website to get more
*   information about commercial licensing:
*
*   http://www.hise.audio/
*
*   HISE is based on the JUCE library,
*   which must be separately licensed for closed source applications:
*
*   http://www.juce.com
*
*   ===========================================================================
*/




#if USE_IPP
#include <ipp.h>

#if IPP_VERSION_MAJOR >= 2021 && IPP_VERSION_MINOR >= 10
#include <ipp/ippcv.h>
#else
#include <ippcv.h>
#endif

#endif

namespace hise {
using namespace juce;

PostGraphicsRenderer::Data::~Data()
{
#if USE_IPP
	if (pBuffer)
		ippsFree(pBuffer);

	if (pSpec)
		ippsFree(pSpec);
#endif
}

void PostGraphicsRenderer::Data::increaseIfNecessary(int minSize)
{
#if USE_IPP
	if (minSize > bufSize)
	{
		if (pBuffer) ippsFree(pBuffer);
		bufSize = minSize;
		pBuffer = ippsMalloc_8u(minSize);
	}
#endif
}

void PostGraphicsRenderer::Data::createPathImage(int width, int height)
{
	if (pathImage.getWidth() != width || pathImage.getHeight() != height)
	{
		pathImage = Image(Image::SingleChannel, width, height, true);
	}
	else
		pathImage.clear({ 0, 0, width, height });
}

bool PostGraphicsRenderer::Data::initGaussianBlur(int kernelSize, float sigma, int width, int height)
{
#if USE_IPP && JUCE_WINDOWS
	auto thisNumPixels = width * height;

	if (thisNumPixels != numPixels || kernelSize != lastKernelSize)
	{
		lastKernelSize = kernelSize;
		numPixels = thisNumPixels;

		IppStatus status = ippStsNoErr;

		IppiSize srcSize = { width, height };

		int pSpecSize, thisBufferSize;
		withoutAlpha.allocate(numPixels * 3, false);

		status = ippiFilterGaussianGetBufferSize(srcSize, kernelSize, ipp8u, 3, &pSpecSize, &thisBufferSize);

		increaseIfNecessary(thisBufferSize);

		if (pSpec)
			ippsFree(pSpec);

		pSpec = (IppFilterGaussianSpec *)ippMalloc(pSpecSize);

		status = ippiFilterGaussianInit(srcSize, kernelSize, sigma, ippBorderRepl, ipp8u, 3, reinterpret_cast<IppFilterGaussianSpec*>(pSpec), static_cast<Ipp8u*>(pBuffer));

		return true;
	}
#endif

	return false;

}

PostGraphicsRenderer::Data::WithoutAlphaConverter::WithoutAlphaConverter(Data& bf_, Image::BitmapData& bd_) :
	bd(bd_),
	bf(bf_)
{
	for (int i = 0; i < bf.numPixels; i++)
	{
		bf.withoutAlpha[i * 3] = bd.data[i * 4];
		bf.withoutAlpha[i * 3 + 1] = bd.data[i * 4 + 1];
		bf.withoutAlpha[i * 3 + 2] = bd.data[i * 4 + 2];
	}
}

PostGraphicsRenderer::Data::WithoutAlphaConverter::~WithoutAlphaConverter()
{
	for (int i = 0; i < bf.numPixels; i++)
	{
		bd.data[i * 4] = bf.withoutAlpha[i * 3];
		bd.data[i * 4 + 1] = bf.withoutAlpha[i * 3 + 1];
		bd.data[i * 4 + 2] = bf.withoutAlpha[i * 3 + 2];
	}
}

PostGraphicsRenderer::PostGraphicsRenderer(DataStack& stackTouse, Image& image, float scaleFactor_) :
	img(image),
	bd(image, Image::BitmapData::readWrite),
	stack(stackTouse),
	scaleFactor(scaleFactor_)
{
	
}

void PostGraphicsRenderer::reserveStackOperations(int numOperationsToAllocate)
{
	int numToAdd = numOperationsToAllocate - stack.size();

	while (--numToAdd >= 0)
		stack.add(new Data());
}

void PostGraphicsRenderer::desaturate()
{
	for (int y = 0; y < bd.height; y++)
	{
		for (int x = 0; x < bd.width; x++)
		{
			Pixel p(bd.getPixelPointer(x, y));

			auto sum = (*p.r / 3 + *p.g / 3 + *p.b / 3);
			*p.r = sum;
			*p.g = sum;
			*p.b = sum;
		}
	}
}

void PostGraphicsRenderer::applyMask(const Path& path, bool invert /*= false*/, bool scale)
{
	auto& bf = getNextData();

	const Path* pathToUse = &path;
	Path other;

	if (scale)
	{
		other = path;
		Rectangle<float> area(0.0f, 0.0f, bd.width, bd.height);
		PathFactory::scalePath(other, area);
		pathToUse = &other;
	}
	else if (scaleFactor != 1.0f)
	{
		other = path;
		//other.applyTransform(AffineTransform::scale(scaleFactor));
		pathToUse = &other;
	}

	bf.createPathImage(bd.width, bd.height);

	Graphics g(bf.pathImage);
	g.setColour(Colours::white);
	g.fillPath(*pathToUse);

	Image::BitmapData pathData(bf.pathImage, Image::BitmapData::readOnly);

	for (int y = 0; y < bd.height; y++)
	{
		for (int x = 0; x < bd.width; x++)
		{
			Pixel p(bd.getPixelPointer(x, y));
			auto ptr = pathData.getPixelPointer(x, y);

			float alpha = (float)*ptr / 255.0f;
			if (invert)
				alpha = 1.0f - alpha;

			*p.r = (uint8)jlimit(0, 255, (int)((float)*p.r * alpha));
			*p.g = (uint8)jlimit(0, 255, (int)((float)*p.g * alpha));
			*p.b = (uint8)jlimit(0, 255, (int)((float)*p.b * alpha));
			*p.a = (uint8)jlimit(0, 255, (int)((float)*p.a * alpha));
		}
	}
}

void PostGraphicsRenderer::addNoise(float noiseAmount)
{
	Random r;

	for (int y = 0; y < bd.height; y++)
	{
		for (int x = 0; x < bd.width; x++)
		{
			Pixel p(bd.getPixelPointer(x, y));

			auto thisNoiseDelta = (r.nextFloat()*2.0f - 1.0f) * noiseAmount;

			auto delta = roundToInt(thisNoiseDelta * 128.0f);

			*p.r = (uint8)jlimit(0, 255, (int)*p.r + delta);
			*p.g = (uint8)jlimit(0, 255, (int)*p.g + delta);
			*p.b = (uint8)jlimit(0, 255, (int)*p.b + delta);
		}
	}
}

void PostGraphicsRenderer::gaussianBlur(int blur)
{
	if(blur != 0)
		stackBlur(blur);

	return;
}

void PostGraphicsRenderer::boxBlur(int blur)
{
	if(blur != 0)
		stackBlur(blur);
}

void PostGraphicsRenderer::stackBlur(int blur)
{
	static constexpr int DownsamplingFactor = 1;
	static constexpr int NumBytesPerPixel = 4;

	if (DownsamplingFactor != 1)
	{
		auto f = img.rescaled(img.getWidth() / DownsamplingFactor, img.getHeight() / DownsamplingFactor, Graphics::ResamplingQuality::lowResamplingQuality);

		gin::applyStackBlurARGB(f, blur / DownsamplingFactor);

		juce::Image::BitmapData srcData(f, juce::Image::BitmapData::readOnly);
		juce::Image::BitmapData dstData(img, juce::Image::BitmapData::writeOnly);

		for (int y = 0; y < f.getHeight(); y++)
		{
			auto yDst = y * DownsamplingFactor;
			auto s = srcData.getLinePointer(y);


			for (int yd = 0; yd < DownsamplingFactor; yd++)
			{
				auto d = dstData.getLinePointer(yDst + yd);

				for (int x = 0; x < f.getWidth(); x++)
				{
					auto xDst = x * DownsamplingFactor;

					for (int xd = 0; xd < DownsamplingFactor; xd++)
					{
						memcpy(d + (xDst + xd)*NumBytesPerPixel, s + x * NumBytesPerPixel, NumBytesPerPixel);
					}
				}
			}
		}
	}
	else
	{
		gin::applyStackBlur(img, blur);
	}

	
}

void PostGraphicsRenderer::applyHSL(float h, float s, float l)
{
	gin::applyHueSaturationLightness(img, h, s, l);
}

void PostGraphicsRenderer::applyGamma(float g)
{
	gin::applyGamma(img, g);
}

void PostGraphicsRenderer::applyGradientMap(ColourGradient g)
{
	gin::applyGradientMap(img, g.getColour(0), g.getColour(1));
}

void PostGraphicsRenderer::applySharpness(int delta)
{
	if (delta > 0)
	{
		for (int i = 0; i < delta; i++)
			gin::applySharpen(img);
	}
	else
	{
		for (int i = 0; i < -delta; i++)
			gin::applySoften(img);
	}
}

void PostGraphicsRenderer::applySepia()
{
	gin::applySepia(img);
}

void PostGraphicsRenderer::applyVignette(float amount, float radius, float falloff)
{
	gin::applyVignette(img, amount, radius, falloff);
}

hise::PostGraphicsRenderer::Data& PostGraphicsRenderer::getNextData()
{
	if (isPositiveAndBelow(stackIndex, stack.size()))
	{
		return *stack[stackIndex++];
	}
	else
	{
		if (stack.size() == 0)
		{
			stack.add(new Data());
			return *stack[0];
		}
		else
			return *stack.getLast();
	}
}

PostGraphicsRenderer::Pixel::Pixel(uint8* ptr) :
	a(ptr + 3),
	r(ptr + 2),
	g(ptr + 1),
	b(ptr)
{

}

void ComponentWithPostGraphicsRenderer::paint(Graphics& g)
{
	if (recursive)
		return;

	if (drawOverParent && getParentComponent() != nullptr)
	{
		ScopedValueSetter<bool> vs(recursive, true);
		img = getParentComponent()->createComponentSnapshot(getBoundsInParent());
	}
	else
	{
		if (img.getWidth() != getWidth() || img.getHeight() != getHeight())
		{
			img = Image(Image::ARGB, getWidth(), getHeight(), true);
		}
		else
		{
			img.clear(getLocalBounds());
		}
	}

	Graphics g2(img);

	paintBeforeEffect(g2);

	PostGraphicsRenderer r(stack, img);
	r.reserveStackOperations(numOps);
	applyPostEffect(r);

	g.drawImageAt(img, 0, 0);
}

void ComponentWithPostGraphicsRenderer::setDrawOverParent(bool shouldDrawOverParent)
{
	drawOverParent = shouldDrawOverParent;
}

void ComponentWithPostGraphicsRenderer::setNumOperations(int numOperations)
{
	numOps = numOperations;
}

}
