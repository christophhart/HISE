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

RouterComponent::ChannelConnector::ChannelConnector(bool isSource_, int index_) :
isSource(isSource_),
index(index_),
selected(false),
used(false),
gainValue(0.0f),
selectedAsSend(false)
{
	addAndMakeVisible(inMeter = new VuMeter());
	inMeter->setType(VuMeter::StereoVertical);
    inMeter->setPeak(0.0f, 0.0f);
	inMeter->setColour(VuMeter::backgroundColour, Colour(0xFF333333));
	inMeter->setColour(VuMeter::ledColour, Colours::lightgrey);
	inMeter->setInterceptsMouseClicks(false, false);
}

void RouterComponent::ChannelConnector::paintOverChildren(Graphics& g)
{
	g.setFont(GLOBAL_BOLD_FONT());

	g.setColour(selected ? (selectedAsSend ? Colours::cyan.withAlpha(0.6f) : Colours::red.withAlpha(0.6f)) : Colours::white.withAlpha(0.6f));
	g.drawRect(getLocalBounds(), 1);

	float h = (float)getHeight() - 3.0f;

	g.drawLine(0.0, isSource ? h : 3.0f, (float)getWidth(), isSource ? h : 3.0f, 6.0f);

	g.setColour(Colours::white);

	Point<int> connector = getConnectionPoint();

	g.fillEllipse((float)connector.getX() - 3.0f, (float)connector.getY() - 3.0f, 6.0f, 6.0f);

	g.setColour(used ? Colours::white : Colours::white.withAlpha(0.2f));

	g.drawText(String(index + 1), getLocalBounds(), Justification::centred, true);
}

void RouterComponent::ChannelConnector::resized()
{
	inMeter->setBounds(0, isSource ? 0 : 5, getWidth() * 2 - 2, getHeight() - 4);
}

Point<int> RouterComponent::ChannelConnector::getConnectionPoint() const
{	return Point<int>(getWidth() / 2, isSource ? getHeight() - 3 : 3); }

Point<int> RouterComponent::ChannelConnector::getConnectionPointInParent() const
{ return getParentComponent()->getLocalPoint(this, getConnectionPoint()); }

void RouterComponent::ChannelConnector::setUsed(bool shouldBeUsed)
{ used = shouldBeUsed; repaint(); }

void RouterComponent::ChannelConnector::setSelected(bool shouldBeSelected, bool shouldBeSelectedAsSend)
{ selected = shouldBeSelected; selectedAsSend = shouldBeSelectedAsSend; repaint(); }

bool RouterComponent::ChannelConnector::isDifferent(const ChannelConnector* otherConnector) const
{ return otherConnector->isSource != isSource; }

void RouterComponent::timerCallback()
{
	if (data.get() == nullptr)
		return;

	for (int i = 0; i < sourceChannels.size(); i++)
	{
		sourceChannels[i]->setGainValue(data->getGainValue(i, true) * 2.0f);
	}
	for (int i = 0; i < destinationChannels.size(); i++)
	{
		destinationChannels[i]->setGainValue(data->getGainValue(i, false) * 2.0f);
	}
}

void RouterComponent::ChannelConnector::setGainValue(float newGainValue)
{
	inMeter->setPeak(newGainValue, newGainValue);
}

RouterComponent::RouterComponent(RoutableProcessor::MatrixData *data_)
{
	setName("Routing Matrix");

	data = data_;
	data->addChangeListener(this);

	auto numChannels = jmax(data->getNumDestinationChannels(), data->getNumSourceChannels());

	Array<int> channelIndexes;

	for (int i = 0; i < numChannels; i++)
		channelIndexes.add(i);


  	data->setEditorShown(channelIndexes, true);
	

	rebuildConnectors();

	setSize(600, 200);
	
	startTimer(30);
	//START_TIMER();
}

RouterComponent::~RouterComponent()
{
	if (data != nullptr)
	{
		data->removeChangeListener(this);

		auto numChannels = jmax(data->getNumDestinationChannels(), data->getNumSourceChannels());

		Array<int> channelIndexes;

		for (int i = 0; i < numChannels; i++)
			channelIndexes.add(i);

		data->setEditorShown(channelIndexes, false);
	}
	
}

