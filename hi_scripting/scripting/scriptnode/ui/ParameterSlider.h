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



struct ParameterKnobLookAndFeel : public LookAndFeel_V3
{
	ParameterKnobLookAndFeel();

	Image cachedImage_smalliKnob_png;
	Image cachedImage_knobRing_png;

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

		void mouseEnter(const MouseEvent& event) override
		{
			updateText();
		}

		void mouseExit(const MouseEvent& event) override
		{
			updateText();
		}

		void editorShown(TextEditor* ed)
		{
			Label::editorShown(ed);

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

		void updateText()
		{
			if (parent->isMouseOverOrDragging(true) || isMouseOver())
				setText(parent->getTextFromValue(parent->getValue()), dontSendNotification);
			else
				setText(parent->getName(), dontSendNotification);
		}

		void startDrag()
		{
			setText(parent->getTextFromValue(parent->getValue()), dontSendNotification);
		}

		void endDrag()
		{
			setText(parent->getName(), dontSendNotification);
		}

        Component::SafePointer<Slider> parent;
	};


	Label* createSliderTextBox(Slider& slider);

	void drawRotarySlider(Graphics& g, int x, int y, int width, int height, float sliderPosProportional, float rotaryStartAngle, float rotaryEndAngle, Slider& s);
};

struct ParameterSlider : public Slider,
	public Slider::Listener,
	public DragAndDropTarget,
	public PooledUIUpdater::SimpleTimer
{
	ParameterSlider(NodeBase* node_, int index);
    ~ParameterSlider();
    
	void checkEnabledState(Identifier, var);
	void updateRange(Identifier, var);
	void timerCallback() override;

	void paint(Graphics& g) override;

	bool isInterestedInDragSource(const SourceDetails& details) override;
	void itemDragEnter(const SourceDetails& dragSourceDetails) override;
	void itemDragExit(const SourceDetails& dragSourceDetails) override;
	void itemDropped(const SourceDetails& dragSourceDetails) override;

	valuetree::PropertyListener connectionListener;
	valuetree::PropertyListener valueListener;
	valuetree::PropertyListener rangeListener;

	void mouseDown(const MouseEvent& e) override;

	void sliderDragStarted(Slider*) override;
	void sliderDragEnded(Slider*) override;

	void sliderValueChanged(Slider*) override;

	String getTextFromValue(double value) override;;
	double getValueFromText(const String& text) override;

	int macroHoverIndex = -1;
	double lastModValue = 0.0f;
	bool modulationActive = false;

	WeakReference<NodeBase::Parameter> parameterToControl;
	ValueTree pTree;
	ParameterKnobLookAndFeel laf;
	NodeBase::Ptr node;
};


struct MacroParameterSlider : public Component
{
	MacroParameterSlider(NodeBase* node, int index);

	void resized() override;

	void mouseDrag(const MouseEvent& event) override;

	void paintOverChildren(Graphics& g) override;

	void setEditEnabled(bool shouldBeEnabled);

private:

	bool editEnabled = false;

	ParameterSlider slider;
};

}
