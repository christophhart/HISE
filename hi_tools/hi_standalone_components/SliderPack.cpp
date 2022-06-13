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

SliderPackData::SliderPackData(UndoManager* undoManager_, PooledUIUpdater* updater) :
stepSize(0.01),
nextIndexToDisplay(-1),
showValueOverlay(true),
flashActive(true),
defaultValue(var(1.0))
{
	

	setNumSliders(NumDefaultSliders);
	setUndoManager(undoManager_);

	sliderRange = Range<double>(0.0, 1.0);
	
	if(updater != nullptr)
		setGlobalUIUpdater(updater);
}

SliderPackData::~SliderPackData()
{
}

void SliderPackData::setRange(double minValue, double maxValue, double stepSize_)
{
	sliderRange = Range<double>(minValue, maxValue);
	stepSize = stepSize_;
}

Range<double> SliderPackData::getRange() const { return sliderRange; }

double SliderPackData::getStepSize() const { return stepSize; }

int SliderPackData::getNumSliders() const 
{
	SimpleReadWriteLock::ScopedReadLock sl(getDataLock());

	return dataBuffer == nullptr ? 0 : dataBuffer->size;
};

void SliderPackData::setValue(int sliderIndex, float value, NotificationType notifySliderPack/*=dontSendNotification*/, bool useUndoManager)
{
	if (auto um = getUndoManager(useUndoManager))
	{
		um->perform(new SliderPackAction(this, sliderIndex, getValue(sliderIndex), value, notifySliderPack));
	}
	else
	{
		{
			FloatSanitizers::sanitizeFloatNumber(value);

			SimpleReadWriteLock::ScopedReadLock sl(getDataLock());

			if (isPositiveAndBelow(sliderIndex, getNumSliders()))
				dataBuffer->setSample(sliderIndex, value);
		}

		internalUpdater.sendContentChangeMessage(notifySliderPack, sliderIndex);
	}
}

float SliderPackData::getValue(int index) const
{
	SimpleReadWriteLock::ScopedReadLock sl(getDataLock());

	if(isPositiveAndBelow(index, getNumSliders()))
		return dataBuffer->getSample(index); 

	return 0.0f;
}

void SliderPackData::setFromFloatArray(const Array<float> &valueArray)
{
	{
		SimpleReadWriteLock::ScopedReadLock sl(getDataLock());

		for (int i = 0; i < valueArray.size(); i++)
		{
			if (i < getNumSliders())
			{
				float v = valueArray[i];
				FloatSanitizers::sanitizeFloatNumber(v);
				setValue(i, v, dontSendNotification);
			}
		}
	}

	internalUpdater.sendContentChangeMessage(sendNotificationAsync, -1);
}

void SliderPackData::writeToFloatArray(Array<float> &valueArray) const
{
	SimpleReadWriteLock::ScopedReadLock sl(getDataLock());

	valueArray.ensureStorageAllocated(getNumSliders());

	memcpy(valueArray.begin(), dataBuffer->buffer.getReadPointer(0), sizeof(float)*getNumSliders());
}

String SliderPackData::dataVarToBase64(const var& data)
{
	Array<float> fd;
	fd.ensureStorageAllocated(data.size());

	if (auto d = data.getArray())
	{
		for (const auto& v : *d)
			fd.add((float)v);
	}

	MemoryBlock mb = MemoryBlock(fd.getRawDataPointer(), fd.size() * sizeof(float));
	return mb.toBase64Encoding();
}

var SliderPackData::base64ToDataVar(const String& b64)
{
	MemoryBlock mb;
	mb.fromBase64Encoding(b64);

	int numElements = (int)(mb.getSize() / sizeof(float));
	auto ptr = (float*)mb.getData();

	Array<var> data;
	data.ensureStorageAllocated(numElements);

	for (int i = 0; i < numElements; i++)
		data.add(ptr[i]);

	return data;
}

