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

#define  DECLARE_ID(x) static const Identifier x(#x);

namespace RingBufferIds
{
	DECLARE_ID(BufferLength);
	DECLARE_ID(NumChannels);
	DECLARE_ID(Active);
}

#undef DECLARE_ID

struct RingBufferComponentBase;

struct SimpleRingBuffer: public ComplexDataUIBase,
						 public ComplexDataUIUpdaterBase::EventListener
{
	/** Use this function as ValidateFunction template. */
	template <int LowerLimit, int UpperLimit> static bool withinRange(int& r)
	{
		if (r >= LowerLimit && r <= UpperLimit)
			return true;

		r = jlimit(LowerLimit, UpperLimit, r);
		return false;
	}

	template <int FixSize> static bool toFixSize(int& v)
	{
		auto mustChange = v != FixSize;
		v = FixSize;
		return mustChange;
	}
	
	/** Just a simple interface class for getting the writer. */
	struct WriterBase
	{
		virtual ~WriterBase() {};

	private:

		JUCE_DECLARE_WEAK_REFERENCEABLE(WriterBase);
	};

	struct PropertyObject: public ReferenceCountedObject
	{
		using Ptr = ReferenceCountedObjectPtr<PropertyObject>;

		PropertyObject(WriterBase* b) :
			writerBase(b)
		{
			// This object must not be created from the DLL space...
			JUCE_ASSERT_MESSAGE_MANAGER_EXISTS;
		};

		virtual int getClassIndex() const { return 0; }

		virtual ~PropertyObject() {};

		virtual RingBufferComponentBase* createComponent();

		/** Override this method and "sanitize the int number (eg. power of two for FFT). 
			
			Return true if you changed the number. 
		*/
		virtual bool validateInt(const Identifier& id, int& v) const
		{
			if (id == RingBufferIds::BufferLength)
				return withinRange<512, 65536*2>(v);

			if (id == RingBufferIds::NumChannels)
				return withinRange<1, 2>(v);

			return false;
		}

		virtual bool allowModDragger() const { return false; };

		virtual void initialiseRingBuffer(SimpleRingBuffer* b)
		{
			buffer = b;
		}

		virtual var getProperty(const Identifier& id) const
		{
			jassert(properties.contains(id));

			if (buffer != nullptr)
			{
				if (id.toString() == "BufferLength")
					return var(buffer->internalBuffer.getNumSamples());

				if (id.toString() == "NumChannels")
					return var(buffer->internalBuffer.getNumChannels());
			}

			return {};
		}

		virtual void setProperty(const Identifier& id, const var& newValue)
		{
			properties.set(id, newValue);

			if (buffer != nullptr)
			{
				if ((id.toString() == "BufferLength") && (int)newValue > 0)
					buffer->setRingBufferSize(buffer->internalBuffer.getNumChannels(), (int)newValue);

				if ((id.toString() == "NumChannels") && (int)newValue > 0)
					buffer->setRingBufferSize((int)newValue, buffer->internalBuffer.getNumSamples());
			}
		}

		NamedValueSet properties;

		virtual void transformReadBuffer(AudioSampleBuffer& b)
		{
		}

		virtual Path createPath(Range<int> sampleRange, Range<float> valueRange, Rectangle<float> targetBounds, double startValue) const;

		Array<Identifier> getPropertyList() const
		{
			Array<Identifier> ids;

			for (const auto& nv : properties)
				ids.add(nv.name);

			return ids;
		}

		template <typename T> T* getTypedBase() { return dynamic_cast<T*>(writerBase.get()); }

	protected:

		WeakReference<WriterBase> writerBase;
		WeakReference<SimpleRingBuffer> buffer;
	};

	using Ptr = ReferenceCountedObjectPtr<SimpleRingBuffer>;

	SimpleRingBuffer();

	bool fromBase64String(const String& b64) override
	{
		return true;
	}

	void setRingBufferSize(int numChannels, int numSamples, bool acquireLock=true)
	{
		validateLength(numSamples);
		validateChannels(numChannels);

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

			setupReadBuffer(externalBuffer);

			if (!currentlyChanged)
			{
				ScopedValueSetter<bool> svs(currentlyChanged, true);
				getUpdater().sendContentRedirectMessage();
			}
		}
	}

	void setupReadBuffer(AudioSampleBuffer& b);

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

	var getReadBufferAsVar() { return externalBufferData; }

	const AudioSampleBuffer& getReadBuffer() const { return externalBuffer; }

	AudioSampleBuffer& getWriteBuffer() { return internalBuffer; }

	void setSamplerate(double newSampleRate)
	{
		sr = newSampleRate;
	}

	double getSamplerate() const { return sr; }

	void setProperty(const Identifier& id, const var& newValue);
	var getProperty(const Identifier& id) const;
	Array<Identifier> getIdentifiers() const;

	bool isConnectedToWriter(WriterBase* b) const { return currentWriter == b; }

	void setPropertyObject(PropertyObject* newObject);

	PropertyObject::Ptr getPropertyObject() const { return properties; }

	WriterBase* getCurrentWriter() const { return currentWriter.get(); }

	void setCurrentWriter(WriterBase* newWriter)
	{
		currentWriter = newWriter;
	}

	struct ScopedPropertyCreator
	{
		ScopedPropertyCreator(ComplexDataUIBase* obj):
			p(dynamic_cast<SimpleRingBuffer*>(obj))
		{
			JUCE_ASSERT_MESSAGE_MANAGER_EXISTS;

		}

		~ScopedPropertyCreator()
		{
			JUCE_ASSERT_MESSAGE_MANAGER_EXISTS;
			
			if(p != nullptr)
				p->refreshPropertyObject();
		}

	private:

		SimpleRingBuffer* p;
	};

    CriticalSection& getReadBufferLock() { return readBufferLock; }
    
