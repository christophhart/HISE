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

#include "LorisState.h"

namespace loris2hise {

LorisState* LorisState::currentInstance = nullptr;

LorisState::LorisState() :
	lastError(juce::Result::ok())
{
	setExceptionHandler(Helpers::reportError);
	setNotifier(Helpers::logMessage);
}

LorisState::~LorisState()
{
	analysedFiles.clear();
	messages.clear();
}

LorisState* LorisState::getCurrentInstance(bool forceCreate /*= false*/)
{
	if (currentInstance == nullptr || forceCreate)
		currentInstance = new LorisState();

	return currentInstance;
}

void LorisState::resetState(void* state)
{
	if (state != getCurrentInstance())
		currentInstance = (LorisState*)state;

	((LorisState*)state)->lastError = juce::Result::ok();
}

MultichannelPartialList* LorisState::getExisting(const File& f)
{
    for (auto af : analysedFiles)
    {
        if (af->matches(f))
            return af;
    }

    return nullptr;
}

void LorisState::reportError(const char* msg)
{
	lastError = juce::Result::fail(msg);
}

bool LorisState::analyse(const juce::File& audioFile, double rootFrequency)
{
	for (const auto& af : analysedFiles)
	{
		if (af->matches(audioFile))
		{
			if (currentOption.enablecache)
			{
				messages.add("Skip " + audioFile.getFileName());
				return true;
			}
			else
			{
				analysedFiles.removeObject(af);
				break;
			}
		}
	}
	juce::AudioFormatManager m;
	m.registerBasicFormats();

    auto driftFactor = std::pow(2.0, currentOption.freqdrift / 1200.0);
    
	analyzer_configure(rootFrequency * 0.8, rootFrequency * currentOption.windowwidth, currentOption.threadController);
	//analyzer_setWindowWidth(rootFrequency * currentOption.windowwidth);
    analyzer_setFreqDrift(rootFrequency * 0.25);

	analyzer_storeNoBandwidth();

	if (!currentOption.initialised)
	{
		currentOption.initLorisParameters();
		currentOption.initialised = true;
	}

	analyzer_setHopTime(currentOption.hoptime);
	analyzer_setCropTime(currentOption.croptime);

	if (juce::ScopedPointer<juce::AudioFormatReader> r = m.createReaderFor(audioFile))
	{
		messages.add("Analyse " + audioFile.getFileName());

		auto newEntry = new MultichannelPartialList(audioFile.getFullPathName(), r->numChannels);

		newEntry->setMetadata(r, rootFrequency);
		newEntry->setOptions(currentOption);

		juce::AudioSampleBuffer bf(r->numChannels, (int)r->lengthInSamples);

		r->read(&bf, 0, (int)r->lengthInSamples, 0, true, true);

		juce::HeapBlock<double> buffer;

		buffer.allocate(bf.getNumSamples(), true);

		for (int c = 0; c < bf.getNumChannels(); c++)
		{
			if (auto s = hise::ThreadController::ScopedStepScaler(currentOption.threadController, c, bf.getNumChannels()))
			{
				if (!s)
					return false;

				for (int i = 0; i < bf.getNumSamples(); i++)
					buffer[i] = bf.getSample(c, i);

				auto list = newEntry->get(c);

				analyze(buffer, bf.getNumSamples(), r->sampleRate, list);
			}
		}

		newEntry->saveAsOriginal();
        //newEntry->prepareToMorph();
        
		analysedFiles.add(newEntry);

		messages.add("... Analysed OK");
		return true;
	}

	return false;
}

double LorisState::getOption(const juce::Identifier &id) const
{
    juce::String msg;
    msg << "Get option " << id;

    Helpers::logMessage(msg.getCharPointer().getAddress());
    
    return (double)currentOption.toJSON()[id];
}

bool LorisState::setOption(const juce::Identifier& id, const juce::var& data)
{
	juce::String msg;
	msg << "Set option " << id << " with value " << data.toString().quoted();

	Helpers::logMessage(msg.getCharPointer().getAddress());

	try
	{
		if (!currentOption.update(id, data))
			return false;

		for (auto& s : analysedFiles)
			s->setOptions(currentOption);
	}
	catch (juce::Result& r)
	{
		lastError = r;
		return false;
	}

	msg = "Updated options to: ";
	msg << "\n" << juce::JSON::toString(currentOption.toJSON());

	Helpers::logMessage(msg.getCharPointer().getAddress());

	return true;
}




}
