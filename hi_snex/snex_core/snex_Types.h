
#pragma once

namespace snex {
using namespace juce;

using FloatType = float;


struct CallbackTypes
{
	static constexpr int Channel = 0;
	static constexpr int Frame = 1;
	static constexpr int Sample = 2;
	static constexpr int NumCallbackTypes = 3;
	static constexpr int Inactive = -1;
};

namespace Types
{
enum ID
{
	Void =			0b00000000,
	Pointer = 0b10001111,
	Float =			0b00010000,
	Double =		0b00100000,
	Integer =		0b01000000,
	Block =			0b10000000,
	Dynamic =		0b11111111
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
#define HNODE_ALLOW_BLOCK_EXCEPTION 0
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
		size_(numSamples),
        data(d)
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

	int size() const noexcept { return size_; } 

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


struct Throw
{
	struct Exception
	{
		int index;
		int maxSize;
	};

	template <typename T, int MaxLimit> static T& get(T* data, int index)
	{
		if (isPositiveAndBelow(index, MaxLimit))
			return data[index];
		else
			throw Exception({ index, MaxLimit });

		static T empty = T();
		return empty;
	}

	template <typename T, int MaxLimit> static const T& get(const T* data, int index)
	{
		return index % MaxLimit;
	}

	template <typename T> static T& get(T* data, int index, int maxLimit)
	{
		return index % maxLimit;
	}

	template <typename T> static const T& get(const T* data, int index, int maxLimit)
	{
		return index % maxLimit;
	}
};

struct Wrap
{
	template <typename T, int MaxLimit> static T& get(T* data, int index)
	{
		return index % MaxLimit;
	}

	template <typename T, int MaxLimit> static const T& get(const T* data, int index)
	{
		return index % MaxLimit;
	}

	template <typename T> static T& get(T* data, int index, int maxLimit)
	{
		return index % maxLimit;
	}

	template <typename T> static const T& get(const T* data, int index, int maxLimit)
	{
		return index % maxLimit;
	}
};

struct Zero
{
	template <typename T, int MaxLimit> static T& get(T* data, int index)
	{
		if (!isPositiveAndBelow(index, MaxLimit))
		{
			static T empty = T(0);
			return empty;
		}

		return data[index];
	}

