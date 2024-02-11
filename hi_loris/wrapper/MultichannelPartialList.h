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
#include "../loris/src/Partial.h"

#include "Properties.h"
#include "Helpers.h"



namespace loris2hise {
using namespace juce;

/** This object contains a Loris::PartialList for each audio channel. */
struct MultichannelPartialList
{
    /** @internal only used internally. */
    MultichannelPartialList(const juce::String& name, int numChannels);

    /** @internal only used internally. */
	~MultichannelPartialList();

    /** @internal only used internally. */
	double convertTimeToSeconds(double timeInput) const;

    /** @internal only used internally. */
	double convertSecondsToTime(double timeInput) const;

    /** @internal only used internally. */
	PartialList* get(int index) { return list[index]; }

    /** @internal only used internally. */
	void setMetadata(juce::AudioFormatReader* r, double root);

    /** @internal This function creates a linear envelope from points supplied as JSON object.
     
        The JSON format is a two-dimensional array with `[x, y]` items.
     
        @example: `[ [0.0, 0.0], [0.5, 0.25], [1.0, 1.0] ]`
     
        This is used by multiple process algorithm that require a time-varying parameter. Note that the `x`
        parameter (=time) is always converted using the global timedomain setting.
    */
	LinearEnvelope* createEnvelopeFromJSON(const juce::var& data);

    /** @internal: argument sanitizer. Throws a juce::Result with the supplied error message. */
	void checkArgs(bool condition, const juce::String& error, const std::function<void()>& additionalCleanupFunction = {});

    /** Iterates all channels, partials and breakpoints (3D!) and calls the given function for each data point.
     
        - state: the LorisState context. This is only used internally.
        - f:  a function with a single argument that contains a reference to the CustomFunctionArgs object that holds the data
            for the breakpoint that should be modified. If the function returns true, the iteration will be aborted.
     
        returns true if the processing was sucessful and false if there was an error.
    */
	bool processCustom(void* obj, const CustomFunctionArgs::Function& f);
    
    /** Processes the partials of each channel with a predefined function.
        
        - command: the function you want to execute (see below)
        - args: a JSON object containing the data parameters for the algorithm.
     
        returns true if the processing was sucessful and false if there was an error.
     
        The list of available process commands is defined in the ProcessIds namespace.
     
        The JSON data for each command:
     
        - reset: an empty object `{}`
        - 
     
    */
	bool process(const juce::Identifier& command, const juce::var& args);
    
    bool createSnapshot(const juce::Identifier& id, double timeSeconds, double* buffer, int& numChannels, int& numHarmonics);
    
	juce::AudioSampleBuffer synthesize();
    
    juce::AudioSampleBuffer renderEnvelope(const Identifier& parameter, int partialIndex);

	bool matches(const juce::File& f) const;

	size_t getRequiredBytes() const;
	int getNumSamples() const;
	int getNumChannels() const;

    /**@ internal **/
	void setOptions(const Options& newOptions);

    /** @internal */
	void saveAsOriginal();

    bool prepareToMorph(bool removeUnlabeled=false);
    
private:

    bool preparedForMorph = false;
    
	Options options;

	juce::String filename;

	int numSamples = 0;
	double sampleRate = 0.0;
	double rootFrequency;

	juce::Array<PartialList*> list;

    juce::Array<LinearEnvelope*> rootFrequencyEnvelopes;
    
	juce::Array<PartialList*> original;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MultichannelPartialList);
};

}
