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

#ifndef __MAINCOMPONENT_H_5C0D5756__
#define __MAINCOMPONENT_H_5C0D5756__

namespace hise { using namespace juce;

class ModulatorSynth;

class OscilloscopeBase: public RingBufferComponentBase
{
protected:

	OscilloscopeBase() :
		RingBufferComponentBase()
	{};

    virtual ~OscilloscopeBase() {};
    
	virtual Colour getColourForAnalyserBase(int colourId) = 0;

	void drawWaveform(Graphics& g);

	void refresh() override
	{
		dynamic_cast<Component*>(this)->repaint();
	}

private:

	void drawPath(const float* l_, int numSamples, int width, Path& p)
	{
		int stride = roundToInt((float)numSamples / width);
		stride = jmax<int>(1, stride * 2);

		if (numSamples != 0)
		{
			p.clear();
			p.startNewSubPath(0.0f, 0.0f);
			p.lineTo(0.0f, 1.0f);
			p.lineTo(0.0f, -1.0f);

			for (int i = stride; i < numSamples; i += stride)
			{
				const int numToCheck = jmin<int>(stride, numSamples - i);

				auto value = jmax<float>(0.0f, FloatVectorOperations::findMaximum(l_ + i, numToCheck));

				p.lineTo((float)i, -1.0f * value);

			};

			for (int i = numSamples - 1; i > 0; i -= stride)
			{
				const int numToCheck = jmin<int>(stride, numSamples - i);

				auto value = jmin<float>(0.0f, FloatVectorOperations::findMinimum(l_ + i, numToCheck));

				p.lineTo((float)i, -1.0f * value);
			};

			p.closeSubPath();
		}
		else
		{
			p.clear();
		}
	}

	void drawOscilloscope(Graphics &g, const AudioSampleBuffer &b)
	{
		auto dataL = b.getReadPointer(0);
		auto dataR = b.getReadPointer(1);
		int size = b.getNumSamples();

		auto asComponent = dynamic_cast<Component*>(this);

		drawPath(dataL, b.getNumSamples(), asComponent->getWidth(), lPath);
		drawPath(dataR, b.getNumSamples(), asComponent->getWidth(), rPath);

		lPath.scaleToFit(0.0f, 0.0f, (float)asComponent->getWidth(), (float)(asComponent->getHeight() / 2), false);
		rPath.scaleToFit(0.0f, (float)(asComponent->getHeight() / 2), (float)asComponent->getWidth(), (float)(asComponent->getHeight() / 2), false);

		g.fillPath(lPath);
		g.fillPath(rPath);
	}

	Path lPath;
	Path rPath;
};

class FFTDisplayBase: public RingBufferComponentBase
{
public:

	enum WindowType
	{
		Rectangle,
		BlackmannHarris,
		Hann,
		Flattop,
		numWindowTypes
	};

	enum Domain
	{
		Phase,
		Amplitude,
		numDomains
	};

	using ConverterFunction = std::function<float(float)>;

	struct Properties
	{
		WindowType window = BlackmannHarris;
		Range<double> dbRange = { -70.0, 0.0 };
		Domain domain = Amplitude;
		ConverterFunction freq2x;
		ConverterFunction gain2y;
	};

	Properties fftProperties;

	void refresh() override
	{
		dynamic_cast<Component*>(this)->repaint();
	}

protected:

	FFTDisplayBase()
#if USE_IPP
		:fftObject(IppFFT::DataType::RealFloat)
#endif
	{}

#if USE_IPP
	IppFFT fftObject;
#endif

	virtual Colour getColourForAnalyserBase(int colourId) = 0;
	virtual double getSamplerate() const = 0;

    virtual ~FFTDisplayBase() {};
    
	void drawSpectrum(Graphics& g);

	Path lPath;
	Path rPath;

	WindowType lastWindowType = numWindowTypes;
	
	AudioSampleBuffer windowBuffer;
	AudioSampleBuffer fftBuffer;
};

class CurveEq;


class FilterDragOverlay : public Component,
	public SettableTooltipClient,
	public SafeChangeListener,
	public Timer
{
public:



	struct Factory : public PathFactory
	{
		String getId() const override { return "FilterIcons"; }

		Path createPath(const String& url) const override;

	};

	enum ColourIds
	{
		bgColour = 125160,
		textColour
	};

	struct Panel;

	int offset = 12;

	struct Listener
	{
		virtual ~Listener() {};

		virtual void bandRemoved(int index) = 0;

		virtual void filterBandSelected(int index) = 0;

		JUCE_DECLARE_WEAK_REFERENCEABLE(Listener);
	};

	FilterDragOverlay(CurveEq* eq_, bool isInFloatingTile_ = false);
	virtual ~FilterDragOverlay();

	void changeListenerCallback(SafeChangeBroadcaster *b) override;
	void checkEnabledBands();

	void timerCallback() override;
	void resized() override;
	void paint(Graphics &g);

	void mouseMove(const MouseEvent &e);
	void mouseDown(const MouseEvent &e);
	void mouseDrag(const MouseEvent &e) override;
	void mouseUp(const MouseEvent& e);

	void addFilterDragger(int index);
	void updateFilters();
	void updateCoefficients();
	void addFilterToGraph(int filterIndex, int filterType);
	void updatePositions(bool forceUpdate);
	Point<int> getPosition(int index);

	virtual void fillPopupMenu(PopupMenu& m, int handleIndex);

	virtual void popupMenuAction(int result, int handleIndex);

	Font font;
	bool isInFloatingTile = false;
	ScopedPointer<LookAndFeel> plaf;

	class FilterDragComponent : public Component
	{
	public:

		FilterDragComponent(FilterDragOverlay& parent_, int index_);;

		void setConstrainer(ComponentBoundsConstrainer *constrainer_);;

		void mouseDown(const MouseEvent& e);
		void mouseUp(const MouseEvent& e);
		void mouseDrag(const MouseEvent& e);
		void mouseWheelMove(const MouseEvent &e, const MouseWheelDetails &d) override;
		void setSelected(bool shouldBeSelected);
		void paint(Graphics &g);;
		void setIndex(int newIndex);;

	private:

		bool draggin = false;

		int index;

		bool selected;

		ComponentBoundsConstrainer *constrainer;
		ComponentDragger dragger;
		FilterDragOverlay& parent;
	};

	void removeFilter(int index);

	double getGain(int y);

	void selectDragger(int index);
	void addListener(Listener* l);
	void removeListener(Listener* l);

protected:

	CurveEq *eq;
	int numFilters = 0;

public:


	struct FFTDisplay : public Component,
		public FFTDisplayBase
	{
		FFTDisplay(FilterDragOverlay& parent_);;

		void paint(Graphics& g) override;
		double getSamplerate() const override;
		Colour getColourForAnalyserBase(int colourId);

		FilterDragOverlay& parent;
	} fftAnalyser;

	FilterGraph filterGraph;

private:

	Array<WeakReference<Listener>> listeners;

	UpdateMerger repaintUpdater;

	int selectedIndex;

	ScopedPointer<ComponentBoundsConstrainer> constrainer;

	OwnedArray<FilterDragComponent> dragComponents;
};

} // namespace hise;
#endif  // __MAINCOMPONENT_H_5C0D5756__
