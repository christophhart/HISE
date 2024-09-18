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

	SliderPackData::Listener::~Listener()
	{}

	void SliderPackData::Listener::sliderAmountChanged(SliderPackData* d)
	{}

	void SliderPackData::Listener::displayedIndexChanged(SliderPackData* d, int newIndex)
	{}

	void SliderPackData::Listener::setSliderPack(SliderPackData* d)
	{
		connectedSliderPack = d;
	}

	void SliderPackData::Listener::onComplexDataEvent(ComplexDataUIUpdaterBase::EventType t, var v)
	{
		switch (t)
		{
		case ComplexDataUIUpdaterBase::EventType::ContentRedirected:
			sliderAmountChanged(connectedSliderPack);
			break;
		case ComplexDataUIUpdaterBase::EventType::ContentChange:
			sliderPackChanged(connectedSliderPack, (int)v);
			break;
		case ComplexDataUIUpdaterBase::EventType::DisplayIndex:
			displayedIndexChanged(connectedSliderPack, (int)v);
			break;
		default:
			break;
		}
	}

	void SliderPackData::startDrag()
	{
		
	}

	bool SliderPackData::fromBase64String(const String& b64)
	{
		fromBase64(b64);

		return getNumSliders() > 0;
	}

	String SliderPackData::toBase64String() const
	{
		return toBase64();
	}

	int SliderPackData::getNextIndexToDisplay() const
	{
		return nextIndexToDisplay;
	}

	void SliderPackData::setDisplayedIndex(int index)
	{
		nextIndexToDisplay = index;
		internalUpdater.sendDisplayChangeMessage((float)index, sendNotificationAsync);
	}

	const float* SliderPackData::getCachedData() const
	{
		return dataBuffer->buffer.getReadPointer(0);
	}

	var SliderPackData::getDataArray() const
	{ return var(dataBuffer.get()); }

	void SliderPackData::setFlashActive(bool shouldBeShown)
	{ flashActive = shouldBeShown; }

	void SliderPackData::setShowValueOverlay(bool shouldBeShown)
	{ showValueOverlay = shouldBeShown; }

	bool SliderPackData::isFlashActive() const
	{ return flashActive; }

	bool SliderPackData::isValueOverlayShown() const
	{ return showValueOverlay; }

	void SliderPackData::setDefaultValue(double newDefaultValue)
	{
		defaultValue = (float)newDefaultValue;
	}

	float SliderPackData::getDefaultValue() const
	{ return defaultValue; }

	void SliderPackData::addListener(Listener* listener)
	{
		listener->setSliderPack(this);
		internalUpdater.addEventListener(listener);
	}

	void SliderPackData::removeListener(Listener* listener)
	{
		listener->setSliderPack(nullptr);
		internalUpdater.removeEventListener(listener);
	}

	void SliderPackData::sendValueChangeMessage(int index, NotificationType notify)
	{
		internalUpdater.sendContentChangeMessage(notify, index);
	}

	void SliderPackData::setUsePreallocatedLength(int numMaxSliders)
	{
		if(numMaxSliders != numPreallocated)
		{
			numPreallocated = numMaxSliders;
            
			if(numPreallocated > 0)
			{
				preallocatedData.calloc(numPreallocated);
                
				int numToCopy = jmin(numMaxSliders, getNumSliders());
                
				FloatVectorOperations::copy(preallocatedData.get(), dataBuffer->buffer.getReadPointer(0), numToCopy);
                
				{
					SimpleReadWriteLock::ScopedWriteLock sl(getDataLock());
					dataBuffer->referToData(preallocatedData.get(), numToCopy);
				}
                
				internalUpdater.sendContentRedirectMessage();
			}
			else
			{
				auto newBuffer = new VariantBuffer(getNumSliders());
                
				FloatVectorOperations::copy(newBuffer->buffer.getWritePointer(0), dataBuffer->buffer.getReadPointer(0), getNumSliders());
                
				swapBuffer(newBuffer, sendNotification);
                
				preallocatedData.free();
			}
            
            
		}
	}

	SliderPackData::SliderPackAction::SliderPackAction(SliderPackData* data_, int sliderIndex_, float oldValue_,
		float newValue_, NotificationType n_):
		UndoableAction(),
		singleValue(true),
		data(data_),
		sliderIndex(sliderIndex_),
		oldValue(oldValue_),
		newValue(newValue_),
		n(n_)
	{}

	SliderPackData::SliderPackAction::SliderPackAction(SliderPackData* data_, const Array<float>& newValues, NotificationType n_):
		UndoableAction(),
		singleValue(false),
		data(data_),
		n(n_)
	{
		newData.addArray(newValues);
		data->writeToFloatArray(oldData);

		for(int i = 0; i < newValues.size(); i++)
		{
			String s;
			s << String(oldData[i]) + " -> " + String(newData[i]);
			DBG(s);
		}
		
	}

	bool SliderPackData::SliderPackAction::perform()
	{
		if (data != nullptr)
		{
			if(singleValue)
				data->setValue(sliderIndex, newValue, n, false);
			else
				data->setFromFloatArray(newData, n, false);
			
			return true;
		}

		return false;
	}

	bool SliderPackData::SliderPackAction::undo()
	{
		if (data != nullptr)
		{
			if(singleValue)
				data->setValue(sliderIndex, oldValue, n, false);
			else
				data->setFromFloatArray(oldData, n, false);

			return true;
		}

		return false;
	}

