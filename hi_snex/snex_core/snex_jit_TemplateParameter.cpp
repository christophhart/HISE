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


namespace snex {
namespace jit {
using namespace juce;
USE_ASMJIT_NAMESPACE;


bool TemplateParameter::ListOps::isParameter(const TemplateParameter::List& l)
{
	for (const auto& p : l)
	{
		if (!p.isTemplateArgument())
			return true;
	}

	return false;
}

bool TemplateParameter::ListOps::isArgument(const TemplateParameter::List& l)
{
	for (const auto& p : l)
	{
		if (p.isTemplateArgument())
		{
			jassert(!isParameter(l));
			return true;
		}
	}

	return false;
}

bool TemplateParameter::ListOps::isArgumentOrEmpty(const List& l)
{
	if (l.isEmpty())
		return true;

	return isArgument(l);
}

bool TemplateParameter::ListOps::match(const List& first, const List& second)
{
	if (first.size() != second.size())
		return false;

	for (int i = 0; i < first.size(); i++)
	{
		auto f = first[i];
		auto s = second[i];

		if (f != s)
			return false;
	}

	return true;
}

bool TemplateParameter::ListOps::isNamed(const List& l)
{
	for (auto& p : l)
	{
		if (!p.argumentId.isValid())
			return false;
	}

	return true;
}

bool TemplateParameter::ListOps::isSubset(const List& all, const List& possibleSubSet)
{
	for (auto tp : possibleSubSet)
	{
		if (!all.contains(tp))
			return false;
	}

	return true;
}

bool TemplateParameter::ListOps::readyToResolve(const List& l)
{
	return isNamed(l) && (isParameter(l) || l.isEmpty());
}

bool TemplateParameter::ListOps::isValidTemplateAmount(const List& argList, int numProvided)
{
	if (numProvided == -1)
		return true;

	int required = 0;

	for (auto& a : argList)
	{
		if (a.constantDefined || a.type.isValid())
			continue;

		if (a.isVariadic())
		{
			return numProvided >= argList.size();
		}

		required++;
	}

	return required == numProvided;
}

juce::String TemplateParameter::ListOps::toString(const List& l, bool includeParameterNames)
{
	if (l.isEmpty())
		return {};

	juce::String s;

	s << "<";

	for (int i = 0; i < l.size(); i++)
	{
		auto t = l[i];

		if (t.isTemplateArgument())
		{
			if (t.t == TypeTemplateArgument)
			{
				s << "typename";

				if (t.isVariadic())
					s << "...";

				s << " " << t.argumentId.getIdentifier();



				if (t.type.isValid())
					s << "=" << t.type.toString();
			}
			else
			{
				s << "int";

				if (t.isVariadic())
					s << "...";

				s << " " << t.argumentId.getIdentifier();

				if (t.constant != 0)
					s << "=" << juce::String(t.constant);
			}


		}
		else
		{


			if (t.isVariadic())
			{
				s << t.argumentId.toString() << "...";
				continue;
			}

			if (includeParameterNames && t.argumentId.isValid())
			{
				s << t.argumentId.toString() << "=";
			}

			if (t.type.isValid())
				s << t.type.toString();
			else
				s << juce::String(t.constant);
		}

		if (i != l.size() - 1)
			s << ", ";
	}

	s << ">";

	return s;
}

TemplateParameter::List TemplateParameter::ListOps::filter(const List& l, const NamespacedIdentifier& id)
{
	List r;

	for (auto& p : l)
	{
		if (p.argumentId.getParent() == id)
			r.add(p);
	}

	return r;
}

TemplateParameter::List TemplateParameter::ListOps::merge(const TemplateParameter::List& arguments, const TemplateParameter::List& parameters, juce::Result& r)
{
	if (arguments.isEmpty() && parameters.isEmpty())
		return {};

	jassert(isArgument(arguments));
	jassert(isParameter(parameters) || parameters.isEmpty());

	if (arguments.isEmpty() && parameters.isEmpty())
		return parameters;

#if JUCE_DEBUG
	for (auto& a : arguments)
	{
		// The argument array must contain Template arguments only...
		jassert(a.isTemplateArgument());
	}

	for (auto& p : parameters)
	{
		// the parameter array must contain template parameters only
		// (ParameterType::Type or ParameterType::Constant)
		jassert(!p.isTemplateArgument());
	}
#endif

	TemplateParameter::List instanceParameters;

	auto numArgs = arguments.size();
	auto numDefinedParameters = parameters.size();
	auto lastArgIsVariadic = arguments.getLast().isVariadic();
	auto lastParamIsVariadic = parameters.getLast().isVariadic();

	ignoreUnused(lastParamIsVariadic);

	if (numDefinedParameters > numArgs && !lastArgIsVariadic)
	{
		r = Result::fail("Too many template parameters");
		return instanceParameters;
	}

	if (lastArgIsVariadic)
		numArgs = numDefinedParameters;

	for (int i = 0; i < numArgs; i++)
	{
		if (isPositiveAndBelow(i, numDefinedParameters))
		{
			TemplateParameter p = parameters[i];

			if (p.isVariadic())
			{
				p.argumentId = arguments.getLast().argumentId;
				instanceParameters.add(p);
				return instanceParameters;
			}

			if (!lastArgIsVariadic || isPositiveAndBelow(i, arguments.size()))
			{
				p.argumentId = arguments[i].argumentId;
				instanceParameters.add(p);
			}
			else
			{
				p.argumentId = arguments.getLast().argumentId;

				//p.argumentId.id = Identifier(p.argumentId.id.toString() + String(i + 1));

				instanceParameters.add(p);
			}


		}
		else
		{
			TemplateParameter p = arguments[i];
			jassert(p.argumentId.isValid());

			if (p.t == TemplateParameter::TypeTemplateArgument)
			{
				jassert(p.type.isValid());
				p.t = TemplateParameter::ParameterType::Type;
			}
			else
				p.t = TemplateParameter::ParameterType::ConstantInteger;

			instanceParameters.add(p);
		}
	}

	for (auto& p : instanceParameters)
	{
		if (!p.isResolvedOrTemplateType())
		{
			r = Result::fail("Missing template specialisation for " + p.argumentId.toString());
		}
	}

	return instanceParameters;
}

TemplateParameter::List TemplateParameter::ListOps::sort(const List& arguments, const List& parameters, juce::Result& r)
{
	//jassert(isArgumentOrEmpty(arguments));
	jassert(isParameter(parameters) || parameters.isEmpty());

	if (arguments.size() != parameters.size())
		return parameters;

	for (auto& p : parameters)
	{
		if (!p.argumentId.isValid())
			return parameters;
	}

	TemplateParameter::List tp;

	for (int i = 0; i < arguments.size(); i++)
	{
		for (int j = 0; j < parameters.size(); j++)
		{
			if (arguments[i].argumentId == parameters[j].argumentId)
			{
				tp.add(parameters[j]);
				break;
			}
		}
	}

	return tp;
}

TemplateParameter::List TemplateParameter::ListOps::mergeWithCallParameters(const List& argumentList, const List& existing, const TypeInfo::List& originalFunctionArguments, const TypeInfo::List& callParameterTypes, Result& r)
{
	jassert(existing.isEmpty() || isParameter(existing));

	List tp = existing;

	jassert(callParameterTypes.size() == originalFunctionArguments.size());

	for (int i = 0; i < originalFunctionArguments.size(); i++)
	{
		auto o = originalFunctionArguments[i];
		auto cp = callParameterTypes[i];

		if (o.isTemplateType())
		{
			// Check if the type is directly used...
			auto typeTouse = cp.withModifiers(o.isConst(), o.isRef());
			TemplateParameter tId(typeTouse);
			tId.argumentId = o.getTemplateId();

			for (auto& existing : tp)
			{
				if (existing.argumentId == tId.argumentId)
				{
					if (existing != tId)
					{
						r = Result::fail("Can't deduce template type from arguments");
						return {};
					}
				}
			}

			tp.addIfNotAlreadyThere(tId);
		}
		else if (auto ctd = o.getTypedIfComplexType<TemplatedComplexType>())
		{
			// check if the type can be deducted by the template parameters...

			auto pt = cp.getTypedIfComplexType<ComplexTypeWithTemplateParameters>();

			jassert(pt != nullptr);

			auto fArgTemplates = ctd->getTemplateInstanceParameters();
			auto fParTemplates = pt->getTemplateInstanceParameters();

			jassert(fArgTemplates.size() == fParTemplates.size());

			for (int i = 0; i < fArgTemplates.size(); i++)
			{
				auto fa = fArgTemplates[i];
				auto fp = fParTemplates[i];

				if (fa.type.isTemplateType())
				{
					auto fpId = fa.type.getTemplateId();

					for (auto& a : argumentList)
					{
						if (a.argumentId == fpId)
						{
							TemplateParameter tId = fp;
							tId.argumentId = fpId;

							tp.addIfNotAlreadyThere(tId);
						}
					}
				}
			}
		}
	}

	return sort(argumentList, tp, r);
}

juce::Result TemplateParameter::ListOps::expandIfVariadicParameters(List& parameterList, const List& parentParameters)
{
	if (parentParameters.isEmpty())
		return Result::ok();

	//DBG("EXPAND");
	//DBG("Parameters: " + TemplateParameter::ListOps::toString(parameterList));
	//DBG("Parent parameters: " + TemplateParameter::ListOps::toString(parentParameters));

	List newList;

	for (const auto& p : parameterList)
	{
		if (p.isVariadic())
		{
			auto vId = p.type.getTemplateId().toString();

			for (auto& pp : parentParameters)
			{
				auto ppId = pp.argumentId.toString();
				if (vId == ppId)
					newList.add(pp);
			}
		}
		else
		{
			newList.add(p);
		}
	}

	std::swap(newList, parameterList);

	//DBG("Expanded: " + TemplateParameter::ListOps::toString(parameterList));
	//DBG("-----------------------------------------------------------");

	return Result::ok();
}

bool TemplateParameter::ListOps::isVariadicList(const List& l)
{
	return l.getLast().isVariadic();
}

bool TemplateParameter::ListOps::matchesParameterAmount(const List& parameters, int expected)
{
	jassert(isParameter(parameters));

	if (parameters.size() == expected)
		return true;

	if (parameters.getLast().isVariadic())
	{
		jassertfalse;
	}

	return false;
}

snex::jit::ComplexType::Ptr TemplatedComplexType::createTemplatedInstance(const TemplateParameter::List& suppliedTemplateParameters, juce::Result& r)
{
	TemplateParameter::List instanceParameters;

	for (const auto& p : d.tp)
	{
		if (p.type.isTemplateType())
		{
			for (const auto& sp : suppliedTemplateParameters)
			{
				if (sp.argumentId == p.type.getTemplateId())
				{
					if (sp.t == TemplateParameter::ConstantInteger)
					{
						TemplateParameter ip(sp.constant);
						ip.argumentId = sp.argumentId;
						instanceParameters.add(ip);
					}
					else
					{
						TemplateParameter ip(sp.type);
						ip.argumentId = sp.argumentId;
						instanceParameters.add(ip);
					}
				}

				if (p.type.isTemplateType() && sp.type.isTemplateType() &&
					p.type.getTemplateId() == sp.type.getTemplateId())
				{
					instanceParameters.add(sp);
				}

			}
		}
		else if (p.isTemplateArgument())
		{
			for (const auto& sp : suppliedTemplateParameters)
			{
				if (sp.argumentId == p.argumentId)
				{
					jassert(sp.isResolved());
					TemplateParameter ip = sp;
					instanceParameters.add(ip);
				}
			}
		}
		else
		{
			jassert(p.isResolved());
			instanceParameters.add(p);
		}
	}

#if JUCE_DEBUG
	for (auto& p : instanceParameters)
	{
		jassert(p.isResolvedOrTemplateType());
	}
#endif

	TemplateObject::ConstructData instanceData = d;
	instanceData.tp = instanceParameters;

	instanceData.r = &r;

	ComplexType::Ptr p = c.makeClassType(instanceData);

	p = instanceData.handler->registerComplexTypeOrReturnExisting(p);

	return p;
}

snex::jit::ComplexType::Ptr TemplatedComplexType::createSubType(SubTypeConstructData* sd)
{
	auto id = sd->id;
	auto sl = sd->l;

	ComplexType::Ptr parentType = this;

	auto sId = TemplateInstance(id.relocate(id.getParent(), c.id.id), sd->l);
	TemplateObject s(sId);

	s.makeClassType = [parentType, id, sl](const TemplateObject::ConstructData& sc)
	{
		auto parent = dynamic_cast<TemplatedComplexType*>(parentType.get());

		auto parentType = parent->createTemplatedInstance(sc.tp, *sc.r);

		if (!sc.r->wasOk())
			return parentType;

		parentType = sc.handler->registerComplexTypeOrReturnExisting(parentType);

		SubTypeConstructData nsd;
		nsd.id = id;
		nsd.l = sl;
		nsd.handler = sc.handler;

		auto childType = parentType->createSubType(&nsd);

		return childType;
	};

	return new TemplatedComplexType(s, d);
}

}
}
