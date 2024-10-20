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


struct MirFunctionCollection;

struct MirCompiler
{
	MirCompiler(jit::GlobalScope& m);

	jit::FunctionCollectionBase* compileMirCode(const String& code);
	jit::FunctionCollectionBase* compileMirCode(const ValueTree& ast);

    void setDataLayout(const Array<ValueTree>& dataTree);
    
	Result getLastError() const;;

	~MirCompiler();

	static void setLibraryFunctions(const Array<StaticFunctionPointer>& functionMap);

	static void* resolve(const char* name);

	static bool isExternalFunction(const String& sig);
    
    String getAssembly() const { return assembly; }
    
	jit::FunctionCollectionBase::Ptr currentFunctionClass;

	private:

	jit::GlobalScope& memory;

	MirFunctionCollection* getFunctionClass();

    Array<ValueTree> dataLayout;
    String assembly;
    
	static Array<StaticFunctionPointer> currentFunctions;
	static void* currentConsole;

	Result r;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MirCompiler);
	
};


}
}
