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

#define OLD_LISTENER_SP 0

/** The data model for a SliderPack component. */
class SliderPackData: public SafeChangeBroadcaster,
					  public ComplexDataUIBase
{
public:

	static constexpr int NumDefaultSliders = 16;

	class Listener : private ComplexDataUIUpdaterBase::EventListener
	{
	public:

		virtual ~Listener() {};

		/** Callback that will be executed when a slider is moved. You can get the actual value with SliderPack::getValue(int index). */
		virtual void sliderPackChanged(SliderPackData *d, int index) = 0;

		/** Will be executed when the amount of sliders has changed. */
		virtual void sliderAmountChanged(SliderPackData* d) {};

		/** Will be executed when the displayed index has changed. */
		virtual void displayedIndexChanged(SliderPackData* d, int newIndex) {};

	private:

		friend class SliderPackData;

		/** @internal (only used by the slider pack data). */
		void setSliderPack(SliderPackData* d)
		{
			connectedSliderPack = d;
		}

		void onComplexDataEvent(ComplexDataUIUpdaterBase::EventType t, var v) override
		{
			switch (t)
			{
			case ComplexDataUIUpdaterBase::EventType::ContentRedirected:
				sliderAmountChanged(connectedSliderPack);
				break;
			case ComplexDataUIUpdaterBase::EventType::ContentChange:
				sliderPackChanged(connectedSliderPack, (int)v);
				break;
			case ComplexDataUIUpdaterBase::EventType::DisplayIndex:
				displayedIndexChanged(connectedSliderPack, (int)v);
				break;
            default:
                break;
			}
		}

		WeakReference<SliderPackData> connectedSliderPack;
	};

#if OLD_LISTENER_SP
	/** Inherit from this class in order to get notified about changes to the slider pack. */
	class Listener
	{
	public:

		virtual ~Listener();

		/** Callback that will be executed when a slider is moved. You can get the actual value with SliderPack::getValue(int index). */
		virtual void sliderPackChanged(SliderPackData *d, int index) = 0;

		/** Will be executed when the amount of sliders has changed. */
		virtual void sliderAmountChanged(SliderPackData* d) {};

		/** Will be executed when the displayed index has changed. */
		virtual void displayedIndexChanged(SliderPackData* d, int newIndex) {};

	private:
		WeakReference<Listener>::Master masterReference;
		friend class WeakReference<Listener>;

	};
#endif

	SliderPackData(UndoManager* undoManager=nullptr, PooledUIUpdater* updater=nullptr);

	~SliderPackData();

	void setRange(double minValue, double maxValue, double stepSize);

	Range<double> getRange() const;

	void startDrag()
	{
		
	}

	static String dataVarToBase64(const var& data);
	static var base64ToDataVar(const String& b64);

	bool fromBase64String(const String& b64) override
	{
		fromBase64(b64);

		return getNumSliders() > 0;
	}

	String toBase64String() const override
	{
		return toBase64();
	}

	double getStepSize() const;

	void setNumSliders(int numSliders);

	int getNumSliders() const;

	void setValue(int sliderIndex, float value, NotificationType notifySliderPack=dontSendNotification, bool useUndoManager=false);

	float getValue(int index) const;

	void setFromFloatArray(const Array<float> &valueArray);

	void writeToFloatArray(Array<float> &valueArray) const;

	String toBase64() const;

	void fromBase64(const String &encodedValues);

	int getNextIndexToDisplay() const
	{
		return nextIndexToDisplay;
	}

	void swapData(const var &otherData, NotificationType n);

	void setDisplayedIndex(int index)
	{
		nextIndexToDisplay = index;
		internalUpdater.sendDisplayChangeMessage((float)index, sendNotificationAsync);
	}

	const float* getCachedData() const
	{
		return dataBuffer->buffer.getReadPointer(0);
	}

	var getDataArray() const { return var(dataBuffer.get()); }

	void setFlashActive(bool shouldBeShown) { flashActive = shouldBeShown; };
	void setShowValueOverlay(bool shouldBeShown) { showValueOverlay = shouldBeShown; };

	bool isFlashActive() const { return flashActive; }
	bool isValueOverlayShown() const { return showValueOverlay; }

	void setDefaultValue(double newDefaultValue)
	{
		defaultValue = (float)newDefaultValue;
	}

	void setNewUndoAction() const;

	/** Register a listener that will receive notification when the sliders are changed. */
	void addListener(Listener *listener)
	{
		listener->setSliderPack(this);
		internalUpdater.addEventListener(listener);
	}

	/** Removes a previously registered listener. */
	void removeListener(Listener *listener)
	{
		listener->setSliderPack(nullptr);
		internalUpdater.removeEventListener(listener);
	}

	void sendValueChangeMessage(int index, NotificationType notify=sendNotificationSync)
	{
		internalUpdater.sendContentChangeMessage(notify, index);
	}

private:

	void swapBuffer(VariantBuffer::Ptr otherBuffer, NotificationType n);

	struct SliderPackAction : public UndoableAction
	{
		SliderPackAction(SliderPackData* data_, int sliderIndex_, float oldValue_, float newValue_, NotificationType n_) :
			UndoableAction(),
			data(data_),
			sliderIndex(sliderIndex_),
			oldValue(oldValue_),
			newValue(newValue_),
			n(n_)
		{};

		bool perform() override
		{
			if (data != nullptr)
			{
				data->setValue(sliderIndex, newValue, n, false);
				return true;
			}

			return false;
		}

		bool undo() override
		{
			if (data != nullptr)
			{
				data->setValue(sliderIndex, oldValue, n, false);
				return true;
			}

			return false;
		}

		WeakReference<SliderPackData> data;
		int sliderIndex;
		float oldValue, newValue;
		NotificationType n;
	};

	bool flashActive;
	bool showValueOverlay;

	VariantBuffer::Ptr dataBuffer;
	
	int nextIndexToDisplay;
	
	Range<double> sliderRange;

	double stepSize;

	float defaultValue;

	JUCE_DECLARE_WEAK_REFERENCEABLE(SliderPackData);
};




