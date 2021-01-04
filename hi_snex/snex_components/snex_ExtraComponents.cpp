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


namespace snex {
using namespace juce;

namespace ui
{

void OptimizationProperties::resetOptimisations()
{
	getGlobalScope().clearOptimizations();

	for (auto i : items)
	{
		if (i->active)
			getGlobalScope().addOptimization(i->id);
	}

	getWorkbench()->triggerRecompile();
}

void Graph::InternalGraph::paint(Graphics& g)
{
	if (lastBuffer.getNumSamples() == 0)
		return;

	g.fillAll(Colour(0x33666666));

	g.setFont(GLOBAL_BOLD_FONT());

	if (l.isEmpty() && r.isEmpty())
	{
		g.setColour(Colours::white.withAlpha(0.5f));
		g.setFont(GLOBAL_BOLD_FONT());
		g.drawText("No signal to draw", getLocalBounds().toFloat(), Justification::centred);
	}
	else
	{
		g.setColour(Colours::white.withAlpha(0.8f));

		if (isHiresMode())
			g.strokePath(l, PathStrokeType(2.0f));
		else
			g.fillPath(l);

		if (stereoMode && !r.isEmpty())
		{
			if (isHiresMode())
				g.strokePath(r, PathStrokeType(2.0f));
			else
				g.fillPath(r);
		}
	}

	if (currentPosition > 0 && numSamples > 0)
	{
		auto pos = getPixelForSample(currentPosition);

		g.setColour(Colours::red);
		g.drawVerticalLine((int)pos, 0.0f, (float)getHeight());
	}

	if (pixelsPerSample > 20.0f)
	{
		for (int i = 0; i < lastBuffer.getNumSamples(); i++)
		{
			auto x = getPixelForSample(i);
			g.setColour(Colours::white.withAlpha(0.3f));
			g.drawVerticalLine(x, 0.0f, (float)getHeight());
		}
	}

	if (!currentPoint.isOrigin())
	{
		float posTotal = (float)currentPoint.getX() / (float)getWidth();
		posTotal = jlimit(0.0f, 1.0f, posTotal);
		int samplePos = jlimit(0, lastBuffer.getNumSamples() - 1, roundToInt(posTotal * (float)lastBuffer.getNumSamples()));

		int x_ = (float)getPixelForSample(samplePos);
		int y_ = (float)getYPixelForSample(samplePos);

		g.fillRect(x_ - 2.0f, y_ - 2.0f, 4.0f, 4.0f);

		g.setColour(Colours::white);
		g.drawVerticalLine(x_, 0.0f, (float)getHeight());

		if (!isTimerRunning())
		{
			bool rightChannel = stereoMode && currentPoint.getY() > (getHeight() / 2);

			float value = lastBuffer.getSample(rightChannel ? 1 : 0, samplePos);

			juce::String v;
			v << "data[" << juce::String(samplePos) << "]: " << Types::Helpers::getCppValueString(value);

			auto f = GLOBAL_MONOSPACE_FONT();

			g.setFont(f);

			juce::Rectangle<float> r(0.0f, 0.0f, f.getStringWidthFloat(v) + 10.0f, f.getHeight() + 5.0f);

			auto x = ((float)currentPoint.getX() - r.getWidth()*0.5f);
			x = jlimit(0.0f, (float)getWidth() - r.getWidth(), x);
			r.setX(x);

			auto y = (float)currentPoint.getY() - 20.0f;

			y = jlimit(0.0f, (float)getHeight() - 20.0f, y);

			r.setY(y);

			g.setColour(Colour(0xEEAAAAAA));
			g.fillRoundedRectangle(r, 2.0f);
			g.setColour(Colours::black.withAlpha(0.9f));
			g.drawRoundedRectangle(r, 2.0f, 1.0f);
			g.drawText(v, r, Justification::centred);
		}


	}
}

void Graph::InternalGraph::setBuffer(const AudioSampleBuffer& b)
{
	if (b.getNumSamples() == 0)
		return;

	if (lastBuffer.getNumChannels() == 0 || lastBuffer.getWritePointer(0) != b.getReadPointer(0))
	{
		lastBuffer = AudioSampleBuffer(b.getNumChannels(), b.getNumSamples());

		for (int i = 0; i < lastBuffer.getNumChannels(); i++)
			FloatVectorOperations::copy(lastBuffer.getWritePointer(i), b.getReadPointer(i), b.getNumSamples());
	}

	calculatePath(l, b, 0);

	stereoMode = b.getNumChannels() == 2;

	if (stereoMode)
		calculatePath(r, b, 1);
	else
		r.clear();

	auto pb = getLocalBounds();

	leftPeaks = b.findMinMax(0, 0, b.getNumSamples());

	if (stereoMode)
	{
		auto lb = pb.removeFromTop(getHeight() / 2).toFloat();
		auto rb = pb.toFloat();

		l.scaleToFit(lb.getX(), lb.getY(), lb.getWidth(), lb.getHeight(), false);
		r.scaleToFit(rb.getX(), rb.getY(), rb.getWidth(), rb.getHeight(), false);

		rightPeaks = b.findMinMax(1, 0, b.getNumSamples());
	}
	else
	{
		auto lb = pb.toFloat();
		l.scaleToFit(lb.getX(), lb.getY(), lb.getWidth(), lb.getHeight(), false);
	}

	repaint();
}

void Graph::InternalGraph::calculatePath(Path& p, const AudioSampleBuffer& b, int channel)
{
	numSamples = b.getNumSamples();
	p.clear();

	if (numSamples == 0)
		return;

	if (b.getMagnitude(channel, 0, b.getNumSamples()) > 0.0f)
	{
		auto delta = (float)b.getNumSamples() / jmax(1.0f, (float)getWidth());

		int samplesPerPixel = jmax(1, (int)delta);

		pixelsPerSample = 1.0f / (float)delta;

		p.startNewSubPath(0.0f, 1.0f);

		for (int i = 0; i < b.getNumSamples(); i += samplesPerPixel)
		{
			int numToDo = jmin(samplesPerPixel, b.getNumSamples() - i);
			auto range = FloatVectorOperations::findMinAndMax(b.getReadPointer(channel, i), numToDo);

			if (range.getEnd() > (-1.0f * range.getStart()))
			{
				p.lineTo((float)i, 1.0f - range.getEnd());
			}
			else
				p.lineTo((float)i, 1.0f - range.getStart());
		}

		if (!isHiresMode())
		{
			p.lineTo((float)(b.getNumSamples() - 1), 1.0f);
			p.closeSubPath();
		}
	}
}

juce::Path Graph::Icons::createPath(const String& t) const
{
	auto url = MarkdownLink::Helpers::getSanitizedFilename(t);

	Path p;

	LOAD_PATH_IF_URL("new-file", SampleMapIcons::newSampleMap);
	LOAD_PATH_IF_URL("open-file", SampleMapIcons::loadSampleMap);
	LOAD_PATH_IF_URL("save-file", SampleMapIcons::saveSampleMap);
	
	return p;
}

void Graph::buttonClicked(Button* b)
{
	if (b == &openFile)
	{
		FileChooser fc("Load test file", {}, "*.wav", true);

		if (fc.browseForFileToOpen())
		{
			auto f = fc.getResult();

			double speed = 0.0;

			if (auto nodeHandler = dynamic_cast<JitNodeCompileThread*>(getWorkbench()->getCompileHandler()))
			{
				auto sourceBuffer = hlac::CompressionHelpers::loadFile(f, speed);

				nodeHandler->setTestBuffer(sourceBuffer);
			}

		}
	}
}

void Graph::postPostCompile(WorkbenchData::Ptr wb)
{
	

	if (auto nodeHandler = dynamic_cast<BackgroundCompileThread*>(wb->getCompileHandler()))
	{
		setBuffer(nodeHandler->getTestBuffer());
	}
}

void ParameterList::rebuild()
{
	auto h = getWorkbench()->getCompileHandler();

	if (auto nodeHandler = dynamic_cast<BackgroundCompileThread*>(h))
	{
		functions.clear();
		sliders.clear();

		int numRows = 0;

		if (currentNode = nodeHandler->getLastNode())
		{
			auto parameters = currentNode->getParameterList();

			for (auto p: parameters)
			{
				auto s = new juce::Slider(p.data.id);
				s->setLookAndFeel(&laf);
				s->setRange(p.data.min, p.data.max, p.data.interval);
				s->setSkewFactor(p.data.skew);
				s->setValue(p.data.defaultValue, dontSendNotification);

				s->setSliderStyle(Slider::SliderStyle::RotaryHorizontalVerticalDrag);
				s->setColour(HiseColourScheme::ComponentFillTopColourId, Colour(0x66333333));
				s->setColour(HiseColourScheme::ComponentFillBottomColourId, Colour(0xfb111111));
				s->setColour(HiseColourScheme::ComponentOutlineColourId, Colours::white.withAlpha(0.3f));
				s->setColour(HiseColourScheme::ComponentTextColourId, Colours::white);
				s->addListener(this);

				functions.add(p.toFunctionData());

				addAndMakeVisible(s);
				s->setSize(128, 48);
				sliders.add(s);
			}

			auto numColumns = jmax(1, getWidth() / 150);
			numRows = sliders.size() / numColumns + 1;
		}

		setSize(getWidth(), numRows * 60);
		resized();
	}

	
}

void ParameterList::sliderValueChanged(Slider* slider)
{
	if (currentNode == nullptr)
		return;

	auto index = sliders.indexOf(slider);

	if (auto f = functions[index])
	{
		auto value = (double)slider->getValue();
		f.callVoid(value);

		getWorkbench()->triggerPostCompileActions();

	}
}

}
}