private:

    CriticalSection readBufferLock;
    
	static PropertyObject* createPropertyObject(int propertyIndex, WriterBase* b);

	public:

	template <typename T> void registerPropertyObject()
	{
		currentPropertyIndex = T::PropertyIndex;
	}

	private:

	void refreshPropertyObject()
	{
		auto pIndex = properties != nullptr ? properties->getClassIndex() : 0;

		if (currentPropertyIndex != pIndex)
		{
			JUCE_ASSERT_MESSAGE_MANAGER_EXISTS;

			auto p = createPropertyObject(currentPropertyIndex, currentWriter.get());

			if (p == nullptr)
				p = new PropertyObject(currentWriter.get());

			setPropertyObject(p);
		}
	}

	int currentPropertyIndex = 0;
	
	bool currentlyChanged = false;

	WeakReference<WriterBase> currentWriter;

	bool validateChannels(int& v);
	bool validateLength(int& v);

	PropertyObject::Ptr properties;

	double sr = -1.0;

	bool active = true;

	AudioSampleBuffer externalBuffer;
	float* externalBufferChannels[NUM_MAX_CHANNELS];
	Array<var> externalBufferData;


	std::atomic<bool> isBeingWritten = { false };
	std::atomic<int> numAvailable = { 0 };
	std::atomic<int> writeIndex = { 0 };
	
	int readIndex = 0;

	AudioSampleBuffer internalBuffer;
	
	int updateCounter = 0;

	JUCE_DECLARE_WEAK_REFERENCEABLE(SimpleRingBuffer);
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
		virtual ~LookAndFeelMethods() {};
		virtual void drawOscilloscopeBackground(Graphics& g, RingBufferComponentBase& ac, Rectangle<float> areaToFill);
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
		setSpecialLookAndFeel(new DefaultLookAndFeel(), true);
	}

	virtual void refresh() = 0;

	virtual Colour getColourForAnalyserBase(int colourId) 
	{ 
		switch (colourId)
		{
		case ColourId::bgColour: return Colours::transparentBlack;
		case ColourId::fillColour: return Colours::white.withAlpha(0.05f);
		case ColourId::lineColour: return Colours::white.withAlpha(0.9f);
		}
		
		return Colours::transparentBlack;
	}

protected:

	SimpleRingBuffer::Ptr rb;

	JUCE_DECLARE_WEAK_REFERENCEABLE(RingBufferComponentBase);
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

	struct ModPlotterPropertyObject : public SimpleRingBuffer::PropertyObject
	{
		static constexpr int PropertyIndex = 1000;

		ModPlotterPropertyObject(SimpleRingBuffer::WriterBase* wb) :
			PropertyObject(wb)
		{
			setProperty(RingBufferIds::BufferLength, 32768 * 2);
			setProperty(RingBufferIds::NumChannels, 1);
		};
		
		int getClassIndex() const override { return PropertyIndex; }

		bool allowModDragger() const override { return true; };

		virtual bool validateInt(const Identifier& id, int& v) const
		{
			if (id == RingBufferIds::BufferLength)
			{
				bool wasPowerOfTwo = isPowerOfTwo(v);

				if (!wasPowerOfTwo)
					v = nextPowerOfTwo(v);
				
				return SimpleRingBuffer::withinRange<4096, 32768 * 4>(v) && wasPowerOfTwo;
			}

			if (id == RingBufferIds::NumChannels)
				return SimpleRingBuffer::toFixSize<1>(v);

			return false;
		}

		RingBufferComponentBase* createComponent() override
		{
			return new ModPlotter();
		}

		void transformReadBuffer(AudioSampleBuffer& b) override
		{
			if (transformFunction)
				transformFunction(b.getWritePointer(0), b.getNumSamples());
		}

		std::function<void(float*, int)> transformFunction;
	};

	ModPlotter();

	void paint(Graphics& g) override;
	
	Rectangle<int> getFixedBounds() const override { return { 0, 0, 256, 80 }; }

	Colour getColourForAnalyserBase(int colourId) override;
	

	int getSamplesPerPixel(float rectangleWidth) const;
	
	void refresh() override;

	Path p;

	RectangleList<float> rectangles;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ModPlotter);
};

