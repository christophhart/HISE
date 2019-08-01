
#pragma once

namespace hnode {
using namespace juce;

using FloatType = float;

namespace Types
{
enum ID
{
	Void =    0b00000000,
	Event =	  0b00001111,
	Float =   0b00010000,
	Double =  0b00100000,
	FpNumber= 0b00110000,
	Integer = 0b01000000,
	Block =   0b10000000,
	Number =  0b01110000,
	Signal =  0b10110000,
	Dynamic = 0b11111111
};

struct OutOfBoundsException
{
	OutOfBoundsException(int index_, int maxSize_) :
		index(index_),
		maxSize(maxSize_)
	{};

	int index;
	int maxSize;
};

struct BlockMismatchException
{
	BlockMismatchException(int size1_, int size2_) :
		size1(size1_),
		size2(size2_)
	{};

	int size1;
	int size2;
};

#ifndef HNODE_ALLOW_BLOCK_EXCEPTION
#define HNODE_ALLOW_BLOCK_EXCEPTION 1
#endif

#if HNODE_ALLOW_BLOCK_EXCEPTION
#define hnode_buffer_except
#define testBounds(index, maxSize) if(!isPositiveAndBelow(index, maxSize)) { throw OutOfBoundsException(index, maxSize); }
#define testEqualSize(size1, size2) if(size1 != size2) { throw BlockMismatchException(size1, size2); }
#else
#define hnode_buffer_except noexcept
#define testBounds(index, maxSize) jassert(isPositiveAndBelow(index, maxSize));
#define testEqualSize(size1, size2) jassert(size1 == size2);
#endif

template <typename T> struct FloatingTypeBlock
{
	using DataType = T;

	FloatingTypeBlock() :
		data(nullptr),
		size_(0)
	{};

	FloatingTypeBlock(const FloatingTypeBlock& other)
	{
		referTo(other);
	}

	void clear()
	{
		if (size() > 0)
			FloatVectorOperations::clear(data, size());
	}

	FloatingTypeBlock(T* d, int numSamples) :
		data(d),
		size_(numSamples)
	{}

	void referTo(const FloatingTypeBlock& other)
	{
		data = other.data;
		size_ = other.size();
	}

	void referTo(T* existingData, int sizeOfExistingData)
	{
		data = existingData;
		size_ = sizeOfExistingData;
	}

	FloatingTypeBlock& operator*(const FloatingTypeBlock& other) hnode_buffer_except
	{
		testEqualSize(size(), other.size());
		FloatVectorOperations::multiply(data, other.data, size());
		return *this;
	}

	FloatingTypeBlock& operator*(T scalar) hnode_buffer_except
	{
		FloatVectorOperations::multiply(data, scalar, size());
		return *this;
	}

	FloatingTypeBlock& operator+(const FloatingTypeBlock& other) hnode_buffer_except
	{
		testEqualSize(size(), other.size());
		FloatVectorOperations::add(data, other.data, size());
		return *this;
	}

	FloatingTypeBlock& operator+(T scalar)
	{
		FloatVectorOperations::add(data, scalar, size());
		return *this;
	}

	FloatingTypeBlock& operator-(const FloatingTypeBlock& other)
	{
		testEqualSize(size(), other.size());
		FloatVectorOperations::subtract(data, other.data, size());
		return *this;
	}

	FloatingTypeBlock& operator-(T scalar)
	{
		FloatVectorOperations::add(data, T(-1) * scalar, size());
		return *this;
	}

	FloatingTypeBlock(AudioBuffer<T>& b, int channelIndex=0, int startSample=0) :
		data(b.getWritePointer(channelIndex, startSample)),
		size_(b.getNumSamples()-startSample)
	{};

	float* begin() const
	{
		return data;
	}

	float* end() const
	{
		return data + size();
	}

	static FloatingTypeBlock<T> createFromAudioSampleBuffer(AudioSampleBuffer* b)
	{
		IF_CONSTEXPR (sizeof(T) == 4)
		{
			return FloatingTypeBlock(*reinterpret_cast<AudioBuffer<T>*>(b));
		}
		else
		{
			FloatingTypeBlock<T> newBlock(b->getNumSamples());
			
            auto dst = newBlock.data;
            auto src = b->getReadPointer(0);
            auto numSamples = newBlock.size();
            
            for (int i = 0; i < numSamples; i++)
            {
                dst[i] = (double)src[i];
            }
            
			return newBlock;
		}
	}

	T& operator[](int index) hnode_buffer_except
	{
		testBounds(index, size());
		return data[index];
	}

	const T& operator[](int index) const hnode_buffer_except
	{
		testBounds(index, size());
		return data[index];
	}

	FloatingTypeBlock<FloatType> getSubBlock(int offset, int subSize) hnode_buffer_except
	{
		testBounds(offset + subSize, size() + 1);

		return { data + offset, subSize };
	}

	int size() const noexcept { return size_; } hnode_buffer_except

	T* getData() hnode_buffer_except {  return data;  }

	const T* getData() const hnode_buffer_except { return data; }

	template <class OtherData> void copyFrom(const OtherData& other, int offsetDst = 0, int offsetSrc = 0, int numToCopy = -1)
	{
		if (numToCopy == -1)
			numToCopy = size();

		testBounds(offsetSrc + numToCopy, other.size()+1);
		testBounds(offsetDst + numToCopy, size()+1);

		IF_CONSTEXPR (sizeof(DataType) == sizeof(OtherData::DataType))
		{
			FloatVectorOperations::copy(data + offsetDst, other.getData() + offsetSrc, numToCopy);
		}
		else
		{
			auto rPtr = other.getData() + offsetSrc;
			auto wPtr = data + offsetDst;

			for (int i = 0; i < numToCopy; i++)
				wPtr[i] = static_cast<T>(rPtr[i]);
		}
	}
	
private:

	int type = (int)Types::ID::Block;
	int size_ = 0;
	T * data = nullptr;
};

using FloatBlock = FloatingTypeBlock<FloatType>;

/** A object containing a variable name and metadata about its type. */
struct Info
{
	Identifier id;		// the assigned variable / pin name
	Types::ID type;		// the type
	int numValues = -1; // if it's a block, the block size
	bool isParameter = true;
};

struct FunctionType
{
	ID returnType;
	Identifier functionName;
	Array<ID> parameters;
	
};

}

using block = Types::FloatBlock;
using event = hise::HiseEvent;

struct hmath
{
	static constexpr double pi = 3.141592653589793238;
	static constexpr double e = 2.71828182845904523536;

static forcedinline double sign(double value) { return value > 0.0 ? 1.0 : -1.0; };
static forcedinline double abs(double value) { return value * sign(value); };
static forcedinline double round(double value) { return roundf((float)value); };
static forcedinline double range(double value, double lower, double upper) { return jlimit<double>(lower, upper, value); };
static forcedinline double min(double value1, double value2) { return jmin<double>(value1, value2); };
static forcedinline double max(double value1, double value2) { return jmax<double>(value1, value2); };
static forcedinline double randomDouble() { return Random::getSystemRandom().nextDouble(); };

static forcedinline float sign(float value) { return value > 0.0f ? 1.0f : -1.0f; };
static forcedinline float abs(float value) { return value * sign(value); };
static forcedinline float round(float value) { return roundf((float)value); };
static forcedinline float range(float value, float lower, float upper) { return jlimit<float>(lower, upper, value); };
static forcedinline float min(float value1, float value2) { return jmin<float>(value1, value2); };
static forcedinline float max(float value1, float value2) { return jmax<float>(value1, value2); };
static forcedinline float random() { return Random::getSystemRandom().nextFloat(); };

static int round(int value) { return value; };
static forcedinline int randInt(int low=0, int high=INT_MAX) { return  Random::getSystemRandom().nextInt(Range<int>((int)low, (int)high)); }


static forcedinline double sin(double a) { return std::sin(a); }
static forcedinline double asin(double a) { return std::asin(a); }
static forcedinline double cos(double a) { return std::cos(a); }
static forcedinline double acos(double a) { return std::acos(a); }
static forcedinline double sinh(double a) { return std::sinh(a); }
static forcedinline double cosh(double a) { return std::cosh(a); }
static forcedinline double tan(double a) { return std::tan(a); }
static forcedinline double tanh(double a) { return std::tanh(a); }
static forcedinline double atan(double a) { return std::atan(a); }
static forcedinline double atanh(double a) { return std::atanh(a); }
static forcedinline double log(double a) { return std::log(a); }
static forcedinline double log10(double a) { return std::log10(a); }
static forcedinline double exp(double a) { return std::exp(a); }
static forcedinline double pow(double base, double exp) { return std::pow(base, exp); }
static forcedinline double sqr(double a) { return square(a); }
static forcedinline double sqrt(double a) { return std::sqrt(a); }
static forcedinline double ceil(double a) { return std::ceil(a); }
static forcedinline double floor(double a) { return std::floor(a); }

static forcedinline float sin(float a) { return std::sinf(a); }
static forcedinline float asin(float a) { return std::asinf(a); }
static forcedinline float cos(float a) { return std::cosf(a); }
static forcedinline float acos(float a) { return std::acosf(a); }
static forcedinline float sinh(float a) { return std::sinhf(a); }
static forcedinline float cosh(float a) { return std::coshf(a); }
static forcedinline float tan(float a) { return std::tanf(a); }
static forcedinline float tanh(float a) { return std::tanhf(a); }
static forcedinline float atan(float a) { return std::atanf(a); }
static forcedinline float atanh(float a) { return std::atanhf(a); }
static forcedinline float log(float a) { return std::logf(a); }
static forcedinline float log10(float a) { return std::log10f(a); }
static forcedinline float exp(float a) { return std::expf(a); }
static forcedinline float pow(float base, float exp) { return std::powf(base, exp); }
static forcedinline float sqr(float a) { return square(a); }
static forcedinline float sqrt(float a) { return std::sqrtf(a); }
static forcedinline float ceil(float a) { return std::ceilf(a); }
static forcedinline float floor(float a) { return std::floorf(a); }

struct wrapped
{
	struct sin { static constexpr char name[] = "sin";
				 static forcedinline float op(float a) { return hmath::sin(a); }
				 static forcedinline double op(double a) { return hmath::sin(a); }
	};

