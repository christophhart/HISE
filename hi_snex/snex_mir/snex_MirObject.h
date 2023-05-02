/*  ===========================================================================
*
*   This file is part of HISE.
*   Copyright 2016 Christoph Hart
*
*   HISE is free software: you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation, either version 3 of the License, or
*   (at your option any later version.
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

#pragma once



struct MIR_context;
struct MIR_module;

namespace snex {
namespace mir {
using namespace juce;

using jit::FunctionData;
using jit::StaticFunctionPointer;



struct MirObject
{
	MirObject();

	Result compileMirCode(const String& code);
	Result compileMirCode(const ValueTree& ast);

    void setDataLayout(const String& b64data);
    
	FunctionData operator[](const String& functionName);

	Result getLastError() const;;

	~MirObject();

	static void example();

	static void setLibraryFunctions(const Array<StaticFunctionPointer>& functionMap);

	static void* resolve(const char* name);

	static bool isExternalFunction(const String& sig);

    
    
    StringArray getGlobalDataIds()
    {
        return dataIds;
    }
    
    
    template <typename T> T getData(const String& dataId, size_t byteOffset=0)
    {
        if(dataItems.contains(dataId))
        {
            auto bytePtr = reinterpret_cast<uint8*>(dataItems[dataId]);
            bytePtr += byteOffset;
            
            return *reinterpret_cast<T*>(bytePtr);
        }
        
        jassertfalse;
        return T();
    }
    
    String getAssembly() const { return assembly; }
    
    ValueTree getGlobalDataLayout();
    
private:

    ValueTree fillLayoutWithData(const String& dataId, const ValueTree& valueData);
    
    void fillRecursive(ValueTree& copy, const String& dataId, size_t offset);
    
    
    ValueTree globalData;
    
    StringArray dataIds;
    String dataLayout;
    String assembly;
    
    HashMap<String, void*> dataItems;
    
	static Array<StaticFunctionPointer> currentFunctions;

	Result r;

	MIR_context* ctx;
	Array<MIR_module*> modules;
};

}
}
