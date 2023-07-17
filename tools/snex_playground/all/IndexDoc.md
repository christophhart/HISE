# Array Accessors in SNEX

One of the most common operations in audio DSP is accessing data from a consecutive block of memory. Usually the data is represented as an array type (so either `span<T, NumElements>` or `dyn<T>`) and you can access and modify the data using
the `[]`-operator:

```cpp
span<float, 128> d1;

d1[12] = 0.5f;

return d1[18];
```

One common problem is that an access (read or write) that goes beyond the boundaries of the array will lead to undefined behaviour (most likely a crash). In the example above, this is not a problem, because the indexes used in the array access
are within the compile-time-known range of the array (0 - 127) and the compiler will not let you do things that would lead to a certain crash (`d1[129]` will not compile). However as soon as either of them are dynamic - an index that is not a constant or an array type with variable length - things get dangerous pretty quickly:

```cpp
span<float, 128> data;

float getValue(int index)
{
	return data[index];
}

// Caller side:

float v1 = getValue(12); // ok
float v1 = getValue(Math.random() * 256); // disaster waiting to happen
```

Now the solution would be to check the range before calling the method:

```cpp
float getValue(int index)
{
	if(index >= 0 && index < 128)
	    return data[index];

	return 0.0f;
}
```

This function is now guaranteed to never crash, however there are a few issues with it:

1. The performance overhead. For each array access there are two comparison operations plus a branch that might be too much overhead in certain situations
2. Bloated code. The vast amount of the function's body is not about the desired functionality, but about the error handling. What might look trivial in this case quickly adds up to a lot of branches and if/else clauses that might distract you from seeing the actual logic of the algorithm
3. Poor error handling. If you access something outside the bounds, it just returns zero. This is most definitely the most sensible default error value, but in some cases you might want to encorporate a better handling of the out-of-bounds case. For example if you create a Waveshaper with a look up table, you might want to return the highest / lowest value if the index is beyond the available range.

In order to address these problems, a dedicated index object was introduced in SNEX:

1. A few compile-time properties will reduce the performance overhead of a exception-safe array access.
2. All the error handling is tucked away into the object and using these index objects looks exactly like plain integer variables.
3. The "error handling" can be specified to address different use-cases

There are also multiple layers of indexes, starting from the low-level integer-based array access all the way up to interpolating algorithms.

## Layer 1: The integer based indexes

The first layer of indexes are the so-called integer-based indexes. As their name suggests, they work just like integer numbers. All integer based index types have two template parameters:

```cpp
index::xxx<int UpperLimit, bool CheckOnAssignment>
```

The `UpperLimit` parameter can be used to define a compile-time constant for the upper boundary (use `0` as `UpperLimit` to deactivate the compile-time upper limit). The performance advantage of an compile-time limit can be very noticeable - especially in conjunction with power-2 values, because it allows some optimizations that would not be possible otherwise.

The `CheckOnAssignment` parameter specifies whether the bounds checking should be performed during assignment of a new value or when the actual index is being read. You can omit this template parameter (in that case it defaults to false). 

```cpp
using BufferSize = 256;

span<float, BufferSize> data1;
span<float, BufferSize> data2;

index::xxx<BufferSize, true> i; // true = check on assign

i = 257; // the bound check will be performed here

data1[i] = 0.5f; // the index is guaranteed to be within the 
	             // bounds, so this operation will just use
                 // the internal integer value

data2[i] = 0.1 // again, no bound check

index::xxx<BufferSize, false> j; // false = Do not check on assign

i = 10204; // the bound check will not be performed here

data1[j] = 0.9f; // but here just before accessing an array
data2[j] = 2.0f; // here it has to check the bounds again
```

As the example shows, the index without `CheckOnAssign` needs to perform twice as much bound checking operations as the other one - however it is easy to come up with a use case that inverts the performance difference, so when performance is really tight, you might want to think about setting this flag to match your use case as good as possible.

> Obviously the `CheckOnAssign` flag is only useful with a defined `UpperLimit` because it can't make assumptions about the upper limit when it's dynamic:

```cpp
index::xxx<0, true> j;

j = 125; // now what? where's the limit?
```

There are three main index types which offer different out-of-bounds behaviour:

1. Clamped. The `index::clamped<...>` type limits the range of possible values from `0` to `UpperLimit` (so that everythin above `UpperLimit - 1` will be `UpperLimit-1` and everything below zero will be zero). This can be used for lookup tables

2. Wrapped. The `index::wrapped<...>` type wraps the index around the boundary using the `%` operator and is the most convenient index type for many DSP applications (delay buffers, loop players, etc).

3. Safe. The `index::safe<>` type does not check the bounds but simply redirects read or write accesses to a container with illegal indexes to a dummy memory address. This is branchless, so you won't get (almost) no performance penalty in the legal branch.

3. Unsafe. If you really, really want to do so, the `index::unsafe<...>` type performs no bounds checking at all. There are a few situations where an unsafe index can be used safely and when the other types are too intrusive.

Now that we've handled the most low-level types, we can start looking at the next layers, which help you write more lean code.

## Layer 2: The floating point based indexes

The next layer extends the integer index types with a floating point index and allows a more direct connection between a input signal (which is always floating point) and an array access operation. 

```cpp
index::xxx<FloatType, IntegerIndexType>
```

