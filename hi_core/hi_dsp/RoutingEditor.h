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
*   along with Foobar.  If not, see <http://www.gnu.org/licenses/>.
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

#ifndef ROUTINGEDITOR_H_INCLUDED
#define ROUTINGEDITOR_H_INCLUDED



class RouterComponent : public Component,
						public SafeChangeListener,
						public Timer
{
public:

	enum Presets
	{
		AllChannels = 10000,
		FirstStereo,
		SecondStereo,
		ThirdStereo,
		AllChannelsToStereo
	};

	class ChannelConnector : public Component
	{
	public:

		ChannelConnector(bool isSource_, int index_);;

		void paintOverChildren(Graphics& g) override;

		void resized()
		{
			inMeter->setBounds(0,isSource ? 0 : 5, getWidth() * 2-2, getHeight() - 4);
		}

		Point<int> getConnectionPoint() const {	return Point<int>(getWidth() / 2, isSource ? getHeight() - 3 : 3); }
		Point<int> getConnectionPointInParent() const { return getParentComponent()->getLocalPoint(this, getConnectionPoint()); }

		void setUsed(bool shouldBeUsed) { used = shouldBeUsed; repaint(); }
		void setSelected(bool shouldBeSelected, bool shouldBeSelectedAsSend) { selected = shouldBeSelected; selectedAsSend = shouldBeSelectedAsSend; repaint(); }
		bool isDifferent(const ChannelConnector *otherConnector) const { return otherConnector->isSource != isSource; }

		void setGainValue(float gainValue);

	private:

		ScopedPointer<VuMeter> inMeter;
		
		int index;
		bool used;
		bool connected;
		bool isSource;
		bool selected;
		bool selectedAsSend;
		float gainValue;
	};


	RouterComponent(RoutableProcessor::MatrixData *data_);;

	~RouterComponent();

	void resized() override;
	void paint(Graphics& g) override;

	void timerCallback() override
	{
		for (int i = 0; i < sourceChannels.size(); i++)
		{
			sourceChannels[i]->setGainValue(data->getGainValue(i, true) * 2.0f);
		}
		for (int i = 0; i < destinationChannels.size(); i++)
		{
			destinationChannels[i]->setGainValue(data->getGainValue(i, false) * 2.0f);
		}
	}

	void deselectAll();
	void changeListenerCallback(SafeChangeBroadcaster* l) override;

	void rebuildConnectors();
	void refreshConnectedState();
	int getConnectorIndex(const ChannelConnector *firstConnector, const ChannelConnector *secondConnector, bool getSource);
	void mouseDown(const MouseEvent &e) override;
	void selectConnector(const MouseEvent & e);

private:

	void setToPreset(int preset);

	OwnedArray<ChannelConnector> sourceChannels;
	OwnedArray<ChannelConnector> destinationChannels;
	Component::SafePointer<ChannelConnector> selectedConnector;
	WeakReference<Processor> processor;
	RoutableProcessor::MatrixData *data;

};

#endif  // ROUTINGEDITOR_H_INCLUDED