SliderPackData::SliderPackData(UndoManager* undoManager_, PooledUIUpdater* updater) :
stepSize(0.01),
nextIndexToDisplay(-1),
showValueOverlay(true),
flashActive(true),
defaultValue(1.0f)
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

	return defaultValue;
}

void SliderPackData::setFromFloatArray(const Array<float> &valueArray, NotificationType n, bool useUndoManager)
{
	if(auto um = getUndoManager(useUndoManager))
	{
		um->perform(new SliderPackAction(this, valueArray, n));
	}
	else
	{
		{
			int numToCopy = jmin(valueArray.size(), getNumSliders());
	        FloatSanitizers::sanitizeArray((float*)valueArray.getRawDataPointer(), numToCopy);

			for(int i = 0; i < valueArray.size(); i++)
			{
				DBG(valueArray[i]);
			}

			SimpleReadWriteLock::ScopedReadLock sl(getDataLock());
	        FloatVectorOperations::copy(dataBuffer->buffer.getWritePointer(0), valueArray.begin(), numToCopy);
		}

		internalUpdater.sendContentChangeMessage(n, -1);
	}
}

void SliderPackData::writeToFloatArray(Array<float> &valueArray) const
{
	SimpleReadWriteLock::ScopedReadLock sl(getDataLock());

	valueArray.ensureStorageAllocated(getNumSliders());
	valueArray.addArray(dataBuffer->buffer.getReadPointer(0), getNumSliders());
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
    if(numPreallocated != 0)
    {
        int numToUse = jmin(numPreallocated, otherBuffer->size);
        FloatVectorOperations::copy(preallocatedData.get(), otherBuffer->buffer.getReadPointer(0), numToUse);
        
        SimpleReadWriteLock::ScopedWriteLock sl(getDataLock());
        dataBuffer->referToData(preallocatedData.get(), numToUse);
    }
    else
    {
        SimpleReadWriteLock::ScopedWriteLock sl(getDataLock());
        std::swap(otherBuffer, dataBuffer);
    }
    
	if(n != dontSendNotification)
		internalUpdater.sendContentRedirectMessage();
}

