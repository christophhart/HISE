/*  ===========================================================================
*
*   This file is part of HISE.
*   Copyright 2016 Christoph Hart
*
*   HISE is free software: you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation, either version 3 of the License, or
*   (at your option any later version.
*
*   HISE is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public License
*   along with HISE.  If not, see <http://www.gnu.org/licenses/>.
*
*   Commercial licences for using HISE in an closed source project are
*   available on request. Please visit the project's website to get more
*   information about commercial licencing:
*
*   http://www.hartinstruments.net/hise/
*
*   HISE is based on the JUCE library,
*   which also must be licenced for commercial applications:
*
*   http://www.juce.com
*
*   ===========================================================================
*/

#pragma once

namespace snex {

using namespace jit;
using namespace juce;



/** A callback collection is a high-level structure which handles the usage of SNEX within HISE.

	It automatically grabs all callbacks, checks their type and creates a list of parameter functions.
	
	It also selects the best processing callback based on these rules:
	
	Frame Processing:
	
	1. processFrame (if available)
	2. processSample
	3. processChannel
	
	Block Processing:
	
	1. processChannel
	2. processFrame
	3. processSample
	
	It contains the runtime memory pool required for executing the callbacks, so as long as this object
	is alive, the functions will work.
*/
struct CallbackCollection
{
	struct Listener
	{
		virtual ~Listener() {};

		virtual void initialised(const CallbackCollection& c) = 0;

		virtual void prepare(double samplerate, int blockSize, int numChannels) = 0;

		JUCE_DECLARE_WEAK_REFERENCEABLE(Listener);
	};

	enum ProcessType
	{
		FrameProcessing,
		BlockProcessing,
		numProcessTypes
	};

	CallbackCollection();

	String getBestCallbackName(int processType) const;

	
	void setupCallbacks();

	int getBestCallback(int processType) const;

	/** Calls the functions "prepare" and "reset" if the specs are valid. */
	void prepare(double sampleRate, int blockSize, int numChannels);

	/** Sets up a listener to be notified about initalisation and preparation. */
	void setListener(Listener* l);

	int bestCallback[numProcessTypes];

	snex::jit::JitObject obj;

	FunctionData callbacks[CallbackTypes::NumCallbackTypes];

	FunctionData resetFunction;
	FunctionData prepareFunction;
	FunctionData eventFunction;
	FunctionData modFunction;

	struct Parameter
	{
		String name;
		FunctionData f;
	};

	Array<Parameter> parameters;

	WeakReference<Listener> listener;
};


struct ParameterHelpers
{
	static FunctionData getFunction(const String& parameterName, JitObject& obj);

	static StringArray getParameterNames(JitObject& obj);
};

}