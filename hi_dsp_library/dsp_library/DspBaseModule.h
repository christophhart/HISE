/*
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
*   information about commercial licencing:
*
*   http://www.hartinstruments.net/hise/
*
*   HISE is based on the JUCE library,
*   which also must be licenced for commercial applications:
*
*   http://www.juce.com
*/

#ifndef DSPBASEMODULE_H_INCLUDED
#define DSPBASEMODULE_H_INCLUDED

namespace hise {using namespace juce;


/** @file */

/** This status codes will be returned by the initialise method.
*
*	It allows better error management.
*	If you the creation should fail, return the specific error code and set the handle to a `nullptr`
*/
enum class LoadingErrorCode
{
	LoadingSuccessful = 0, ///< Everything went OK
	Uninitialised, ///< something went wrong during initialisation
	MissingLibrary, ///< the library could not be found in the library folder.
	NoValidLibrary, ///< The library seems to be missing a initialise() method with the correct signature
	NoVersionMatch, ///< The version does not match. This error code will be never thrown automatically, but it is there to communicate the problem back to the Script Engine.
	KeyInvalid, ///< The license key that was passed to the initialise() method didn't match the one of the library.
	numErrorCodes
};


/** This interface class is the base class for all modules that can be loaded as plugin into the script FX processor. 
*
*	A DspBaseObject is a extremely simple, C-API driven class that performs audio processing called from a Script Processor
*	There are three main concepts for interaction with Javascript:
*
*	### Callbacks
*
*	There are two functions that are called at specific events that perform the actual logic of the module:
*	- the prepareToPlay() method, which should be used to setup the things needed for processing.
*	- the processBlock() method, which will be called periodically to process audio data.
*
*	There are corresponding callbacks in Script Processors (with even the same name), so using them in Javascript is pretty easy:
*
*	@code{.js}
*	function prepareToPlay(sampleRate, blockSize)
*	{
*		module.prepareToPlay(sampleRate, blockSize);
*	};
*
*	function processBlock(channels)
*	{
*		modules.processBlock(channels);
*		modules >> channels; // does the same thing
*		modules << channels; // does the same thing
*	}
*   @endcode
*
*	As you can see, the `>>` and `<<` operators are overloaded and call processBlock for the left side with the right side as argument.
*	It might be irritating at first that the two diametral operators do the same thing, but thinking LTR makes it better...

*	### Parameters
*
*	Parameters are floating point values that can be accessed by bracket subscription in Javascript:
*
*   @code{.js}
*	module[2] = 0.707; // 2 is magically Parameter "Gain" (see below...)
*	var x = module[2] // x is now 0.7070000001 :);
*   @endcode
*
*	Although standard Javascript is not typed, using an non-numerical value produces a runtime-error.
*	It is important to know that the access is not thread safe by default, so considering using `Atomics<float>` 
*	in your implementation for read/write access (which would be the getParameter(), setParameter() methods.  
*	If you despise the "magic-numberness" of this system, there is something to help you with that:
*
*	### Constants
*
*	Constants are constant values which can be accessed using the .dot Operator. They can hold integer numbers, floating point numbers, text (up to 512 characters)
*	and float arrays.
*
*	@code{.js}
*	var gainParameterId = module.Gain; // gainParameterId is 2 (finally...)
*	module[module.Gain] = 0.707 // no more magic number
*	var myPersonalPi = module.PI // myPersonalPi is 3.13 because everybody was wrong all the time
*   @endcode
*
*	If you're wondering about performance here, be aware that they are resolved at compile time (even the dot operator)
*	so it is equally fast than just typing the number yourself.	
*
*	There is a little detail that makes a significant difference: the constant is not really a constant, but a constant reference to a `var` object.
*	For numbers and texts, this doesn't make a difference, but you can use this trick to access internal data buffers (and even write to them!):
*
*	@code{.js}
*	1.0 >> module.internalDataBuffer; // fills the data buffer with 1.0f
*   @endcode
*
*	With this trick, you can load audio files into the module, get the tempoary fft data for analysis.
*	
*	All constants are resolved (= their getConstant() method is called) when the module is loaded. Except for constants that hold
*	float arrays, they are resolved everytime you call prepareToPlay (because changes are great, you might want to resize the buffers).
*
*	The implementation is not as straight forward as it could be, because it is not possible to create `var` objects and throw them across a DLL boundary,
*	so you have to overload multiple functions for every possible type and return `true` if the given index should be filled with the specific type.
*
*/
class DspBaseObject
{
public:

	// ================================================================================================================

	DspBaseObject();
	virtual ~DspBaseObject();

	// ================================================================================================================