The `FloatType` must be either `float` or `double` (depending on the input type) and the `IntegerIndexType` must be a integer index type. The floating point index type will then inherit all properties from the `IntegerIndexType`.

There are two types of floating point integer types:

1. Normalised. The `index::normalised<...>` type can be assigned with a value between 0 and 1 and will upscale it to the array size in the access.
2. Unscaled. The `index::unscaled<...>` type has the same input / output range as its underlying `IntegerIndexType`.

```cpp
// this type will wrap around 128 and check the bounds on access
using MyIndexType = index::wrapped<128, false;

span<128, float> data;

// this type will also wrap around 128 and check the bounds on access
using MyNormalisedFloatType = index::normalised<float, MyIndexType>;

MyNormalisedFloatType i;

i = 0.5f; // this assignment value can be taken directly from the input
          // signal without any visible conversion

float s1 = data[i]; // access the 64th element (128.0f * 0.5f)

i = 0.5001f;

float s2 = data[i]; // still access the 64th element
```

As you can see, the values `s1` and `s2` will have the same value despite the fact that the index is slightly different. This is because there is no interpolation that takes the residue into account, so this is where the next layer comes in handy.

## Layer 3: The interpolating indexes

This layer extends on the floating point index and calculates an interpolated value. It just takes an existing floating point index as template parameter:

```cpp
index::xxx<FloatingIndexType>
```

There are two interpolation types available:

1. Linear interpolation: `interpolators::lerp<...>`
2. Cubic interpolation: `interpolators::hermite<...>`

> If you want to read up on the difference between the interpolation types, there are plenty of resources around.

```cpp
using MyFloatType = index::normalised<float, index::wrapped<512, false>>;
using MyInterpolator = index::lerp<MyFloatType>;

span<float, 512> data;

MyInterpolator i(0.8912f);

auto s = data[i]; // calculates the interpolated value
```

Be aware that unlike the other index types, the interpolator indexes calculate a new value and thus do not return a reference to the memory, so you can't use them for assigning a value. However this should not be a real issue in real world use cases, since you most probably only want a read-only access when you're interpolating.

```cpp
MyInterpolator i(90.0f);

data[i] = 90.0f; // will not compile
```

Another neat feature of the interpolating indexes is the ability to operate on (interleaved) multi-channel data with almost the same syntax:

```cpp
using DolbyFrame = span<float, 6>;

// these are dynamic arrays of surround channel frames
dyn<StereoFrame> input;
dyn<StereoFrame> output;

index::hermite<index::normalised<index::safe<0>> interpolator;

for(auto& f: input)
{
	// This looks like an unsuspicious array access, but
	// what it actually does is a cubic interpolation on
	// 6 channels of audio!
	output = input[interpolator];

	interpolator += uptimeDelta;
}
```


### Bonus Level: Iteration

Iterating over an array is a pretty basic operation and whenever you want to iterate over a single array,
the range-based for loop is always the best choice:

```cpp
dyn<float> b1;

for(auto& s: b1)
    s = 90.0f;
```

However there are a few use cases where you want to access multiple arrays inside the same loop, eg. for copying
an array to another. The most simple solution for this is a plain ol' while loop with a plain ol' integer variable
as index:

```cpp
span<float, 19> src = { 1.0f };
span<float, 43> dst = { 0.0f };

int i = 0;

while(i < src.size())
{
	dst[i] = src[i];

	// Never forget incrementing the variable here or you'll
	// be stuck forever copying the first element...
	i++;
}
```

This code will work fine, however it's a remarkable case of beginner's luck that we've checked the source 
and not the destination for its size in the condition because otherwise we would get a read access violation.
Fixing this would require to add another condition to the while loop:

```cpp
while(i < src.size() && i < dst.size())
{
	dst[i] = src[i];
	i++;
}
```



What started out as a pretty basic operation will again be surrounded by a lot of boiler plate code that is very prone to typing errors. Again, special index types to the rescue. In this case, the two interesting index types are `index::safe` and `index::unsafe`, so we'll take a look at how to leverage those two to get a better syntax and how it might affect performance:

```cpp
span<float, 19> src = { 1.0f };
span<float, 43> dst = { 0.0f };

index::previous<0, false> i;

while(i.next(src, dst))
{
	dst[i] = src[i];
}
```

The `isInside()` function can be called with an arbitrary amount of containers to check and will return true as long as the index is below all of them. The performance should be exactly the same as when you type out all the conditions you need. However there is one other way of doing a safe loop which might have an impact on performance. 


```cpp
span<float, 19> src = { 1.0f };
span<float, 43> dst = { 0.0f };

// We'll be using the pre-increment operator in the 
// loop so in order to catch the first item, we'll have
// to start at -1;
index::safe<0> i(-1);

for(auto& s: dst)
{
	// Copies the element without any branching.
	// If src is smaller than dst, it will read
	// from a dummy address (which will always return zero)
    s = src[++i];

#if 0
    // this would be the aquivalent alternative with branching
    if(i.isInside(src))
    	s = src[++i];
    else
    	s = 0.0f;
#endif
}
```


```cpp

span<float, 125> data;

index::forward<0> i;

index::reverse<0> r;

index::stride<0, 12> s;


float v = 0.0f;

while(i.next(data))
{
	data[i] = v;
	v += 0.1f;
}



for(auto i: idx.iterate(data))
{
	data[i] = 90;
}



```
