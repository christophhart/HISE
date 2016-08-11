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
*
*   ===========================================================================
*/

#ifndef DSPINSTANCE_H_INCLUDED
#define DSPINSTANCE_H_INCLUDED

#include <atomic>

/** This objects is a wrapper around the actual DSP module that is loaded from a plugin.
*
*	It contains the glue code for accessing it per Javascript and is reference counted to manage the lifetime of the external module.
*/
class DspInstance : public ConstScriptingObject,
					public AssignableObject
{
public:

	/** Creates a new instance from the given Factory with the supplied name. */
	DspInstance(const DspFactory *f, const String &moduleName_);
	~DspInstance();

	void initialise();
	void unload();

	Identifier getObjectName() const override { RETURN_STATIC_IDENTIFIER("DspModule") };

	/** Calls the setup method of the external module. */
	void prepareToPlay(double sampleRate, int samplesPerBlock);

	/** Calls the processMethod of the external module. */
	void processBlock(const var &data);

	/** Sets the float parameter with the given index. */
	void setParameter(int index, float newValue);

	/** Returns the parameter with the given index. */
	var getParameter(int index) const;

	/** Sets a String value. */
	void setStringParameter(int index, String value);

	/** Gets the string value. */
	String getStringParameter(int index);

	/** Enables / Disables the processing. */
	void setBypassed(bool shouldBeBypassed);

	/** Checks if the processing is enabled. */
	bool isBypassed() const;

	void assign(const int index, var newValue) override;
	var getAssignedValue(int index) const override;
	int getCachedIndex(const var &name) const override;

	/** Applies the module on the data. */
	void operator >>(const var &data);

	/** Applies the module on the data. */
	void operator <<(const var &data);

	/** Returns an informative String. */
	var getInfo() const;

	struct Wrapper;

private:

	void throwError(const String &errorMessage)
	{
		throw String(errorMessage);
	}

	const String moduleName;

	DspBaseObject *object;
	DspFactory::Ptr factory;

	std::atomic<bool> bypassed;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DspInstance)
};




#endif  // DSPINSTANCE_H_INCLUDED