	/** Overwrite this method and setup the processing for the given specifications. 
	*
	*	This method will be called always before calls to processBlock(), so you can setup the working buffers etc. here.
	*	the sample rate is obvious, but the block size only specifices the maximal size that can be fed into (it may be smaller). 
	*	You can safely assume that the block size is always a multiple of 4 to make some SSE stuff without bothering 
	*	about the edge cases.
	*
	*	This method will be called whenever one of the parameter changes (sample rate adjustment, audio buffer size adjustment), 
	*	as well as when the script will be compiled. After calling this method, every constants containing float arrays will be
	*	recalculated (their getConstant() method will be called) so you can adjust their sizes too.
	*/
	virtual void prepareToPlay(double sampleRate, int blockSize) = 0;

	/** Overwrite this method and do your processing on the given sample data. 
	*
	*	@param data: a 'numChannels' sized-array of 'numSamples'-sized float arrays 
	*	@param numChannels: the channel amount. This can be either '1' or '2', so you must handle both cases.
	*	@param numSamples: the sample amount: this will be max. the amount specified in the last prepareToPlay() call.
	*/
	virtual void processBlock(float **data, int numChannels, int numSamples) = 0;

	// =================================================================================================================
    
	/** Return the number of parameters for this module. This must be a constant. */
	virtual int getNumParameters() const = 0;

	/** This must return the value for each identifier. You might want to make this thread-aware by using Atomic<float> data members. */
	virtual float getParameter(int index) const = 0;

	/** This must set the parameter to your value. You might want to make this thread-aware by using Atomic<float> data members. */
	virtual void setParameter(int index, float newValue) = 0;
	
	/** Returns a string parameter. Use this to pass text values around (eg. for file I/O). */
	virtual const char* getStringParameter(int index, size_t& textLength);

	/** Sets a string parameter. Use this to pass text values around (eg. for file I/O). 
	*
	*	You need to create a copy from the char pointer (because it was allocated using another heap)
	*	@see HelperFunctions::writeString()
	*/
	virtual void setStringParameter(int index, const char* text, size_t textLength);

	// =================================================================================================================

	/** Overwrite this method if your module has constants that*/
	virtual int getNumConstants() const;;

	/** Overwrite this method and write the id of the constant old-school C-style into the given char pointer (max 64 characters)
	*
	*	You can use HelperFunctions::writeString() to avoid getting your hands dirty in too much C-ness...
	*
	*	@param index: the index of the constant.
	*	@param name: the name of the constant as plain old C-style string.
	*	@param size: the size of the name
	*/
	virtual void getIdForConstant(int index, char* name, int &size) const noexcept;

	/** Overwrite this method and fill in the value of the constant if the given index should be a float value. 
	*
	*	@param index: the index of the constant.
	*	@param value: the value that this constant should hold
	*	@returns true if the constant index is a float value
	*			 false, if it should look further in the other overloaded getConstant() functions.
	*/
	virtual bool getConstant(int index, float& value) const noexcept;

	/** Overwrite this method and fill in the value if the given index should be an integer value.
	*
	*	Unlike the other getConstant() methods, the default implementation returns the index itself if the index
	*	is a valid parameterIndex. Use this in conjunction with the macro FILL_PARAMETER_ID() to automatically
	*	create constants that refer to their parameter slots via their enum name.
	*
	*	@param index: the index of the constant.
	*	@param value: the value that this constant should hold
	*	@returns true if the constant index is a int value
	*			 false, if it should look further in the other overloaded getConstant() functions.
	*/
	virtual bool getConstant(int index, int& value) const noexcept;

	/** Overwrite this method and fill in the value if the given index should be an String (max 512 characters).
	*
	*	@param index: the index of the constant.
	*	@param text: the text that this constant should hold
	*	@returns true if the constant index is a String value
	*			 false, if it should look further in the other overloaded getConstant() functions.
	*/
	virtual bool getConstant(int index, char* text, size_t& size) const noexcept;

	/** Overwrite this method and pass a pointer to your float array data if the given index should be an float buffer. 
	*
	*	You can use HelperFunctions::writeString() to avoid getting your hands dirty in too much C-ness...
	*
	*	Unlike the other methods, this method requires that you allocate the data and pass a pointer to the data pointer back.
	*	You must make sure you deallocate the data when the library will be destroyed.
	*	Although this is theoretically a constant, the only thing that is constant here is the pointer to the data. That means you can
	*	alter the content of the float array as you like (as long as you don't change the size).
	*/
	virtual bool getConstant(int index, float** data, int &size) noexcept;

	// =================================================================================================================

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DspBaseObject)
};

} // namespace hise

#endif  // DSPBASEMODULE_H_INCLUDED