	struct tanh { static constexpr char name[] = "tanh";
				  static forcedinline float op(float a) { return hmath::tanh(a); }
				  static forcedinline double op(double a) { return hmath::tanh(a); }
	};
};


static void throwIfSizeMismatch(const block& b1, const block& b2)
{
	if (b1.size() != b2.size())
		throw String("Size mismatch");
}

#define tBlock Types::FloatingTypeBlock<T>
#define vOpBinary(name, vectorOp) template <typename T> static forcedinline tBlock& name(tBlock& b1, const tBlock& b2) { \
	throwIfSizeMismatch(b1, b2); \
	vectorOp(b1.data, b2.data.b1.size()); \
	return b1; \
};

#define vOpScalar(name, vectorOp) template <typename T> static forcedinline tBlock& name(tBlock& b1, T s) { \
	vectorOp(b1.data, s, b1.size()); \
	return b1; \
};

vOpBinary(vmul, FloatVectorOperations::multiply);
vOpBinary(vadd, FloatVectorOperations::add);
vOpBinary(vsub, FloatVectorOperations::subtract);
vOpBinary(vcopy, FloatVectorOperations::copy);

vOpScalar(vset, FloatVectorOperations::fill);
vOpScalar(vmuls, FloatVectorOperations::multiply);
vOpScalar(vadds, FloatVectorOperations::add);

#undef tBlock
#undef vOpBinary
#undef vOpScalar

};


}