class AhdsrGraph : public RingBufferComponentBase,
	public Component
{
public:

	enum class Parameters
	{
		Attack,
		AttackLevel,
		Hold,
		Decay,
		Sustain,
		Release,
		AttackCurve,
		DecayCurve, // not used
		numParameters
	};

	enum class State
	{
		ATTACK, 
		HOLD, 
		DECAY, 
		SUSTAIN, 
		RETRIGGER, 
		RELEASE, 
		IDLE 
	};

	struct LookAndFeelMethods
	{
		virtual ~LookAndFeelMethods() {};

		virtual void drawAhdsrBackground(Graphics& g, AhdsrGraph& graph);
		virtual void drawAhdsrPathSection(Graphics& g, AhdsrGraph& graph, const Path& s, bool isActive);
		virtual void drawAhdsrBallPosition(Graphics& g, AhdsrGraph& graph, Point<float> p);

	};

	struct DefaultLookAndFeel : public RingBufferComponentBase::LookAndFeelMethods,
		public LookAndFeelMethods,
		public LookAndFeel_V3
	{
		
	};

	enum ColourIds
	{
		bgColour,
		fillColour,
		lineColour,
		outlineColour,
		numColourIds
	};

	AhdsrGraph();
	~AhdsrGraph();

	void paint(Graphics &g);
	void setUseFlatDesign(bool shouldUseFlatDesign);

	void onComplexDataEvent(ComplexDataUIUpdaterBase::EventType e, var newValue) override;

	void resized() override
	{
		rebuildGraph();
	}

	void refresh() override;

	void rebuildGraph();

	Path envelopePath;

	int getCurrentStateIndex() const { return (int)ballPos; }

private:

	float ballPos = -1.0f;

	bool flatDesign = false;

	float attack = 0.0f;
	float attackLevel = 0.0f;
	float hold = 0.0f;
	float decay = 0.0f;
	float sustain = 0.f;
	float release = 0.f;
	float attackCurve = 0.f;

	
	Path attackPath;
	Path holdPath;
	Path decayPath;
	Path releasePath;
};

class OscilloscopeBase : public RingBufferComponentBase
{
protected:

	OscilloscopeBase() :
		RingBufferComponentBase()
	{};

	virtual ~OscilloscopeBase() {};

	void drawWaveform(Graphics& g);

	void refresh() override
	{
		dynamic_cast<Component*>(this)->repaint();
	}


private:

	void drawPath(const float* l_, int numSamples, int width, Path& p);

	void drawOscilloscope(Graphics &g, const AudioSampleBuffer &b);

	Path lPath;
	Path rPath;
};

class FFTDisplayBase : public RingBufferComponentBase
{
public:

	enum WindowType
	{
		Rectangle,
		BlackmannHarris,
		Hann,
		Flattop,
		numWindowTypes
	};

	enum Domain
	{
		Phase,
		Amplitude,
		numDomains
	};

	using ConverterFunction = std::function<float(float)>;

	struct Properties
	{
		WindowType window = BlackmannHarris;
		Range<double> dbRange = { -50.0, 0.0 };
		Domain domain = Amplitude;
		ConverterFunction freq2x;
		ConverterFunction gain2y;

		void applyFFT(SimpleRingBuffer::Ptr p);
	};

	Properties fftProperties;

	void refresh() override
	{
		SafeAsyncCall::repaint(dynamic_cast<Component*>(this));
	}

protected:

	FFTDisplayBase()
	{}


    ScopedPointer<juce::dsp::FFT> fftObject;
    
	virtual double getSamplerate() const = 0;

	virtual ~FFTDisplayBase() {};

	void drawSpectrum(Graphics& g);

	Path lPath;
	Path rPath;

	WindowType lastWindowType = numWindowTypes;

	AudioSampleBuffer windowBuffer;
	AudioSampleBuffer fftBuffer;
};


class GoniometerBase : public RingBufferComponentBase
{
public:

	GoniometerBase() = default;

	void refresh() override
	{
		dynamic_cast<Component*>(this)->repaint();
	}

protected:

	void paintSpacialDots(Graphics& g);

private:

	struct Shape
	{
		Shape() {};

		Shape(const AudioSampleBuffer& buffer, Rectangle<int> size);

		RectangleList<float> points;

		static Point<float> createPointFromSample(float left, float right, float size);

		void draw(Graphics& g, Colour c);
	};

	Shape shapes[6];
	int shapeIndex = 0;
};


} // namespace hise


