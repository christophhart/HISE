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
 *   Commercial licenses for using HISE in an closed source project are
 *   available on request. Please visit the project's website to get more
 *   information about commercial licensing:
 *
 *   http://www.hise.audio/
 *
 *   HISE is based on the JUCE library,
 *   which also must be licenced for commercial applications:
 *
 *   http://www.juce.com
 *
 *   ===========================================================================
 */

#pragma once

extern "C" {


struct ErrorC
{
    int error = 0;
    int expected = 0;
    int actual = 0;
};

}


namespace scriptnode
{
using namespace hise;
using namespace juce;

#ifndef THROW_SCRIPTNODE_ERRORS
#if HI_EXPORT_AS_PROJECT_DLL
// Do not throw errors across the DLL boundary
#define THROW_SCRIPTNODE_ERRORS 0
#else
// Don't care, throw the errors
#define THROW_SCRIPTNODE_ERRORS 1
#endif
#endif



struct Error: public ErrorC
{
    enum ErrorCode
    {
        OK,
        NoMatchingParent,
        RootIdMismatch,
        ChannelMismatch,
        BlockSizeMismatch,
        IllegalFrameCall,
        IllegalBlockSize,
        SampleRateMismatch,
        InitialisationError,
        TooManyChildNodes,
        TooManyParameters,
        CompileFail,
        NodeDebuggerEnabled,
        RingBufferMultipleWriters,
        DeprecatedNode,
        IllegalPolyphony,
        IllegalMonophony,
        IllegalBypassConnection,
        IllegalCompilation,
        IllegalFaustNode,
        IllegalFaustChannelCount,
        OldFaustVersion,
        CloneMismatch,
        IllegalMod,
        UnscaledModRangeMismatch,
        UncompiledThirdPartyNode,
        NoGlobalManager,
        numErrorCodes
    };
    
    bool isOk() const { return error == ErrorCode::OK; }
    
    static void throwError(ErrorCode code, int expected_ = 0, int actual_ = 0);
};

#if !THROW_SCRIPTNODE_ERRORS
struct DynamicLibraryErrorStorage
{
	static Error currentError;
};
#endif

}
