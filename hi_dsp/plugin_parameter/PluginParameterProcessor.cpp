
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

void PluginParameterAudioProcessor::addScriptedParameters()
{
	ModulatorSynthChain* synthChain = dynamic_cast<MainController*>(this)->getMainSynthChain();

	jassert(synthChain != nullptr);

	Processor::Iterator<JavascriptMidiProcessor> iter(synthChain);

	while (JavascriptMidiProcessor *sp = iter.getNextProcessor())
	{
		if (sp->isFront())
		{
			ScriptingApi::Content *content = sp->getScriptingContent();

			for (int i = 0; i < content->getNumComponents(); i++)
			{
				ScriptingApi::Content::ScriptComponent *c = content->getComponent(i);

				const bool wantsAutomation = c->getScriptObjectProperty(ScriptingApi::Content::ScriptComponent::Properties::isPluginParameter);
				const bool isAutomatable = c->isAutomatable();

				if (wantsAutomation && !isAutomatable)
				{
					// You specified a parameter for a unsupported component type...
					jassertfalse;
				}

				if (wantsAutomation && isAutomatable)
				{
					ScriptedControlAudioParameter *newParameter = new ScriptedControlAudioParameter(content->getComponent(i), this, sp, i);
					addParameter(newParameter);
				}
			}
		}
	}
}


void PluginParameterAudioProcessor::setNonRealtime(bool isNonRealtime) noexcept
{
	dynamic_cast<MainController*>(this)->getSampleManager().setNonRealtime(isNonRealtime);

	AudioProcessor::setNonRealtime(isNonRealtime);
}

void PluginParameterAudioProcessor::handleLatencyInPrepareToPlay(double samplerate)
{
	if (getLatencySamples() != lastLatencySamples && getLatencySamples() != 0)
	{
		lastLatencySamples = getLatencySamples();
		updateHostDisplay();

		int numChannels = AudioProcessor::getBusesLayout().getMainOutputChannels();

		bypassedLatencyDelays.clear();

		for (int i = 0; i < numChannels; i++)
		{
			bypassedLatencyDelays.add(new DelayLine<32768>());
			bypassedLatencyDelays.getLast()->prepareToPlay(samplerate);
			bypassedLatencyDelays.getLast()->setFadeTimeSamples(0);
			bypassedLatencyDelays.getLast()->setDelayTimeSamples(lastLatencySamples);
		}
	}
}

void PluginParameterAudioProcessor::handleLatencyWhenBypassed(AudioSampleBuffer& buffer, MidiBuffer&)
{
	if (lastLatencySamples != 0)
	{
		jassert(buffer.getNumChannels() <= bypassedLatencyDelays.size());

		auto numChannels = jmin(buffer.getNumChannels(), bypassedLatencyDelays.size());

		for (int i = 0; i < numChannels; i++)
			bypassedLatencyDelays[i]->processBlock(buffer.getWritePointer(i, 0), buffer.getNumSamples());
	}
}

void PluginParameterAudioProcessor::setScriptedPluginParameter(Identifier id, float newValue)
{
	for (int i = 0; i < getNumParameters(); i++)
	{
		if (ScriptedControlAudioParameter * sp = static_cast<ScriptedControlAudioParameter*>(getParameters().getUnchecked(i)))
		{
			if (sp->getId() == id)
			{
				sp->setParameterNotifyingHost(i, newValue);

			}
		}
	}
}

} // namespace hise