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
 *   which also must be licenced for commercial applications:
 *
 *   http://www.juce.com
 *
 *   ===========================================================================
 */

#pragma once

namespace scriptnode
{
using namespace juce;
using namespace hise;



struct ParameterKnobLookAndFeel : public GlobalHiseLookAndFeel
{
	ParameterKnobLookAndFeel();

	Image cachedImage_smalliKnob_png;
	Image cachedImage_knobRing_png;
	Image withoutArrow;

	Font getLabelFont(Label&) override;

	struct SliderLabel : public NiceLabel
	{
		SliderLabel(Slider& s) :
			parent(&s)
		{
            auto tmp = Component::SafePointer<SliderLabel>(this);
			auto n = parent->getName();

			auto f = [tmp, n]()
			{
				if(tmp.getComponent() != nullptr)
					tmp.getComponent()->setText(n, dontSendNotification);
			};

			MessageManager::callAsync(f);
		};

		void mouseEnter(const MouseEvent& ) override
		{
			updateText();
		}

		void mouseExit(const MouseEvent& ) override
		{
			updateText();
		}

        void textEditorReturnKeyPressed(TextEditor& t) override
        {
            NiceLabel::textEditorReturnKeyPressed(t);
            
            enableTextSwitch = true;
        }
        
		void editorShown(TextEditor* ed)
		{
            enableTextSwitch = false;
			Label::editorShown(ed);

            ed->setJustification(Justification::centred);
			ed->setText(parent->getTextFromValue(parent->getValue()), dontSendNotification);
			ed->selectAll();
			ed->setBounds(getLocalBounds());
		}

		void resized() override
		{
			if (getCurrentTextEditor() == nullptr)
				setText(parent->getName(), dontSendNotification);
		}

		~SliderLabel()
		{
            
		}

		void updateText();
        
		void startDrag()
		{
			setText(parent->getTextFromValue(parent->getValue()), dontSendNotification);
		}

		void endDrag()
		{
			setText(parent->getName(), dontSendNotification);
		}

        
        bool enableTextSwitch = true;
        Component::SafePointer<Slider> parent;
	};


	Label* createSliderTextBox(Slider& slider);