void SliderPackData::setNumSliders(int numSliders)
{
	if (numSliders <= 0)
		return;

	if (getNumSliders() != numSliders)
	{
        if(numPreallocated != 0)
        {
            int numToUse = jmin(numSliders, numPreallocated);
            
            {
                SimpleReadWriteLock::ScopedWriteLock sl(getDataLock());
                dataBuffer->referToData(preallocatedData.get(), numToUse);
            }
            
            internalUpdater.sendContentRedirectMessage();
            return;
        }
        
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

void SliderPack::updateSliderRange()
{
	auto rToUse = data->getRange();
	auto stepSize = data->getStepSize();

	for (int i = 0; i < sliders.size(); i++)
	{
		Slider *s = sliders[i];

		s->setRange(rToUse, stepSize);

		float v = (float)data->getValue(i);
		v = FloatSanitizers::sanitizeFloatNumber(v);

		s->setValue((double)v, dontSendNotification);
		s->repaint();
	}

	repaint();
}

void SliderPack::updateSliderColours()
{
	for(auto s: sliders)
	{
		s->setColour(Slider::backgroundColourId, findColour(Slider::backgroundColourId));
		s->setColour(Slider::textBoxOutlineColourId, Colours::transparentBlack);
		s->setColour(Slider::thumbColourId, findColour(Slider::thumbColourId));
		s->setColour(Slider::trackColourId, findColour(Slider::trackColourId));
	}
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

    setTextAreaPopup(getLocalBounds().removeFromRight(100).removeFromTop(40));
    
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
    
	NotificationType n = sendNotificationSync;

	bool useUndo = true;

	if (callbackOnMouseUp)
	{
		n = dontSendNotification;
		useUndo = false;
	}

    auto currentValue = (float)s->getValue();
    
	if(this->toggleMaxMode)
	{
		data->setValue(index, currentStepSequencerInputValue, n, useUndo);
	}
	else
	{
		data->setValue(index, currentValue, n, useUndo);
	}
}

void SliderPack::mouseDown(const MouseEvent &e)
{
	CHECK_MIDDLE_MOUSE_DOWN(e);

	if (!isEnabled()) return;

	int x = e.getEventRelativeTo(this).getMouseDownPosition().getX();
	int y = e.getEventRelativeTo(this).getMouseDownPosition().getY();

	auto n = sendNotificationSync;

	if(callbackOnMouseUp)
		n = dontSendNotification;

	if(toggleMaxMode)
	{
        int sliderIndex = getSliderIndexForMouseEvent(e);
        
        if(isPositiveAndBelow(sliderIndex, data->getNumSliders()))
        {
			auto rng = sliders[sliderIndex]->getRange();
			auto thisValue = sliders[sliderIndex]->getValue();

			auto useGhostNoteValue = e.mods.isAnyModifierKeyDown();
			auto ghostNoteValue = rng.getStart() + 0.5 * rng.getLength();

			if(thisValue == rng.getStart())
			{
				if(useGhostNoteValue)
					currentStepSequencerInputValue = ghostNoteValue;
				else
					currentStepSequencerInputValue = rng.getEnd();
			}
			else
			{
				if(useGhostNoteValue == (thisValue == ghostNoteValue))
					currentStepSequencerInputValue = rng.getStart();
				else
					currentStepSequencerInputValue = useGhostNoteValue ? ghostNoteValue : rng.getEnd();
			}
        }
	}

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

		
		if(!callbackOnMouseUp)
			getData()->setDisplayedIndex(sliderIndex);

		Slider *s = sliders[sliderIndex];

		if (s == nullptr)
			return;

		double normalizedValue = (double)(getHeight() - y) / (double)getHeight();

		double value = toggleMaxMode ? currentStepSequencerInputValue : s->proportionOfLengthToValue(normalizedValue);

		currentlyDragged = true;
		currentlyDraggedSlider = sliderIndex;

		s->setValue(value, n);

		currentlyDraggedSliderValue = s->getValue();

		lastDragIndex = sliderIndex;
		lastDragValue = currentlyDraggedSliderValue;
	}

	repaint();
}

void SliderPack::mouseDrag(const MouseEvent &e)
{
	CHECK_MIDDLE_MOUSE_DRAG(e);

	if (!isEnabled()) return;

	int x = e.getEventRelativeTo(this).getPosition().getX();
	int y = e.getEventRelativeTo(this).getPosition().getY();

	Rectangle<int> thisBounds(0, 0, getWidth(), getHeight());

	auto n = sendNotificationSync;

	if(callbackOnMouseUp)
		n = dontSendNotification;

	if (!rightClickLine.getStart().isOrigin())
	{
		if (!thisBounds.contains(Point<int>(x, y)))
		{
			if (x > getWidth()) x = getWidth();

			if (y > getHeight()) y = getHeight();

			if (y < 0) y = 0;

			if (x < 0) x = 0;
		}

		repaintWithTextBox(Rectangle<float>(rightClickLine.getStart(), rightClickLine.getEnd()).toNearestInt().expanded(5));

		rightClickLine.setEnd((float)x, e.mods.isShiftDown() ? rightClickLine.getStartY() : (float)y);

        repaintWithTextBox(Rectangle<float>(rightClickLine.getStart(), rightClickLine.getEnd()).toNearestInt().expanded(5));
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

			double value = toggleMaxMode ? currentStepSequencerInputValue : s->proportionOfLengthToValue(normalizedValue);

			if(isPositiveAndBelow(currentlyDraggedSlider, sliders.size()))
                repaintWithTextBox(sliders[currentlyDraggedSlider]->getBoundsInParent());

			currentlyDragged = true;
			currentlyDraggedSlider = sliderIndex;
			currentlyDraggedSliderValue = value;

			s->setValue(value, n);

			currentlyDraggedSliderValue = s->getValue();

			if(isPositiveAndBelow(currentlyDraggedSlider, sliders.size()))
                repaintWithTextBox(sliders[currentlyDraggedSlider]->getBoundsInParent());
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
					s->setValue(v, n);
			}
		}

		lastDragIndex = sliderIndex;
		lastDragValue = currentlyDraggedSliderValue;
	}
}

