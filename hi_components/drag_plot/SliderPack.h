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

#ifndef SLIDERPACK_H_INCLUDED
#define SLIDERPACK_H_INCLUDED

namespace hise { using namespace juce;

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

	void swapData(Array<var> &otherData)
	{
		values = var(otherData);

		sendChangeMessage();
	}

	void setDisplayedIndex(int index)
	{
		nextIndexToDisplay = index;
		sendAllocationFreeChangeMessage();
	}

	var getDataArray() const { return values; }

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

	var values;

	//Array<float> values;
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

	SET_GENERIC_PANEL_ID("ArrayEditor");

	/** Creates a new SliderPack. */
	SliderPack(SliderPackData *data=nullptr);

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

	const SliderPackData* getData() const { return data; }

	void resized() override;
	void setValuesFromLine();
	int getNumSliders();
private:

	SliderPackData dummyData;

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


} // namespace hise
#endif  // SLIDERPACK_H_INCLUDED
