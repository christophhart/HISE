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

/** This class will open the dynamic libraries and close them when it is deleted. 

	In order to use this class, subclass it and provide a folder for each OS where it can find the dynamic library files, then call init() in order to load that library. */
class RLottieManager
{
public:

	~RLottieManager() {};

	/** A handy shortcut to keep references to this object. */
	using Ptr = WeakReference<RLottieManager>;

	/** Call this in order to load the dynamic library. It will return a Result if something went wrong. You can call this function multiple times, it will skip the second initialisation. */
	Result init();

	/** Returns the result of the initialisation. */
	Result getInitResult() const { return lastResult; }

protected:

	RLottieManager();

	/** Override this method and return the library folder. */
	virtual File getLibraryFolder() const = 0;

private:

	Result lastResult;

	Result returnInitFailure(const String& s);

	bool initialised = false;

	/** Just a bunch of aliases for the C function prototypes. */
	using RenderFunction = decltype(lottie_animation_render);
	using CreateFunction = decltype(lottie_animation_from_data);
	using DestroyFunction = decltype(lottie_animation_destroy);
	using GetFrameRateFunction = decltype(lottie_animation_get_framerate);
	using GetNumFramesFunction = decltype(lottie_animation_get_totalframe);

	/** @internal */
	Lottie_Animation* createAnimation(const String& jsonData);

	/** @internal */
	void destroy(Lottie_Animation* animation);

	/** @internal */
	RenderFunction* getRenderFunction();

	/** @internal */
	int getNumFrames(Lottie_Animation* animation);

	/** @internal */
	double getFrameRate(Lottie_Animation* animation);

	friend class RLottieAnimation;

	File getLibFile();

	CreateFunction* createFunction = nullptr;
	DestroyFunction* destroyFunction = nullptr;
	RenderFunction* renderFunction = nullptr;
	GetFrameRateFunction* frameRateFunction = nullptr;
	GetNumFramesFunction* numFramesFunction = nullptr;

	ScopedPointer<DynamicLibrary> dynLib;

	JUCE_DECLARE_WEAK_REFERENCEABLE(RLottieManager)
};

}