String SliderPackData::toBase64() const
{
	auto data = getCachedData();
	MemoryBlock mb = MemoryBlock(data, getNumSliders() * sizeof(float));
	return mb.toBase64Encoding();
}

void SliderPackData::fromBase64(const String &encodedValues)
{
	if (encodedValues.isEmpty()) return;

	MemoryBlock mb;

	mb.fromBase64Encoding(encodedValues);

	if (int numElements = (int)(mb.getSize() / sizeof(float)))
	{
		VariantBuffer::Ptr newBuffer = new VariantBuffer(numElements);

		memcpy(newBuffer->buffer.getWritePointer(0), mb.getData(), mb.getSize());

		swapBuffer(newBuffer, sendNotification);
	}
}

void SliderPackData::swapData(const var &otherData, NotificationType n)
{
	if (otherData.isArray())
	{
		VariantBuffer::Ptr newBuffer = new VariantBuffer(otherData.size());

		for (int i = 0; i < newBuffer->size; i++)
		{
			auto v = (float)otherData[i];;
			FloatSanitizers::sanitizeFloatNumber(v);
			(*newBuffer)[i] = (float)v;
		}

		swapBuffer(newBuffer, n);
	}
	else if (otherData.isBuffer())
	{
		swapBuffer(otherData.getBuffer(), n);
	}
}

void SliderPackData::setNewUndoAction() const
{
	
}

void SliderPackData::swapBuffer(VariantBuffer::Ptr otherBuffer, NotificationType n)
{
	SimpleReadWriteLock::ScopedWriteLock sl(getDataLock());
	std::swap(otherBuffer, dataBuffer);

	if(n != dontSendNotification)
		internalUpdater.sendContentRedirectMessage();
}

void SliderPackData::setNumSliders(int numSliders)
{
	if (numSliders == 0)
		return;

	if (getNumSliders() != numSliders)
	{
		int numToCopy = jmin<int>(numSliders, getNumSliders());

		VariantBuffer::Ptr newBuffer = new VariantBuffer(numSliders);

		for (int i = 0; i < numSliders; i++)
		{
			if (i < numToCopy)
				newBuffer->setSample(i, getValue(i));
			else
				newBuffer->setSample(i, defaultValue);
		}

		swapBuffer(newBuffer, sendNotification);
	}
}


SliderPack::SliderPack(SliderPackData *data_):
data(data_),
currentlyDragged(false),
currentlyDraggedSlider(-1),
currentlyDraggedSliderValue(0.0),
defaultValue(0.0),
dummyData(new SliderPackData(nullptr, nullptr))
{
	setSpecialLookAndFeel(new SliderLookAndFeel(), true);

	if (data == nullptr)
	{
		data = dummyData.get();
		data->setNumSliders(128);
	}
		
	getData()->addListener(this);

	setRepaintsOnMouseActivity(true);

	setColour(Slider::backgroundColourId, Colour(0x22000000));
	setColour(Slider::textBoxOutlineColourId, Colours::white.withAlpha(0.2f));
	setColour(Slider::thumbColourId, Colours::white.withAlpha(0.6f));

	rebuildSliders();
}

void SliderPack::setNumSliders(int numSliders)
{
	data->setNumSliders(numSliders);
}

void SliderPack::updateSliders()
{
	for (int i = 0; i < sliders.size(); i++)
	{
		Slider *s = sliders[i];

        float v = (float)data->getValue(i);
        v = FloatSanitizers::sanitizeFloatNumber(v);
        
		s->setValue((double)v, dontSendNotification);

	}

	if (getWidth() != 0) 
		resized();
}


double SliderPack::getValue(int sliderIndex)
{
	return data->getValue(sliderIndex);
}

void SliderPack::setValue(int sliderIndex, double newValue)
{
	data->setValue(sliderIndex, (float)newValue, sendNotificationAsync, false);
}

