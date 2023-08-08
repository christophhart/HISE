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

RoutableProcessor::MatrixData::MatrixData(RoutableProcessor *p) :
owningProcessor(p),
numSourceChannels(2),
numDestinationChannels(2),
resizeAllowed(false),
allowEnablingOnly(false)
{
	memset(numEditors, 0, sizeof(int)*NUM_MAX_CHANNELS);
    
	resetToDefault();
}

RoutableProcessor::MatrixData::~MatrixData()
{
	// should be cleaned up properly...
	jassert(!anyActive);
}

void RoutableProcessor::MatrixData::clearAllConnections()
{
	for (int i = 0; i < NUM_MAX_CHANNELS; i++)
	{
		channelConnections[i] = -1;
		sendConnections[i] = -1;
	}

	refreshSourceUseStates();
}

bool RoutableProcessor::MatrixData::isUsed(int sourceChannel) const noexcept
{
	if (sourceChannel < 0 || sourceChannel >= getNumSourceChannels())
	{
		return false;
	}

	return channelConnections[sourceChannel] != -1 || sendConnections[sourceChannel] != -1;
}

bool RoutableProcessor::MatrixData::toggleEnabling(int sourceChannel) noexcept
{
	SimpleReadWriteLock::ScopedWriteLock sl(getLock());

	if (sourceChannel < 0 || sourceChannel >= getNumSourceChannels())
	{
		return false;
	}

	if (channelConnections[sourceChannel] == sourceChannel)
	{
		removeConnection(sourceChannel, sourceChannel);
	}
	else
	{
		addConnection(sourceChannel, sourceChannel);
	}

	refreshSourceUseStates();

	return true;
}

bool RoutableProcessor::MatrixData::toggleSendEnabling(int sourceChannel) noexcept
{
	SimpleReadWriteLock::ScopedWriteLock sl(getLock());

	if (sourceChannel < 0 || sourceChannel >= getNumSourceChannels())
	{
		return false;
	}

	if (channelConnections[sourceChannel] == sourceChannel)
	{
		removeSendConnection(sourceChannel, sourceChannel);
	}
	else
	{
		addSendConnection(sourceChannel, sourceChannel);
	}

	refreshSourceUseStates();

	return true;
}

bool RoutableProcessor::MatrixData::toggleConnection(int sourceChannel, int destinationChannel) noexcept
{
	SimpleReadWriteLock::ScopedWriteLock sl(getLock());

	if (sourceChannel < 0 || sourceChannel >= getNumSourceChannels() || destinationChannel < 0 || destinationChannel >= getNumDestinationChannels())
	{
		return false;
	}

	if (channelConnections[sourceChannel] == destinationChannel)
	{
		removeConnection(sourceChannel, destinationChannel);
	}
	else
	{
		addConnection(sourceChannel, destinationChannel);
	}

	refreshSourceUseStates();

	return true;
}

bool RoutableProcessor::MatrixData::addConnection(int sourceChannel, int destinationChannel) noexcept
{
	SimpleReadWriteLock::ScopedWriteLock sl(getLock());

	auto sourceChannelValid = isPositiveAndBelow(sourceChannel, getNumSourceChannels());
	auto destinationChannelValid = isPositiveAndBelow(destinationChannel, getNumDestinationChannels());

	if (!destinationChannelValid && thisAsProcessor == thisAsProcessor->getMainController()->getMainSynthChain())
	{
		destinationChannelValid = isPositiveAndBelow(destinationChannel, HISE_NUM_PLUGIN_CHANNELS);
	}

	if (!sourceChannelValid || !destinationChannelValid)
	{
		return false;
	}
	
	channelConnections[sourceChannel] = destinationChannel;

	if (numAllowedConnections == 2)
	{
		int numConnections = 0;

		for (int i = 0; i < getNumSourceChannels(); i++)
		{
			if (channelConnections[i] != -1)
			{
				numConnections++;
			}
		}

		if (numConnections > 2)
		{
			const bool removeOldEvenConnection = sourceChannel % 2 == 0;

			for (int i = removeOldEvenConnection ? 0 : 1; i < getNumSourceChannels(); i += 2)
			{
				if (i == sourceChannel) continue;
				channelConnections[i] = -1;
			}
		}
	}

	refreshSourceUseStates();

	return true;
}

