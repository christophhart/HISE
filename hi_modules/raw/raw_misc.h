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

#include "JuceHeader.h"

namespace hise {
using namespace juce;

namespace raw
{

/** A wrapper around a task that is supposed to take a while and is not allowed to be executed simultaneously to the audio rendering.

Some tasks that interfere with the audio rendering (eg. adding / removing modules, swapping samples) have to make sure that the audio thread
is not running in parallel. Therefore HISE has a "suspended task" system which kills all voices, waits until the voices have gracefully faded out (including
FX tails for reverb effects), then suspends the audio rendering until the task has been completed on a background thread with a minimal amount of locking.

In HISE, this system is used for each one of these events:

- Compilation of scripts
- Hot swapping of samplemaps
- Loading User Presets
- Adding / Removing Modules

You can see this system in action if you do one of these things in HISE while playing notes.

In order to use that system from your own C++ code, just create a instance of this object with a given lambda that follows the SafeFunctionCall definition:

```
auto f = [mySampleMap](Processor* p)
{
// You know it's a sampler, so no need for a dynamic_cast
auto sampler = static_cast<ModulatorSampler*>(p);

// Call whatever you need to do.
// (Actually this call makes no sense, since it's already wrapped in a suspended execution)
sampler->loadSampleMap(mySampleMap);

// The lambda definition requires you to return this.
return SafeFunctionCall::OK;
};

TaskAfterSuspension<> myTask(mc, f);
```

This makes sure that the lambda will be executed on the given thread after all voices are killed. However this doesn't necessarily mean asynchronous execution:
if you're calling this on the target thread and all voices are already killed, it will be executed synchronously. This reduces the debugging headaches of
asynchronous callbacks wherever possible.
*/
template <int TargetThread = IDs::Thread::Loading> class TaskAfterSuspension : public ControlledObject
{
public:

	TaskAfterSuspension(MainController* mc, const ProcessorFunction& f) :
		ControlledObject(mc)
	{
		mc->getKillStateHandler().killVoicesAndCall(mc->getMainSynthChain(), f, (MainController::KillStateHandler::TargetThread)TargetThread);
	};
};

/** A lightweight wrapper around a reference. */
template <class ProcessorType> class Reference : public ControlledObject,
												 private SafeChangeListener
{
public:

	/** A lambda that will be executed when the parameter changes. The newValue argument will contain the current value. */
	using ParameterCallback = std::function<void(float newValue)>;

	/** Creates a reference to the processor with the given ID. If the ID is empty, it just looks for the first processor of the specified type. */
	Reference(MainController* mc, const String& id = String(), bool addAsListener=false);
	~Reference();

	/** Adds a lambda callback to a dedicated parameter that will be fired on changes. */
	void addParameterToWatch(int parameterIndex, const ParameterCallback& callbackFunction);

	/** Returns a raw pointer to the connected Processor. */
	ProcessorType* getProcessor();

	/** Returns a raw const pointer to the connected Processor. */
	const ProcessorType* getProcessor() const;

private:

	void changeListenerCallback(SafeChangeBroadcaster *) override;

	struct ParameterValue
	{
		int index;
		float lastValue;
		ParameterCallback callbackFunction;
	};

	Array<ParameterValue> watchedParameters;

	WeakReference<ProcessorType> processor;
};

/** A object that handles the embedded resources (audio files, images and samplemaps). */
class Pool : public hise::ControlledObject
{
public:

	Pool(MainController* mc) :
		ControlledObject(mc)
	{};

	/** By default, this object assumes that the data is already loaded (if not, it fails with an assertion). 
		Call this to allow the Pool object to actually load the items from the embedded data. */
	void allowLoadingOfUnusedResources()
	{
		allowUnusedSources = true;
	}

	/** Loads an audio file into a AudioSampleBuffer. */
	AudioSampleBuffer loadAudioFile(const String& id)
	{
		auto pool = getMainController()->getCurrentAudioSampleBufferPool();

		PoolReference ref(pool, "{PROJECT_FOLDER}" + id, ProjectHandler::AudioFiles);

		auto entry = pool->loadFromReference(ref, allowUnusedSources ? PoolHelpers::LoadAndCacheStrong :
			PoolHelpers::DontCreateNewEntry
		);

		if (auto b = entry.getData())
		{
			return AudioSampleBuffer(*b);
		}

		jassertfalse;
		return {};
	}

	/** Loads an Image from the given ID. */
	Image loadImage(const String& id)
	{
		auto pool = getMainController()->getCurrentImagePool();

		PoolReference ref(pool, "{PROJECT_FOLDER}" + id, ProjectHandler::Images);

		auto entry = pool->loadFromReference(ref, allowUnusedSources ? PoolHelpers::LoadAndCacheStrong :
			PoolHelpers::DontCreateNewEntry
		);

		if (auto img = entry.getData())
		{
			return Image(*img);
		}

		jassertfalse;
		return {};
	}

private:

	bool allowUnusedSources = false;
};



/** A wrapper around a plugin parameter (not yet functional). */
class Parameter : private SafeChangeListener
{
public:

	/** A connection to a internal attribute of a Processor. */
	struct Connection
	{
		WeakReference<Processor> processor;
		int index;
		NormalisableRange<float> parameterRange;
		float lastValue;
	};

	/** Creates a parameter. */
	Parameter(MainController* mc)
	{

	};

	/** Adds a parameter connection. */
	void addConnection(const Connection& c)
	{

	}

private:

	void changeListenerCallback(SafeChangeBroadcaster *b) override
	{

	}

	Array<Connection> connections;

};



}

} // namespace hise;