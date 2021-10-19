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


#ifndef HISELOSSLESSAUDIOFORMAT_H_INCLUDED
#define HISELOSSLESSAUDIOFORMAT_H_INCLUDED

namespace hlac {


class HiseLosslessAudioFormat : public juce::AudioFormat
{
public:

	

	HiseLosslessAudioFormat();

	~HiseLosslessAudioFormat() {}

	bool canHandleFile(const juce::File& fileToTest) override;

    juce::Array<int> getPossibleSampleRates() override;
    juce::Array<int> getPossibleBitDepths() override;

	bool canDoMono() override;
	bool canDoStereo() override;
	bool isCompressed() override;

	

    juce::StringArray getQualityOptions() override;

    juce::AudioFormatReader* createReaderFor (juce::InputStream* sourceStream, bool deleteStreamIfOpeningFails) override;
    juce::AudioFormatWriter* createWriterFor (juce::OutputStream* streamToWriteTo, double sampleRateToUse, unsigned int numberOfChannels, int /*bitsPerSample*/, const juce::StringPairArray& metadataValues, int /*qualityOptionIndex*/) override;

    juce::MemoryMappedAudioFormatReader* createMemoryMappedReader (juce::FileInputStream* fin) override;

    juce::MemoryMappedAudioFormatReader* createMemoryMappedReader (const juce::File& file) override;

    juce::HeapBlock<uint32_t> blockOffsets;
};

} // namespace hlac

#endif  // HISELOSSLESSAUDIOFORMAT_H_INCLUDED
