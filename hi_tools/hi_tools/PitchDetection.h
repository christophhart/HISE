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

namespace hise { using namespace juce;

#if HISE_INCLUDE_PITCH_DETECTION
/** A wrapper class around the dywapitchtrack library that provides JUCE-type interface methods. */
class PitchDetection
{
public:

	static double detectPitch(float* fullData, int numSamples, double sampleRate);

	/** Scans a whole file and returns the pitch. 
	*
	*	You have to supply a working buffer that is used to read the data from the file.
	*	It must be stereo and have the desired length that can be obtained by getNumSamplesNeeded()
	*/
	static double detectPitch(const File &fileToScan, AudioSampleBuffer &workingBuffer, double sampleRate, double estimatedFrequency = -1.0);;

	/** detects the pitch in the audio buffer. */
	static double detectPitch(const AudioSampleBuffer &buffer, int startSample, int numSamples, double sampleRate);;

	/** Returns the number of samples that is needed to detect 50 Hz. */
	static int getNumSamplesNeeded(double sampleRate);

	/** Returns the number of samples that is needed to detect the given frequency. */
	static int getNumSamplesNeeded(double sampleRate, double minFrequencyToAnalyse);
};

#endif

} // namespace hise


