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


RLottieManager::RLottieManager():
	lastResult(Result::fail("This Manager is not initialised. Call init() before using it"))
{
	
}

juce::Result RLottieManager::init()
{
	lastResult = Result::ok();

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

	return Result::ok();
}

juce::Result RLottieManager::returnInitFailure(const String& s)
{
	dynLib = nullptr;

	createFunction = nullptr;
	destroyFunction = nullptr;
	renderFunction = nullptr;
	frameRateFunction = nullptr;
	numFramesFunction = nullptr;

	lastResult = Result::fail(s);

	return lastResult;
}

Lottie_Animation* RLottieManager::createAnimation(const String& jsonData)
{
	if (createFunction != nullptr)
	{
		auto hash = String(jsonData.hashCode());

		return createFunction(jsonData.getCharPointer(), hash.getCharPointer(), "D:\\");
	}

	return nullptr;
}

void RLottieManager::destroy(Lottie_Animation* animation)
{
	if (destroyFunction != nullptr)
		destroyFunction(animation);
}

hise::RLottieManager::RenderFunction* RLottieManager::getRenderFunction()
{
	return renderFunction;
}

int RLottieManager::getNumFrames(Lottie_Animation* animation)
{
	if (animation != nullptr && numFramesFunction != nullptr)
		return (int)numFramesFunction(animation);

	return 0;
}

double RLottieManager::getFrameRate(Lottie_Animation* animation)
{
	if (animation != nullptr && frameRateFunction != nullptr)
		return frameRateFunction(animation);

	return 0.0;
}

juce::File RLottieManager::getLibFile()
{
	auto root = getLibraryFolder();

#if JUCE_WINDOWS

#if JUCE_64BIT
	return root.getChildFile("rlottie_x64.dll");
#else
	return root.getChildFile("rlottie_x86.dll");
#endif

#endif

	jassertfalse;
	return File();
}



}