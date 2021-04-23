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

struct SimpleRingBuffer: public ComplexDataUIBase,
						 public ComplexDataUIUpdaterBase::EventListener
{
	// Use this function to validate the processing specs within the given
	// context.
	using ValidateFunction = std::function<bool(int&)>;

	/** Use this function as ValidateFunction template. */
	template <int LowerLimit, int UpperLimit> static bool withinRange(int& r)
	{
		if (r >= LowerLimit && r <= UpperLimit)
			return false;

		r = jlimit(LowerLimit, UpperLimit, r);
		return true;
	}
	
	static constexpr int RingBufferSize = 65536;

	using Ptr = ReferenceCountedObjectPtr<SimpleRingBuffer>;

	using TransformFunction = std::function<void(SimpleRingBuffer::Ptr p)>;

	SimpleRingBuffer();

	bool fromBase64String(const String& b64) override
	{
		return true;
	}

	void setRingBufferSize(int numChannels, int numSamples, bool acquireLock=true)
	{
		validateLengthFunction(numSamples);
		validateChannelFunction(numChannels);

		if (numChannels != internalBuffer.getNumChannels() ||
			numSamples != internalBuffer.getNumSamples())
		{
			jassert(!isBeingWritten);

			SimpleReadWriteLock::ScopedWriteLock sl(getDataLock(), acquireLock);
			internalBuffer.setSize(numChannels, numSamples);
			internalBuffer.clear();
			numAvailable = 0;
			writeIndex = 0;
			updateCounter = 0;

			getUpdater().sendContentRedirectMessage();
		}
	}

	void setupReadBuffer(AudioSampleBuffer& b)
	{
		// must be called during a write lock
		jassert(getDataLock().writeAccessIsLocked());
		b.setSize(internalBuffer.getNumChannels(), internalBuffer.getNumSamples());
		b.clear();
	}

	void setValidateFunctions(const ValidateFunction& cf, const ValidateFunction& lf);

	String toBase64String() const override { return {}; }

	void clear();
	int read(AudioSampleBuffer& b);
	void write(double value, int numSamples);

	void write(const float** data, int numChannels, int numSamples);

	void write(const AudioSampleBuffer& b, int startSample, int numSamples);

	void onComplexDataEvent(ComplexDataUIUpdaterBase::EventType t, var n) override;

	void setActive(bool shouldBeActive)
	{
		active = shouldBeActive;
	}

	bool isActive() const noexcept
	{
		return active;
	}

	const AudioSampleBuffer& getReadBuffer() const { return externalBuffer; }

	AudioSampleBuffer& getWriteBuffer() { return internalBuffer; }

	void setSamplerate(double newSampleRate)
	{
		sr = newSampleRate;
	}

	double getSamplerate() const { return sr; }

	void setTransformFunction(const TransformFunction& tf)
	{
		transformFunction = tf;
		getUpdater().sendDisplayChangeMessage(0.0f, sendNotificationAsync, true);
	}

	virtual void setProperty(const Identifier& id, const var& newValue);
	virtual var getProperty(const Identifier& id) const;
	virtual Array<Identifier> getIdentifiers() const;

private:

	static bool dontChange(int&) { return false; }

	TransformFunction transformFunction;

	ValidateFunction validateChannelFunction = dontChange;
	ValidateFunction validateLengthFunction = dontChange;


	double sr = -1.0;

	bool active = true;

	AudioSampleBuffer externalBuffer;
	
	std::atomic<bool> isBeingWritten = { false };
	std::atomic<int> numAvailable = { 0 };
	std::atomic<int> writeIndex = { 0 };
	
	int readIndex = 0;

	AudioSampleBuffer internalBuffer;
	
	int updateCounter = 0;
};


struct RingBufferComponentBase : public ComplexDataUIBase::EditorBase,
								 public ComplexDataUIUpdaterBase::EventListener
{
	enum ColourId
	{
		bgColour = 12,
		fillColour,
		lineColour,
		numColourIds
	};

	void onComplexDataEvent(ComplexDataUIUpdaterBase::EventType e, var newValue) override;
	void setComplexDataUIBase(ComplexDataUIBase* newData) override;

	struct LookAndFeelMethods
	{
		virtual void drawOscilloscopeBackground(Graphics& g, RingBufferComponentBase& ac);
		virtual void drawOscilloscopePath(Graphics& g, RingBufferComponentBase& ac, const Path& p);
		virtual void drawGonioMeterDots(Graphics& g, RingBufferComponentBase& ac, const RectangleList<float>& dots, int index);
		virtual void drawAnalyserGrid(Graphics& g, RingBufferComponentBase& ac, const Path& p);
	};

	struct DefaultLookAndFeel : public GlobalHiseLookAndFeel,
								public LookAndFeelMethods
	{

	};

	RingBufferComponentBase()
	{
		setSpecialLookAndFeel(new DefaultLookAndFeel());
	}

	virtual void refresh() = 0;

	virtual Colour getColourForAnalyserBase(int colourId) = 0;

protected:

	SimpleRingBuffer::Ptr rb;
};

struct ComponentWithDefinedSize
{
	virtual ~ComponentWithDefinedSize() {}

	/** Override this and return a rectangle for the desired size (it only uses width & height). */
	virtual Rectangle<int> getFixedBounds() const = 0;
};

struct ModPlotter : public Component,
					public RingBufferComponentBase,
					public ComponentWithDefinedSize
{
	enum ColourIds
	{
		backgroundColour,
		pathColour,
		outlineColour,
		numColourIds
	};

	ModPlotter();

	void paint(Graphics& g) override;
	
	Rectangle<int> getFixedBounds() const override { return { 0, 0, 256, 80 }; }

	virtual Colour getColourForAnalyserBase(int colourId) { return Colours::transparentBlack; }

	int getSamplesPerPixel(float rectangleWidth) const;
	
	void refresh() override;

	Path p;

	RectangleList<float> rectangles;
};


} // namespace hise


