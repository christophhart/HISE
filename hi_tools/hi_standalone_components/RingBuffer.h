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
		virtual ~WriterBase();;

	private:

		JUCE_DECLARE_WEAK_REFERENCEABLE(WriterBase);
	};

	struct PropertyObject: public ReferenceCountedObject
	{
		using Ptr = ReferenceCountedObjectPtr<PropertyObject>;

		struct ScopedPathProfiler
		{
			ScopedPathProfiler(const PropertyObject& po);

			~ScopedPathProfiler();

			const PropertyObject& obj;
			bool running = false;
		};

		PropertyObject(WriterBase* b);;

		virtual int getClassIndex() const;

		virtual ~PropertyObject();;

		virtual RingBufferComponentBase* createComponent();

		/** Override this method and "sanitize the int number (eg. power of two for FFT). 
			
			Return true if you changed the number. 
		*/
		virtual bool validateInt(const Identifier& id, int& v) const;

		virtual bool allowModDragger() const;;

		virtual void initialiseRingBuffer(SimpleRingBuffer* b);

		virtual void onGlobalUpdaterInit(SimpleRingBuffer* b, PooledUIUpdater* updater);;

		virtual var getProperty(const Identifier& id) const;

		virtual void setProperty(const Identifier& id, const var& newValue);
		
		Array<std::pair<String, var>> properties;  

		virtual void transformReadBuffer(AudioSampleBuffer& b);

		virtual Path createPath(Range<int> sampleRange, Range<float> valueRange, Rectangle<float> targetBounds, double startValue) const;

		Array<Identifier> getPropertyList() const;

		template <typename T> T* getTypedBase() { return dynamic_cast<T*>(writerBase.get()); }

		void setPropertyInternal(const String& c, var nv);

		var getPropertyInternal(const String& c, var defaultValue=var()) const;

		void openTrackEvent();

		/** Override this for display buffers that might contain a state (eg. flex ahdsr display) */
		virtual String exportAsBase64() const { return {}; }

		/** Override this for display buffers that might contain a state (eg. flex ahdsr display) */
		virtual void restoreFromBase64(const String& b64) { };

	protected:

		WeakReference<WriterBase> writerBase;
		WeakReference<SimpleRingBuffer> buffer;

		mutable hise::DebugSession::ProfileDataSource::Ptr pathSource;
		int currentTrackEvent = 0;
	};

	using Ptr = ReferenceCountedObjectPtr<SimpleRingBuffer>;

	SimpleRingBuffer();

	bool fromBase64String(const String& b64) override;

	void setGlobalUIUpdater(PooledUIUpdater* updater) override
	{
		ComplexDataUIBase::setGlobalUIUpdater(updater);

		if(auto obj = getPropertyObject())
		{
			obj->onGlobalUpdaterInit(this, updater);
		}
	}

	void setRingBufferSize(int numChannels, int numSamples, bool acquireLock=true);

	void setupReadBuffer(AudioSampleBuffer& b);

	void setMaxLength(double newMaxLength)
	{
		maxLength = newMaxLength;

		interpolatedWriteIndex = 0.0;
		interpolatedReadIndex = 0.0;
	}

	String toBase64String() const override;

	void clear();
	int read(AudioSampleBuffer& b);
	void write(double value, int numSamples);

	void write(const float** data, int numChannels, int numSamples);

	void write(const AudioSampleBuffer& b, int startSample, int numSamples);

	void onComplexDataEvent(ComplexDataUIUpdaterBase::EventType t, var n) override;

	void setActive(bool shouldBeActive);

	bool isActive() const noexcept;

	var getReadBufferAsVar();

	const AudioSampleBuffer& getReadBuffer() const;

	AudioSampleBuffer& getWriteBuffer();

	void setSamplerate(double newSampleRate);

	double getSamplerate() const;

	void setProperty(const Identifier& id, const var& newValue);
	var getProperty(const Identifier& id) const;
	Array<Identifier> getIdentifiers() const;

	bool isConnectedToWriter(WriterBase* b) const;

	void setPropertyObject(PropertyObject* newObject);

	PropertyObject::Ptr getPropertyObject() const;

	WriterBase* getCurrentWriter() const;

	void setCurrentWriter(WriterBase* newWriter);

	struct ScopedPropertyCreator
	{
		ScopedPropertyCreator(ComplexDataUIBase* obj);

		~ScopedPropertyCreator();

	private:

		SimpleRingBuffer* p;
	};

    CriticalSection& getReadBufferLock();

	int getMaxLengthInSamples() const;

