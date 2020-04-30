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
 *   which also must be licenced for commercial applications:
 *
 *   http://www.juce.com
 *
 *   ===========================================================================
 */

#pragma once

namespace scriptnode
{
using namespace juce;
using namespace hise;

struct RangeHelpers
{
	static bool isRangeId(const Identifier& id);

	static bool isIdentity(NormalisableRange<double> d);

	static Array<Identifier> getRangeIds();

	/** Checks if the MinValue and MaxValue are inverted, sets the property Inverted and returns the result. */
	static bool checkInversion(ValueTree& data, ValueTree::Listener* listenerToExclude, UndoManager* um);

	/** Creates a NormalisableRange from the ValueTree properties. It doesn't update the Inverted property. */
	static NormalisableRange<double> getDoubleRange(const ValueTree& t);

	static void storeDoubleRange(ValueTree& d, bool isInverted, NormalisableRange<double> r, UndoManager* um);
};


struct SimpleRingBuffer
{
	static constexpr int RingBufferSize = 65536;

	SimpleRingBuffer();

	void clear();
	int read(AudioSampleBuffer& b);
	void write(double value, int numSamples);

	bool isBeingWritten = false;

	int numAvailable = 0;
	int writeIndex = 0;
	float buffer[RingBufferSize];
};


}