void SliderPack::mouseUp(const MouseEvent &e)
{
	CHECK_MIDDLE_MOUSE_UP(e);

	if (!isEnabled()) return;

	currentlyDragged = false;

	if(!rightClickLine.getStart().isOrigin())
	{
		setValuesFromLine();
		return;
	}

	if (callbackOnMouseUp)
	{
		Array<float> newData;
		newData.ensureStorageAllocated(getNumSliders());

		for(int i = 0; i < getNumSliders(); i++)
			newData.add((float)sliders[i]->getValue());
		
		getData()->setFromFloatArray(newData, sendNotificationAsync, true);
	}
	
	repaint();
}

void SliderPack::mouseExit(const MouseEvent &)
{
	if (!isEnabled()) return;

	showOverlayOnMove = false;
	currentlyDragged = false;
	currentlyHoveredSlider = -1;
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
		if (auto l = getSpecialLookAndFeel<LookAndFeelMethods>())
			l->drawSliderPackRightClickLine(g, *this, rightClickLine);
	}

	else if ((currentlyDragged || showOverlayOnMove) && data->isValueOverlayShown())
	{
		const double logFromStepSize = log10(data->getStepSize());
		const int unit = -roundToInt(logFromStepSize);
		String textToDraw = " #" + String(currentlyDraggedSlider) + ": " + String(currentlyDraggedSliderValue, unit) + suffix + " ";

		if (auto l = getSpecialLookAndFeel<LookAndFeelMethods>())
			l->drawSliderPackTextPopup(g, *this, textToDraw);
	}
}

void SliderPack::setValuesFromLine()
{
	Array<float> newValues;

	newValues.ensureStorageAllocated(getNumSliders());

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

			newValues.add(value);
		}
		else
		{
			newValues.add((float)s->getValue());
		}
	}

	data->setFromFloatArray(newValues, sendNotificationAsync, true);
	
	repaint();

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
	if (!isEnabled() || toggleMaxMode) 
		return;

	if (e.mods.isShiftDown())
	{
		for (int i = 0; i < data->getNumSliders(); i++)
		{
			data->setValue(i, data->getDefaultValue(), sendNotification);
		}
	}
	else
	{
		int x = e.getEventRelativeTo(this).getMouseDownPosition().getX();

		int sliderIndex = (int)((float)x / (float)getWidth() * sliders.size());

		data->setValue(sliderIndex, data->getDefaultValue(), sendNotification);
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

	updateSliderColours();
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
			s->setComponentID(String(i));
			//s->setLookAndFeel(getSpecialLookAndFeel<LookAndFeel>());
			s->setInterceptsMouseClicks(false, false);
			s->addListener(this);
			s->setSliderStyle(Slider::SliderStyle::LinearBarVertical);
			s->setTextBoxStyle(Slider::TextEntryBoxPosition::NoTextBox, true, 0, 0);
			s->setRange(data->getRange().getStart(), data->getRange().getEnd(), data->getStepSize());
		}

		updateSliderColours();
		
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