void SliderPack::setSliderPackData(SliderPackData* newData)
{
	if (data != newData)
	{
		if (data != nullptr)
			data->removeListener(this);

		data = newData;

		slidersNeedRebuild = true;
		startTimer(30);

		if (data != nullptr)
			data->addListener(this);
	}
	
}

void SliderPack::resized()
{
	int w = getWidth();

    if(data != nullptr && (sliderWidths.isEmpty() || getNumSliders()+1 != (sliderWidths.size())))
    {
        float widthPerSlider = (float)w / (float)data->getNumSliders();
        
		float x = 0.0f;

        for (int i = 0; i < sliders.size(); i++)
        {
			int thisXPos = std::floor(x);

			auto subMod = std::fmod(x, 1.0f);

			int thisWidth = std::floor(widthPerSlider + subMod) - 1;

			sliders[i]->setBounds(thisXPos, 0, thisWidth, getHeight());

			x += widthPerSlider;
			
        }
    }
    else
    {
        int x = 0;
        auto totalWidth = (double)w;
        
        for(int i = 0; i < sliders.size(); i++)
        {
            auto normalizedWidth = (double)sliderWidths[i+1] - (double)sliderWidths[i];
            
            auto sliderWidth = jmax(5, roundToInt(normalizedWidth * totalWidth));
            
            sliders[i]->setBounds(x, 0, sliderWidth, getHeight());
            x += sliderWidth;
        }
    }
}

void SliderPack::update()
{
	for (int i = 0; i < sliders.size(); i++)
	{
        float v = (float)data->getValue(i);
        v = FloatSanitizers::sanitizeFloatNumber(v);
        
		sliders[i]->setValue((double)v, dontSendNotification);
	}
}

SliderPack::~SliderPack()
{
	if (auto d = getData())
		data->removeListener(this);
}

void SliderPack::sliderValueChanged(Slider *s)
{
	int index = sliders.indexOf(s);

	if(data.get() == nullptr) return;
    
	data->setValue(index, (float)s->getValue(), sendNotificationSync, true);
}

void SliderPack::mouseDown(const MouseEvent &e)
{
	if (!isEnabled()) return;

	int x = e.getEventRelativeTo(this).getMouseDownPosition().getX();
	int y = e.getEventRelativeTo(this).getMouseDownPosition().getY();

	if (e.mods.isRightButtonDown() || e.mods.isCommandDown())
	{
		Point<float> start((float)x, (float)y);
		rightClickLine = Line<float>(start, start);
	}
	else
	{
		rightClickLine = {};
		data->startDrag();

		int sliderIndex = getSliderIndexForMouseEvent(e);

		

		getData()->setDisplayedIndex(sliderIndex);

		Slider *s = sliders[sliderIndex];

		if (s == nullptr)
			return;

		double normalizedValue = (double)(getHeight() - y) / (double)getHeight();

		double value = s->proportionOfLengthToValue(normalizedValue);

		currentlyDragged = true;
		currentlyDraggedSlider = sliderIndex;

		s->setValue(value, sendNotificationSync);

		currentlyDraggedSliderValue = s->getValue();

		lastDragIndex = sliderIndex;
		lastDragValue = currentlyDraggedSliderValue;
	}

	repaint();
}

