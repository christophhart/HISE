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
/** A subclass that is specialised on building parameter template classes. */
struct ParameterBuilder : public TemplateClassBuilder
{
	ParameterBuilder(Compiler& c, const Identifier& id);

	struct Helpers
	{
		static ParameterBuilder createWithTP(Compiler& c, const Identifier& n);
		static FunctionData createCallPrototype(StructType* st, const Inliner::Func& highlevelFunc);

		/** This method is the default for any single parameter connection. It creates a member pointer to the given target. */
		static void initSingleParameterStruct(const TemplateObject::ConstructData& cd, StructType* st);

		static Operations::Statement::Ptr createSetParameterCall(ComplexType::Ptr targetType, int parameterIndex, SyntaxTreeInlineData* d, Operations::Statement::Ptr input);

		/** This function builder creates the connect function that sets the member pointer to the given target. */
		static FunctionData connectFunction(StructType* st);

		static bool isParameterClass(const TypeInfo& type);

		static void forwardToListElements(StructType* parent, const TemplateParameter::List& list, StructType** parameterType, int& index)
		{
			index = 0;
			*parameterType = dynamic_cast<StructType*>(TemplateClassBuilder::Helpers::getSubTypeFromTemplate(parent, index).get());

			if ((*parameterType)->id.getIdentifier() == Identifier("list"))
			{
				index = list[0].constant;
				*parameterType = dynamic_cast<StructType*>(TemplateClassBuilder::Helpers::getSubTypeFromTemplate(*parameterType, index).get());
			}
		}

		static int getParameterListOffset(StructType* container, int index)
		{
			auto parameterType = dynamic_cast<StructType*>(TemplateClassBuilder::Helpers::getSubTypeFromTemplate(container, 0).get());

			jassert(isParameterClass(TypeInfo(parameterType)));

			if (parameterType->id.getIdentifier() == Identifier("list"))
			{
				return (int)parameterType->getMemberOffset(index);
			}
			else
			{
				jassert(index == 0);
				return 0;
			}
		}
	};

	void setConnectFunction(const FunctionBuilder& f = Helpers::connectFunction)
	{
		addFunction(f);
	}
};

}

namespace Types {
using namespace juce;

struct ParameterLibraryBuilder: public LibraryBuilderBase
{
	ParameterLibraryBuilder(Compiler& c, int numChannels) :
		LibraryBuilderBase(c, numChannels)
	{};

	Identifier getFactoryId() const override { RETURN_STATIC_IDENTIFIER("parameter"); }

	Result registerTypes() override;
};

}
}