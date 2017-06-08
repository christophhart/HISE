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
*   which must be separately licensed for cloused source applications:
*
*   http://www.juce.com
*
*   ===========================================================================
*/

#ifndef PITCH_DETECTION_H_INCLUDED
#define PITCH_DETECTION_H_INCLUDED

/** A wrapper class around the dywapitchtrack library that provides JUCE-type interface methods. */
class PitchDetection
{
public:

	/** Scans a whole file and returns the pitch. 
	*
	*	You have to supply a working buffer that is used to read the data from the file.
	*	It must be stereo and have the desired length that can be obtained by getNumSamplesNeeded()
	*/
	static double detectPitch(const File &fileToScan, AudioSampleBuffer &workingBuffer, double sampleRate)
	{
		const int numSamplesPerDetection = workingBuffer.getNumSamples();

		AudioFormatManager afm;
		afm.registerBasicFormats();

		ScopedPointer<AudioFormatReader> afr = afm.createReaderFor(new FileInputStream(File(fileToScan)));

		int64 startSample = 0;
		double pitch = 0.0;

		while(pitch == 0.0 && (startSample + numSamplesPerDetection < afr->lengthInSamples))
		{
			afr->read(&workingBuffer, 0, workingBuffer.getNumSamples(), startSample, true, true);
			pitch = detectPitch(workingBuffer, 0, numSamplesPerDetection, sampleRate);
			startSample += numSamplesPerDetection;
		}

		return pitch;
	};

	/** detects the pitch in the audio buffer. */
	static double detectPitch(AudioSampleBuffer &buffer, int startSample, int numSamples, double sampleRate)
	{
		Array<double> doubleSamples;

		doubleSamples.ensureStorageAllocated(numSamples);

		for(int i = 0; i < numSamples; i++)
		{
			const double value = (double)(buffer.getSample(0, startSample + i) + buffer.getSample(1, startSample + i)) / 2.0;
			doubleSamples.set(i, value);
		}

		dywapitchtracker tracker;
		dywapitch_inittracking(&tracker);

		const double pitchResult = dywapitch_computepitch(&tracker, doubleSamples.getRawDataPointer(), 0, numSamples);

		return pitchResult * (sampleRate / 44100.0);
	};

	/** Returns the number of samples that is needed to detect 50 Hz. */
	static int getNumSamplesNeeded(double sampleRate)
	{
		return dywapitch_neededsamplecount((int)(50.0 * (44100.0 / sampleRate)));
	}
};

#endif