bool RoutableProcessor::MatrixData::removeConnection(int sourceChannel, int destinationChannel) noexcept
{
	SimpleReadWriteLock::ScopedWriteLock sl(getLock());

	if (sourceChannel < 0 || sourceChannel >= getNumSourceChannels() || destinationChannel < 0 || destinationChannel >= getNumDestinationChannels())
	{
		return false;
	}

	channelConnections[sourceChannel] = -1;

	if (numAllowedConnections == 2)
	{
		int numConnections = 0;

		for (int i = 0; i < getNumSourceChannels(); i++)
		{
			if (channelConnections[i] != -1)
			{
				numConnections++;
			}
		}

		if (numConnections < 2)
		{
			const int index = sourceChannel % 2 != 0 ? 1 : 0;

			channelConnections[index] = index;
		}
	}

	refreshSourceUseStates();

	return true;
}

bool RoutableProcessor::MatrixData::toggleSendConnection(int sourceChannel, int destinationChannel) noexcept
{
	SimpleReadWriteLock::ScopedWriteLock sl(getLock());

	if (sourceChannel < 0 || sourceChannel >= getNumSourceChannels() || destinationChannel < 0 || destinationChannel >= getNumDestinationChannels())
	{
		return false;
	}

	if (sendConnections[sourceChannel] == destinationChannel)
	{
		removeSendConnection(sourceChannel, destinationChannel);
	}
	else
	{
		addSendConnection(sourceChannel, destinationChannel);
	}

	refreshSourceUseStates();

	return true;
}

bool RoutableProcessor::MatrixData::addSendConnection(int sourceChannel, int destinationChannel) noexcept
{
	SimpleReadWriteLock::ScopedWriteLock sl(getLock());

	if (sourceChannel < 0 || sourceChannel >= getNumSourceChannels() || destinationChannel < 0 || destinationChannel >= getNumDestinationChannels())
	{
		return false;
	}

	sendConnections[sourceChannel] = destinationChannel;

	if (numAllowedConnections == 2)
	{
		int numConnections = 0;

		for (int i = 0; i < getNumSourceChannels(); i++)
		{
			if (sendConnections[i] != -1)
			{
				numConnections++;
			}
		}

		if (numConnections > 2)
		{
			const bool removeOldEvenConnection = sourceChannel % 2 == 0;

			for (int i = removeOldEvenConnection ? 0 : 1; i < getNumSourceChannels(); i += 2)
			{
				if (i == sourceChannel) continue;
				sendConnections[i] = -1;
			}
		}
	}

	refreshSourceUseStates();

	return true;
}

bool RoutableProcessor::MatrixData::removeSendConnection(int sourceChannel, int destinationChannel) noexcept
{
	SimpleReadWriteLock::ScopedWriteLock sl(getLock());

	if (sourceChannel < 0 || sourceChannel >= getNumSourceChannels() || destinationChannel < 0 || destinationChannel >= getNumDestinationChannels())
	{
		return false;
	}

	sendConnections[sourceChannel] = -1;

	if (numAllowedConnections == 2)
	{
		int numConnections = 0;

		for (int i = 0; i < getNumSourceChannels(); i++)
		{
			if (sendConnections[i] != -1)
			{
				numConnections++;
			}
		}

		if (numConnections < 2)
		{
			const int index = sourceChannel % 2 != 0 ? 1 : 0;

			sendConnections[index] = index;
		}
	}

	refreshSourceUseStates();

	return true;
}

int RoutableProcessor::MatrixData::getConnectionForSourceChannel(int sourceChannel) const noexcept
{
	if (sourceChannel < 0 || sourceChannel >= getNumSourceChannels())
	{
		return -1;
	}

	const int destination = channelConnections[sourceChannel];

	if (destination < numDestinationChannels)
	{
		return destination;
	}
	else return -1;
}

int RoutableProcessor::MatrixData::getSendForSourceChannel(int sourceChannel) const noexcept
{
	if (sourceChannel < 0 || sourceChannel >= getNumSourceChannels())
	{
		return -1;
	}

	const int destination = sendConnections[sourceChannel];

	if (destination < numDestinationChannels)
	{
		return destination;
	}
	else return -1;
}

