/*
  ==============================================================================

    hi_native_jit.cpp
    Created: 8 Mar 2017 12:09:56am
    Author:  Christoph

  ==============================================================================
*/

#include "JuceHeader.h"

namespace juce
{
#include "../VariantBuffer.h"
#include "../VariantBuffer.cpp"
}

#include "../src/asmjit/asmjit.h"

#include "../hi_native_jit_public.h"

#define INCLUDE_BUFFERS 1
#define INCLUDE_GLOBALS 1
#define INCLUDE_CONDITIONALS 1
#define INCLUDE_FUNCTION_CALLS 1

using namespace asmjit;
using namespace juce;

#include "TokenIterator.h"
#include "Parser.h"
#include "AsmJitInterface.h"
#include "JitFunctions.h"

#include "JitScope.cpp"
#include "FunctionParserBase.h"
#include "FunctionParser.h"
#include "GlobalParser.h"
#include "JitCompiler.cpp"
#include "JitDspModule.cpp"
#include "JitUnitTests.cpp"

float JITTest::test()
{
	
	return 0.0f;
};