void RouterComponent::resized()
{
	if (data == nullptr)
		return;

	const int width = jmin<int>(getWidth()-16, jmax<int>(data->getNumSourceChannels(), data->getNumDestinationChannels()) * 60);

	Rectangle<int> routingBounds(0, 0, width, findParentComponentOfClass<ProcessorEditorBody>() != nullptr ? 128 : 192);

	routingBounds.setCentre(getLocalBounds().getCentreX(), getLocalBounds().getCentreY());

	const int widthPerSourceChannel = routingBounds.getWidth() / data->getNumSourceChannels();
	const int widthPerDestinationChannel = routingBounds.getWidth() / data->getNumDestinationChannels();

	const int widthPerChannel = jmin<int>(widthPerSourceChannel, widthPerDestinationChannel);

	const int sourceXOffset = (routingBounds.getWidth() - data->getNumSourceChannels() * widthPerChannel) / 2;
	const int destinationXOffset = (routingBounds.getWidth() - data->getNumDestinationChannels() * widthPerChannel) / 2;

	for (int i = 0; i < data->getNumSourceChannels(); i++)
	{
		sourceChannels[i]->setBounds(sourceXOffset + routingBounds.getX() + i * widthPerChannel + 1, routingBounds.getY() + 20, widthPerChannel - 2, 32);
	}
	for (int i = 0; i < data->getNumDestinationChannels(); i++)
	{
		destinationChannels[i]->setBounds(destinationXOffset + routingBounds.getX() + i * widthPerChannel + 1, routingBounds.getBottom() - 56, widthPerChannel - 2, 32);
	}
}

void RouterComponent::paint(Graphics& g)
{
	//g.setGradientFill(ColourGradient(Colour(0xBB444444), 0.0f, 0.0f,
								//	 Colour(0xBB222222), 0.0f, (float)getHeight(), false));

	//g.fillRoundedRectangle(0.0f, 0.0f, (float)getWidth(), (float)getHeight(), 6.0f);
	
	
	g.setColour(Colours::white);
	g.setFont(GLOBAL_BOLD_FONT());

	if (data == nullptr)
	{
		g.drawText("Matrix was deleted, reopen this window", getLocalBounds().toFloat(), Justification::centred);
		return;
	}
		

	g.drawText(data->getSourceName(), 0, sourceChannels[0]->getY() - 20, getWidth(), 20, Justification::centred, true);

	g.drawText(data->getTargetName(), 0, destinationChannels[0]->getBottom(), getWidth(), 20, Justification::centred, true);

	g.setColour(Colours::white.withAlpha(0.7f));

	

	for (int i = 0; i < sourceChannels.size(); i++)
	{
		auto thisComponent = sourceChannels[i];
		auto nextComponent = sourceChannels[i + 1];

		auto b = thisComponent->getBounds().toFloat();

		if (i % 2 == 0)
		{
			g.drawLine(b.getX(), b.getY() - 2.0f, nextComponent != nullptr ? nextComponent->getRight() : b.getRight(), b.getY() - 2.0f, 2.0f);
		}


		const int j = data->getConnectionForSourceChannel(i);

		if (j != -1)
		{
			Point<int> start = sourceChannels[i]->getConnectionPointInParent();
			Point<int> end = destinationChannels[j]->getConnectionPointInParent();

			g.drawLine((float)start.getX(), (float)start.getY(), (float)end.getX(), (float)end.getY(), 2);
		}
	}

	g.setColour(Colour(0xffAAAAFF));

	for (int i = 0; i < sourceChannels.size(); i++)
	{
		const int j = data->getSendForSourceChannel(i);

		if (j != -1)
		{
			Point<int> start = sourceChannels[i]->getConnectionPointInParent();
			Point<int> end = destinationChannels[j]->getConnectionPointInParent();

			g.drawLine((float)start.getX(), (float)start.getY(), (float)end.getX(), (float)end.getY(), 2);
		}
	}
}

void RouterComponent::deselectAll()
{
	selectedConnector = nullptr;

	for (int i = 0; i < data->getNumSourceChannels(); i++)
	{
		sourceChannels[i]->setSelected(false, false);

	}

	for (int i = 0; i < data->getNumDestinationChannels(); i++)
	{
		destinationChannels[i]->setSelected(false, false);

	}
}

void RouterComponent::changeListenerCallback(SafeChangeBroadcaster* )
{
	rebuildConnectors();
	resized();
	repaint();
}

void RouterComponent::rebuildConnectors()
{
	sourceChannels.clear();
	destinationChannels.clear();

	for (int i = 0; i < data->getNumSourceChannels(); i++)
	{
		sourceChannels.add(new ChannelConnector(true, i));
		addAndMakeVisible(sourceChannels.getLast());
		sourceChannels.getLast()->addMouseListener(this, false);
	}

	for (int i = 0; i < data->getNumDestinationChannels(); i++)
	{
		destinationChannels.add(new ChannelConnector(false, i));
		addAndMakeVisible(destinationChannels.getLast());
		destinationChannels.getLast()->addMouseListener(this, false);
	}

	refreshConnectedState();
}

