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

namespace loris2hise
{

#define DECLARE_ID(x) static const juce::Identifier x(#x);

namespace OptionIds
{
	DECLARE_ID(timedomain);
	DECLARE_ID(enablecache);
	DECLARE_ID(windowwidth);
	DECLARE_ID(freqfloor);
	DECLARE_ID(ampfloor);
	DECLARE_ID(sidelobes);
	DECLARE_ID(freqdrift);
	DECLARE_ID(hoptime);
	DECLARE_ID(croptime);
	DECLARE_ID(bwregionwidth);

    /** @internal */
	static juce::Array<juce::Identifier> getAllIds()
	{
		return { timedomain, enablecache, windowwidth, freqfloor, ampfloor, sidelobes, freqdrift, hoptime, croptime, bwregionwidth };
    };
}

namespace ParameterIds
{
    DECLARE_ID(rootFrequency);
    DECLARE_ID(frequency);
    DECLARE_ID(phase);
    DECLARE_ID(gain);
    DECLARE_ID(bandwidth);

    static juce::Array<juce::Identifier> getAllIds()
    {
        return { rootFrequency, frequency, phase, gain, bandwidth };
    };
};

/** This contains a list of all available commands that can be passed into process. */
namespace ProcessIds
{
    /** Resets the partials to the original. Use this if you have cached the file and want*/
	DECLARE_ID(reset);
	DECLARE_ID(shiftTime);
	DECLARE_ID(shiftPitch);
	DECLARE_ID(applyFilter);
	DECLARE_ID(scaleFrequency);
	DECLARE_ID(dilate);
	DECLARE_ID(custom);

    /** @internal */
	static juce::Array<juce::Identifier> getAllIds()
	{
		return { reset, shiftTime, shiftPitch, scaleFrequency, dilate, applyFilter, custom };
	}

}

#undef DECLARE_ID

}
