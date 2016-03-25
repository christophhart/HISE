/*
  ==============================================================================

    This file was auto-generated!

    It contains the basic startup code for a Juce application.

  ==============================================================================
*/

//==============================================================================
PluginParameterAudioProcessor::PluginParameterAudioProcessor(const String &pluginName):	
	name(pluginName)
{
	for(int i = 0; i < 32; i++)
	{
		parameterSlots[i] = nullptr;
	}


}

//==============================================================================

int PluginParameterAudioProcessor::getNumParameters()
{
    return 32;
}

int PluginParameterAudioProcessor::addPluginParameter(PluginParameterModulator *existingModToBeAdded)
{
	ScopedLock sl(lock);

	for(int i = 0; i < 32; i++)
	{
		if(parameterSlots[i] == nullptr)
		{
			parameterSlots[i] = existingModToBeAdded;
			return i;
		}
	}

	// Something went wrong!
	jassertfalse;
	return -1;
}

/** Remove the plugin parameter from the list. */
void PluginParameterAudioProcessor::removePluginParameter(PluginParameterModulator *modToRemove)
{
	ScopedLock sl(lock);

	for(int i = 0; i < 32; i++)
	{
		if(parameterSlots[i] == modToRemove)
		{
			parameterSlots[i] = nullptr;
			
			return;
		}
	}
}


float PluginParameterAudioProcessor::getParameter (int index)
{
	ScopedLock sl(lock);

	if( index < 32 && parameterSlots[index].get() != nullptr )
	{
		PluginParameterModulator *pm = dynamic_cast<PluginParameterModulator*>(parameterSlots[index].get());
			
		return pm->getAttribute(PluginParameterModulator::Value);
	}
	else return 0.0f;
}

void PluginParameterAudioProcessor::setParameter (int index, float newValue)
{
	ScopedLock sl(lock);

	if(index < 32 && parameterSlots[index].get() != nullptr)
	{
		parameterSlots[index].get()->setAttribute(PluginParameterModulator::Value, newValue, sendNotification);
	}
}


int PluginParameterAudioProcessor::getParameterFromName(String parameter_name)
{
	ScopedLock sl(lock);

	for(int i = 0; i < 32 ; i++)
	{
		if(parameter_name == parameterSlots[i]->getId()) return i;
	}
	return -1;
}


const String PluginParameterAudioProcessor::getParameterName (int index)
{
	return index < 32 && parameterSlots[index].get() != nullptr ? parameterSlots[index]->getId() : "Unused";
}

const String PluginParameterAudioProcessor::getParameterText (int index)
{
	if( index < 32 && parameterSlots[index].get() != nullptr )
	{
		PluginParameterModulator *pm = dynamic_cast<PluginParameterModulator*>(parameterSlots[index].get());
			
		return pm->getParameterAsText();
	}
	else return String::empty;
}

//==============================================================================
void PluginParameterAudioProcessor::getStateInformation (MemoryBlock& /*destData*/)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.

	

}

void PluginParameterAudioProcessor::setStateInformation (const void* //data
														 , int //sizeInBytes
														 )
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.



}

