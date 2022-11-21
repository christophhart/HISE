/** Copyright 2019 Christoph Hart

	Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

	The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

	Note: Be aware that the rLottie wrapper files are licensed under a more permissive license than the
	rest of the HISE codebase. The MIT license only applies where stated in the header.
*/


namespace hise {
using namespace juce;


RLottieManager::RLottieManager():
	lastResult(Result::fail("This Manager is not initialised. Call init() before using it"))
{
	
}

juce::Result RLottieManager::init()
{
	lastResult = Result::ok();

#if HISE_RLOTTIE_DYNAMIC_LIBRARY
	if (dynLib != nullptr)
		return Result::ok();

	auto file = getLibFile();

	if (!file.existsAsFile())
	{
		return returnInitFailure("The file " + file.getFullPathName() + " can't be found.");
	}

	dynLib = new DynamicLibrary();

	auto ok = dynLib->open(file.getFullPathName());
	
	if (!ok)
		return returnInitFailure("The file exists, but the rLottie library can't be loaded correctly");

	if (auto cf = dynLib->getFunction("lottie_animation_from_data"))
	{
		createFunction = (CreateFunction*)cf;
	}
	else
		return returnInitFailure("The init function can't be found. Make sure you've build the library sucessfully.");

	if (auto df = dynLib->getFunction("lottie_animation_destroy"))
	{
		destroyFunction = (DestroyFunction*)df;
	}
	else
		return returnInitFailure("The destroy function can't be found. Make sure you've build the library sucessfully.");

	if (auto rf = dynLib->getFunction("lottie_animation_render"))
	{
		renderFunction = (RenderFunction*)rf;
	}
	else
		return returnInitFailure("The render function can't be found. Make sure you've build the library sucessfully.");

	if (auto ff = dynLib->getFunction("lottie_animation_get_framerate"))
	{
		frameRateFunction = (GetFrameRateFunction*)ff;
	}
	else
		return returnInitFailure("The framerate function can't be found. Make sure you've build the library sucessfully.");

	if (auto nf = dynLib->getFunction("lottie_animation_get_totalframe"))
	{
		numFramesFunction = (GetNumFramesFunction*)nf;
	}
	else
		return returnInitFailure("The numFrames function can't be found. Make sure you've build the library sucessfully.");
#endif
    
	return Result::ok();
}

juce::Result RLottieManager::returnInitFailure(const String& s)
{
#if HISE_RLOTTIE_DYNAMIC_LIBRARY
	dynLib = nullptr;

	createFunction = nullptr;
	destroyFunction = nullptr;
	renderFunction = nullptr;
	frameRateFunction = nullptr;
	numFramesFunction = nullptr;
#endif
    
	lastResult = Result::fail(s);

	return lastResult;
}

Lottie_Animation* RLottieManager::createAnimation(const String& jsonData)
{
#if HISE_RLOTTIE_DYNAMIC_LIBRARY
	if (createFunction != nullptr)
	{
		auto hash = String(jsonData.hashCode());

		return createFunction(jsonData.getCharPointer(), hash.getCharPointer(), "D:\\");
	}

	return nullptr;
#else
    
    auto hash = String(jsonData.hashCode());
    
    return lottie_animation_from_data(jsonData.getCharPointer(), hash.getCharPointer(), "D:\\");
#endif
}

void RLottieManager::destroy(Lottie_Animation* animation)
{
#if HISE_RLOTTIE_DYNAMIC_LIBRARY
	if (destroyFunction != nullptr)
		destroyFunction(animation);
#else
    lottie_animation_destroy(animation);
#endif
}

#if HISE_RLOTTIE_DYNAMIC_LIBRARY
hise::RLottieManager::RenderFunction* RLottieManager::getRenderFunction()
{
	return renderFunction;
}
#endif

int RLottieManager::getNumFrames(Lottie_Animation* animation)
{
#if HISE_RLOTTIE_DYNAMIC_LIBRARY
	if (animation != nullptr && numFramesFunction != nullptr)
		return (int)numFramesFunction(animation);

	return 0;
#else
    return (int)lottie_animation_get_totalframe(animation);
#endif
}

double RLottieManager::getFrameRate(Lottie_Animation* animation)
{
#if HISE_RLOTTIE_DYNAMIC_LIBRARY
	if (animation != nullptr && frameRateFunction != nullptr)
		return frameRateFunction(animation);

	return 0.0;
#else
    return lottie_animation_get_framerate(animation);
#endif
}

#if HISE_RLOTTIE_DYNAMIC_LIBRARY
juce::File RLottieManager::getLibFile()
{
	auto root = getLibraryFolder();

#if JUCE_WINDOWS
	return root.getChildFile("rlottie_x64.dll");
#elif JUCE_MAC
    return root.getChildFile("librlottie.dylib");
#elif JUCE_LINUX
	return root.getChildFile("librlottie.so");
#else
	jassertfalse;
	return File();
#endif
}
#endif



}
