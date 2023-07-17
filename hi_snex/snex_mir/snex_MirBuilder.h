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
struct MIR_func;

namespace snex {
namespace mir {
using namespace juce;

struct InstructionCollection;
struct State;

struct MirBuilder
{
	MirBuilder(MIR_context* ctx_, const ValueTree& v_);

	~MirBuilder();

	Result parse();

	MIR_module* getModule() const;

	String getMirText() const;

    void setDataLayout(const Array<ValueTree>& data);
    
    ValueTree getGlobalData();
    
private:

	State* currentState = nullptr;

	Result parseInternal(const ValueTree& v);

	bool checkAndFinish(const Result& r);

	ValueTree root;
};

}
}
