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


#ifndef TCCCONTEXT_H_INCLUDED
#define TCCCONTEXT_H_INCLUDED

class TccContext;

#define TCC_CPP 1
#define TCC_HISE 1

struct TccLibraryFunctions
{
#include "TccLibrary.h"
};

struct TCCState;

/** A RAII wrapper around the TCC compiler. */
class TccContext
{
public:

	// ================================================================================================================

	/** Creates a new TccContext. 
	*
	*	You must call openContext() before compiling or pushing functions to the context. */
	TccContext(const File &f);

	~TccContext();

	// ================================================================================================================

	/** Opens the context for compiling. */
	void openContext();

	/** Closes the context. after a compilation. */
	void closeContext();

	// ================================================================================================================

	/** Adds a function to the compiled bytecode. */
	void addFunction(void* functionPointer, const String &name);

	/** Get a function from the compiled bytecode. */
	void* getFunction(String functionName);

	// ================================================================================================================

	/** Compiles a C file. */
	int compile(const File& f);

	/** Compiles a String containg C code. */
	int compile(const String &code);

private:

	static void debugTccError(void* opaque, const char* message);

    static void printDataArray(float* d, int numSamples);

	// ================================================================================================================

	SharedResourcePointer<DynamicLibrary> dll;
	const File tccDirectory;

	TCCState* state;
	MemoryBlock compiledData;
	
	static bool activeContextExists;

	File f;

	// ================================================================================================================

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TccContext);
};




#endif  // TCCCONTEXT_H_INCLUDED
