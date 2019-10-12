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

#pragma once

namespace hise {
using namespace juce;

/** A JUCE component that displays a Lottie animation. */
class RLottieComponent: public Component,
						private Timer
{
public:

	/** Creates a RLottieComponent. You have to pass in the global RLottieManager instance. 
	
		After you've created an instance, call loadAnimation() and then either play() or setFrameNormalised(). */
	RLottieComponent(RLottieManager* manager_);

	/** Sets the frame with a normalised value (0...1). */
	void setFrameNormalised(double normalisedPosition);

	/** Returns the current animation position. */
	double getCurrentFrameNormalised() const;
	
	/** Starts playing the animation with the given framerate. */
	void play();

	/** Stops the playback. */
	void stop();

	/** Load an animation from the JSON code. */
	void loadAnimation(const String& jsonCode);

	/** @internal */
	void resized() override;

	/** @internal */
	void paint(Graphics& g) override;

	/** Sets the background colour. If you pass in a opaque colour it might improve the rendering performance. */
	void setBackgroundColour(Colour c);

private:

	/** @internal */
	void timerCallback() override;

	Colour bgColour = Colours::black;
	int currentFrame = 0;
	ScopedPointer<RLottieAnimation> currentAnimation;

	WeakReference<RLottieManager> manager;
};

}