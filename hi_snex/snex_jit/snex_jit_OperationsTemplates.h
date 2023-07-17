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
USE_ASMJIT_NAMESPACE;




struct Operations::TemplateDefinition : public Statement,
	public ClassDefinitionBase
{
	SET_EXPRESSION_ID(TemplateDefinition);

	TemplateDefinition(Location l, const TemplateInstance& classId, NamespaceHandler& handler_, Statement::Ptr statements_) :
		Statement(l),
		templateClassId(classId),
		statements(statements_),
		handler(handler_)
	{
#if JUCE_DEBUG
		for (const auto& p : getTemplateArguments())
			jassert(p.isTemplateArgument());
#endif
	}

	void process(BaseCompiler* compiler, BaseScope* scope) override;

	ValueTree toValueTree() const override
	{
		auto t = Statement::toValueTree();

		juce::String s;
		s << templateClassId.toString();
		s << TemplateParameter::ListOps::toString(getTemplateArguments());

		t.setProperty("Type", s, nullptr);

		return t;
	}

	bool isTemplate() const override
	{
		return true;
	}

	TemplateParameter::List getTemplateArguments() const
	{
		return handler.getTemplateObject(templateClassId).argList;
	}

	Statement::Ptr clone(Location l) const override
	{
		auto cs = statements->clone(l);
		auto s = new TemplateDefinition(l, templateClassId, handler, cs);
		clones.add(s);
		return s;
	}

	TypeInfo getTypeInfo() const override
	{
		return {};
	}

	/** This will be called by the parser to create a new object.

		It will add a ClassStatement as child statement and return the created
		ComplexType.
	*/
	ComplexType::Ptr createTemplate(const TemplateObject::ConstructData& d);

	TemplateInstance templateClassId;
	NamespaceHandler& handler;
	Statement::Ptr statements;
	mutable List clones;
};


struct Operations::TemplatedFunction : public Statement,
	public FunctionDefinitionBase
{
	SET_EXPRESSION_ID(TemplatedFunction);

	TemplatedFunction(Location l, const Symbol& s, const TemplateParameter::List& tp) :
		Statement(l),
		FunctionDefinitionBase(s),
		templateParameters(tp)
	{
		for (auto& l : templateParameters)
		{
			jassert(l.isTemplateArgument());

			if (!s.id.isParentOf(l.argumentId))
			{
				jassert(l.argumentId.isExplicit());
				l.argumentId = s.id.getChildId(l.argumentId.getIdentifier());
			}
		}
	}

	void createFunction(const TemplateObject::ConstructData& d);

	Ptr clone(Location l) const override
	{
		Ptr f = new TemplatedFunction(l, { data.id, data.returnType }, templateParameters);
		as<TemplatedFunction>(f)->parameters = parameters;
		as<TemplatedFunction>(f)->code = code;
		as<TemplatedFunction>(f)->codeLength = codeLength;
		as<TemplatedFunction>(f)->data.args.addArray(data.args);

		cloneChildren(f);

		clones.add(f);

		return f;
	}

	ValueTree toValueTree() const override
	{
		return Statement::toValueTree();
	}

	TypeInfo getTypeInfo() const override { return {}; }

	Function* getFunctionWithTemplateAmount(const NamespacedIdentifier& id, int numTemplateParameters);

	Statement::List collectFunctionInstances();

	void process(BaseCompiler* compiler, BaseScope* scope) override;

	TemplateParameter::List templateParameters;

	mutable Statement::List clones;
};

}
}