void SliderPack::mouseDrag(const MouseEvent &e)
{
	if (!isEnabled()) return;

	int x = e.getEventRelativeTo(this).getPosition().getX();
	int y = e.getEventRelativeTo(this).getPosition().getY();

	Rectangle<int> thisBounds(0, 0, getWidth(), getHeight());

	if (!rightClickLine.getStart().isOrigin())
	{
		if (!thisBounds.contains(Point<int>(x, y)))
		{
			if (x > getWidth()) x = getWidth();

			if (y > getHeight()) y = getHeight();

			if (y < 0) y = 0;

			if (x < 0) x = 0;
		}

		rightClickLine.setEnd((float)x, (float)y);

		repaint();
	}
	else
	{
		if (!thisBounds.contains(Point<int>(x, y)))
		{
			if (x > getWidth()) x = getWidth();

			if (y > getHeight()) y = getHeight();

			if (y < 0) y = 0;

			if (x < 0) x = 0;
		}

		int sliderIndex = getSliderIndexForMouseEvent(e);

		if (sliderIndex < 0) sliderIndex = 0;
		if (sliderIndex >= sliders.size()) sliderIndex = sliders.size() - 1;

		if (auto s = sliders[sliderIndex])
		{
			double normalizedValue = (double)(getHeight() - y) / (double)getHeight();

			double value = s->proportionOfLengthToValue(normalizedValue);

			currentlyDragged = true;
			currentlyDraggedSlider = sliderIndex;
			currentlyDraggedSliderValue = value;

			s->setValue(value, sendNotificationSync);

			currentlyDraggedSliderValue = s->getValue();

			repaint();
		}

		if (std::abs(sliderIndex - lastDragIndex) > 1)
		{
			bool inv = sliderIndex > lastDragIndex;

			auto start = inv ? lastDragIndex : sliderIndex;
			auto end = inv ? sliderIndex : lastDragIndex;
			auto startValue = inv ? lastDragValue : currentlyDraggedSliderValue;
			auto endValue = inv ? currentlyDraggedSliderValue : lastDragValue;
			auto delta = 1.0f / (float)(end - start);

			float alpha = 0.0f;

			for (int i = start; i < end; i++)
			{
				auto v = startValue + (endValue - startValue) * alpha;
				alpha += delta;

				if (auto s = sliders[i])
					s->setValue(v, sendNotificationSync);
			}

		}

		lastDragIndex = sliderIndex;
		lastDragValue = currentlyDraggedSliderValue;
	}
}

void SliderPack::mouseUp(const MouseEvent &e)
{
	if (!isEnabled()) return;

	currentlyDragged = false;

	if(!rightClickLine.getStart().isOrigin()) setValuesFromLine();

	repaint();
}

void SliderPack::mouseExit(const MouseEvent &)
{
	if (!isEnabled()) return;

	currentlyDragged = false;
	repaint();
}

void SliderPack::paintOverChildren(Graphics &g)
{
	if (data.get() == nullptr) return;

	if (displayAlphas.size() != sliders.size())
	{
		return;
	}

	if (isTimerRunning() && data->isFlashActive())
	{
		for (int i = 0; i < displayAlphas.size(); i++)
		{
			if (displayAlphas[i] > 0.0f)
			{
				const bool biPolar = sliders[i]->getMinimum() < 0;

				

				auto v = (int)sliders[i]->getPositionOfValue(sliders[i]->getValue());

				int x = sliders[i]->getX();
				
				int w = sliders[i]->getWidth();
				
				int y;
				int h;

				if (biPolar)
				{
					int h_half = sliders[i]->getHeight() / 2;

					y = v < h_half ? v : h_half;
					h = v < h_half ? (h_half - y) : (v - h_half);
				}
				else
				{
					y = v;
					h = sliders[i]->getHeight() - v;
				}

				if (auto l = getSpecialLookAndFeel<LookAndFeelMethods>())
					l->drawSliderPackFlashOverlay(g, *this, i, { x, y, w, h }, displayAlphas[i]);
			}
		}
	}

	if (rightClickLine.getLength() != 0)
	{
		g.setColour(Colours::white.withAlpha(0.6f));

		g.drawLine(rightClickLine, 1.0f);

		Rectangle<float> startDot(rightClickLine.getStart().getX() - 2.0f, rightClickLine.getStart().getY() - 2.0f, 4.0f, 4.0f);

		Rectangle<float> endDot(rightClickLine.getEnd().getX() - 2.0f, rightClickLine.getEnd().getY() - 2.0f, 4.0f, 4.0f);

		g.drawRoundedRectangle(startDot, 2.0f, 1.0f);

		g.drawRoundedRectangle(endDot, 2.0f, 1.0f);

		g.setColour(Colours::white.withAlpha(0.2f));

		g.fillRoundedRectangle(startDot, 2.0f);
		g.fillRoundedRectangle(endDot, 2.0f);

		
	}

	else if (currentlyDragged && data->isValueOverlayShown())
	{
		const double logFromStepSize = log10(data->getStepSize());

		const int unit = -roundToInt(logFromStepSize);

		g.setColour(Colours::white.withAlpha(0.3f));

		String textToDraw = " #" + String(currentlyDraggedSlider) + ": " + String(currentlyDraggedSliderValue, unit) + suffix + " ";

		const int w = GLOBAL_MONOSPACE_FONT().getStringWidth(textToDraw);

		Rectangle<int> r(getWidth() - w, 0, w, 18);

		g.fillRect(r);

		g.setColour(Colours::white.withAlpha(0.4f));

		g.drawRect(r, 1);

		g.setColour(Colours::black);
		g.setFont(GLOBAL_MONOSPACE_FONT());

		g.drawText(textToDraw, r, Justification::centredRight, true);
	}
}