void RouterComponent::refreshConnectedState()
{
	for (int i = 0; i < destinationChannels.size(); i++)
	{
		destinationChannels[i]->setUsed(false);
	}

	for (int i = 0; i < sourceChannels.size(); i++)
	{
		const int j = data->getConnectionForSourceChannel(i);

		bool sourceIsUsed = false;

		if (j >= 0 && j < destinationChannels.size())
		{
			sourceIsUsed = true;
			destinationChannels[j]->setUsed(true);
		}

		ChannelConnector *cc = sourceChannels[i];

		cc->setUsed(sourceIsUsed);
	}
}

int RouterComponent::getConnectorIndex(const ChannelConnector *firstConnector, const ChannelConnector *secondConnector, bool getSource)
{
	int index = getSource ? sourceChannels.indexOf(firstConnector) : destinationChannels.indexOf(firstConnector);

	if (index == -1) return getSource ? sourceChannels.indexOf(secondConnector) : destinationChannels.indexOf(secondConnector);
	else return index;
}

void RouterComponent::mouseDown(const MouseEvent &e)
{
	CHECK_MIDDLE_MOUSE_DOWN(e);

	if (e.mods.isRightButtonDown())
	{
		PopupLookAndFeel laf;
		PopupMenu m;

		enum
		{
			Copy = 200000,
			Paste,
			Clear
		};

		m.setLookAndFeel(&laf);

		if (data->resizingIsAllowed())
		{
			PopupMenu channelAmount;

			for (int i = 2; i <= NUM_MAX_CHANNELS; i += 2)
			{
				channelAmount.addItem(i, String(i), "Channels");
			}

			m.addSectionHeader("Change Channel Amount");
			m.addSubMenu("Change Channel Amount", channelAmount);
		}

		m.addSectionHeader("Set to Preset Routing");
		m.addItem((int)RoutableProcessor::Presets::AllChannels, "All Channels");
		m.addItem((int)RoutableProcessor::Presets::AllChannelsToStereo, "All Channels to Stereo");
		m.addItem((int)RoutableProcessor::Presets::FirstStereo, "First Stereo only");
		m.addItem((int)RoutableProcessor::Presets::SecondStereo, "Second Stereo only");
		m.addItem((int)RoutableProcessor::Presets::ThirdStereo, "Third Stereo only");

		m.addSectionHeader("Load Save");
		m.addItem(Copy, "Copy Channel Routing");
		m.addItem(Paste, "Paste Channel Routing");
		m.addItem(Clear, "Clear Channel Routing");

		const int result = m.show();

		if (result == 0) return;

		if (result <= NUM_MAX_CHANNELS)
		{
			data->setNumSourceChannels(result);
		}
		else if (result == Copy)
		{
			auto xml = data->exportAsValueTree().createXml();

			SystemClipboard::copyTextToClipboard(xml->createDocument(""));

		}
		else if (result == Paste)
		{
			auto xml = XmlDocument::parse(SystemClipboard::getTextFromClipboard());
			if (xml != nullptr)
			{
				ValueTree v = ValueTree::fromXml(*xml);

				if (v.isValid() && v.getType() == Identifier("RoutingMatrix"))
				{
					data->restoreFromValueTree(v);
				}
			}

		}
		else if (result == Clear)
		{
			data->clearAllConnections();
		}

		else
		{
			setToPreset(result);
		}
	}
	else
	{
		selectConnector(e);
	}
}


void RouterComponent::selectConnector(const MouseEvent & e)
{
	const bool useSend = e.mods.isShiftDown();

	ChannelConnector *oldChannel = selectedConnector.getComponent();

	selectedConnector = dynamic_cast<ChannelConnector*>(e.eventComponent);

	if (selectedConnector != nullptr)
	{
		if (data->onlyEnablingAllowed())
		{
			int i = destinationChannels.indexOf(selectedConnector);

			if (i == -1) i = sourceChannels.indexOf(selectedConnector);

			useSend ? data->toggleSendEnabling(i) : data->toggleEnabling(i);
			deselectAll();
			refreshConnectedState();
			repaint();
			return;
		}

		if (oldChannel != nullptr)
		{
			if (oldChannel->isDifferent(selectedConnector))
			{
				const int sourceIndex = getConnectorIndex(selectedConnector, oldChannel, true);
				const int destinationIndex = getConnectorIndex(selectedConnector, oldChannel, false);

				useSend ? data->toggleSendConnection(sourceIndex, destinationIndex) : data->toggleConnection(sourceIndex, destinationIndex);

				refreshConnectedState();

				deselectAll();
				repaint();
			}
			else
			{
				oldChannel->setSelected(false, false);
				oldChannel->repaint();
				selectedConnector->setSelected(true, useSend);

			}
		}
		else
		{
			selectedConnector->setSelected(true, useSend);
			selectedConnector->repaint();

		}


	}
	else
	{
		deselectAll();
	}

}


void RouterComponent::setToPreset(int preset)
{
	data->loadPreset((RoutableProcessor::Presets)preset);
}

} // namespace hise