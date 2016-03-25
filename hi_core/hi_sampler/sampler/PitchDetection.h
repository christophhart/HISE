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