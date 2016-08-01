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
*   Commercial licences for using HISE in an closed source project are
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


/** This interface class is the base class for all modules that can be loaded as plugin into the script FX processor. 
*
*	A DspBaseObject can have parameters and constants which can be conveniently accessed via Javascript like this:
*
*       @code{.js}
*	    // Parameter:
*		module["Gain"] = 0.2;
*		var x = module["Gain"];
*
*		// Constant
*		var y = module.PI;
*       @endcode
*
*   This class can be used to compile little plugins as dynamic library and load them into the script processors to
*   extend HISE with your custom C++ DSP code. There is a template project for the Introjucer in the HISE repository
*   that can be used as a starting point for this.
*
*   They can also be used as static libraries and compiled into your finished plugin to avoid DLL problems.
*	
*	The constants are not created witin your class. Instead
*/
class DspBaseObject
{
public:

	// ================================================================================================================

	/** Overwrite this method and setup the processing for the given specifications. */
	virtual void prepareToPlay(double sampleRate, int blockSize) = 0;

	/** Overwrite this method and do your processing on the given sample data. */
	virtual void processBlock(float **data, int numChannels, int numSamples) = 0;

	// =================================================================================================================

	DspBaseObject() {};

    virtual ~DspBaseObject() {}
    
protected:

	friend class DspInstance;

	/** Return the number of parameters for this module. This must be a constant. */
	virtual int getNumParameters() const = 0;

	/** Return a Identifier for each parameter Index. 
	*
	*	This must be a constant Identifier and will be used to access properties in Javascript. */
	virtual const Identifier &getIdForParameter(int index) const = 0;

	/** This must return the value for each identifier. 
	*
	*	If the value is not a primitive type, make sure that the lifetime is managed by your module. */
	virtual float getParameter(int index) const = 0;

	/** This must set the parameter to your value. */
	virtual void setParameter(int index, float newValue) = 0;

	// =================================================================================================================

	/** Overwrite this method if your module has constants that*/
	virtual int getNumConstants() const { return 0; };

	/** Overwrite this method and write the id of the constant old-school C-style into the given char pointer (max 64 characters)
	*
	*	@param index: the index of the constant.
	*	@param name: the name of the constant as plain old C-style string.
	*	@param size: the size of the name
	*/
	virtual void getIdForConstant(int index, char* name, int &size) const noexcept{};

	/** Overwrite this method and fill in the value of the constant if the given index should be a float value. 
	*
	*	@returns true if the constant index is a float value.	
	*/
	virtual bool getConstant(int index, float& value) const noexcept{ return false; };

	/** Overwrite this method and fill in the value if the given index should be an integer value.
	*
	*	@returns true if the constant index is a int value
	*			 false, if it should look further in the other overloaded getConstant() functions.
	*/
	virtual bool getConstant(int index, int& value) const noexcept{ return false; };

	/** Overwrite this method and fill in the value if the given index should be an String (max 512 characters).
	*
	*	@returns true if the constant index is a String value
	*			 false, if it should look further in the other overloaded getConstant() functions.
	*/
	virtual bool getConstant(int index, char* text, size_t& size) const noexcept{ return false; };

	/** Overwrite this method and pass a pointer to your float array data if the given index should be an float buffer. 
	*
	*	Unlike the other methods, this method requires that you allocate the data and pass a pointer to the data pointer back.
	*	You must make sure you deallocate the data when the library will be destroyed.
	*	Although this is theoretically a constant, the only thing that is constant here is the pointer to the data. That means you can
	*	alter the float array as you like.
	*/
	virtual bool getConstant(int index, float** data, int &size) noexcept{ return false; };


	// =================================================================================================================

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DspBaseObject)
};

#endif  // DSPBASEMODULE_H_INCLUDED