	template <typename T, int MaxLimit> static const T& get(const T* data, int index)
	{
		if (!isPositiveAndBelow(index, MaxLimit))
		{
			static T empty = T(0);
			return empty;
		}

		return data[index];
	}
};


template <int S> struct wrap
{
	wrap<S>& operator= (int v)
	{
		value = v % S;
		return *this;
	}

	wrap<S>& operator++()
	{
		int rt = value;

		if (value++ >= S)
			value = 0;

		return rt;
	}

	int value = 0;
};


template <class T, int Size> struct ref_span
{
	template <class Other> ref_span(Other& o) :
		data(o.begin())
	{
		auto otherSize = o.end() - o.begin();
		jassert(otherSize == Size);
	}

	T& operator[](int index)
	{
		return *data + index;
	}

	const T& operator[](int index) const
	{
		return *data + index;
	}

	T* begin() const
	{
		return const_cast<T*>(data);
	}

#if 0
	ref_span<float4, Size / 4>& toSimd() const
	{
		jassert(float4::isAlignedTo16Byte(data));
		static_assert(Size % 4 == 0, "Not Simdable");
		static_assert(std::is_same<T, float>(), "Not float");

		return *reinterpret_cast<ref_span<float4, Size / 4>*>(this);
	}
#endif

	T* end() const
	{
		return const_cast<T*>(data) + Size;
	}

private:

	const T* data;
};

template <class T, int MaxSize> struct span
{
	using DataType = T;
	using Type = span<T, MaxSize>;

	static constexpr int s = MaxSize;

	span()
	{
		memset(data, 0, sizeof(T)*MaxSize);
	}

	span(const std::initializer_list<T>& l)
	{
		if (l.size() == 1)
		{
			for (int i = 0; i < MaxSize; i++)
			{
				data[i] = *l.begin();
			}
		}
		else
			memcpy(data, l.begin(), sizeof(T)*MaxSize);
		
	}

	static constexpr size_t getSimdSize()
	{
		static_assert(isSimdable(), "not SIMDable");
		
		if (std::is_same<T, float>())
			return 4;
		else
			return 2;
	}

	Type& operator=(const T& t)
	{
		static_assert(isSimdable(), "Can't add non SIMDable types");

		constexpr int numLoop = MaxSize / getSimdSize();

		if (std::is_same<T, float>())
		{
			auto dst = (float*)data;

            auto v_ = (float)t;
            
			for (int i = 0; i < numLoop; i++)
			{
				auto v = _mm_load_ps1(&v_);
				_mm_store_ps(dst, v);

				dst += getSimdSize();
			}
		}

		return *this;
	}

	operator DataType()
	{
		if (MaxSize == 1)
			return *begin();
	}

	Type& operator+=(const T& scalar)
	{
		*this + scalar;
		return *this;
	}

	Type& operator+(const T& scalar)
	{
		static_assert(isSimdable(), "Can't add non SIMDable types");

		constexpr int numLoop = MaxSize / getSimdSize();
		auto dst = (float*)data;
        auto v_ = (float)scalar;
        
		auto v = _mm_load_ps1(&v_);

		for (int i = 0; i < numLoop; i++)
		{
			auto r = _mm_add_ps(_mm_load_ps(dst), v);
			_mm_store_ps(dst, r);

			dst += getSimdSize();
		}

		return *this;
	}

	Type& operator=(const Type& other)
	{
		static_assert(isSimdable(), "Can't add non SIMDable types");

		constexpr int numLoop = MaxSize / getSimdSize();
		auto src = (float*)other.data;
		auto dst = (float*)data;

		for (int i = 0; i < numLoop; i++)
		{
			auto v = _mm_load_ps((float*)src);
			_mm_store_ps(dst, v);

			src += getSimdSize();
			dst += getSimdSize();
		}

		return *this;
	}

	Type& operator+ (const Type& other)
	{
		auto dst = (float*)data;
		auto src = (float*)other.data;
		constexpr int numLoop = MaxSize / getSimdSize();

		for (int i = 0; i < numLoop; i++)
		{
			auto v = _mm_load_ps(dst);
			auto r = _mm_add_ps(v, _mm_load_ps(src));
			_mm_store_ps(dst, r);

			dst += getSimdSize();
			src += getSimdSize();
		}

		return *this;
	}

	Type& operator +=(const Type& other)
	{
		static_assert(isSimdable(), "Can't add non SIMDable types");
		
		constexpr int numLoop = MaxSize / getSimdSize();

		auto dst = (float*)data;
		auto src = (float*)other.data;

		for (int i = 0; i < numLoop; i++)
		{
			auto v = _mm_load_ps(dst);
			auto r = _mm_add_ps(v, _mm_load_ps(src));
			_mm_store_ps(dst, r);

			dst += getSimdSize();
			src += getSimdSize();
		}
		
		return *this;
	}

	T accumulate() const
	{
		T v = T(0);

		for (int i = 0; i < MaxSize; i++)
		{
			v += data[i];
		}

		return v;
	}

	static constexpr bool isSimdType()
	{
		return (std::is_same<T, float>() && MaxSize == 4) ||
			(std::is_same<T, double>() && MaxSize == 2);
	}

	static constexpr bool isSimdable()
	{
		return (std::is_same<T, float>() && MaxSize % 4 == 0) ||
			(std::is_same<T, double>() && MaxSize % 2 == 0);
	}

	bool isAlignedTo16Byte() const
	{
		return isAlignedTo16Byte(*this);
	}

	template <int SliceLength> ref_span<T, SliceLength> slice()
	{
		return ref_span<T, SliceLength>(*this);
	}



	template <class Other> static bool isAlignedTo16Byte(Other& d)
	{
		return reinterpret_cast<uint64_t>(d.begin()) % 16 == 0;
	}

	

	ref_span<T, MaxSize> getRef() const
	{
		return ref_span<T, MaxSize>(*this);
	}

	template <int ChannelAmount> span<span<T, MaxSize/ChannelAmount>, ChannelAmount>& split()
	{
		static_assert(MaxSize % ChannelAmount == 0, "Can't split with slice length ");

		return *reinterpret_cast<span<span<T, MaxSize / ChannelAmount>, ChannelAmount>*>(this);
	}

	const T& operator[](int index) const
	{
		return data[index];
	}

	T& operator[](int index)
	{
		return data[index];
	}

	T* begin() const
	{
		return const_cast<T*>(data);
	}

	T* end() const
	{
		return const_cast<T*>(data + MaxSize);
	}

	void fill(const T& value)
	{
		for (auto& v : *this)
			v = value;
	}

	constexpr int size()
	{
		return MaxSize;
	}

	static constexpr int alignment()
	{
		if (MaxSize * sizeof(T) > 16)
			return 16;
		else
			return MaxSize * sizeof(T);
	}

	alignas(alignment()) T data[MaxSize];
};

using float4 = span<float, 4>;

template <class T> struct dyn
{
	using DataType = T;
	
	dyn() :
		data(nullptr),
		size(0)
	{}


	template<class Other> dyn(Other& o) :
		data(o.begin()),
		size(o.end() - o.begin())
	{}

	template<class Other> dyn(Other& o, size_t size_) :
		data(o.begin()),
		size(size_)
	{}

	template<class Other> dyn(Other* data_, size_t size_) :
		data(data_),
		size(size_)
	{}

	template<class Other> dyn(Other& o, size_t size_, size_t offset) :
		data(o.begin() + offset),
		size(size_)
	{
		auto fullSize = o.end() - o.begin();
		jassert(offset + size < fullSize);
	}

	template <int NumChannels> span<dyn<T>, NumChannels> split()
	{
		span<dyn<T>, NumChannels> r;

		int newSize = size / NumChannels;

		T* d = data;

		for (auto& e : r)
		{
			e = dyn<T>(d, newSize);
			d += newSize;
		}

		return r;
	}

	const T& operator[](int index) const
	{
		return data[index];
	}

	T& operator[](int index)
	{
		return data[index];
	}

	T* begin() const
	{
		return const_cast<T*>(data);
	}

	T* end() const
	{
		return const_cast<T*>(data) + size;
	}

	T* data;
	size_t size = 0;
};

namespace Interleaver
{