SliderPack::LookAndFeelMethods::~LookAndFeelMethods()
{}

void SliderPack::sliderAmountChanged(SliderPackData* d)
{
	slidersNeedRebuild = true;
	startTimer(30);
}

void SliderPack::sliderPackChanged(SliderPackData* s, int index)
{
	for (int i = 0; i < sliders.size(); i++)
	{
		auto shouldBe = data->getValue(i);

		if (sliders[i]->getValue() != shouldBe)
			sliders[i]->setValue(shouldBe, dontSendNotification);
	}
}

int SliderPack::getCurrentlyDraggedSliderIndex() const
{ return currentlyDraggedSlider; }

double SliderPack::getCurrentlyDraggedSliderValue() const
{ return currentlyDraggedSliderValue; }

void SliderPack::setComplexDataUIBase(ComplexDataUIBase* newData)
{
	if (auto sp = dynamic_cast<SliderPackData*>(newData))
		setSliderPackData(sp);
}

void SliderPack::mouseMove(const MouseEvent& mouseEvent)
{
	auto thisIndex = getSliderIndexForMouseEvent(mouseEvent);

	showOverlayOnMove = mouseEvent.mods.isShiftDown();

	if(showOverlayOnMove)
	{
		currentlyDraggedSlider = thisIndex;

		if(auto s = sliders[currentlyDraggedSlider])
		{
			currentlyDraggedSliderValue = s->getValue();
		}
		
		repaint();
	}
	
	if(thisIndex != currentlyHoveredSlider)
	{
		if(isPositiveAndBelow(currentlyHoveredSlider, sliders.size()))
            repaintWithTextBox(sliders[currentlyHoveredSlider]->getBoundsInParent());

		currentlyHoveredSlider = thisIndex;

		if(isPositiveAndBelow(currentlyHoveredSlider, sliders.size()))
            repaintWithTextBox(sliders[currentlyHoveredSlider]->getBoundsInParent());
	}
}

void SliderPack::notifyListeners(int index, NotificationType n)
{
	if (getData() != nullptr)
	{
		getData()->sendValueChangeMessage(index, n);
	}
}

const SliderPackData* SliderPack::getData() const
{ return data; }

SliderPackData* SliderPack::getData()
{ return data; }

void SliderPack::setSpecialLookAndFeel(LookAndFeel* l, bool shouldOwn)
{
	EditorBase::setSpecialLookAndFeel(l, shouldOwn);
	sliders.clear();
	rebuildSliders();
}

void SliderPack::setSliderWidths(const Array<var>& newWidths)
{
	sliderWidths = newWidths;
	resized();
}

void SliderPack::addListener(Listener* l)
{
	if (auto d = getData())
		d->addListener(l);
}

void SliderPack::removeListener(Listener* listener)
{
	if (auto d = getData())
		d->removeListener(listener);
}

