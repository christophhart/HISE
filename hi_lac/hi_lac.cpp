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

#include "hi_lac.h"

#include "hlac/BitCompressors.cpp"
#include "hlac/CompressionHelpers.cpp"
#include "hlac/SampleBuffer.cpp"
#include "hlac/HlacEncoder.cpp"
#include "hlac/HlacDecoder.cpp"
#include "hlac/HlacAudioFormatWriter.cpp"
#include "hlac/HlacAudioFormatReader.cpp"
#include "hlac/HiseLosslessAudioFormat.cpp"

#if PERFETTO


void MelatoninPerfetto::beginSession(uint32_t buffer_size_kb /*= 80000*/)
{
	perfetto::TraceConfig cfg;
	cfg.add_buffers()->set_size_kb(buffer_size_kb); // 80MB is the default
	auto* ds_cfg = cfg.add_data_sources()->mutable_config();
	ds_cfg->set_name("track_event");
	session = perfetto::Tracing::NewTrace();
	session->Setup(cfg);
	session->StartBlocking();
}



juce::File MelatoninPerfetto::endSession(bool shouldWriteFile)
{
	// Make sure the last event is closed for this example.
	perfetto::TrackEvent::Flush();

	// Stop tracing
	session->StopBlocking();

    if(shouldWriteFile)
        return writeFile();
    
    return juce::File();
}

juce::File MelatoninPerfetto::getDumpFileDirectory()
{
#if JUCE_WINDOWS
	return juce::File::getSpecialLocation(juce::File::SpecialLocationType::userDesktopDirectory);
#else
	return juce::File::getSpecialLocation(juce::File::SpecialLocationType::userHomeDirectory).getChildFile("Downloads");
#endif
}

MelatoninPerfetto::MelatoninPerfetto()
{
	perfetto::TracingInitArgs args;
	// The backends determine where trace events are recorded. For this example we
	// are going to use the in-process tracing service, which only includes in-app
	// events.
	args.backends = perfetto::kInProcessBackend;
	perfetto::Tracing::Initialize(args);
	perfetto::TrackEvent::Register();
}

juce::File MelatoninPerfetto::writeFile()
{
	// Read trace data
	std::vector<char> trace_data(session->ReadTraceBlocking());

	const auto file = getDumpFileDirectory();

#if JUCE_DEBUG
	auto mode = juce::String("-DEBUG-");
#else
	auto mode = juce::String("-RELEASE-");
#endif

	const auto currentTime = juce::Time::getCurrentTime().formatted("%Y-%m-%d_%H%M");
    auto childFile = file.getChildFile("perfetto" + mode + currentTime + ".pftrace");

	if(customFileLocation != juce::File())
	{
		childFile = customFileLocation;
	}
    else if(tempFile != nullptr)
        childFile = juce::File(tempFile->getFile());
    
	if (auto output = childFile.createOutputStream())
	{
		output->setPosition(0);
		output->write(&trace_data[0], trace_data.size() * sizeof(char));
		DBG("Wrote perfetto trace to: " + childFile.getFullPathName());
		lastFile = childFile;
		return childFile;
	}

	DBG("Failed to write perfetto trace file. Check for missing permissions.");
	jassertfalse;
	return juce::File{};
}


PERFETTO_TRACK_EVENT_STATIC_STORAGE();
#endif
