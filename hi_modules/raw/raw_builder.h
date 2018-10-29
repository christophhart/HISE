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

namespace hise
{

/** @brief A gateway drug to the full HISE world.
*/
namespace raw
{

using namespace juce;

struct AttributeItem
{
	int index;
	float value;
};

/** a collection of attribute key/value pairs. */
using AttributeCollection = std::vector<AttributeItem>;

/** This enumerations can be used for the basic targets. */
namespace ChainIndexes
{
constexpr int Direct = -1;

/** The slot index for MIDI Processors. */
constexpr int Midi = hise::ModulatorSynth::MidiProcessor;

/** The slot index for Gain modulators. */
constexpr int Gain = hise::ModulatorSynth::GainModulation;

/** the slot index for Pitch modulators. */
constexpr int Pitch = hise::ModulatorSynth::PitchModulation;

/** the slot index for FX. */
constexpr int FX = hise::ModulatorSynth::EffectChain;
};

/** The builder is a low overhead helper class that provides functions to add modules.

Create one of those, supply the main controller instance and call its methods to build up the architecture of your plugin.
*/
class Builder
{
public:

	Builder(MainController* mc_) :
		mc(mc_)
	{
	};

	/** Finds the module with the given ID. */
	template <class T> T* find(const String& name);

	/** Adds the given module to the parent processor. Specify the chainIndex for modulators / effects. */
	template <class T> void add(T* processor, Processor* parent, int chainIndex = ChainIndexes::Direct);

	/** Creates a module of the given class and adds it to the parent with the specified
	chainIndex. See ChainIndexes.

	This only works with HISE modules that are registered at one of the factories, so if you want to add a custom module, use the add() function instead.
	*/
	template <class T> T* create(Processor* parent, int chainIndex = ChainIndexes::Direct);

	/** Creates a module from the given Base64 encoded String and adds it to the parent module with the suppliedChainIndex. */
	Processor* createFromBase64State(const String& base64EncodedString, Processor* parent, int chainIndex = ChainIndexes::Direct);

	/** Sets all the attributes from the given collection.

	You can use std::initialiser_lists for a clean syntax:

	```
	AttributeCollection c =
	{
		{ SimpleEnvelope::Attack, 10.0f },
		{ SimpleEnvelope::Release, 248.0f }
	};

	for(auto* envelope: myEnvelopes)
		builder.setAttributes(envelope, c);
	```
	*/
	void setAttributes(Processor* p, const AttributeCollection& collection);

private:

	template <class T> T* addInternal(Processor* p, Chain* c);

	MainController * mc;
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






} // namespace raw

}