	template <class T> static bool isContinousMemory(const T& t)
	{
		auto ptr = t.begin();
		auto e = t.end();

		auto elementSize = sizeof(T::DataType);

		auto size = (e - ptr) * elementSize;

		auto realSize = reinterpret_cast<uint64_t>(e) - reinterpret_cast<uint64_t>(ptr);

		return realSize == size;
	}

	template <int C> static auto& simd(span<float, C>& t)
	{
		static_assert(t.isSimdable(), "is not SIMDable");
		jassert(t.isAlignedTo16Byte());

		static_assert(C % 4 == 0, "Can't split with slice length ");

		return *reinterpret_cast<span<float4, C / 4>*>(&t);
	}
};




}



using block = Types::FloatBlock;
using event = hise::HiseEvent;

#if JUCE_LINUX
#define std_
#else
#define std_ std
#endif

struct hmath
{

constexpr static double PI = 3.1415926535897932384626433832795;
constexpr static double E = 2.7182818284590452353602874713527;
constexpr static double SQRT2 = 1.4142135623730950488016887242097;
constexpr static double FORTYTWO = 42.0; // just for unit test purposes, the other ones choke because of 
                                   // String conversion imprecisions...

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
static forcedinline float fmod(float x, float y) { return std::fmod(x, y); };

template <class SpanT> static auto sumSpan(const SpanT& t) { return t.accumulate(); };



static forcedinline int min(int value1, int value2) { return jmin<int>(value1, value2); };
static forcedinline int max(int value1, int value2) { return jmax<int>(value1, value2); };
static int round(int value) { return value; };
static forcedinline int randInt(int low=0, int high=INT_MAX) { return  Random::getSystemRandom().nextInt(Range<int>((int)low, (int)high)); }

static forcedinline double map(double normalisedInput, double start, double end) { return jmap<double>(normalisedInput, start, end); }
static forcedinline double sin(double a) { return std_::sin(a); }
static forcedinline double asin(double a) { return std_::asin(a); }
static forcedinline double cos(double a) { return std_::cos(a); }
static forcedinline double acos(double a) { return std_::acos(a); }
static forcedinline double sinh(double a) { return std_::sinh(a); }
static forcedinline double cosh(double a) { return std_::cosh(a); }
static forcedinline double tan(double a) { return std_::tan(a); }
static forcedinline double tanh(double a) { return std_::tanh(a); }
static forcedinline double atan(double a) { return std_::atan(a); }
static forcedinline double atanh(double a) { return std_::atanh(a); }
static forcedinline double log(double a) { return std_::log(a); }
static forcedinline double log10(double a) { return std_::log10(a); }
static forcedinline double exp(double a) { return std_::exp(a); }
static forcedinline double pow(double base, double exp) { return std_::pow(base, exp); }
static forcedinline double sqr(double a) { return a * a; }
static forcedinline double sqrt(double a) { return std_::sqrt(a); }
static forcedinline double ceil(double a) { return std_::ceil(a); }
static forcedinline double floor(double a) { return std_::floor(a); }
static forcedinline double db2gain(double a) { return Decibels::decibelsToGain(a); }
static forcedinline double gain2db(double a) { return Decibels::gainToDecibels(a); }
static forcedinline double fmod(double x, double y) 
{ 
	return std::fmod(x, y); 
};

static forcedinline float map(float normalisedInput, float start, float end) { return jmap<float>(normalisedInput, start, end); }
static forcedinline float sin(float a) { return std_::sinf(a); }
static forcedinline float asin(float a) { return std_::asinf(a); }
static forcedinline float cos(float a) { return std_::cosf(a); }
static forcedinline float acos(float a) { return std_::acosf(a); }
static forcedinline float sinh(float a) { return std_::sinhf(a); }
static forcedinline float cosh(float a) { return std_::coshf(a); }
static forcedinline float tan(float a) { return std_::tanf(a); }
static forcedinline float tanh(float a) { return std_::tanhf(a); }
static forcedinline float atan(float a) { return std_::atanf(a); }
static forcedinline float atanh(float a) { return std_::atanhf(a); }
static forcedinline float log(float a) { return std_::logf(a); }
static forcedinline float log10(float a) { return std_::log10f(a); }
static forcedinline float exp(float a) { return std_::expf(a); }
static forcedinline float pow(float base, float exp) { return std_::powf(base, exp); }
static forcedinline float sqr(float a) { return a * a; }
static forcedinline float sqrt(float a) { return std_::sqrtf(a); }
static forcedinline float ceil(float a) { return std_::ceil(a); }
static forcedinline float floor(float a) { return std_::floor(a); }
static forcedinline float db2gain(float a) { return Decibels::decibelsToGain(a); }
static forcedinline float gain2db(float a) { return Decibels::gainToDecibels(a); }

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
