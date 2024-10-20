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

#pragma once

namespace hise { using namespace juce;

class ModulatorSynth;
class CurveEq;

class FilterDragOverlay : public Component,
	public SettableTooltipClient,
	public Processor::OtherListener,
	public Timer
{
public:

	struct FilterResizeAction : public UndoableAction
	{
		FilterResizeAction(CurveEq* eq_, int index_, bool add, double freq_=0.0, double gain_=0.0);;

		bool perform() override;
		bool undo() override;

		WeakReference<CurveEq> eq;
		int index;
		bool isAddAction;

		double freq;
		double gain;
		int type;
		double q;
		bool enabled;
	};

	enum class SpectrumVisibility
	{
		Dynamic,
		AlwaysOn,
		AlwaysOff
	};

	struct DragData
	{
		DragData() = default;

		bool selected;
		bool enabled;
		bool dragging;
		bool hover;
		float frequency;
		float q;
		float gain;
		String type;
	};

	struct LookAndFeelMethods
	{
		virtual ~LookAndFeelMethods() {};
		virtual void drawFilterDragHandle(Graphics& g, FilterDragOverlay& o, int index, Rectangle<float> handleBounds, const DragData& d);
	};

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

	void otherChange(Processor* p) override
	{
		checkEnabledBands();
		updateFilters();
		updatePositions(true);
	}

	void checkEnabledBands();

	void timerCallback() override;
	void resized() override;
	void paint(Graphics &g);
	void paintOverChildren(Graphics& g);

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

	void lookAndFeelChanged() override;

	bool shouldResetOnDoubleClick() const noexcept { return resetOnDoubleClick; }

	void setResetOnDoubleClick(bool shouldReset) { resetOnDoubleClick = shouldReset; }

	void setAllowContextMenu(bool shouldAllow)
	{
		allowContextMenu = shouldAllow;
	}

	void setGainRange(double maxGain)
	{
		gainRange = jlimit(1.0, 36.0, maxGain);
		filterGraph.setGainRange(maxGain);
	}

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

		void mouseEnter(const MouseEvent& e);
		void mouseExit(const MouseEvent& e);
		void mouseDown(const MouseEvent& e);
		void mouseUp(const MouseEvent& e);
		void mouseDrag(const MouseEvent& e);
		void mouseWheelMove(const MouseEvent &e, const MouseWheelDetails &d) override;
		void mouseDoubleClick(const MouseEvent& e);
		void setSelected(bool shouldBeSelected);
		void paint(Graphics &g);;
		void setIndex(int newIndex);;

		bool isSelected() const { return selected; }
		bool isDragging() const { return !menuActive && draggin; }
		bool isOver() const { return !menuActive && over; }
		int getIndex() const { return index; }

	private:

		bool down = false;
		bool over = false;
		bool draggin = false;
		int index;
		bool selected;
		bool menuActive = false;

		float dragQStart = 1.0f;

		ComponentBoundsConstrainer *constrainer;
		ComponentDragger dragger;
		FilterDragOverlay& parent;
	};

	void removeFilter(int index);

	double getGain(int y);

	void selectDragger(int index, NotificationType n = sendNotificationSync);
	void addListener(Listener* l);
	void removeListener(Listener* l);

	void setAllowFilterResizing(bool shouldBeAllowed);
	void setSpectrumVisibility(SpectrumVisibility m);
	void setUndoManager(UndoManager* newUndoManager);
	void setEqAttribute(int b, int filterIndex, float value);

protected:

	WeakReference<CurveEq> eq;
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


	UndoManager* um = nullptr;

	bool allowContextMenu = true;
	double gainRange = 24.0;

	bool resetOnDoubleClick = false;
	bool allowFilterResizing = true;
	SpectrumVisibility fftVisibility = SpectrumVisibility::Dynamic;
	LookAndFeelMethods defaultLaf;
	Array<WeakReference<Listener>> listeners;
	UpdateMerger repaintUpdater;
	int selectedIndex;
	ScopedPointer<ComponentBoundsConstrainer> constrainer;
	OwnedArray<FilterDragComponent> dragComponents;
};

} // namespace hise;

