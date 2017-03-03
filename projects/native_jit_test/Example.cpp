
struct Buffer
{
	int setSize(Buffer& b, int size)
	{
		b.buffer = new VariantBuffer(size);

		return 1;
	}

	float getSample(Buffer& b, int index)
	{
#if !ENABLE_SCRIPTING_SAFECHECKS
		return b.buffer.buffer.getReadPointer()[index];
#else
		return b.buffer.get() != nullptr ? b.getSample(index) : 0.0f;
#endif
	};

	template<typename R> R setSample(Buffer& b, int index, float value)
	{
#if !ENABLE_SCRIPTING_SAFECHECKS
		b.buffer.buffer.getWritePointer()[index] = value;
#else
		if(b.buffer.get() != nullptr) b.setSample(index, value);
#endif
		
		return R();
	}

	VariantBuffer::Ptr buffer;
};

template <typename R, typename A, typename B> R assertEqual(A a, B b)
{
	if(a != static_cast<A>(b))
	{
		throw String("Assertion failure: " + String(a) + ", Expected: " + String(b));

		return static_cast<R>(1);
	}

	return static_cast<R>(0);
};

// A simple Lowpass Filter module =============================================
class LowPassFilter: public NativeJitModule
{
public: // Public variables & methods (accessible via Class.name) =============

	float cutOff;	
	Buffer buffer; 

	int bufferSize;

	void calculateCoefficients() {};
	
	void prepareToPlay(double sampleRate, int samplesPerBlock) override
	{
		int x = buffer.setSize(512);

		initBuffer(x);

		assertEqual(x, a);

		// Setup the processing here...
	};
	
protected: // Internally called functions =====================================
	
	void init() override
	{
		// Initialise your values here...
	};

	int initBuffer()
	{
		buffer.length;
	}

	float processSample(float input) override
	{
		// Define the mono processing function here...

		buffer[i] = input;
		input = buffer[i + delay % buffer.length];
	};
	
	void processFrame(float& left, float& right) override
	{
		// Define the stereo processing function here...
	};

private: // Private variables (inaccessible from Javascript) ==================

	float lastValue = 1.0f;
};
