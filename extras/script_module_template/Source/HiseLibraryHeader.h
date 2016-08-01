/*
  ==============================================================================

    HiseLibraryHeader.h
    Created: 1 Aug 2016 9:50:02pm
    Author:  Christoph Hart

  ==============================================================================
*/

#ifndef HISELIBRARYHEADER_H_INCLUDED
#define HISELIBRARYHEADER_H_INCLUDED

#define RETURN_STATIC_IDENTIFIER(x) const static Identifier id(x); return id;

#define SET_STRING(variableName, content) strcpy(variableName, content); size = strlen(variableName); 

// Replace this with file contents when API is in stable state...
#include "../../../hi_scripting/scripting/api/BaseFactory.h"
#include "../../../hi_scripting/scripting/api/DspBaseModule.h"

#if JUCE_WINDOWS
#define DLL_EXPORT extern "C" __declspec(dllexport)
#else
#define DLL_EXPORT extern "C" __attribute__((visibility("default")))
#endif

static Factory<DspBaseObject> baseObjects;

DLL_EXPORT void destroyDspObject(DspBaseObject* handle) { delete handle; }

DLL_EXPORT void initialise();

DLL_EXPORT bool matchPassword(const char* password);

DLL_EXPORT const void *getModuleList() { return &baseObjects.getIdList(); }

DLL_EXPORT DspBaseObject* createDspObject(const char *name)
{
    if (DspBaseObject *b = baseObjects.createFromId(Identifier(name))) return b;
    
    return nullptr;
}


#endif  // HISELIBRARYHEADER_H_INCLUDED
