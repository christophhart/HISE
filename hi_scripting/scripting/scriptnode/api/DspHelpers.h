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

using PrepareSpecs = snex::Types::PrepareSpecs;
using ProcessDataDyn = snex::Types::ProcessDataDyn;

class NodeBase;

struct DspHelpers
{
	/** Increases the buffer size to match the process specs. */
	static void increaseBuffer(AudioSampleBuffer& b, const PrepareSpecs& ps);

	static void increaseBuffer(snex::Types::heap<float>& b, const PrepareSpecs& ps, bool clearForFrame=true);

	using ParameterCallback = std::function<void(double)>;

	static void setErrorIfFrameProcessing(const PrepareSpecs& ps);

	static void setErrorIfNotOriginalSamplerate(const PrepareSpecs& ps, NodeBase* n);

	/** Returns a ParameterCallback with the given range. */

#if 0
	static ParameterCallback getFunctionFrom0To1ForRange(InvertableParameterRange r, const ParameterCallback& originalFunction);
#endif

	forcedinline static double findPeak(const float* data, int numSamples)
	{
		auto r = FloatVectorOperations::findMinAndMax(data, numSamples);
		return jmax<float>(std::abs(r.getStart()), std::abs(r.getEnd()));
	}

	static void validate(PrepareSpecs sp, PrepareSpecs rp);

	static void throwIfFrame(PrepareSpecs ps);

	template <typename ProcessDataType> forcedinline static double findPeak(const ProcessDataType& data)
	{
		double max = 0.0;

		for (auto ch : data)
			max = jmax(max, findPeak(ch.getRawReadPointer(), data.getNumSamples()));

		return max;
	}
};

}
