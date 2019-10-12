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


RLottieAnimation::RLottieAnimation(RLottieManager* manager, const String& data)
{
	animation = manager->createAnimation(data);
	rf = manager->getRenderFunction();

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
	canvas = Image(Image::ARGB, width, height, true);
}

int RLottieAnimation::getNumFrames() const
{
	return numFrames;
}

double RLottieAnimation::getFrameRate() const
{
	return frameRate;
}

void RLottieAnimation::render(Graphics& g, int frame, Point<int> topLeft)
{
	if (isValid())
	{
		Image::BitmapData bd(canvas, Image::BitmapData::ReadWriteMode::writeOnly);

		rf(animation, (size_t)frame, reinterpret_cast<uint32*>(bd.data), canvas.getWidth(), canvas.getHeight(), canvas.getWidth() * 4);
	}

	g.drawImageAt(canvas, topLeft.x, topLeft.y);
}

bool RLottieAnimation::isValid() const
{
	return rf != nullptr && animation != nullptr;
}

}