void SliderPack::setValuesFromLine()
{
	data->setNewUndoAction();

    int lastIndex = -1;
    
	for (int i = 0; i < sliders.size(); i++)
	{
		Slider *s = sliders[i];

		Rectangle<float> sliderArea((float)s->getX(), 0.0f, (float)sliders[i]->getWidth(), (float)getHeight());

		if (sliderArea.intersects(rightClickLine))
		{
			Line<float> midLine(sliderArea.getCentreX(), 0.0f, sliderArea.getCentreX(), (float)getHeight());

			const double y = (double)midLine.getIntersection(rightClickLine).getY();

			double normalizedValue = ((double)getHeight() - y) / (double)getHeight();

			double value = s->proportionOfLengthToValue(normalizedValue);

            data->setValue(i, value, dontSendNotification, true);
            lastIndex = -1;
		}
	}
    
    data->getUpdater().sendContentChangeMessage(sendNotificationAsync, lastIndex);

	rightClickLine = Line<float>(0.0f, 0.0f, 0.0f, 0.0f);
}

void SliderPack::displayedIndexChanged(SliderPackData* d, int newIndex)
{
	auto f = [](SliderPack& s)
	{
		for (int i = 0; i < s.getNumSliders(); i++)
			s.sliders[i]->setValue(s.getValue(i), dontSendNotification);
	};

	SafeAsyncCall::call<SliderPack>(*this, f);
	
	if (currentDisplayIndex != newIndex)
	{
		currentDisplayIndex = newIndex;

        if(newIndex != -1)
        {
            displayAlphas.set(newIndex, 0.4f);
            startTimer(30);
        }
	}
}

void SliderPack::timerCallback()
{
	if (data.get() == nullptr) return;

	if (slidersNeedRebuild)
	{
		rebuildSliders();
		slidersNeedRebuild = false;
		stopTimer();
	}

	if (!data->isFlashActive()) return;

	bool repaintThisTime = false;

	for (int i = 0; i < displayAlphas.size(); i++)
	{
		if (displayAlphas[i] > 0.0f)
		{
			repaintThisTime = true;

			displayAlphas.set(i, displayAlphas[i] - 0.05f);
		}
	}

	if (repaintThisTime) repaint();
	else stopTimer();
}

void SliderPack::mouseDoubleClick(const MouseEvent &e)
{
	if (!isEnabled()) return;

	if (e.mods.isShiftDown())
	{
		for (int i = 0; i < data->getNumSliders(); i++)
		{
			data->setValue(i, (float)defaultValue, sendNotification);
		}
	}
	else
	{
		int x = e.getEventRelativeTo(this).getMouseDownPosition().getX();

		int sliderIndex = (int)((float)x / (float)getWidth() * sliders.size());

		data->setValue(sliderIndex, (float)defaultValue, sendNotification);
	}
}

