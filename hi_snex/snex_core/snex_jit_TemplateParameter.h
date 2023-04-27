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


struct TemplateParameter
{
	enum VariadicType
	{
		Single,
		Variadic,
		numVariadicTypes
	};

	enum ParameterType
	{
		Empty,
		ConstantInteger,
		Type,
		IntegerTemplateArgument,
		TypeTemplateArgument,
		numTypes
	};

	TemplateParameter() :
		t(Empty),
		type({}),
		variadic(VariadicType::Single),
		constant(0),
		constantDefined(false)
	{};

	TemplateParameter(const NamespacedIdentifier& id, int value, bool defined, VariadicType vType = VariadicType::Single) :
		t(IntegerTemplateArgument),
		type({}),
		argumentId(id),
		constant(value),
		constantDefined(defined),
		variadic(vType)
	{
		jassert(isTemplateArgument());
	}

	TemplateParameter(const NamespacedIdentifier& id, const TypeInfo& defaultType = {}, VariadicType vType = VariadicType::Single) :
		t(TypeTemplateArgument),
		type(defaultType),
		argumentId(id),
		constant(0),
		variadic(vType)
	{
		jassert(isTemplateArgument());
	}


	TemplateParameter(int c, VariadicType vType = VariadicType::Single) :
		type(TypeInfo()),
		constant(c),
		variadic(vType),
		t(ParameterType::ConstantInteger),
		constantDefined(true)
	{
		jassert(!isTemplateArgument());
	};

	TemplateParameter(const TypeInfo& t, VariadicType vType = VariadicType::Single) :
		type(t),
		constant(0),
		variadic(vType),
		t(ParameterType::Type)
	{

	};

	bool isTemplateArgument() const
	{
		return t == IntegerTemplateArgument || t == TypeTemplateArgument;
	}

	bool operator !=(const TemplateParameter& other) const
	{
		return !(*this == other);
	}

	bool operator==(const TemplateParameter& other) const
	{
		bool tMatch = t == other.t;
		bool typeMatch = type == other.type;
		bool cMatch = constant == other.constant;
		bool cdMatch = constantDefined == other.constantDefined;
		return tMatch && typeMatch && cMatch && cdMatch;
	}

	bool matchesTemplateType(const TypeInfo& t) const
	{
		jassert(argumentId.isValid());
		return t.getTemplateId() == argumentId;
	}

	TemplateParameter withId(const NamespacedIdentifier& id) const
	{
		// only valid with parameters...
		jassert(!isTemplateArgument());

		auto c = *this;
		c.argumentId = id;
		return c;
	}

    ValueTree createDataLayout() const
    {
        ValueTree v("TemplateParameter");
        v.setProperty("ID", argumentId.id.toString(), nullptr);
        
        if(t == ParameterType::ConstantInteger && constantDefined)
        {
            v.setProperty("ParameterType", "Integer", nullptr);
            v.setProperty("Value", constant, nullptr);
        }
        else
        {
            v.setProperty("ParameterType", "Type", nullptr);
            v.setProperty("Type", type.toStringWithoutAlias(), nullptr);
        }
        
        return v;
    }
    
	bool isVariadic() const
	{
		return variadic == VariadicType::Variadic;
	}

	bool isResolvedOrTemplateType() const
	{
		return isResolved() || type.isTemplateType();
	}

	bool isResolved() const
	{
		jassert(!isTemplateArgument());

		if (t == Type)
			return type.isValid();
		else
			return constantDefined;
	}

	using List = Array<TemplateParameter>;



	struct ListOps
	{
		static juce::String toString(const List& l, bool includeParameterNames = true);

		static List filter(const List& l, const NamespacedIdentifier& id);

		static List merge(const List& arguments, const List& parameters, juce::Result& r);

		static List sort(const List& arguments, const List& parameters, juce::Result& r);

		static List mergeWithCallParameters(const List& argumentList, const List& existing, const TypeInfo::List& originalFunctionArguments, const TypeInfo::List& callParameterTypes, Result& r);

		static Result expandIfVariadicParameters(List& parameterList, const List& parentParameters);

		static bool isVariadicList(const List& l);

		static bool matchesParameterAmount(const List& parameters, int expected);

		static bool isParameter(const List& l);

		static bool isArgument(const List& l);

		static bool isArgumentOrEmpty(const List& l);

		static bool match(const List& first, const List& second);

		static bool isNamed(const List& l);

		static bool isSubset(const List& all, const List& possibleSubSet);

		static bool readyToResolve(const List& l);

		static bool isValidTemplateAmount(const List& argList, int numProvided);
	};

	TypeInfo type;
	int constant;
	bool constantDefined = false;
	VariadicType variadic = VariadicType::Single;
	ParameterType t;
	NamespacedIdentifier argumentId;
};

/** This should be used whenever a class instance is being identified instead of a normal NamespacedIdentifier.

	It provides additional template parameters that have been used to instantiate the actual template object
	(eg. class template parameters for a templated function).
*/
struct TemplateInstance
{
	TemplateInstance(const NamespacedIdentifier& id_, const TemplateParameter::List& tp_) :
		id(id_),
		tp(tp_)
	{
		jassert(tp.isEmpty() || TemplateParameter::ListOps::isParameter(tp));

#if JUCE_DEBUG
		String s;
		s << id.toString() << TemplateParameter::ListOps::toString(tp, false);
		debugName = s.toStdString();
#endif
	}

	bool operator==(const TemplateInstance& other) const
	{
		return id == other.id && TemplateParameter::ListOps::match(tp, other.tp);
	}

	bool isValid() const
	{
		return id.isValid();
	}

	bool isParentOf(const TemplateInstance& other) const
	{
		return id.isParentOf(other.id) && TemplateParameter::ListOps::isSubset(other.tp, tp);
	}

	TemplateInstance getChildIdWithSameTemplateParameters(const Identifier& childId)
	{
		auto c = *this;
		c.id = id.getChildId(childId);
		return c;
	}

	String toString() const
	{
		String s;
		s << id.toString();
		s << TemplateParameter::ListOps::toString(tp, false);
		return s;
	}

	NamespacedIdentifier id;
	TemplateParameter::List tp;

#if JUCE_DEBUG
	std::string debugName;
#endif
};

struct SubTypeConstructData
{
	NamespaceHandler* handler;
	NamespacedIdentifier id;
	TemplateParameter::List l;
	Result r = Result::ok();
};



}
}
