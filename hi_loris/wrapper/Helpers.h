/*
 * This file is part of the HISE loris_library codebase (https://github.com/christophhart/loris-tools).
 * Copyright (c) 2023 Christoph Hart
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include "../loris/src/loris.h"

#include "ThreadController.h"
#include "../loris/src/Breakpoint.h"
#include "../loris/src/PartialUtils.h"

namespace loris2hise
{
using namespace juce;

enum class TimeDomainType
{
	Seconds,
	Samples,
	Normalised,
	Frequency,
	numTimeDomainTypes
};

/** The configuration settings for the LorisState. You can change these using loris_config(). */
struct Options
{
    /** Creates a JSON version of this object. */
	var toJSON() const;

    /** Returns all options for the timedomain property. Currently: `["seconds", "samples", "0oto1"]`*/
	static StringArray getTimeDomainOptions();

    /** @internal: init the values. */
	void initLorisParameters();

    /** updates the configuration of the given property with the value. */
	bool update(const Identifier& id, const var& value);

    /** The timedomain used for all time (x-axis) values.
     
        By default, loris uses seconds for all time values, but you can change it for more convenient
        calculations. The possible options are:
     
        - "seconds" (default): the time in seconds (not milliseconds!)
        - "samples": the time in samples depending on the sample rate of the file
        - "0to1": the time in a normalized version from 0.0 (start of file) to 1.0 (end of file).
     */
	TimeDomainType currentDomainType = TimeDomainType::Seconds;
    
    /** The lowest frequency that is considered as harmonic content. */
    double freqfloor = 40.0;
    
    /** The lowest amplitude that is considered relevant, amplitudes below that value are considered below the noise floor. */
	double ampfloor = 90.0;
    
    /** The gain of the side lobes of the analysis window. */
	double sidelobes = 90.0;
    
    /** The max frequency drift that can occur in the sample (in cent). This defines a tolerance of pitch variations. */
    double freqdrift = 50.0;
    
    /** The time between analysis windows. */
	double hoptime = 0.0129;
    
    /** the time between I don't know. */
	double croptime = 0.0129;
    
    /** ??? */
	double bwregionwidth = 1.0;
    
    /** Enables caching of the input file. If this is true, then analyze calls to previously analyzed files are skipped. */
	bool enablecache = true;
    
    /** The window width scale factor. */
	double windowwidth = 1.0;

	/** If this is true, it will also call the loris_setXXX methods. */
	bool initialised = false;

	hise::ThreadController* threadController = nullptr;
};



/** This struct will be used as argument for the custom function. */
struct CustomFunctionArgs
{
    /** The function pointer type that is passed into loris_process_custom(). */
    using FunctionType = bool(*)(CustomFunctionArgs&);
    
    /** @internal The function object alias. */
    using Function = std::function<bool(CustomFunctionArgs&)>;
    
    /** Creates a function args from the breakpoint, the channel index and the LorisState context. */
    CustomFunctionArgs(void* obj_, const Loris::Breakpoint& b, int channelIndex_,
        int partialIndex_,
        double sampleRate_,
        double time_,
        double rootFrequency_) :
        channelIndex(channelIndex_),
        partialIndex(partialIndex_),
        sampleRate(sampleRate_),
        rootFrequency(rootFrequency_),
        obj(obj_),
        time(time_)
    {
        frequency = b.frequency();
        phase = b.phase();
        gain = b.amplitude();
        bandwidth = b.bandwidth();
    };
    
    // Constants ========================================
    
    /** The channel in the supplied audio file. */
    int channelIndex = 0;
    
    /** The index of the partial. */
    int partialIndex = 0;
    
    /** The sample rate of the file. */
    double sampleRate = 44100.0;
    
    /** the root frequency that was passed into loris_analyze. */
    double rootFrequency = 0.0;
    
    /** @internal: a pointer to the state context. */
    void* obj = nullptr;

    // Variable properties ==============================
    
    /** The time of the breakpoint. The domain depends on the `timedomain` configuration option. */
    double time = 0.0;
    
    /** The frequency of partial at the breakpoint in Hz. */
    double frequency = 0.0;
    
    /** The phase of the the partial at the breakpoint in radians (0 ... 2*PI). */
    double phase = 0.0;
    
    /** The amplitude of the partial. */
    double gain = 1.0;
    
    /** The "noisiness" of the partial at the given breakpoint. (0.0 = pure sinusoidal, 1.0 = full noise). */
    double bandwidth = 0.0;
};

/** @internal Some helper functions. */
struct Helpers
{
	static void reportError(const char* msg);

	static void logMessage(const char* msg);
};

}