void SliderPack::paint(Graphics &g)
{
	if (auto l = getSpecialLookAndFeel<LookAndFeelMethods>())
	{
		l->drawSliderPackBackground(g, *this);
	}
	
}

void SliderPack::setSuffix(const String &suffix_)
{
	suffix = suffix_;
}

int SliderPack::getNumSliders()
{
	return sliders.size();
}



void SliderPack::setFlashActive(bool flashShouldBeActive)
{
	if (data != nullptr)
		data->setFlashActive(flashShouldBeActive);
}

void SliderPack::setColourForSliders(int colourId, Colour c)
{
	// when the sliderpack gets updated, it fetches the colour from here...
	setColour(colourId, c);

	sliders.clear();
	rebuildSliders();
}

void SliderPack::setShowValueOverlay(bool shouldShowValueOverlay)
{
	if (data != nullptr)
		data->setShowValueOverlay(shouldShowValueOverlay);
}

void SliderPack::setStepSize(double stepSize)
{
	if (data != nullptr)
	{
		data->setRange(data->getRange().getStart(), data->getRange().getEnd(), stepSize);
	}
}


void SliderPack::rebuildSliders()
{
	if (auto d = getData())
	{
		displayAlphas.clear();

		auto numSliders = d->getNumSliders();

		displayAlphas.insertMultiple(0, 0.0f, numSliders);

		int numToRemove = sliders.size() - numSliders;

		int numToAdd = -1 * numToRemove;

		for (int i = 0; i < numToRemove; i++)
		{
			sliders.removeLast();
		}
			
		for (int i = 0; i < numToAdd; i++)
		{
			Slider *s = new Slider();
			addAndMakeVisible(s);
			sliders.add(s);
			s->setLookAndFeel(getSpecialLookAndFeel<LookAndFeel>());
			s->setInterceptsMouseClicks(false, false);
			s->addListener(this);
			s->setSliderStyle(Slider::SliderStyle::LinearBarVertical);
			s->setTextBoxStyle(Slider::TextEntryBoxPosition::NoTextBox, true, 0, 0);

			s->setRange(data->getRange().getStart(), data->getRange().getEnd(), data->getStepSize());
			s->setColour(Slider::backgroundColourId, findColour(Slider::backgroundColourId));
			s->setColour(Slider::textBoxOutlineColourId, Colours::transparentBlack);
			s->setColour(Slider::thumbColourId, findColour(Slider::thumbColourId));
			s->setColour(Slider::trackColourId, findColour(Slider::trackColourId));
		}

		updateSliders();
	}
}

int SliderPack::getSliderIndexForMouseEvent(const MouseEvent& e)
{
	int x = e.getEventRelativeTo(this).getPosition().getX();

	auto w = (float)getWidth();

	float xNormalized = jlimit<float>(0.0f, 0.999f, (float)x / w);

	if (sliderWidths.size() == 0)
	{
		return (int)(xNormalized * (float)sliders.size());
	}
	else
	{
		for (int i = 0; i < sliderWidths.size()-1; i++)
		{
			float nextValue = (float)sliderWidths[i+1];
			
			if (xNormalized <= nextValue)
				return i;
		}

		return 0;
	}
}