void RoutableProcessor::MatrixData::resetToDefault()
{
	for (int i = 0; i < NUM_MAX_CHANNELS; i++)
	{
		channelConnections[i] = -1;
		sendConnections[i] = -1;
	}

	channelConnections[0] = 0;
	channelConnections[1] = 1;
    
    FloatVectorOperations::clear(targetGainValues, NUM_MAX_CHANNELS);
    FloatVectorOperations::clear(sourceGainValues, NUM_MAX_CHANNELS);

	refreshSourceUseStates();
}

ValueTree RoutableProcessor::MatrixData::exportAsValueTree() const
{
	ValueTree v("RoutingMatrix");

	v.setProperty("NumSourceChannels", numSourceChannels, nullptr);

	for (int i = 0; i < getNumSourceChannels(); i++)
	{
		v.setProperty("Channel" + String(i), channelConnections[i], nullptr);
		v.setProperty("Send" + String(i), sendConnections[i], nullptr);
	}
    
	return v;
}

void RoutableProcessor::MatrixData::restoreFromValueTree(const ValueTree &v)
{
	if (!(v.getType() == Identifier("RoutingMatrix"))) return;

	clearAllConnections();

	setNumSourceChannels(v.getProperty("NumSourceChannels", 2));

	for (int i = 0; i < getNumSourceChannels(); i++)
	{
		Identifier id = "Channel" + String(i);

		if (v.hasProperty(id))
		{
			const int dest = (int)v.getProperty(id, -1);

			channelConnections[i] = dest;
		}

		Identifier id2 = "Send" + String(i);

		if (v.hasProperty(id2))
		{
			const int dest = (int)v.getProperty(id2, -1);

			sendConnections[i] = dest;
		}
	}

	refreshSourceUseStates();
}

void RoutableProcessor::MatrixData::setNumSourceChannels(int newNumChannels, NotificationType notifyProcessors)
{
	jassert(newNumChannels <= NUM_MAX_CHANNELS);

	newNumChannels = jmin<int>(newNumChannels, NUM_MAX_CHANNELS);

	if (newNumChannels != numSourceChannels)
	{
		{
			SimpleReadWriteLock::ScopedWriteLock sl(getLock());
			numSourceChannels = jmax<int>(1, newNumChannels);
			refreshSourceUseStates();
		}

		if (notifyProcessors == sendNotification) 
			owningProcessor->numSourceChannelsChanged();
	}
}

void RoutableProcessor::MatrixData::setNumDestinationChannels(int newNumChannels, NotificationType notifyProcessors)
{
	jassert(newNumChannels <= NUM_MAX_CHANNELS);

	newNumChannels = jmin<int>(newNumChannels, NUM_MAX_CHANNELS);

	if (newNumChannels != numDestinationChannels)
	{
		SimpleReadWriteLock::ScopedWriteLock sl(getLock());

		numDestinationChannels = jmax<int>(1, newNumChannels);
		refreshSourceUseStates();
	}

	if (notifyProcessors == sendNotification) owningProcessor->numDestinationChannelsChanged();
}

void RoutableProcessor::MatrixData::handleDisplayValues(const AudioSampleBuffer& input, const AudioSampleBuffer& output, bool useOutput)
{
	if (anyChannelActive())
	{
		auto numToCheck = jmin(input.getNumSamples(), output.getNumSamples());

        float thisPeaks[NUM_MAX_CHANNELS];
        
		for (int i = 0; i < input.getNumChannels(); i++)
		{
			if (isEditorShown(i))
				thisPeaks[i] = input.getMagnitude(i, 0, numToCheck);
			else
				thisPeaks[i] = 0.0f;
		}

		setGainValues(thisPeaks, true);
		
		if (useOutput)
		{
			for (int i = 0; i < output.getNumChannels(); i++)
			{
				if (isEditorShown(i))
					thisPeaks[i] = output.getMagnitude(i, 0, numToCheck);
				else
					thisPeaks[i] = 0.0f;
			}
		}
		
        setGainValues(thisPeaks, false);
	}
}

SimpleReadWriteLock& RoutableProcessor::MatrixData::getLock() const
{
	return lock;
}

void RoutableProcessor::MatrixData::setTargetProcessor(Processor *p)
{
	targetProcessor = p;
}