private:

	

    CriticalSection readBufferLock;
    
	static PropertyObject* createPropertyObject(int propertyIndex, WriterBase* b);

	public:

	template <typename T> void registerPropertyObject()
	{
		currentPropertyIndex = T::PropertyIndex;
	}

	private:

	void refreshPropertyObject();

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

	double maxLength = -1.0;
	double interpolatedReadIndex = 0.0;
	double interpolatedWriteIndex = 0.0;

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
		virtual ~LookAndFeelMethods();;
		virtual void drawOscilloscopeBackground(Graphics& g, RingBufferComponentBase& ac, Rectangle<float> areaToFill);
		virtual void drawOscilloscopePath(Graphics& g, RingBufferComponentBase& ac, const Path& p);
		virtual void drawGonioMeterDots(Graphics& g, RingBufferComponentBase& ac, const RectangleList<float>& dots, int index);
		virtual void drawAnalyserGrid(Graphics& g, RingBufferComponentBase& ac, const Path& p);
	};

	struct DefaultLookAndFeel : public GlobalHiseLookAndFeel,
								public LookAndFeelMethods
	{

	};

	RingBufferComponentBase();

	virtual void refresh() = 0;

	virtual Colour getColourForAnalyserBase(int colourId);

	void setUseCustomColours(bool shouldUseCustomColours)
	{
		useCustomColours = shouldUseCustomColours;
	}

protected:

	bool useCustomColours = false;

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

		ModPlotterPropertyObject(SimpleRingBuffer::WriterBase* wb);;
		
		int getClassIndex() const override;

		bool allowModDragger() const override;;

		virtual bool validateInt(const Identifier& id, int& v) const;

		RingBufferComponentBase* createComponent() override;

		void transformReadBuffer(AudioSampleBuffer& b) override;

		std::function<void(float*, int)> transformFunction;
	};

	ModPlotter();

	void paint(Graphics& g) override;
	
	Rectangle<int> getFixedBounds() const override;

	Colour getColourForAnalyserBase(int colourId) override;
	

	int getSamplesPerPixel(float rectangleWidth) const;
	
	void refresh() override;

	void setUseFixRange(Range<float> r)
	{
		startRange = r;
		useFixRange = true;
		hasNegativeValues = r.getStart() < 0.0f;
	}

	bool hasNegativeValues = false;
	Range<float> startRange = { 0.0f, 1.0f };
	bool useFixRange = false;

	Path p;

	RectangleList<float> rectangles;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ModPlotter);
};

struct flex_ahdsr_base: public SimpleRingBuffer::WriterBase
						
{
	enum class SpecialParameters
	{
		Attack,
		Hold,
		Decay,
		Sustain,
		Release,
		Mode,
		AttackLevel,
		AttackCurve,
		DecayCurve,
		ReleaseCurve,
		numParameters
	};

	enum class Mode
	{
		Trigger = 0,
		Note,
		Loop
	};

	enum class ParameterType
	{
		Curve = 0,
		Time,
		Level,
		TimeLevel,
		numTypes
	};

	enum class State
	{
		IDLE = 0,
		ATTACK,
		HOLD,
		DECAY,
		SUSTAIN,
		RELEASE,
		DONE
	};


