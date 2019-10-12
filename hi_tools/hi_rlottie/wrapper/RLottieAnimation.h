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
/** This object encapsulates a Lottie animation. You can create one of these and control them
    directly or use a RLottieComponent, which wraps this into a readymade JUCE component. 
*/
class RLottieAnimation
{
public:

	/** Creates a new RLottieAnimation from the given JSON data string. */
	RLottieAnimation(RLottieManager* manager, const String& data);

	~RLottieAnimation();

	/** Sets the size of the internal canvas. Call this before rendering. */
	void setSize(int width, int height);

	/** Returns the number of frames in the animation. */
	int getNumFrames() const;

	/** Returns the framerate of the given animation. */
	double getFrameRate() const;
	
	/** Renders the frame of the animation to the Graphics context. */
	void render(Graphics& g, int frame, Point<int> topLeft);

	/** Checks whether the animation could be parsed correctly. */
	bool isValid() const;

private:

	int numFrames = 0;
	double frameRate = 0.0;

	RLottieManager::RenderFunction* rf = nullptr;
	Image canvas;
	Lottie_Animation* animation;
	WeakReference<RLottieManager> manager;
};


}