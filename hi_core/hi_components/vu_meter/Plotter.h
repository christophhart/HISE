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

#ifndef PLOTTER_H_INCLUDED
#define PLOTTER_H_INCLUDED



class ProcessorEditorHeader;
class Modulator;



//==============================================================================
/** A plotter component that displays the Modulator value
*	@ingroup debugComponents
*
*	You can add values with addValue(). This should be periodic 
*	(either within the audio callback or with a designated timer in your Editor, as
*	the plotter only writes something if new data is added.
*/
class Plotter    : public Component,
				   public SettableTooltipClient,
				   public AsyncUpdater,
				   public Slider::Listener,
				   public AutoPopupDebugComponent
{
public:
	Plotter(BaseDebugArea *area);
    ~Plotter();

	enum ColourIds
	{
		backgroundColour = 0x100,
		outlineColour = 0x10,
		pathColour = 0x001,
		pathColour2
	};


	/** Clears the queue and triggers a repaint of the component.
	*/
	void handleAsyncUpdate() override;

    void paint (Graphics&) override;
    void resized() override;

	void sliderValueChanged (Slider* ) override { setSpeed((int)speedSlider->getValue()); };

	/** Adds a value to the queue to be displayed at the next timerCallback(). */
	void addValue(const Modulator *m, float addedValue);

	/** If set to true, you don't need any modulators, but call addValue(float newValue) directly. */
	void setFreeMode(bool shouldUseFreeMode);

	/** Adds a value to the queue without having a modulator attached. */
	void addValue(float addedValue);

	/** Changes the speed of the modulator. */
	void setSpeed(int newSpeed)
	{
		smoothLimit = newSpeed;
		smoothedValue = 0.0f;
		smoothIndex = 0;
	};

	void mouseDown(const MouseEvent &e) override;

	void resetPlotter();

	void addPlottedModulator(Modulator *m);

	void removePlottedModulator(Modulator *m);

private:

	bool freeMode;

	struct PlotterQueue
	{
		PlotterQueue(Modulator *m):
			attachedMod(m),
			pos(0)
		{
			for(int i = 0; i < 1024; i++)
			{
				data[i] = 0.0f;
			}
		}

		void reset()
		{
			for(int i = 0; i<1024; i++) data[i] = 0.0f;
			pos = 0;

		}

		bool isActive() const { return attachedMod.get() != nullptr; };

		void addValue(float addedValue)
		{
			//jassert(pos < 1024);

			if(pos >= 1024) return;

			const float newValue = addedValue;
			
			data[pos++] = newValue;
			
		}

		float data[1024];

		int pos;

		WeakReference<Modulator> attachedMod;
	};

	PlotterQueue freeModePlotterQueue;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Plotter)

	

	// the internal buffer (sized 1200 for 2x interpolation
	float internalBuffer[1024];

	OwnedArray<PlotterQueue> modQueue;

	int currentQueuePosition;
	int currentRingBufferPosition;
	
	float smoothedValue;
	int smoothIndex;
	int smoothLimit;

    ScopedPointer<Slider> speedSlider;
	
};




#endif  // PLOTTER_H_INCLUDED