	static constexpr int NumUIValues = (int)SpecialParameters::numParameters; // position + currentState
	static constexpr int NumStates = (int)State::DONE;
	
	struct DragHandlerBase
	{
		virtual ~DragHandlerBase() {};
		virtual bool handleAdditionalDrag(int parameterIndex, double value) { return false; };
	};

	struct Helpers
	{
		static double getTimeSkew() { return 0.2; }
		static double getReleasePoint() { return 0.66; }

		static int getAttributeIndex(State s, ParameterType t);

		static String getLabel(State s, ParameterType t, float* uiValues);

		static double toNorm(State s, ParameterType t, double value);
		static double fromNorm(State s, ParameterType t, double value);

		static float getYAt(const Path& p, float xPos)
		{
			PathFlatteningIterator i (p, {});

		    while (i.next())
		    {
				Range<float> lr(i.x1, i.x2);

				if(lr.contains(xPos) && !i.closesSubPath) // ignore the closing line...
				{
					float alpha = (xPos - lr.getStart()) / lr.getLength();
					return Interpolator::interpolateLinear(i.y1, i.y2, alpha);
				}
		    }

			return -1.0f;
		}
	};

	

	virtual ~flex_ahdsr_base() {};

	class FlexAhdsrGraph: public RingBufferComponentBase,
						  public Component
	{
	public:

		static constexpr int Margin = 10;

		FlexAhdsrGraph()
		{
			setColour(RingBufferComponentBase::ColourId::bgColour, Colours::black.withAlpha(0.6f));
			setColour(RingBufferComponentBase::ColourId::fillColour, Colours::white.withAlpha(0.2f));
			setColour(RingBufferComponentBase::ColourId::lineColour, Colours::white);
			setColour(HiseColourScheme::ColourIds::ComponentTextColourId, Colours::white);

			setSpecialLookAndFeel(new DefaultLookAndFeel(), true);
			refresh();
		};

		struct LookAndFeelMethods
		{
			virtual ~LookAndFeelMethods() {};

			virtual void drawFlexAhdsrBackground(Graphics& g, FlexAhdsrGraph& graph);
			virtual void drawFlexAhdsrSegment(Graphics& g, FlexAhdsrGraph& graph, State s, const Path& segment, bool hover, bool active);
			virtual void drawFlexAhdsrFullPath(Graphics& g, FlexAhdsrGraph& graph);
			virtual void drawFlexAhdsrDragPoint(Graphics& g, FlexAhdsrGraph& graph, State s, Point<float> dragPoint, bool hover, bool down);
			virtual void drawFlexAhdsrCurvePoint(Graphics& g, FlexAhdsrGraph& graph, State s, Point<float> curvePoint, bool hover, bool down);
			virtual void drawFlexAhdsrPosition(Graphics& g, FlexAhdsrGraph& graph, State s, Point<float> pointOnPath);
			virtual void drawFlexAhdsrText(Graphics& g, FlexAhdsrGraph& graph, const String& text);
		};

		struct DefaultLookAndFeel : public RingBufferComponentBase::LookAndFeelMethods,
			public LookAndFeelMethods,
			public LookAndFeel_V3
		{
			
		};

		void onComplexDataEvent(ComplexDataUIUpdaterBase::EventType t, var data) override
		{
			if(t == ComplexDataUIUpdaterBase::EventType::DisplayIndex)
			{
				auto displayValue = (float)data;

				positionWithinState = std::fmod(displayValue, 1.0f);
				currentPlayState = (State)(int)displayValue;
				SafeAsyncCall::repaint(this);
				return;
			}

			RingBufferComponentBase::onComplexDataEvent(t, data);

			if(t == ComplexDataUIUpdaterBase::EventType::ContentChange ||
			   t == ComplexDataUIUpdaterBase::EventType::ContentRedirected)
			{
				refresh();
			}
		}

		void paint(Graphics& g) override;

		void refresh() override;

		void resized() override;

		void mouseDown(const MouseEvent& e) override;
		void mouseDrag(const MouseEvent& e) override;
		void mouseMove(const MouseEvent& e) override;
		void mouseExit(const MouseEvent& e) override;

		void handleDrag(const MouseEvent& e, ParameterType pt)
		{
			auto isX = pt == ParameterType::Time;
			auto delta = isX ? (0.8f * (float)e.getDistanceFromDragStartX() / (float)getWidth()) : (-1.0f * (float)e.getDistanceFromDragStartY() / (float)getHeight());

			if(currentHoverMode == ParameterType::TimeLevel && isX)
				delta *= 5.0f;

			if(pt == ParameterType::Curve && currentHoverState == State::ATTACK)
				delta *= -1.0f;

			auto newValue = jlimit(0.0f, 1.0f, ((isX || pt == ParameterType::Curve) ? downValue.getX() : downValue.getY()) + delta);
			auto pIndex = Helpers::getAttributeIndex(currentHoverState, pt);

			if(pIndex != -1)
			{
				if(auto b = dynamic_cast<flex_ahdsr_base*>(rb->getCurrentWriter()))
				{
					newValue = Helpers::fromNorm(currentHoverState, pt, newValue);
					b->handleUIDrag(pIndex, newValue);
				}
			}
		}

		bool useOneDimensionalDrag = false;
		float curveTolerance = 20.0f;

		Path fullPath;
		float values[NumUIValues];

		std::vector<Rectangle<float>> boxes;
		std::vector<Rectangle<float>> hitboxes;
		std::vector<Point<float>> curvePoints;
		std::vector<Point<float>> dragPoints;
		std::vector<Path> segments;

		Point<float> sustainPoint;

		State currentHoverState = State::DONE;
		State currentPlayState = State::IDLE;
		ParameterType currentHoverMode = ParameterType::numTypes;
		Point<float> downValue = {};
		float positionWithinState = 0.0f;
	};

