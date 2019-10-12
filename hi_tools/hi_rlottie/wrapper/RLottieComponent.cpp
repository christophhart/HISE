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

void RLottieComponent::loadAnimation(const String& jsonCode)
{
	currentAnimation = new RLottieAnimation(manager, jsonCode);
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
		currentAnimation->render(g, currentFrame, { 0, 0 });
}

void RLottieComponent::setBackgroundColour(Colour c)
{
	bgColour = c;

	setOpaque(c.getAlpha() == 0xff);
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