void SliderPack::setCallbackOnMouseUp(bool shouldFireOnMouseUp)
{
	callbackOnMouseUp = shouldFireOnMouseUp;
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

void SliderPack::LookAndFeelMethods::drawSliderPackRightClickLine(Graphics& g, SliderPack& s, Line<float> lineToDraw)
{
	g.setColour(Colours::white.withAlpha(0.6f));
	g.drawLine(lineToDraw, 1.0f);

	Rectangle<float> startDot(lineToDraw.getStart().getX() - 2.0f, lineToDraw.getStart().getY() - 2.0f, 4.0f, 4.0f);
	Rectangle<float> endDot(lineToDraw.getEnd().getX() - 2.0f, lineToDraw.getEnd().getY() - 2.0f, 4.0f, 4.0f);

	g.drawRoundedRectangle(startDot, 2.0f, 1.0f);
	g.drawRoundedRectangle(endDot, 2.0f, 1.0f);

	g.setColour(Colours::white.withAlpha(0.2f));

	g.fillRoundedRectangle(startDot, 2.0f);
	g.fillRoundedRectangle(endDot, 2.0f);
}

void SliderPack::LookAndFeelMethods::drawSliderPackTextPopup(Graphics& g, SliderPack& s, const String& textToDraw)
{
	g.setColour(Colours::white.withAlpha(0.3f));

	const int w = GLOBAL_MONOSPACE_FONT().getStringWidth(textToDraw);

	Rectangle<int> r(s.getWidth() - w, 0, w, 18);

	g.fillRect(r);

	g.setColour(Colours::white.withAlpha(0.4f));

	g.drawRect(r, 1);

	g.setColour(Colours::black);
	g.setFont(GLOBAL_MONOSPACE_FONT());

	g.drawText(textToDraw, r, Justification::centredRight, true);
}

#if HI_RUN_UNIT_TESTS

struct SliderPackUnitTest: public UnitTest
{
    SliderPackUnitTest():
      UnitTest("Testing Slider Packs")
    {}
    
    void runTest() override
    {
        testSetGet();
        testPreallocatedMode();
        testUpdater();
        testUndo();
    }
    
    void testSetGet()
    {
        beginTest("Test Slider Pack getters / setters");
        
        SliderPackData d;
        
        d.setNumSliders(91);
        d.setValue(31, 0.2f);
        auto enc = d.toBase64();
        
        expectEquals(d.getValue(31), 0.2f, "set value");
        
        d.setValue(31, 0.5f);
        
        expectEquals(d.getValue(31), 0.5f, "set new value");
        
        d.fromBase64(enc);
        
        expectEquals(d.getValue(31), 0.2f, "restore value with Base64");
        
        SliderPackData d1, d2;
        
        Random r;
        
        for(int i = 0; i < d1.getNumSliders(); i++)
            d1.setValue(i, r.nextFloat());
        
        enc = d1.toBase64();
        
        d2.fromBase64(enc);
        
        for(int i = 0; i < d1.getNumSliders(); i++)
        {
            expectEquals(d1.getValue(i), d2.getValue(i), "Base64 not equal at index " + String(i));
        }
        
        SliderPackData d3;
        
        d3.setNumSliders(91);
        
        expectEquals(d3.getNumSliders(), 91, "get/setNumSliders() not working");
        
        d3.setNumSliders(0);
        
        expectEquals(d3.getNumSliders(), 91, "ignore zero numSliders");
        
        d3.setNumSliders(-1);
        
        expectEquals(d3.getNumSliders(), 91, "ignore negative numSliders");
        
        
    }
    
    void testPreallocatedMode()
    {
        beginTest("test SliderPack preallocated mode");
        SliderPackData d;
        
        d.setNumSliders(32);
        
        d.setValue(31, 0.75f);
        d.setNumSliders(16);
        d.setValue(31, 0.34f);
        d.setNumSliders(32);
        
        constexpr float DEFAULT_VALUE = 0.85f;
        
        d.setDefaultValue(DEFAULT_VALUE);
        
        expectEquals(d.getValue(31), 1.0f, "don't retain slider value after resizing when no preallocation is used");
        
        d.setUsePreallocatedLength(32);
        
        d.setValue(31, 0.65f);
        d.setNumSliders(16);
        
        expectEquals(d.getValue(31), DEFAULT_VALUE, "Return zero if index > numSliders");
        
        d.setValue(31, 0.34f);
        d.setNumSliders(32);
        expectEquals(d.getValue(31), 0.65f, "retain slider value after resizing");
        
        d.setNumSliders(16);
        
        d.swapData(new VariantBuffer(14), dontSendNotification);
        d.setNumSliders(32);
        expectEquals(d.getValue(31), 0.65f, "retain slider value after swapping smaller buffer");
        
        d.setNumSliders(12);
        d.swapData(new VariantBuffer(32), dontSendNotification);
        
        expectEquals(d.getValue(31), 0.0f, "overwrite slider value after swapping bigger buffer");
        
        Array<float> externalArray;
        
        for(int i = 0; i < 64; i++)
            externalArray.add(0.9f);
        
        d.setFromFloatArray(externalArray);
        
        expectEquals(d.getValue(31), 0.9f, "overwrite with setFromFloatArray");
        
        expectEquals(d.getValue(35), DEFAULT_VALUE, "do not write after numPreallocated");
        
        d.setValue(90, 0.2f);
        expectEquals(d.getValue(90), DEFAULT_VALUE, "do not write after numPreallocated 2");
        
        d.setNumSliders(92);
        
        expectEquals(d.getNumSliders(), 32, "limit numSliders to preallocated size");
        
        d.setValue(5, 0.2f);
        
        
        d.setUsePreallocatedLength(0);
        
        expectEquals(d.getValue(5), 0.2f, "value is retained after deactivating preallocated size");
        
        d.setNumSliders(91);
        
        expectEquals(d.getNumSliders(), 91, "getNumSliders works after deactivating preallocated length");
        
        d.setValue(50, 0.4f);
        
        expectEquals(d.getValue(50), 0.4f, "value > old preallocated size is retained");
    }
    
    struct TestListener: public ComplexDataUIUpdaterBase::EventListener
    {
        TestListener(SliderPackData& d, UnitTest& t):
          source(d),
          test(t)
        {
            d.getUpdater().addEventListener(this);
        }
        
        ~TestListener()
        {
            source.getUpdater().removeEventListener(this);
        }
        
        void onComplexDataEvent(ComplexDataUIUpdaterBase::EventType t, var data_) override
        {
            if(t == ComplexDataUIUpdaterBase::EventType::DisplayIndex)
                return;
            
            eventType = t;
            data = data_;
        }
        
        void expectEventType(ComplexDataUIUpdaterBase::EventType t, const String& errorMessage)
        {
            test.expectEquals((int)eventType, (int)t, errorMessage);
        }
        
        
        UnitTest& test;
        var data;
        ComplexDataUIUpdaterBase::EventType eventType = ComplexDataUIUpdaterBase::EventType::numEventTypes;
        SliderPackData& source;
    };
    
    void testUpdater()
    {
        beginTest("Test SliderPack Updater");
        
        SliderPackData d;
        
        using EventType = ComplexDataUIUpdaterBase::EventType;
        
        {
            TestListener l(d, *this);
            
            d.setNumSliders(12);
            l.expectEventType(EventType::ContentRedirected, "setNumSliders triggers ContentRedirect");
        }
        
        {
            TestListener l(d, *this);
            
            d.setUsePreallocatedLength(0);
            l.expectEventType(EventType::numEventTypes, "No change in preallocation doesn't trigger ContentRedirected");
            
            d.setUsePreallocatedLength(91);
            l.expectEventType(EventType::ContentRedirected, "Changing preallocated size triggers ContentRedirected");
        }
        
        {
            TestListener l(d, *this);
            
            d.fromBase64(d.toBase64());
            l.expectEventType(EventType::ContentRedirected, "Restoring from Base64 triggers ContentRedirected");
        }
        
        {
            TestListener l(d, *this);
            
            d.swapData(var(new VariantBuffer(32)), sendNotificationSync);
            l.expectEventType(EventType::ContentRedirected, "swapData triggers ContentRedirected");
        }
        
        {
            TestListener l(d, *this);
            d.setValue(1, 0.4f, sendNotificationSync);
            l.expectEventType(EventType::ContentChange, "setValue triggers ContentChange");
        }
        
        {
            Array<float> list = {1.0f, 2.0f, 3.0f, 4.0f};
            
            TestListener l(d, *this);
            d.setFromFloatArray(list, sendNotificationSync);
            
            l.expectEventType(EventType::ContentChange, "setFromFloatArray triggers ContentChange");
        }
        
    }
    
    void testUndo()
    {
        beginTest("testing undo");
        
        SliderPackData d;
        UndoManager m;
        d.setUndoManager(&m);
        
        d.setValue(1, 0.5f, sendNotificationSync, true);
        
        expectEquals(d.getValue(1), 0.5f, "perform works");
        
        TestListener l(d, *this);
        
        m.undo();
        
        expectEquals(d.getValue(1), 1.0f, "undo works");
        
        using EventType = ComplexDataUIUpdaterBase::EventType;
        
        l.expectEventType(EventType::ContentChange, "undo causes content change");
    }
};

static SliderPackUnitTest spTests;


#endif

} // namespace hise
