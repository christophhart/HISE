
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
*   Commercial licences for using HISE in an closed source project are
*   available on request. Please visit the project's website to get more
*   information about commercial licencing:
*
*   http://www.hartinstruments.net/hise/
*
*   HISE is based on the JUCE library,
*   which also must be licenced for commercial applications:
*
*   http://www.juce.com
*
*   ===========================================================================
*/

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
					// You specified a parameter for a unsupported widget type...
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