/** A Component which contains multiple Sliders which support dragging & bipolar display. 
	@ingroup hise_ui
	
	This class is driven by the SliderPackData class, which acts as data container.
*/
class SliderPack : public Component,
				   public Slider::Listener,
				   public SliderPackData::Listener,
				   public Timer,
				   public ComplexDataUIBase::EditorBase
{
public:

	using Listener = SliderPackData::Listener;

	struct LookAndFeelMethods
	{
		virtual ~LookAndFeelMethods() {};

		virtual void drawSliderPackBackground(Graphics& g, SliderPack& s);
		virtual void drawSliderPackFlashOverlay(Graphics& g, SliderPack& s, int sliderIndex, Rectangle<int> sliderBounds, float intensity);
		virtual void drawSliderPackRightClickLine(Graphics& g, SliderPack& s, Line<float> lineToDraw);
		virtual void drawSliderPackTextPopup(Graphics& g, SliderPack& s, const String& textToDraw);
	};

	struct SliderLookAndFeel : public BiPolarSliderLookAndFeel,
							   public LookAndFeelMethods
	{

		void drawLinearSlider(Graphics &g, int /*x*/, int /*y*/, int width,
			int height, float /*sliderPos*/, float /*minSliderPos*/,
			float /*maxSliderPos*/, const Slider::SliderStyle style, Slider &s) override;
	};

	SET_GENERIC_PANEL_ID("ArrayEditor");

	/** Creates a new SliderPack. */
	SliderPack(SliderPackData *data=nullptr);

	~SliderPack();

	void sliderAmountChanged(SliderPackData* d) override
	{
		slidersNeedRebuild = true;
		startTimer(30);
	}

	void sliderPackChanged(SliderPackData *s, int index) override
	{
		for (int i = 0; i < sliders.size(); i++)
		{
			auto shouldBe = data->getValue(i);

			if (sliders[i]->getValue() != shouldBe)
				sliders[i]->setValue(shouldBe, dontSendNotification);
		}
	} 

	void displayedIndexChanged(SliderPackData* d, int newIndex) override;

	void timerCallback() override;

	/** Sets the number of sliders shown. This clears all values. */
	void setNumSliders(int numSliders);

	/** Returns the value of the slider index. If the index is bigger than the slider amount, it will return -1. */
	double getValue(int sliderIndex);

	/** Sets the value of one of the sliders. If the index is bigger than the slider amount, it will do nothing. */
	void setValue(int sliderIndex, double newValue);

	int getCurrentlyDraggedSliderIndex() const { return currentlyDraggedSlider; }
	double getCurrentlyDraggedSliderValue() const { return currentlyDraggedSliderValue; }

	void setSliderPackData(SliderPackData* newData);

	void setComplexDataUIBase(ComplexDataUIBase* newData) override
	{
		if (auto sp = dynamic_cast<SliderPackData*>(newData))
			setSliderPackData(sp);
	}

	void updateSliderRange();

	void updateSliders();

#if 0
	void changeListenerCallback(SafeChangeBroadcaster *b) override;
#endif

	void mouseDown(const MouseEvent &e) override;
	void mouseDrag(const MouseEvent &e) override;
	void mouseUp(const MouseEvent &e) override;
	void mouseDoubleClick(const MouseEvent &e) override;
	void mouseExit(const MouseEvent &e) override;
	void mouseMove(const MouseEvent&) override { repaint(); }

	void update();

	void sliderValueChanged(Slider *s) override;

	void notifyListeners(int index, NotificationType n)
	{
		if (getData() != nullptr)
		{
			getData()->sendValueChangeMessage(index, n);
		}
	}

	void paintOverChildren(Graphics &g) override;

	void paint(Graphics &g);

	void setSuffix(const String &suffix);

	/** Sets the double click return value. */
	void setDefaultValue(double defaultValue);

	void setColourForSliders(int colourId, Colour c);

	const SliderPackData* getData() const { return data; }
	SliderPackData* getData() { return data; }

	void resized() override;
	void setValuesFromLine();

	/** Returns the number of slider. */
	int getNumSliders();

	void setSpecialLookAndFeel(LookAndFeel* l, bool shouldOwn /* = false */) override
	{
		EditorBase::setSpecialLookAndFeel(l, shouldOwn);
		sliders.clear();
		rebuildSliders();
	}

	void setFlashActive(bool setFlashActive);
	void setShowValueOverlay(bool shouldShowValueOverlay);
	void setStepSize(double stepSize);
    
	/** Set the slider widths to the given proportions. 
		
		For example { 0.25, 0.5, 0.25 } will make the middle slider twice as big. 
	*/
    void setSliderWidths(const Array<var>& newWidths)
    {
        sliderWidths = newWidths;
        resized();
    }
    
	void addListener(Listener* l)
	{
		if (auto d = getData())
			d->addListener(l);
	}

	void removeListener(Listener* listener)
	{
		if (auto d = getData())
			d->removeListener(listener);
	}

	void setCallbackOnMouseUp(bool shouldFireOnMouseUp)
	{
		callbackOnMouseUp = shouldFireOnMouseUp;
	}

private:

	int lastDragIndex = -1;
	float lastDragValue = -1.0f;

	bool slidersNeedRebuild = false;

	void rebuildSliders();

	int currentDisplayIndex = -1;

	int getSliderIndexForMouseEvent(const MouseEvent& e);
    
	ReferenceCountedObjectPtr<SliderPackData> dummyData;
	String suffix;

	double defaultValue;

	Array<float> displayAlphas;

    Array<var> sliderWidths;
    
	Line<float> rightClickLine;

	bool currentlyDragged;

	bool callbackOnMouseUp = false;

	int currentlyDraggedSlider;

	double currentlyDraggedSliderValue;

	WeakReference<SliderPackData> data;
	OwnedArray<Slider> sliders;

	JUCE_DECLARE_WEAK_REFERENCEABLE(SliderPack);
};


} // namespace hise
#endif  // SLIDERPACK_H_INCLUDED
