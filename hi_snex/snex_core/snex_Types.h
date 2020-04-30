
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
	Pointer =		0b10001111,
	Float =			0b00010000,
	Double =		0b00100000,
	Integer =		0b01000000,
	Block =			0b10000000,
	Dynamic =		0b11111111
};

/** This will identify each snex array type by using the constexpr static variable
    T::ArrayType
*/
enum class ArrayID
{
	SpanType,
	DynType,
	HeapType,
	ProcessDataType,
	FrameProcessorType
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





}
