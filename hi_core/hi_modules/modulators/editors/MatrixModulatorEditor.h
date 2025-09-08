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

class MatrixModulatorBody: public ProcessorEditorBody,
						    public ControlledObject,
							public MatrixContent::Parent
{
public:

	static constexpr int Margin = 10;
	static constexpr int TopRowHeight = 64;
	static constexpr int ButtonHeight = 24;
	static constexpr int HeaderHeight = 24;

	struct RangeEditor: public Component
	{
		struct Editor: public Component
		{
			Editor(ValueTree v, const Identifier& id_, UndoManager* um);

			void paint(Graphics& g) override;
			void resized() override;

			void calculateWidth(int height)
			{
				tb = { 0.0f, 0.0f, f.getStringWidthFloat(id) + 10.0f, (float)height };
			}

			void update(const var& newValue);

			void onUpdate(const Identifier& id, const var& newValue)
			{
				auto v = newValue.toString();

				if(editor.isVisible())
					editor.setText(newValue.toString(), dontSendNotification);
				else if(selector.isVisible())
					selector.setText(newValue.toString(), dontSendNotification);
				else if(zeroToggle.isVisible())
					zeroToggle.setToggleState((bool)newValue, dontSendNotification);
			}

			valuetree::PropertyListener listener;
			UndoManager* um;
			Font f;
			Rectangle<float> tb;
			ValueTree rangeTree;
			String id;
			TextEditor editor;
			ComboBox selector;
			ToggleButton zeroToggle;
			GlobalHiseLookAndFeel laf;
		};

		RangeEditor(ValueTree rangeTree, UndoManager* um);

		void paint(Graphics& g) override;
		void resized() override;

		

		ValueTree data;
		OwnedArray<Editor> editors;
	};

	static ValueTree createValueTreeFromRange(Processor* p, bool isInputData);

	MatrixModulatorBody(ProcessorEditor* parent);;

	void updateRangeProperty(bool isInputRange, const Identifier& id, const var& newValue);
	void updateValueSlider();
	void updateGui() override;

	void paint(Graphics& g) override;
	void resized() override;
	int getBodyHeight() const override;

	valuetree::PropertyListener inputRangeListener, outputRangeListener;

	RangeEditor inputRangeEditor, outputRangeEditor;

	ToggleButton editRangeButton;

	GlobalHiseLookAndFeel laf;
	valuetree::ChildListener rowListener;
	HiSlider valueSlider, smoothingSlider;
	MatrixContent content;
	MatrixContent::Controller controller;
	TextButton autofixButton;
	TextEditor targetIdEditor;
};




} // namespace hise

