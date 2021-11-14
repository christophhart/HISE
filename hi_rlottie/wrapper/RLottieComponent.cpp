/** Copyright 2019 Christoph Hart

	Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

	The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

	Note: Be aware that the rLottie wrapper files are licensed under a more permissive license than the 
	rest of the HISE codebase. The MIT license only applies where stated in the header.
*/

namespace hise {
using namespace juce;

RLottieComponent::RLottieComponent(RLottieManager* manager_) :
	manager(manager_)
{
	setOpaque(true);
}

void RLottieComponent::setFrameNormalised(double normalisedPosition)
{
	if (currentAnimation != nullptr)
	{
		auto frameIndex = roundToInt(normalisedPosition * (double)currentAnimation->getNumFrames());

		currentFrame = frameIndex;
		repaint();
	}
}

void RLottieComponent::stop()
{
	stopTimer();
}

double RLottieComponent::getCurrentFrameNormalised() const
{
	if (currentAnimation != nullptr)
	{
		auto nf = (double)currentAnimation->getNumFrames();

		if (nf > 0.0)
			return (double)currentFrame / nf;
	}

	return 0.0;
}

void RLottieComponent::play()
{
	if (currentAnimation != nullptr)
	{
		auto frameRate = currentAnimation->getFrameRate();

		if (frameRate > 0.0)
			startTimer(1000.0 / frameRate);
	}
}

void RLottieComponent::loadAnimation(const String& jsonCode, bool useOversampling)
{
	currentAnimation = new RLottieAnimation(manager, decompressIfBase64(jsonCode));

	if (useOversampling)
		currentAnimation->setScaleFactor(2.0f);

	currentFrame = 0;

	resized();
	repaint();
}

void RLottieComponent::resized()
{
	if (currentAnimation != nullptr)
	{
		currentAnimation->setSize(getWidth(), getHeight());
	}
}

void RLottieComponent::paint(Graphics& g)
{
	g.fillAll(bgColour);

	if (currentAnimation != nullptr)
	{
		currentAnimation->setFrame(currentFrame);
		currentAnimation->render(g, { 0, 0 });
	}
}

void RLottieComponent::setBackgroundColour(Colour c)
{
	bgColour = c;

	setOpaque(c.getAlpha() == 0xff);
}

juce::String RLottieComponent::decompressIfBase64(const String& s)
{
#if HI_ZSTD_INCLUDED
	if (!s.startsWithChar('{'))
	{
		MemoryBlock mb;

		if (mb.fromBase64Encoding(s))
		{
			String t;
			zstd::ZDefaultCompressor comp;
			comp.expand(mb, t);
			return t;
		}
		else
		{
			// Hmm, it's not a "valid" JSON but no base64 encoded thingie either...
			jassertfalse;
		}
	}

	return s;
#else
	return s;
#endif
}



void RLottieComponent::timerCallback()
{
	if (currentAnimation != nullptr && currentAnimation->getNumFrames() > 0)
	{
		currentFrame = (currentFrame + 1) % currentAnimation->getNumFrames();

		repaint();
	}
}

}