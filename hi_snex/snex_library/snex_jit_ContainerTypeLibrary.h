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

namespace snex {

namespace jit
{
struct ContainerNodeBuilder : public TemplateClassBuilder
{
	ContainerNodeBuilder(Compiler& c, const Identifier& id, int numChannels_);

	void addHighLevelInliner(const Identifier& functionId, const Inliner::Func& inliner);
	void addAsmInliner(const Identifier& functionId, const Inliner::Func& inliner);
	void deactivateCallback(const Identifier& id);

	void flush() override;

	struct Helpers
	{
		static FunctionData constructorFunction(StructType* st);
		static Result defaultForwardInliner(InlineData* b);
		static Identifier getFunctionIdFromInlineData(InlineData* b);
		static FunctionData getParameterFunction(StructType* st);
		static FunctionData setParameterFunction(StructType* st);
	};

private:

	
	bool isScriptnodeCallback(const Identifier& id) const;

	Array<FunctionData> callbacks;
	int numChannels;
};
}

namespace Types {
using namespace juce;

struct ContainerLibraryBuilder: public LibraryBuilderBase
{
	ContainerLibraryBuilder(Compiler& c, int numChannels):
		LibraryBuilderBase(c, numChannels)
	{}

	Identifier getFactoryId() const override { RETURN_STATIC_IDENTIFIER("container"); }

	Result registerTypes() override;
};


}
}