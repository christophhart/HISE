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


#include "libtcc.h"

#include "TccLibrary.cpp"


#define CALL_TCC_FUNCTION(name, variableName, rVar, ...) name variableName = (name)dll->getFunction(#name); rVar = variableName(__VA_ARGS__);
#define CALL_VOID_TCC_FUNCTION(name, variableName, ...) name variableName = (name)dll->getFunction(#name); variableName(__VA_ARGS__);

void TccContext::debugTccError(void* , const char* message)
{
	String errorMessage(message);

	errorMessage = errorMessage.replace("<string>:", "!Line ");
	errorMessage = errorMessage.replace("warning", "error");

	errorMessage = errorMessage.replace("tcc:", "!TCC compilation ");

	Logger::writeToLog(errorMessage);
}



bool TccContext::activeContextExists = false;

TccContext::TccContext() :
state(nullptr),
#if JUCE_WINDOWS
tccDirectory(File::getSpecialLocation(File::userApplicationDataDirectory).getChildFile("Hart Instruments/tcc/"))
#else
tccDirectory(File::getSpecialLocation(File::userApplicationDataDirectory).getChildFile("Application Support/Hart Instruments/tcc/"))
#endif
{
#if JUCE_WINDOWS



#if JUCE_32BIT
	dll->open(tccDirectory.getChildFile("libtcc_x86.dll").getFullPathName());
#else
	dll->open(tccDirectory.getChildFile("libtcc_x64.dll").getFullPathName());
#endif

#else
    
    File f = tccDirectory.getChildFile("libtcc.dylib");
    jassert(f.existsAsFile());
    
    dll->open(tccDirectory.getChildFile("libtcc.dylib").getFullPathName());
    
    
#endif
}

TccContext::~TccContext()
{
	if (state != nullptr)
	{
		compiledData.setSize(0);
		closeContext();
	}
}

void TccContext::openContext()
{
	if (activeContextExists)
	{
		// You must only have one active compile context open at a time...
		jassertfalse;
		state = nullptr;
		return;
	}

	if (dll->getNativeHandle() != nullptr)
	{
		CALL_TCC_FUNCTION( tcc_new, createState, state);
		CALL_VOID_TCC_FUNCTION( tcc_set_output_type, setOutputTyoe, state, TCC_OUTPUT_MEMORY);
		CALL_VOID_TCC_FUNCTION( tcc_set_error_func, setErrorFunc, state, NULL, debugTccError);
		CALL_VOID_TCC_FUNCTION( tcc_add_library_path, addLibraryFunction, state, tccDirectory.getFullPathName().getCharPointer());
        
		CALL_VOID_TCC_FUNCTION( tcc_add_include_path, addIncludeFunction, state, tccDirectory.getFullPathName().getCharPointer());
		

#if JUCE_MAC
		CALL_VOID_TCC_FUNCTION( tcc_set_options, setOptions, state, "-static -Werror");
		addSymbolFunction(state, "__GNUC__", "5"); // Avoid compiler warning about unsupported compiler...
        addIncludeFunction(state, "/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX10.11.sdk/usr/include/");
		CALL_VOID_TCC_FUNCTION(tcc_add_library, addBaseLibrary, state, "m");
	
#else
		CALL_VOID_TCC_FUNCTION(tcc_set_options, setOptions, state, "-Werror");

		addIncludeFunction( state, tccDirectory.getChildFile("include").getFullPathName().getCharPointer());
#endif
		
		TccLibraryFunctions::addFunctionsToContext(this);
        
		activeContextExists = true;
	}
}

void TccContext::closeContext()
{
	if (state != nullptr)
	{
		CALL_VOID_TCC_FUNCTION( tcc_delete, deleteFunc, state);
		state = nullptr;
		activeContextExists = false;
	}
}

void TccContext::addFunction(void* functionPointer, const String &name)
{
	if (state != nullptr)
	{
		CALL_VOID_TCC_FUNCTION( tcc_add_symbol, f, state, name.getCharPointer(), functionPointer);
	}
}

void* TccContext::getFunction(String functionName)
{
	if (state != nullptr)
	{
		void*f2;
		CALL_TCC_FUNCTION( tcc_get_symbol, getFunc, f2, state, functionName.getCharPointer());
		return f2;
	}

	return nullptr;
}

int TccContext::compile(const File& f)
{
	String content = f.loadFileAsString();
	return compile(content);
}

int TccContext::compile(const String &code)
{
	if (state != nullptr)
	{
		int x;
		CALL_TCC_FUNCTION( tcc_compile_string, cs, x, state, code.getCharPointer());

		if (x == 0)
		{
			int relocationSize;
			CALL_TCC_FUNCTION( tcc_relocate, relocFunc, relocationSize, state, NULL);

			if (relocationSize > 0)
			{
				compiledData.setSize(relocationSize);

				int relocationSucessful;
				CALL_TCC_FUNCTION(tcc_relocate, relocFunc2, relocationSucessful, state, compiledData.getData());
			}
			else return -1;
		}

		return x;
	};

	return -1;
}


#undef CALL_TCC_FUNCTION
#undef CALL_VOID_TCC_FUNCTION

