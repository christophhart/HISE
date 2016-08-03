/** @mainpage HISE DSP Module API
*	@author Christoph Hart
*	@version 1.0
*	
This API allows building DSP libraries that can load different DSP modules into a script processor.
A DSP Module is a little class that contains common functions / callbacks found in typical effect processing APIs.

A DSP Module has this features:

- Callback functions which do the setup / actual processing
- a parameter system to set / get float parameters as well 
- a constant system (which actually can do more than constants...). 

For a detailed documentation, take a look at the DspBaseObject class.

In order to maximise ABI compatibility, crossing library boundaries is only done purely in C.

## Getting Started

### Setting up the project

There is a template project for the Introjucer that contains all settings needed to build a library:

http://github.com/link.

The best practice would be copying this template, customize the settings and build it.

### Implement the library

You need to subclass DspBaseObject, overwrite its virtual functions and register it in the initialise() method:

@code{.cpp}
class MyEffect: public DspBaseObject
{
// ... Implement everything needed
};

LoadingErrorCode initialise(const char* c)
{
// Register your module (you can register as many classes as you want.
HelperFunctions::registerDspModule<MyEffect>();
// ...

// Return success!
return LoadingErrorCode::LoadingSuccessful;
};
@endcode

### Installing the library

The library must be placed in the OS-specific subfolder:

- **Windows:** `USER_APP_DATA_DIRECTORY/Hart Instruments/dll`
- **OSX:** `USER_LIBRARY_FOLDER/Application Support/Hart Instruments/lib`

There are some naming conventions for Windows .DLLs: use the suffix `_x86.dll` for 32bit builds and `_x64.dll` for 64bit builds.
(The OSX libraries are universal binaries so they don't need to be named separately.)

If you use the above template, the build location will be adjusted automatically to these folders, but you might want to know what's going on
when building an installer that contains your library...

### Loading the library in Javascript

Libraries can be loaded in all script processors that process audio data (Script Time Variant Modulator, Script Envelope, Script Synth and Script FX).

You load a library by using the library name stripped of all platform specific information:

@code{.js}
// Filename: 'example_library_x64.dll' (Win 64bit dynamic library)
var ex_lib = Libraries.load("example_library");

// Filename: 'example_library_x86.dll' (Win 32bit dynamic library)
var ex_lib = Libraries.load("example_library");

// Filename: 'example_library.dylib' (Win 32bit dynamic library)
var ex_lib = Libraries.load("example_library");
@endcode

The `Libraries.load()` function has a second argument which is supposed to be a string and is passed to the initialise method. You can use this for two things:

#### Version checking

Simply pass a version string and check if it matches the defined version number in your Library

#### Copy protection

Libraries can be protected against unlicenced loading by supplying a String key as second argument. In this case, it will be passed
to your initialise() method and you can check if the key is valid. If you don't care about it, leave it empty. If you just want minimum security against
occasional dudes trying to rip your precious algorithms, just pass a string and compare it (it won't stand a chance against even the dumbest cracker tough).

If you are really protective, you might want to create a RSA Key Pair along with some hash string and go crazy on the cryptography...

The error code can be checked by calling

@code{.js}
if(ex_lib.getErrorCode() != ex_lib.LoadingSucessful)
{
	handleMissingLibrary();
}
@endcode

### Using the library in Javascript

A library can create multiple DSP Modules. There are two functions:

@code{.js}
var moduleList = ex_lib.getModuleList();		// Returns an array containing all module IDs.
const var module = ex_lib.load("ModuleName");	// Loads the module with the given name.
@endcode

Notice the const keyword when assigning a module. This allows faster code because reference to this variable will be resolved on compile time.

That's it. Take a look at the DspBaseObject documentation on how to use the modules in Javascript.

## Copyright

This header (and its included files) are less restrictively licenced than the rest of the HISE codebase.
This allows close source development of HISE DSP modules without the restrictions of the GPL v3 licence-.
You'll need the juce_core module to build DSP modules (but it is also licenced pretty liberately). A amalgamated
compatible version can be found in the Github Repository of this API.

> The MIT License (MIT)
> Copyright (c) 2016 Hart Instruments
>
> Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files
> (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify,
> merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the
> Software is furnished to do so, subject to the following conditions:
>
> The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
>
> THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
> OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
> LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR
> IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

*/

