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
namespace jit {
using namespace juce;


struct SyntaxTreeInlineData;
struct AsmInlineData;

struct InlineData
{
	virtual ~InlineData() {};

	virtual bool isHighlevel() const = 0;

	SyntaxTreeInlineData* toSyntaxTreeData() const;
	AsmInlineData* toAsmInlineData() const;

	Array<TemplateParameter> templateParameters;
};


struct Inliner : public ReferenceCountedObject
{
	using Ptr = ReferenceCountedObjectPtr<Inliner>;
	using List = ReferenceCountedArray<Inliner>;
	using Func = std::function<Result(InlineData* d)>;

	

	enum InlineType
	{
		HighLevel,
		Assembly,
		AutoReturnType,
		numInlineTypes
	};

	Inliner(const NamespacedIdentifier& id, const Func& asm_, const Func& highLevel_) :
		asmFunc(asm_),
		highLevelFunc(highLevel_)
	{
		if (hasHighLevelInliner())
			inlineType = HighLevel;

		if (hasAsmInliner())
			inlineType = Assembly;
	};

	static Inliner* createFromType(const NamespacedIdentifier& id, InlineType type, const Func& f)
	{
		if (type == Assembly)
		{
			auto inliner = createAsmInliner(id, f);
			inliner->inlineType = type;
			return inliner;
		}
		else
			return createHighLevelInliner(id, f);
	}

	static Inliner* createHighLevelInliner(const NamespacedIdentifier& id, const Func& highLevelFunc)
	{
		return new Inliner(id, {}, highLevelFunc);
	}

	static Inliner* createAsmInliner(const NamespacedIdentifier& id, const Func& asmFunc)
	{
		return new Inliner(id, asmFunc, {});
	}

	bool hasHighLevelInliner() const
	{
		return (bool)highLevelFunc;
	}

	bool hasAsmInliner() const
	{
		return (bool)asmFunc;
	}

	Result process(InlineData* d) const;

	InlineType inlineType = numInlineTypes;

	//const NamespacedIdentifier functionId;
	const Func asmFunc;
	const Func highLevelFunc;

	// Optional: returns a list of all inliners that need to be compiled before this function
	Func precodeGenFunc;
	// Optional: returns a TypeInfo
	Func returnTypeFunction;
};





}
}