float RoutableProcessor::MatrixData::getGainValue(int channelIndex, bool getSource) const
{
    if(auto sl =  SimpleReadWriteLock::ScopedTryReadLock(getLock()))
    {
        auto ptr = getSource ? sourceGainValues : targetGainValues;
        int numValues = (getSource ? numSourceChannels : numDestinationChannels);
        
        if(isPositiveAndBelow(channelIndex, numValues))
            return ptr[channelIndex];
    }
    
    return 0.0f;
}

void RoutableProcessor::MatrixData::setGainValues(float *numMaxChannelValues, bool isSourceValue)
{
	if (auto sl = SimpleReadWriteLock::ScopedTryReadLock(getLock()))
	{
        auto dst = isSourceValue ? sourceGainValues : targetGainValues;
        int numValues = (isSourceValue ? numSourceChannels : numDestinationChannels);
        
        auto useDecayFactors = upDecayFactor != 1.0f || downDecayFactor != 1.0f;
        
        const float s = Decibels::decibelsToGain(SilenceThreshold);
        
        if(useDecayFactors)
        {
            for(int i = 0; i < numValues; i++)
            {
                auto lastValue = dst[i];
                auto newValue = numMaxChannelValues[i];
                
                if(newValue > lastValue)
                    newValue = upDecayFactor * newValue + (1.0f - upDecayFactor) * lastValue;
                else
                    newValue = downDecayFactor * lastValue + (1.0f - downDecayFactor) * newValue;
                
                if(newValue < s)
                    newValue = 0.0f;
                
                dst[i] = newValue;
            }
        }
        else
        {
            memcpy(dst, numMaxChannelValues, numValues * sizeof(float));
        }
	}
}

void RoutableProcessor::MatrixData::loadPreset(Presets newPreset)
{
	Presets pr = (Presets)newPreset;

	clearAllConnections();

	switch (pr)
	{
	case Presets::AllChannels:
		for (int i = 0; i < getNumSourceChannels(); i++)
		{
			addConnection(i, i);
		}
		break;
	case Presets::FirstStereo:
		addConnection(0, 0);
		addConnection(1, 1);
		break;
	case Presets::SecondStereo:
		addConnection(2, 2);
		addConnection(3, 3);

		break;
	case Presets::ThirdStereo:
		addConnection(4, 4);
		addConnection(5, 5);

		break;
	case Presets::AllChannelsToStereo:
		for (int i = 0; i < getNumSourceChannels(); i++)
		{
			addConnection(i, i % 2 != 0 ? 1 : 0);
		}
		break;
	default:
		break;
	}
}

void RoutableProcessor::MatrixData::refreshSourceUseStates()
{
	if (numAllowedConnections == 2)
	{
		for (int i = 0; i < numSourceChannels; i++)
		{
			if (channelConnections[i] != -1)
			{
				owningProcessor->leftSourceChannel = i;
				owningProcessor->leftTargetChannel = channelConnections[i];
				break;
			}

		}

		for (int i = numSourceChannels - 1; i >= 0; i--)
		{
			if (channelConnections[i] != -1)
			{
				owningProcessor->rightSourceChannel = i;
				owningProcessor->rightTargetChannel = channelConnections[i];
				break;
			}
		}
	}
	else
	{
		owningProcessor->leftSourceChannel = -1;
		owningProcessor->rightSourceChannel = -1;
		owningProcessor->leftTargetChannel = -1;
		owningProcessor->rightTargetChannel = -1;

		for (int i = 0; i < numSourceChannels; i++)
		{
			if (channelConnections[i] != -1)
			{
				if (owningProcessor->leftSourceChannel == -1)
				{
					owningProcessor->leftSourceChannel = i;
					owningProcessor->leftTargetChannel = channelConnections[i];
				}
				else
				{
					owningProcessor->rightSourceChannel = i;
					owningProcessor->rightTargetChannel = channelConnections[i];
					break;
				}
			}
		}
	}
	

	owningProcessor->connectionChanged();

	sendChangeMessage();
}


RoutableProcessor::RoutableProcessor() :
data(this),
leftSourceChannel(-1),
rightSourceChannel(-1),
leftTargetChannel(-1),
rightTargetChannel(-1)
{

}

RoutableProcessor::~RoutableProcessor()
{}