#ifndef HISELIBRARYHEADER_H_INCLUDED
#define HISELIBRARYHEADER_H_INCLUDED



/** @file */ 

/** returns a static Identifier called `id`. */
#define RETURN_STATIC_IDENTIFIER(x) const static Identifier id(x); return id;

/** Adds a case statement for the enum and returns the name of the enum. */
#define FILL_PARAMETER_ID(enumClass, enumId, size, text) case (int)enumClass::enumId: size = HelperFunctions::writeString(text, #enumId); break;

// Replace this with file contents when API is in stable state...
#include "BaseFactory.h"
#include "DspBaseModule.h"


#if JUCE_WINDOWS
/** \internal */
#define DLL_EXPORT extern "C" __declspec(dllexport)
#else
/** \internal */
#define DLL_EXPORT extern "C" __attribute__((visibility("default")))
#endif

// The Factory used to create all modules from this library.
static Factory<DspBaseObject> baseObjects;


/** Contains some helper functions that abstract gory details. */
namespace HelperFunctions
{
	/** checks if the given version number (format "1.0.0") matches the version number specified in the Introjucer project. */
	bool matchesVersionNumber(const char* versionString)
	{
		return strcmp(ProjectInfo::versionString, versionString) == 0;
	}

	/** Writes the String literal 'content' into 'location' and returns the string length. 
	*
	*	@code
	*	int stringLength = HelperFunctions::writeString(loc, "Gain"); // stringLength will be 4...
	*	@endcode
	*/
	size_t writeString(char* location, const char* content)
	{
		strcpy(location, content);
		
		return strlen(content);
	}

	/** Creates a String from a different heap. This is rather slow because it makes a byte-wise copy of the other string, but better safe than sorry! */
	String createStringFromChar(const char* charFromOtherHeap, size_t length)
	{
		std::string s;
		s.reserve(length);

		for (int i = 0; i < length; i++)
			s.push_back(*charFromOtherHeap++);

		return String(s);
	}

	

	/** Registeres the module passed in as template parameter. */
	template <class T> void registerDspModule() { baseObjects.registerType<T>(); }
};



/** Contains functions that are called internally to load / unload the library. */
namespace InternalLibraryFunctions
{
	/** Returns a handle to a list of all modules that can be created with this library.
	*
	*	The void* pointer must be casted to an Array<Identifier>* pointer.
	*/
	DLL_EXPORT const void *getModuleList() { return &baseObjects.getIdList(); }

	/** Creates a module with the given name if it was registered with the base factory. */
	DLL_EXPORT DspBaseObject* createDspObject(const char *name)
	{
		if (DspBaseObject *b = baseObjects.createFromId(Identifier(name))) return b;
		return nullptr;
	}

	/** Destroys the given module that was created using createDspObject(). */
	DLL_EXPORT void destroyDspObject(DspBaseObject* handle) { delete handle; }
}

/** Overwrite this method and register all modules that you want to create with this library
*
*	This method will be called only if the library is not already loaded.
*
*	You can return an LoadingErrorCode if something is supposed to be wrong:
*
*	@code
*	LoadingErrorCode initialise(const char* args)
*	{
*		// Check if the given text matches the version of the library
*		if(HelperFunctions::matchesVersionNumber(args)
*		{
*			return LoadingErrorCode::NoVersionMatch;
*		}
*
*		HelperFunctions::registerDspModule<YourModuleClass>(); // YourModuleClass must be subclassed from DspBaseObject
*		HelperFunctions::registerDspModule<AnotherModuleClass>();
*
*		return LoadingErrorCode::LoadingSuccessful;
*	}
*	@endcode
*/
DLL_EXPORT LoadingErrorCode initialise(const char* args);

#endif  // HISELIBRARYHEADER_H_INCLUDED
