/** Copyright 2019 Christoph Hart

	Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

	The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

	Note: Be aware that the rLottie wrapper files are licensed under a more permissive license than the
	rest of the HISE codebase. The MIT license only applies where stated in the header.
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

	using Ptr = WeakReference<RLottieAnimation>;

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
	void render(Graphics& g, Point<int> topLeft);

	/** Checks whether the animation could be parsed correctly. */
	bool isValid() const;

	/** Sets the frame. Call this before calling render. */
	void setFrame(int frameNumber);

	/** Returns the current frame. */
	int getCurrentFrame() const;

	/** Set a scale factor that is applied to the internal canvas. */
	void setScaleFactor(float newScaleFactor);

private:

	int originalWidth = 0;
	int originalHeight = 0;
	float scaleFactor = 1.0f;

	int lastFrame = -1;

	int currentFrame = 0;
	int numFrames = 0;
	double frameRate = 0.0;

#if HISE_RLOTTIE_DYNAMIC_LIBRARY
	RLottieManager::RenderFunction* rf = nullptr;
#endif
    
	Image canvas;
	Lottie_Animation* animation;
	WeakReference<RLottieManager> manager;

	JUCE_DECLARE_WEAK_REFERENCEABLE(RLottieAnimation);
};


}
