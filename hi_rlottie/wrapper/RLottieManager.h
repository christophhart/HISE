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




/** This class will open the dynamic libraries and close them when it is deleted. 

	In order to use this class, subclass it and provide a folder for each OS where it can find the dynamic library files, then call init() in order to load that library. */
class RLottieManager
{
public:

	virtual ~RLottieManager()
    {
#if HISE_RLOTTIE_DYNAMIC_LIBRARY
        dynLib = nullptr;
#endif
    };

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

	

	/** @internal */
	Lottie_Animation* createAnimation(const String& jsonData);

	/** @internal */
	void destroy(Lottie_Animation* animation);

	/** @internal */
	int getNumFrames(Lottie_Animation* animation);

	/** @internal */
	double getFrameRate(Lottie_Animation* animation);

	friend class RLottieAnimation;

#if HISE_RLOTTIE_DYNAMIC_LIBRARY
    
    /** Just a bunch of aliases for the C function prototypes. */
    using RenderFunction = decltype(lottie_animation_render);
    using CreateFunction = decltype(lottie_animation_from_data);
    using DestroyFunction = decltype(lottie_animation_destroy);
    using GetFrameRateFunction = decltype(lottie_animation_get_framerate);
    using GetNumFramesFunction = decltype(lottie_animation_get_totalframe);
    
    /** @internal */
    RenderFunction* getRenderFunction();
    
    CreateFunction* createFunction = nullptr;
    DestroyFunction* destroyFunction = nullptr;
    RenderFunction* renderFunction = nullptr;
    GetFrameRateFunction* frameRateFunction = nullptr;
    GetNumFramesFunction* numFramesFunction = nullptr;
    
    File getLibFile();
	ScopedPointer<DynamicLibrary> dynLib;
    
#endif

	JUCE_DECLARE_WEAK_REFERENCEABLE(RLottieManager)
};

}
