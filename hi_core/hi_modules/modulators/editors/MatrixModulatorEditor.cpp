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


namespace hise { using namespace juce;




MatrixModulatorBody::RangeEditor::Editor::Editor(ValueTree v, const Identifier& id_, UndoManager* um_):
	rangeTree(v),
	id(id_.toString()),
	f(GLOBAL_BOLD_FONT()),
	um(um_) 
{
	if(id == MatrixIds::TextConverter.toString())
	{
		selector.addItemList(ValueToTextConverter::getAvailableTextConverterModes(), 1);
		selector.setLookAndFeel(&laf);
		laf.setDefaultColours(selector);
		addAndMakeVisible(selector);
		selector.onChange = [this]() { update(selector.getText()); };
		selector.setTextWhenNothingSelected("Select mode");
	}
	else if (id == MatrixIds::UseMidPositionAsZero.toString())
	{
		addAndMakeVisible(zeroToggle);
		zeroToggle.setLookAndFeel(&laf);
		zeroToggle.onClick = [this]() { update(zeroToggle.getToggleState()); };
		laf.setDefaultColours(zeroToggle);
	}
	else
	{
		editor.setFont(GLOBAL_FONT());
		editor.onReturnKey = [this](){ update(editor.getText()); };
		editor.onFocusLost = [this](){ update(editor.getText()); };

		addAndMakeVisible(editor);
		GlobalHiseLookAndFeel::setTextEditorColours(editor);
	}

	listener.setCallback(v, { id}, valuetree::AsyncMode::Asynchronously, BIND_MEMBER_FUNCTION_2(Editor::onUpdate));
}

void MatrixModulatorBody::RangeEditor::Editor::paint(Graphics& g)
{
	g.setColour(Colours::white.withAlpha(0.7f));
	g.setFont(f);
	g.drawText(id, tb, Justification::centred);
}

void MatrixModulatorBody::RangeEditor::Editor::resized()
{
	auto b = getLocalBounds();
	tb = b.removeFromLeft(f.getStringWidthFloat(id) + 10).toFloat();
	getChildComponent(0)->setBounds(b);
}

void MatrixModulatorBody::RangeEditor::Editor::update(const var& newValue)
{
	jassert(rangeTree.hasProperty(id));
	auto v = newValue.toString();
	rangeTree.setProperty(id, newValue, um);
}

MatrixModulatorBody::RangeEditor::RangeEditor(ValueTree rangeTree, UndoManager* um):
	data(rangeTree)
{
	for(const auto& id: MatrixModulator::getRangeIds(rangeTree.getType() == MatrixIds::InputRange))
	{
		auto ne = new Editor(rangeTree, id, um);
		addAndMakeVisible(ne);
		editors.add(ne);
	}
}

void MatrixModulatorBody::RangeEditor::paint(Graphics& g)
{
	auto t = data.getType().toString();
	g.setFont(GLOBAL_BOLD_FONT());
	g.setColour(Colours::white.withAlpha(0.9f));
	g.drawText(t, getLocalBounds().removeFromLeft(80).toFloat(), Justification::left);
}

void MatrixModulatorBody::RangeEditor::resized()
{
	auto b = getLocalBounds();
	b.removeFromLeft(80);

	for(auto e: editors)
	{
		e->calculateWidth(getHeight());

		auto tw = (int)e->tb.getWidth() + 90;
		e->setBounds(b.removeFromLeft(tw));
	}
}

ValueTree MatrixModulatorBody::createValueTreeFromRange(Processor* p, bool isInputData)
{
	auto mm = static_cast<MatrixModulator*>(p);
	return mm->getRangeData(isInputData);
}

MatrixModulatorBody::MatrixModulatorBody(ProcessorEditor* parent):
	ProcessorEditorBody(parent),
	ControlledObject(parent->getProcessor()->getMainController()),
	valueSlider("Value"),
	smoothingSlider("SmoothingTime"),
	content(getMainController(),  dynamic_cast<MatrixModulator*>(getProcessor())->getMatrixTargetId(), false),
	controller(getMainController(), *this),
	inputRangeEditor(createValueTreeFromRange(getProcessor(), true), getMainController()->getControlUndoManager()),
	outputRangeEditor(createValueTreeFromRange(getProcessor(), false), getMainController()->getControlUndoManager()),
    editRangeButton("Edit value range"),
	autofixButton("Presets")
{
	auto p = dynamic_cast<MatrixModulator*>(getProcessor());
	valueSlider.setup(p, MatrixModulator::Value, "Value");
	valueSlider.setSliderStyle(Slider::SliderStyle::RotaryHorizontalVerticalDrag);

	auto obj = new DynamicObject();

	obj->setProperty("ContextMenu", 0);
	obj->setProperty("ScaleModulation", (int)ModifierKeys::rightButtonModifier);

	valueSlider.setModifierObject(var(obj));
		
	smoothingSlider.setup(p, MatrixModulator::SmoothingTime, "Smoothing");
	smoothingSlider.setMode(HiSlider::Mode::Time, { 0.0, 200.0, 1.0 });
	smoothingSlider.setSliderStyle(Slider::SliderStyle::RotaryHorizontalVerticalDrag);

	auto mm = dynamic_cast<MatrixModulator*>(parent->getProcessor());

	controller.setValueTree(mm->globalMatrixData, 
	                        mm->getMatrixTargetId(), 
	                        getMainController()->getControlUndoManager());

	addAndMakeVisible(smoothingSlider);
	addAndMakeVisible(valueSlider);
	addAndMakeVisible(content);
	addAndMakeVisible(controller);

	addChildComponent(inputRangeEditor);
	addChildComponent(outputRangeEditor);
	addChildComponent(autofixButton);
	addAndMakeVisible(editRangeButton);
	laf.setDefaultColours(editRangeButton);
	

	autofixButton.onClick = [this]()
	{
		PopupMenu m;
		m.setLookAndFeel(&laf);

		m.addSectionHeader("Load range preset");

		using Presets = MatrixIds::Helpers::RangePresets;

		auto names = MatrixIds::Helpers::getRangePresetNames();

		std::array<bool, (int)MatrixIds::Helpers::RangePresets::numPresets> allowed = {
			true,
			true,
			true,
			true,
			true,
			true,
			true,
			true,
			true
		};

		auto mod = dynamic_cast<MatrixModulator*>(this->getProcessor());

		if(mod->getMode() == Modulation::PitchMode)
		{
			for(auto& m: allowed)
				m = false;

			allowed[(int)Presets::Pitch1Octave] = true;
			allowed[(int)Presets::Pitch2Octaves] = true;
			allowed[(int)Presets::Pitch1Semitone] = true;
		}
		else
		{
			allowed[(int)Presets::Pitch1Octave] = false;
			allowed[(int)Presets::Pitch2Octaves] = false;
			allowed[(int)Presets::Pitch1Semitone] = false;
		}
		
		for(int i = 0; i < (int)Presets::numPresets; i++)
		{
			m.addItem(1 + i, names[i], allowed[i], false);
		}

		if(auto r = m.show())
		{
			auto pr = (Presets)(r - 1);

			auto rd = MatrixIds::Helpers::getRangePreset(pr);

			auto um = getMainController()->getControlUndoManager();
			auto set = RangeHelpers::IdSet::ScriptComponents;

			if(auto gc = ProcessorHelpers::getFirstProcessorWithType<GlobalModulatorContainer>(getMainController()->getMainSynthChain()))
			{
				auto targetId = dynamic_cast<MatrixModulator*>(getProcessor())->getMatrixTargetId();

				gc->matrixProperties.rangeData[targetId] = rd;
				gc->matrixProperties.sendChangeMessage(targetId);
			}
			
		}
	};

	autofixButton.setLookAndFeel(&laf);

	editRangeButton.setLookAndFeel(&laf);

	editRangeButton.onClick = [this]()
	{
		auto on = editRangeButton.getToggleState();
		content.setVisible(!on);
		inputRangeEditor.setVisible(on);
		outputRangeEditor.setVisible(on);
		autofixButton.setVisible(on);
		targetIdEditor.setVisible(on);
	};

	content.setLookAndFeel(&laf);
	content.resizeFunction = [this]()
	{
		refreshBodySize();
	};

	controller.setCSS(css);

	inputRangeListener.setCallback(inputRangeEditor.data, MatrixModulator::getRangeIds(true), valuetree::AsyncMode::Asynchronously,
	                               [this](const Identifier& id, const var& newValue)
	                               {
		                               updateRangeProperty(true, id.toString(), newValue);
	                               });

	outputRangeListener.setCallback(outputRangeEditor.data, MatrixModulator::getRangeIds(false), valuetree::AsyncMode::Asynchronously,
	                                [this](const Identifier& id, const var& newValue)
	                                {
		                                updateRangeProperty(false, id.toString(), newValue);
	                                });

	addAndMakeVisible(targetIdEditor);

	GlobalHiseLookAndFeel::setTextEditorColours(targetIdEditor);
	targetIdEditor.setText(p->getMatrixTargetId(), dontSendNotification);
	targetIdEditor.setTextToShowWhenEmpty("targetId", Colours::grey);
	targetIdEditor.setVisible(false);
	targetIdEditor.onReturnKey = [this, p]()
	{
		p->setCustomTargetId(targetIdEditor.getText());
	};
}

void MatrixModulatorBody::updateRangeProperty(bool isInputRange, const Identifier& id, const var& newValue)
{
	updateValueSlider();
	return;

#if 0
	auto mm = dynamic_cast<MatrixModulator*>(getProcessor());

	if(id == MatrixIds::TextConverter)
	{
		mm->vtcMode = newValue.toString();
		updateValueSlider();
		return;
	}
		
	auto value = (double)newValue;
	FloatSanitizers::sanitizeDoubleNumber(value);

	auto r = isInputRange ? mm->inputRange : mm->outputRange;
	auto intervalToUse = r.rng.interval;

	if(intervalToUse == 0.0)
		intervalToUse = 0.001;

	if(id.toString() == "min")
	{
		r.rng.start = jmin(r.rng.end - intervalToUse, value);
	}
	if(id.toString() == "max")
	{
		r.rng.end = jmax(r.rng.start + intervalToUse, value);
	}
	if(id.toString() == "middlePosition")
	{
		if(r.rng.getRange().contains(value) && r.rng.start != value)
			r.rng.setSkewForCentre(value);
	}
	if(id.toString() == "stepSize")
	{
		r.rng.interval = value;
	}

	mm->setValueRange(isInputRange, r);
	updateValueSlider();
#endif
}

void MatrixModulatorBody::updateValueSlider()
{
	auto mm = dynamic_cast<MatrixModulator*>(getProcessor());

	auto vtc = ValueToTextConverter::createForMode(mm->rangeData.converter);

	valueSlider.setNormalisableRange(mm->rangeData.inputRange.rng);
	valueSlider.valueFromTextFunction = vtc;
	valueSlider.textFromValueFunction = vtc;
	valueSlider.updateText();

	for(auto r: content.rows)
	{
		r->setIntensityConverter(mm);
	}
}

void MatrixModulatorBody::updateGui()
{
	valueSlider.updateValue();
	smoothingSlider.updateValue();
}

void MatrixModulatorBody::paint(Graphics& g)
{
	auto tb = getLocalBounds().reduced(Margin).removeFromTop(TopRowHeight);
	g.setColour (Colour (0xAAFFFFFF));
	g.setFont (GLOBAL_BOLD_FONT().withHeight(26.0f));
	g.drawText(getProcessor()->getName().toLowerCase(), tb.toFloat(), Justification::centredRight);
}

void MatrixModulatorBody::resized()
{
	auto b = getLocalBounds().reduced(2 * Margin, Margin);
	auto top = b.removeFromTop(TopRowHeight);
	auto bottom = b.removeFromBottom(controller.getHeightToUse(getWidth()));

	top.removeFromBottom(16);
	
	valueSlider.setBounds(top.removeFromLeft(128));
	top.removeFromLeft(2*Margin);
	smoothingSlider.setBounds(top.removeFromLeft(128));
	top.removeFromLeft(2*Margin);
	editRangeButton.setBounds(top.removeFromLeft(128).reduced(0, 10));

	targetIdEditor.setBounds(top.removeFromLeft(148).reduced(10));

	content.setBounds(b);

	b.removeFromTop(Margin);
	inputRangeEditor.setBounds(b.removeFromTop(28));
	b.removeFromTop(Margin);

	auto outputRow = b.removeFromTop(28);
	autofixButton.setBounds(outputRow.removeFromRight(100));
	outputRangeEditor.setBounds(outputRow);
	

	controller.setBounds(bottom);
}

int MatrixModulatorBody::getBodyHeight() const
{
	int h = 0;
	h += Margin;
	h += TopRowHeight;
	h += Margin;
	h += content.getHeightToUse(-1);
	h += Margin;
	h += HeaderHeight;
	h += const_cast<MatrixContent::Controller*>(&controller)->getHeightToUse(getWidth());
	h += Margin;
	return h;
}



} // namespace hise
