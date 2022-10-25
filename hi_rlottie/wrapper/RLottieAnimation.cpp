/** Copyright 2019 Christoph Hart

	Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

	The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

	Note: Be aware that the rLottie wrapper files are licensed under a more permissive license than the
	rest of the HISE codebase. The MIT license only applies where stated in the header.
*/

namespace hise {
using namespace juce;


RLottieAnimation::RLottieAnimation(RLottieManager* manager, const String& data)
{
	animation = manager->createAnimation(RLottieComponent::decompressIfBase64(data));
    
#if HISE_RLOTTE_DYNAMIC_LIBRARY
	rf = manager->getRenderFunction();
#endif

	numFrames = manager->getNumFrames(animation);
	frameRate = manager->getFrameRate(animation);
}

RLottieAnimation::~RLottieAnimation()
{
	if (manager != nullptr && animation != nullptr)
		manager->destroy(animation);
}

void RLottieAnimation::setSize(int width, int height)
{
	originalWidth = width;
	originalHeight = height;

	auto newWidth = roundToInt((float)originalWidth * scaleFactor);
	auto newHeight = roundToInt((float)originalHeight * scaleFactor);

	if (newWidth != canvas.getWidth() || newHeight != canvas.getHeight())
	{
		canvas = Image(Image::ARGB, newWidth, newHeight, true);
	}
}

int RLottieAnimation::getNumFrames() const
{
	return numFrames;
}

double RLottieAnimation::getFrameRate() const
{
	return frameRate;
}

void RLottieAnimation::render(Graphics& g, Point<int> topLeft)
{
	if (isValid() && isPositiveAndBelow(currentFrame, numFrames+1) && lastFrame != currentFrame)
	{
		Image::BitmapData bd(canvas, Image::BitmapData::ReadWriteMode::writeOnly);

#if HISE_RLOTTE_DYNAMIC_LIBRARY
		rf(animation, (size_t)currentFrame, reinterpret_cast<uint32*>(bd.data), canvas.getWidth(), canvas.getHeight(), canvas.getWidth() * 4);
#else
        lottie_animation_render(animation, (size_t)currentFrame, reinterpret_cast<uint32*>(bd.data), canvas.getWidth(), canvas.getHeight(), canvas.getWidth() * 4);
#endif

		lastFrame = currentFrame;
	}

	if (scaleFactor == 1.0f)
	{
		g.drawImageAt(canvas, topLeft.x, topLeft.y);
	}
	else
	{
		g.drawImageTransformed(canvas, AffineTransform::scale(1.0f / scaleFactor));
	}
}

bool RLottieAnimation::isValid() const
{
    auto ok = animation != nullptr;
    
#if HISE_RLOTTIE_DYNAMIC_LIBRARY
    ok &= rf != nullptr;
#endif
    
    ok &= canvas.isValid();
    
    return ok;
}

void RLottieAnimation::setFrame(int frameNumber)
{
	currentFrame = jlimit(0, numFrames, frameNumber);
}

int RLottieAnimation::getCurrentFrame() const
{
	return currentFrame;
}

void RLottieAnimation::setScaleFactor(float newScaleFactor)
{
	if (scaleFactor != newScaleFactor)
	{
		scaleFactor = newScaleFactor;

		setSize(originalWidth, originalHeight);
	}
}

}