	void drawRotarySlider(Graphics& g, int x, int y, int width, int height, float sliderPosProportional, float rotaryStartAngle, float rotaryEndAngle, Slider& s);
};



struct ParameterSlider : public Slider,
	public Slider::Listener,
	public DragAndDropTarget,
    public hise::Learnable,
	public PooledUIUpdater::SimpleTimer
{
	
	/** This function will find all valuetrees that are connected to the given connection source tree. 
		
		This is used by the `Copy range to source` function as well as the warning display on 
		unscaled modulation draggers.

	*/
	static Array<ValueTree> getValueTreesForSourceConnection(const ValueTree& connectionSourceTree);

	struct ParameterIcons : public PathFactory
	{
		Path createPath(const String& path) const override;
	};

	struct RangeButton : public Component
	{
		RangeButton()
		{
			setRepaintsOnMouseActivity(true);
		};

		void mouseUp(const MouseEvent& e) override
		{
			findParentComponentOfClass<ParameterSlider>()->showRangeComponent(false);
		}

		void paint(Graphics& g) override
		{
			auto p = ParameterIcons().createPath("range");

			

			auto over = isMouseOver(true);
			auto down = isMouseButtonDown(true);

			PathFactory::scalePath(p, getLocalBounds().toFloat().reduced(isMouseButtonDown() ? 2.0f : 1.0f));

			float alpha = 0.0f;
			
			if (getParentComponent()->isMouseOverOrDragging(true))
				alpha += 0.05f;

			if (over)
				alpha = 0.4f;

			if (down)
				alpha += 0.2f;

			g.setColour(Colours::white.withAlpha(alpha));

			g.fillPath(p);
		}
	} rangeButton;

	struct RangeComponent;

	ParameterSlider(NodeBase* node_, int index);
    ~ParameterSlider();
    
	void updateOnConnectionChange(ValueTree p, bool wasAdded);

	void checkEnabledState();
	void updateRange(Identifier, var);
	void paint(Graphics& g) override;

	void timerCallback() override;

	bool isInterestedInDragSource(const SourceDetails& details) override;
	void itemDragEnter(const SourceDetails& dragSourceDetails) override;
	void itemDragExit(const SourceDetails& dragSourceDetails) override;
	void itemDropped(const SourceDetails& dragSourceDetails) override;

	Array<NodeContainer::MacroParameter*> getConnectedMacroParameters();

	valuetree::RecursiveTypedChildListener connectionListener;
	
	valuetree::PropertyListener valueListener;
	valuetree::PropertyListener defaultValueListener;
	valuetree::PropertyListener rangeListener;
	valuetree::PropertyListener automationListener;

	/** Returns either the Connection or the ModulationTarget, or SwitchTarget tree if it's connected. */
	ValueTree getConnectionSourceTree();

	bool matchesConnection(const ValueTree& c) const;

	void mouseDown(const MouseEvent& e) override;

	void mouseUp(const MouseEvent& e) override;

	void mouseDrag(const MouseEvent& e) override;

	void mouseEnter(const MouseEvent& e) override;

	void mouseMove(const MouseEvent& e) override;

	void mouseExit(const MouseEvent& e) override;

	void resized() override;

	void showRangeComponent(bool temporary);

	void mouseDoubleClick(const MouseEvent&) override;

	void sliderDragStarted(Slider*) override;
	void sliderDragEnded(Slider*) override;

	void sliderValueChanged(Slider*) override;

	String getTextFromValue(double value) override;;
	double getValueFromText(const String& text) override;

	double getValueToDisplay() const;

	bool isControllingFrozenNode() const;

	void repaintParentGraph();



	int macroHoverIndex = -1;
	double lastModValue = 0.0f;
	bool modulationActive = false;
	bool isReadOnlyModulated = false;

	WeakReference<NodeBase::Parameter> parameterToControl;
	ValueTree pTree;
	ParameterKnobLookAndFeel laf;
	NodeBase::Ptr node;
	ScopedPointer<RangeComponent> currentRangeComponent;
	var currentConnection;
	const int index;
	double lastDisplayValue = -1.0;
	bool illegal = false;
    
    bool skipTextUpdate = false;
    
    float blinkAlpha = 0.0f;

};


struct MacroParameterSlider : public Component,
                              public PathFactory
{
	struct Dragger: public Component,
				    public SettableTooltipClient
	{
		Dragger(MacroParameterSlider& p_):
		  parent(p_),
		  targetIcon(parent.createPath("drag"))
		{
			setTooltip("Drag to control other sliders");
			setRepaintsOnMouseActivity(true);
			setMouseCursor(ModulationSourceBaseComponent::createMouseCursor());
		};

		void paint(Graphics& g) override
		{
			auto c = isMouseButtonDown() ? Colour(SIGNAL_COLOUR) : Colours::white;

			float alpha = 0.3f;

			if(isMouseOverOrDragging())
				alpha += 0.2f;

			if(isMouseButtonDown())
				alpha += 0.5f;
			
			g.setColour(c.withAlpha(alpha));
			g.fillPath(targetIcon);
		}

		void resized() override
		{
			auto b = getLocalBounds().toFloat();
			b = b.removeFromTop(16.0f);
			b = b.withSizeKeepingCentre(b.getHeight(), b.getHeight());
			parent.scalePath(targetIcon, b);
		}
		void mouseDown(const MouseEvent& e) override
		{
			
		}

		Component* getCircleComponent() { return static_cast<Component*>(&parent.slider); }

		void mouseUp(const MouseEvent& e) override;

		void mouseDrag(const MouseEvent& e) override;

		MacroParameterSlider& parent;
		Path targetIcon;
		
	};

    MacroParameterSlider(NodeBase* node, int index);

    Path createPath(const String& url) const override;
    
	void resized() override;

    void mouseDown(const MouseEvent& event) override;

    void mouseDrag(const MouseEvent& event) override;

    void mouseUp(const MouseEvent& e) override;

	void mouseEnter(const MouseEvent& e) override;
	void mouseExit(const MouseEvent& e) override;

	WeakReference<NodeBase::Parameter> getParameter();

	void paintOverChildren(Graphics& g) override;

	void setEditEnabled(bool shouldBeEnabled);

	bool keyPressed(const KeyPress& key) override;

	void focusGained(FocusChangeType) override { repaint(); }

	void focusLost(FocusChangeType) override { repaint(); }

	bool editEnabled = false;
	bool dragging = false;

	Component* getDragComponent() { return &dragger; }

private:

    void checkAllParametersForWarning(const Identifier&, const var&);
    
    void updateWarningButton(const ValueTree& v, const Identifier& id);
    
    void updateWarningOnConnectionChange(const ValueTree& c, bool wasAdded);
    
	ParameterSlider slider;
    
    HiseShapeButton warningButton;
	HiseShapeButton deleteButton;

	Dragger dragger;

    valuetree::RecursivePropertyListener rangeWatcher;
    valuetree::PropertyListener sourceRangeWatcher;
    valuetree::ChildListener sourceConnectionWatcher;
};

}