void SliderPack::SliderLookAndFeel::drawLinearSlider(Graphics &g, int /*x*/, int /*y*/, int width, int height, float /*sliderPos*/, float /*minSliderPos*/, float /*maxSliderPos*/, const Slider::SliderStyle style, Slider &s)
{
	if (style == Slider::SliderStyle::LinearBarVertical)
	{
		const bool isBiPolar = s.getMinimum() < 0.0 && s.getMaximum() > 0.0;

		float leftY;
		float actualHeight;

		const float max = (float)s.getMaximum();
		const float min = (float)s.getMinimum();

		g.fillAll(s.findColour(Slider::ColourIds::backgroundColourId));

		if (isBiPolar)
		{
			const float value = (-1.0f * (float)s.getValue() - min) / (max - min);

			leftY = (value < 0.5f ? value * (float)(height) : 0.5f * (float)(height));
			actualHeight = fabs(0.5f - value) * (float)(height);
		}
		else
		{
			const double value = s.getValue();
			const double normalizedValue = (value - s.getMinimum()) / (s.getMaximum() - s.getMinimum());
			const double proportion = pow(normalizedValue, s.getSkewFactor());

			//const float value = ((float)s.getValue() - min) / (max - min);

			actualHeight = (float)proportion * (float)(height);
			leftY = (float)height - actualHeight;
		}

		Colour c = s.findColour(Slider::ColourIds::thumbColourId);

		g.setGradientFill(ColourGradient(c.withMultipliedAlpha(s.isEnabled() ? 1.0f : 0.4f),
			0.0f, 0.0f,
			c.withMultipliedAlpha(s.isEnabled() ? 1.0f : 0.3f).withMultipliedBrightness(0.9f),
			0.0f, (float)height,
			false));

		g.fillRect(0.0f, leftY, (float)(width + 1), actualHeight + 1.0f);

		if (width > 4)
		{
			//g.setColour(Colours::white.withAlpha(0.3f));
			g.setColour(s.findColour(Slider::trackColourId));
			g.drawRect(0.0f, leftY, (float)(width + 1), actualHeight + 1.0f, 1.0f);
		}


	}
	else
	{
		const bool isBiPolar = s.getMinimum() < 0.0 && s.getMaximum() > 0.0;

		float leftX;
		float actualWidth;

		const float max = (float)s.getMaximum();
		const float min = (float)s.getMinimum();

		g.fillAll(Colour(0xfb333333));

		if (isBiPolar)
		{
			const float value = ((float)s.getValue() - min) / (max - min);

			leftX = 2.0f + (value < 0.5f ? value * (float)(width - 2) : 0.5f * (float)(width - 2));
			actualWidth = fabs(0.5f - value) * (float)(width - 2);
		}
		else
		{
			const double value = s.getValue();
			const double normalizedValue = (value - s.getMinimum()) / (s.getMaximum() - s.getMinimum());
			const double proportion = pow(normalizedValue, s.getSkewFactor());

			//const float value = ((float)s.getValue() - min) / (max - min);

			leftX = 2;
			actualWidth = (float)proportion * (float)(width - 2);
		}

		g.setGradientFill(ColourGradient(Colour(0xff888888).withAlpha(s.isEnabled() ? 0.8f : 0.4f),
			0.0f, 0.0f,
			Colour(0xff666666).withAlpha(s.isEnabled() ? 0.8f : 0.4f),
			0.0f, (float)height,
			false));
		g.fillRect(leftX, 2.0f, actualWidth, (float)(height - 2));
	}
}

void SliderPack::LookAndFeelMethods::drawSliderPackBackground(Graphics& g, SliderPack& s)
{
	Colour background = s.findColour(Slider::ColourIds::backgroundColourId);

	g.setGradientFill(ColourGradient(background, 0.0f, 0.0f,
		background.withMultipliedBrightness(0.8f), 0.0f, (float)s.getHeight(), false));

	g.fillAll();
	Colour outline = s.findColour(Slider::ColourIds::textBoxOutlineColourId);
	g.setColour(outline);
	g.drawRect(s.getLocalBounds(), 1);
}

void SliderPack::LookAndFeelMethods::drawSliderPackFlashOverlay(Graphics& g, SliderPack& s, int sliderIndex, Rectangle<int> sliderBounds, float intensity)
{
	g.setColour(Colours::white.withAlpha(intensity));
	g.fillRect(sliderBounds);
}

} // namespace hise
