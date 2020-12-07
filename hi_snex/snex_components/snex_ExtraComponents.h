/*  ===========================================================================
*
*   This file is part of HISE.
*   Copyright 2016 Christoph Hart
*
*   HISE is free software: you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation, either version 3 of the License, or
*   (at your option any later version.
*
*   HISE is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public License
*   along with HISE.  If not, see <http://www.gnu.org/licenses/>.
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

#pragma once

namespace snex {
using namespace juce;

namespace ui
{

struct OptimizationProperties : public WorkbenchComponent
{
	struct Item : public Component
	{
		Item(const String& id_) :
			id(id_)
		{

		}

		void paint(Graphics& g) override
		{
			if (active)
			{
				g.fillAll(Colours::white.withAlpha(0.2f));
			}

			g.setFont(GLOBAL_BOLD_FONT());
			g.setColour(Colours::white);
			g.drawText(id, getLocalBounds().toFloat(), Justification::centred);
		}

		void mouseDown(const MouseEvent& e) override
		{
			active = !active;

			findParentComponentOfClass<OptimizationProperties>()->resetOptimisations();
			repaint();
		}

		bool active = true;
		String id;
	};

	OptimizationProperties(WorkbenchData* data) :
		WorkbenchComponent(data)
	{
		data->addListener(this);

		for (auto o : OptimizationIds::getAllIds())
			addOptimization(o);

		for (auto i : items)
			i->active = getGlobalScope().getOptimizationPassList().contains(i->id);

		setSize(200, items.size() * 20);
	}

	~OptimizationProperties()
	{
		getWorkbench()->removeListener(this);
	}

	static Identifier getId() { RETURN_STATIC_IDENTIFIER("Optimisations"); };

	int getFixedHeight() const override
	{
		return items.size() * 20;
	}

	void recompiled(WorkbenchData::Ptr r) override
	{

	}

	void resized() override
	{
		auto b = getLocalBounds();

		for (auto i : items)
			i->setBounds(b.removeFromTop(20));
	}

	void resetOptimisations();

	void addOptimization(const String& id)
	{
		auto i = new Item(id);
		addAndMakeVisible(i);
		items.add(i);
	}

	OwnedArray<Item> items;
};


struct Graph : public WorkbenchComponent,
			   public ButtonListener
{
	bool barebone = false;
	int boxWidth = 128;

	struct Icons: public PathFactory
	{
		Path createPath(const String& p) const override;

		String getId() const override { return "GraphPaths"; };
	};

	struct InternalGraph : public Component,
		public Timer
	{
		void paint(Graphics& g) override;

		void timerCallback() override
		{
			stopTimer();
			repaint();
		}

		void setBuffer(AudioSampleBuffer& b);

		void calculatePath(Path& p, AudioSampleBuffer& b, int channel);

		void mouseMove(const MouseEvent& e) override
		{
			currentPoint = e.getPosition();
			startTimer(1200);
			repaint();
		}

		void mouseExit(const MouseEvent&) override
		{
			stopTimer();
			currentPoint = {};
			repaint();
		}

		void mouseWheelMove(const MouseEvent& e, const MouseWheelDetails& wheel) override
		{
			if (e.mods.isAnyModifierKeyDown())
			{
				zoomFactor = jlimit(1.0f, 32.0f, zoomFactor + (float)wheel.deltaY * 5.0f);
				findParentComponentOfClass<Graph>()->resized();
				setBuffer(lastBuffer);
			}
			else
				getParentComponent()->mouseWheelMove(e, wheel);
		}

		bool isHiresMode() const
		{
			return pixelsPerSample > 10.0f;
		}

		int getYPixelForSample(int sample)
		{
			auto x = (float)getPixelForSample(sample);
			Line<float> line(x, 0.0f, x, (float)getHeight());
			return roundToInt(l.getClippedLine(line, true).getEndY());
		}

		int getPixelForSample(int sample)
		{
			if (lastBuffer.getNumSamples() == 0)
				return 0;

			Path::Iterator iter(l);

			auto asFloat = (float)sample / (float)lastBuffer.getNumSamples();
			asFloat *= (float)getWidth();

			while (iter.next())
			{
				if (iter.x1 >= asFloat)
					return roundToInt(iter.x1);
			}

			return 0;
		}

		AudioSampleBuffer lastBuffer;

		float pixelsPerSample = 1;

		Point<int> currentPoint;

		int numSamples = 0;
		int currentPosition = 0;
		Path l;
		Path r;

		Range<float> leftPeaks;
		Range<float> rightPeaks;
		bool stereoMode = false;

		float zoomFactor = 1.0f;

	} internalGraph;

	Viewport viewport;

	Graph(WorkbenchData* data, bool barebone_ = false) :
		WorkbenchComponent(data),
		newFile("New File", this, iconFactory),
		openFile("Open File", this, iconFactory),
		saveFile("Save File", this, iconFactory),
		currentResult(Result::ok())
	{
		data->addListener(this);

		addAndMakeVisible(newFile);
		addAndMakeVisible(openFile);
		addAndMakeVisible(saveFile);

		addAndMakeVisible(viewport);
		viewport.setViewedComponent(&internalGraph, false);
	}

	~Graph()
	{
		getWorkbench()->removeListener(this);
	}

	void buttonClicked(Button* b) override
	{
		if (b == &openFile)
		{
			FileChooser fc("Load test file", getWorkbench()->getConnectedFile().getParentDirectory().getParentDirectory(), "*.wav", true);

			if (fc.browseForFileToOpen())
			{
				auto f = fc.getResult();

				double speed = 0.0;

				sourceBuffer = hlac::CompressionHelpers::loadFile(f, speed);
				recalculate();
			}
		}
	};

	void recalculate();

	void paint(Graphics& g)
	{
		auto b = getLocalBounds().removeFromRight(50);

		b = b.removeFromTop(viewport.getMaximumVisibleHeight());

		g.setColour(Colours::white);

		if (internalGraph.stereoMode)
		{
			auto left = b.removeFromTop(b.getHeight() / 2).toFloat();
			auto right = b.toFloat();

			auto lMax = left.removeFromTop(18);
			auto lMin = left.removeFromBottom(18);

			g.drawText(juce::String(internalGraph.leftPeaks.getStart(), 1), lMin, Justification::left);
			g.drawText(juce::String(internalGraph.leftPeaks.getEnd(), 1), lMax, Justification::left);

			auto rMax = right.removeFromTop(18);
			auto rMin = right.removeFromBottom(18);

			g.drawText(juce::String(internalGraph.rightPeaks.getStart(), 1), rMin, Justification::left);
			g.drawText(juce::String(internalGraph.rightPeaks.getEnd(), 1), rMax, Justification::left);
		}
		else
		{
			auto left = b.removeFromTop(b.getHeight()).toFloat();

			auto lMax = left.removeFromTop(18);
			auto lMin = left.removeFromBottom(18);

			g.drawText(juce::String(internalGraph.leftPeaks.getStart(), 1), lMin, Justification::left);
			g.drawText(juce::String(internalGraph.leftPeaks.getEnd(), 1), lMax, Justification::left);
		}
	}

	void recompiled(WorkbenchData::Ptr p) override
	{
		currentNode = p->getCompiledNode();

		if (currentNode != nullptr)
			currentResult = p->getLastResult();
		else
			currentResult = currentNode->r;

		if (currentResult.wasOk())
			recalculate();
	}

	static Identifier getId() { RETURN_STATIC_IDENTIFIER("SnexGraph"); }


	void resized() override
	{
		auto b = getLocalBounds();
		
		auto buttons = b.removeFromLeft(32);

		newFile.setBounds(buttons.removeFromTop(32).reduced(3));
		openFile.setBounds(buttons.removeFromTop(32).reduced(3));
		saveFile.setBounds(buttons.removeFromTop(32).reduced(3));

		internalGraph.setBounds(0, 0, viewport.getWidth() * internalGraph.zoomFactor, viewport.getMaximumVisibleHeight());
		viewport.setBounds(b);
		internalGraph.setBounds(0, 0, viewport.getWidth() * internalGraph.zoomFactor, viewport.getMaximumVisibleHeight());

		repaint();
	}

	void setBuffer(AudioSampleBuffer& b)
	{
		resized();
		internalGraph.setBuffer(b);
	}

	void setCurrentPosition(int newPos)
	{
		internalGraph.currentPosition = newPos;
		repaint();
	}

	Icons iconFactory;

	HiseShapeButton newFile, openFile, saveFile;

	AudioSampleBuffer sourceBuffer;

	AudioSampleBuffer outputBuffer;

	JitCompiledNode::Ptr currentNode;

	Result currentResult;
};


struct ParameterList : public WorkbenchComponent,
					   public SliderListener
{
	ParameterList(WorkbenchData* data) :
		WorkbenchComponent(data)
	{};

	Array<FunctionData> functions;

	void updateFromJitObject(JitObject& obj)
	{
		StringArray names = ParameterHelpers::getParameterNames(obj);

		functions.clear();
		sliders.clear();

		for (int i = 0; i < names.size(); i++)
		{
			auto s = new juce::Slider(names[i]);
			s->setLookAndFeel(&laf);
			s->setRange(0.0, 1.0, 0.01);
			s->setSliderStyle(Slider::SliderStyle::RotaryHorizontalVerticalDrag);
			s->setColour(HiseColourScheme::ComponentFillTopColourId, Colour(0x66333333));
			s->setColour(HiseColourScheme::ComponentFillBottomColourId, Colour(0xfb111111));
			s->setColour(HiseColourScheme::ComponentOutlineColourId, Colours::white.withAlpha(0.3f));
			s->setColour(HiseColourScheme::ComponentTextColourId, Colours::white);
			s->addListener(this);

			functions.add(ParameterHelpers::getFunction(names[i], obj));

			addAndMakeVisible(s);
			s->setSize(128, 48);
			sliders.add(s);
		}

		auto numColumns = jmax(1, getWidth() / 150);
		auto numRows = sliders.size() / numColumns + 1;

		setSize(getWidth(), numRows * 60);
		resized();
	}

	void sliderValueChanged(Slider* slider) override
	{
		auto index = sliders.indexOf(slider);

		if (auto f = functions[index])
		{
			auto value = slider->getValue();

			jassertfalse;
#if 0
			auto parent = findParentComponentOfClass<SnexPlayground>();

			parent->currentParameter = getName();
			parent->pendingParam = [f, value]()
			{
				f.callVoid(value);
			};
#endif
		}
	}

	void resized() override
	{
		auto numColumns = jmax(1, getWidth() / 150);
		auto numRows = sliders.size() / numColumns + 1;

		int x = 0;
		int y = 0;
		int i = 0;

		for (int row = 0; row < numRows; row++)
		{
			x = 0;

			for (int column = 0; column < numColumns; column++)
			{
				if (auto s = sliders[i])
				{
					sliders[i++]->setTopLeftPosition(x, y + 5);
					x += 150;
				}
				else
					break;
			}

			y += 50;
		}
	}

	hise::GlobalHiseLookAndFeel laf;
	juce::OwnedArray<juce::Slider> sliders;
};


}


}