void RoutableProcessor::MatrixData::setDecayCoefficients(float newUpDecayFactor, float newDownDecayFactor)
{
	upDecayFactor = jlimit(0.0f, 1.0f, newUpDecayFactor);
	downDecayFactor = jlimit(0.0f, 1.0f, newDownDecayFactor);
}

bool RoutableProcessor::MatrixData::resizingIsAllowed() const noexcept
{ return resizeAllowed; }

void RoutableProcessor::MatrixData::setAllowResizing(bool shouldAllowResizing) noexcept
{ resizeAllowed = shouldAllowResizing; }

bool RoutableProcessor::MatrixData::onlyEnablingAllowed() const noexcept
{ return allowEnablingOnly; }

int RoutableProcessor::MatrixData::getNumAllowedConnections() const
{ return numAllowedConnections; }

int RoutableProcessor::MatrixData::getNumSourceChannels() const
{ return numSourceChannels; }

int RoutableProcessor::MatrixData::getNumDestinationChannels() const
{ return numDestinationChannels; }

void RoutableProcessor::MatrixData::setOnlyEnablingAllowed(bool noRouting)
{ allowEnablingOnly = noRouting; }

void RoutableProcessor::MatrixData::setNumAllowedConnections(int newNum) noexcept
{ numAllowedConnections = newNum; }

bool RoutableProcessor::MatrixData::isEditorShown(int channelIndex) const noexcept
{ 
	jassert(isPositiveAndBelow(channelIndex, NUM_MAX_CHANNELS));

	return numEditors[channelIndex] > 0; 
}

bool RoutableProcessor::MatrixData::anyChannelActive() const noexcept
{
	return anyActive;
}

void RoutableProcessor::MatrixData::setEditorShown(Array<int> channelIndexes, bool isShown) noexcept
{
	for (auto& c : channelIndexes)
	{
		if (isPositiveAndBelow(c, NUM_MAX_CHANNELS))
		{
			if (isShown)
				numEditors[c]++;
			else
				numEditors[c] = jmax(0, numEditors[c] - 1);
		}
	}

	anyActive = false;

	for (int i = 0; i < NUM_MAX_CHANNELS; i++)
		anyActive |= (numEditors[i] != 0);
}

String RoutableProcessor::MatrixData::getTargetName() const
{ 
	if (!isProcessorMatrix())
		return "Output";

	return targetProcessor.get() != nullptr ? targetProcessor.get()->getId() : "HISE Output"; 
}

String RoutableProcessor::MatrixData::getSourceName() const
{ 
	if (!isProcessorMatrix())
		return "Input";

	return thisAsProcessor->getId(); 
}

void RoutableProcessor::MatrixData::init(Processor* pTouse)
{
	thisAsProcessor = pTouse != nullptr ? pTouse : dynamic_cast<Processor*>(owningProcessor);
	resetToDefault();
}

bool RoutableProcessor::MatrixData::isProcessorMatrix() const
{
	return dynamic_cast<Processor*>(owningProcessor) != nullptr;
}

void RoutableProcessor::connectionChanged()
{}

const RoutableProcessor::MatrixData& RoutableProcessor::getMatrix() const
{ return data; }

RoutableProcessor::MatrixData& RoutableProcessor::getMatrix()
{ return data; }

int RoutableProcessor::getLeftSourceChannel() const
{ return leftSourceChannel; }

int RoutableProcessor::getRightSourceChannel() const
{ return rightSourceChannel; }

int RoutableProcessor::getLeftDestinationChannel() const
{ return leftTargetChannel; }

int RoutableProcessor::getRightDestinationChannel() const
{ return leftTargetChannel; }

void RoutableProcessor::editRouting(Component *childComponent)
{
#if USE_BACKEND

	AlertWindowLookAndFeel laf;

	auto editor = GET_BACKEND_ROOT_WINDOW(childComponent);

	if (editor != nullptr)
	{
		String id = dynamic_cast<Processor*>(this)->getId();

		editor->getRootFloatingTile()->showComponentInRootPopup(new RouterComponent(&getMatrix()), childComponent, childComponent->getLocalBounds().getCentre());

	}
#else 

	ignoreUnused(childComponent);

#endif
	
}

} // namespace hise
