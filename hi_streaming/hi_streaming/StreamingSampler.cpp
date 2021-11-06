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

namespace hise { using namespace juce;

#define LOG_SAMPLE_RENDERING 0

void StreamingHelpers::increaseBufferIfNeeded(hlac::HiseSampleBuffer& b, int numSamplesNeeded)
{
	// The channel amount must be set correctly in the constructor
	jassert(b.getNumChannels() > 0);

	if (b.getNumSamples() < numSamplesNeeded)
	{
		b.setSize(b.getNumChannels(), numSamplesNeeded);
		b.clear();
	}
}

bool StreamingHelpers::preloadSample(StreamingSamplerSound* s, const int preloadSize, String& errorMessage)
{
	try
	{
		s->setPreloadSize(s->hasActiveState() ? preloadSize : 0, true);
		s->closeFileHandle();

		return true;
	}
	catch (StreamingSamplerSound::LoadingError e)
	{
		errorMessage = "Error loading sample " + e.fileName + ": " + e.errorDescription;
		return false;
	}
}

hise::StreamingHelpers::BasicMappingData StreamingHelpers::getBasicMappingDataFromSample(const ValueTree& sampleData)
{
	BasicMappingData data;

	static const Identifier hiKey("HiKey");
	static const Identifier loKey("LoKey");
	static const Identifier loVel("LoVel");
	static const Identifier hiVel("HiVel");
	static const Identifier root("Root");

	data.highKey = (int8)(int)sampleData.getProperty(hiKey);
	data.lowKey = (int8)(int)sampleData.getProperty(loKey);
	data.lowVelocity = (int8)(int)sampleData.getProperty(loVel);
	data.highVelocity = (int8)(int)sampleData.getProperty(hiVel);
	data.rootNote = (int8)(int)sampleData.getProperty(root);

	return data;
}

} // namespace hise