/*
  ==============================================================================

    SliderPack.h
    Created: 8 Aug 2015 6:22:36pm
    Author:  Chrisboy

  ==============================================================================
*/

#ifndef SLIDERPACK_H_INCLUDED
#define SLIDERPACK_H_INCLUDED

/** The data model for a SliderPack widget. */
class SliderPackData: public SafeChangeBroadcaster
{
public:

	SliderPackData();

	~SliderPackData();

	void setRange(double minValue, double maxValue, double stepSize);

	Range<double> getRange() const;

	double getStepSize() const;

	void setNumSliders(int numSliders);

	int getNumSliders() const;

	void setValue(int sliderIndex, float value, NotificationType notifySliderPack=dontSendNotification);

	float getValue(int index) const;

	void setFromFloatArray(const Array<float> &valueArray);

	void writeToFloatArray(Array<float> &valueArray) const;

	String toBase64() const;

	void fromBase64(const String &encodedValues);

	int getNextIndexToDisplay() const
	{
		return nextIndexToDisplay;
	}

	void clearDisplayIndex()
	{
		nextIndexToDisplay = -1;
	}

	void setDisplayedIndex(int index)
	{
		nextIndexToDisplay = index;
		sendChangeMessage();
	}
	void setFlashActive(bool shouldBeShown) { flashActive = shouldBeShown; };
	void setShowValueOverlay(bool shouldBeShown) { showValueOverlay = shouldBeShown; };

	bool isFlashActive() const { return flashActive; }
	bool isValueOverlayShown() const { return showValueOverlay; }

private:

	bool flashActive;
	bool showValueOverlay;

	WeakReference<SliderPackData>::Master masterReference;

	friend class WeakReference < SliderPackData > ;

	int nextIndexToDisplay;
	

	Range<double> sliderRange;

	double stepSize;

	Array<float> values;
};


/** A widget which contains multiple Sliders which support dragging & bipolar display. */
class SliderPack : public Component,
				   public Slider::Listener,
				   public SafeChangeListener,
				   public Timer
{
public:

	class Listener
	{
	public:

		virtual ~Listener();
		virtual void sliderPackChanged(SliderPack *s, int index ) = 0;

	private:
		WeakReference<Listener>::Master masterReference;
		friend class WeakReference<Listener>;
		
	};

	/** Creates a new SliderPack. */
	SliderPack(SliderPackData *data);

	~SliderPack();

	void addListener(Listener *listener)
	{
		listeners.addIfNotAlreadyThere(listener);
	}

	void removeListener(Listener *listener)
	{
		listeners.removeAllInstancesOf(listener);
	}

	void timerCallback() override;

	/** Sets the number of sliders shown. This clears all values. */
	void setNumSliders(int numSliders);

	/** Returns the value of the slider index. If the index is bigger than the slider amount, it will return -1. */
	double getValue(int sliderIndex);

	/** Sets the value of one of the sliders. If the index is bigger than the slider amount, it will do nothing. */
	void setValue(int sliderIndex, double newValue);

	void updateSliders();

	void changeListenerCallback(SafeChangeBroadcaster *b) override;

	void mouseDown(const MouseEvent &e) override;

	void mouseDrag(const MouseEvent &e) override;

	void mouseUp(const MouseEvent &e) override;

	void mouseDoubleClick(const MouseEvent &e) override;

	void mouseExit(const MouseEvent &e) override;

	void update();

	void sliderValueChanged(Slider *s) override;

	void paintOverChildren(Graphics &g) override;

	void paint(Graphics &g);

	void setSuffix(const String &suffix);

	void setDisplayedIndex(int displayIndex);

	void setDefaultValue(double defaultValue);

	void resized() override;
	void setValuesFromLine();
	int getNumSliders();
private:

	Array<WeakReference<Listener>> listeners;

	String suffix;

	double defaultValue;

	Array<float> displayAlphas;

	Line<float> rightClickLine;

	bool currentlyDragged;

	int currentlyDraggedSlider;

	double currentlyDraggedSliderValue;

	BiPolarSliderLookAndFeel laf;

	WeakReference<SliderPackData> data;

	OwnedArray<Slider> sliders;
};



#endif  // SLIDERPACK_H_INCLUDED
