/*
  ==============================================================================

    RoutingEditor.h
    Created: 9 Nov 2015 9:00:55am
    Author:  Christoph

  ==============================================================================
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