	struct Properties : public SimpleRingBuffer::PropertyObject
	{
		static constexpr int PropertyIndex = 2003;

		int getClassIndex() const override { return PropertyIndex; }

		Properties(SimpleRingBuffer::WriterBase* b) :
			PropertyObject(b),
			base(getTypedBase<flex_ahdsr_base>())
		{}

		RingBufferComponentBase* createComponent();

		bool validateInt(const Identifier& id, int& v) const override;

		Path createPath(Range<int> sampleRange, Range<float> valueRange, Rectangle<float> targetBounds, double) const override;

		void transformReadBuffer(AudioSampleBuffer& b) override;

		void restoreFromBase64(const String& b64) override
		{
			if(b64.isNotEmpty() && base != nullptr)
			{
				MemoryBlock mb;
				mb.fromBase64Encoding(b64);

				MemoryInputStream mis(mb, false);

				for(int i = 0; i < NumUIValues; i++)
				{
					if(i == (int)SpecialParameters::Mode)
						continue;

					auto v = mis.readFloat();
					FloatSanitizers::sanitizeFloatNumber(v);
					base->handleUIDrag(i, (double)v);
				}
			}
		}

		String exportAsBase64() const override
		{
			if(base != nullptr)
			{
				float buffer[NumUIValues];
				base->refreshUI(buffer);

				MemoryOutputStream mos;

				for(int i = 0; i < NumUIValues; i++)
				{
					if(i == (int)SpecialParameters::Mode)
						continue;

					mos.writeFloat(buffer[i]);
				}

				mos.flush();
				return mos.getMemoryBlock().toBase64Encoding();
			}

			return {};
		}

		WeakReference<flex_ahdsr_base> base;
	};

	virtual void refreshUI(float* bufferData) = 0;

	virtual void handleUIDrag(int parameterIndex, double attributeValue) = 0;

	JUCE_DECLARE_WEAK_REFERENCEABLE(flex_ahdsr_base);
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

	void resized() override;

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


