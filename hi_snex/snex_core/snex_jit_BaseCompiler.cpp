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

    void BaseCompiler::executeOptimization(ReferenceCountedObject* statement, BaseScope* scope)
    {
        if(currentOptimization == nullptr)
            return;
        
        Operations::Statement::Ptr ptr(dynamic_cast<Operations::Statement*>(statement));
        
		if (dynamic_cast<OptimizationPass*>(currentOptimization)->processStatementInternal(this, scope, ptr))
		{
			throw BaseCompiler::OptimisationSucess();
		}
    }

	void BaseCompiler::optimize(ReferenceCountedObject* statement, BaseScope* scope, bool useExistingPasses)
	{
		OwnedArray<OptimizationPassBase> constExprPasses;

		OwnedArray<OptimizationPassBase>* toUse = nullptr;

		if (useExistingPasses)
		{
			toUse = &passes;
		}
		else
		{
			OptimizationFactory f;

			Array<Identifier> optList = { OptimizationIds::BinaryOpOptimisation, OptimizationIds::ConstantFolding };

			for (const auto& id : optList)
				constExprPasses.add(f.createOptimization(id));

			toUse = &constExprPasses;
		}

		Operations::Statement::Ptr ptr(dynamic_cast<Operations::Statement*>(statement));

		bool noMoreOptimisationsPossible = false;

		while (!noMoreOptimisationsPossible)
		{
			try
			{
				for (auto o : *toUse)
				{
					currentOptimization = o;
					ptr->process(this, scope);
				}

				noMoreOptimisationsPossible = true;
			}
			catch (OptimisationSucess& )
			{
				if (ptr->parent == nullptr)
				{
					return;
				}
				
				if(useExistingPasses)
					logMessage(MessageType::VerboseProcessMessage, "Repeat optimizations");
			}
		}
	}


	juce::StringArray BaseCompiler::getOptimizations() const
	{
		StringArray sa;
		for (auto o : optimisationIds)
			sa.add(o.toString());

		return sa;
	}

	snex::Types::ID BaseCompiler::getRegisterType(const TypeInfo& t) const
	{

		return t.getRegisterType(allowSmallObjectOptimisation());
	}

	snex::jit::TypeInfo BaseCompiler::convertToNativeTypeIfPossible(const TypeInfo& t) const
	{
		auto nt = t.getRegisterType(allowSmallObjectOptimisation());

		if (nt == Types::ID::Pointer)
			return t;
		else
			return TypeInfo(nt, t.isConst(), t.isRef());
	}

	bool BaseCompiler::fitsIntoNativeRegister(ComplexType* t) const
	{
		TypeInfo info(t);
		return getRegisterType(info) != Types::ID::Pointer;
	}

	bool BaseCompiler::allowSmallObjectOptimisation() const
	{
		return optimisationIds.contains(OptimizationIds::SmallObjectOptimisation);
	}

	void BaseCompiler::setInbuildFunctions()
	{
		inbuildFunctions = new InbuiltFunctions(this);
	}

	snex::jit::MathFunctions& BaseCompiler::getMathFunctionClass()
	{
		if (mathFunctions == nullptr)
		{
			auto bType = namespaceHandler.getComplexType(NamespacedIdentifier("block"));
			mathFunctions = new MathFunctions(false, bType);
		}
		
		return *dynamic_cast<MathFunctions*>(mathFunctions.get());
	}

	BaseCompiler::BaseCompiler(NamespaceHandler& handler) :
		namespaceHandler(handler),
		registerPool(this)
	{
		TemplateObject spanClass({NamespacedIdentifier("span"), {}});
		auto sId = spanClass.id;

		NamespaceHandler::InternalSymbolSetter iss(handler);

		spanClass.id = sId;
		spanClass.argList.add(TemplateParameter(sId.id.getChildId("DataType")));
		spanClass.argList.add(TemplateParameter(sId.id.getChildId("NumElements"), 0, false));

		spanClass.makeClassType = [](const TemplateObject::ConstructData& d)
		{
			ComplexType::Ptr p;

			if (!d.expectTemplateParameterAmount(2))
				return p;

			if (!d.expectType(0))
				return p;

			if (!d.expectIsNumber(1))
				return p;

			p = new SpanType(d.tp[0].type, d.tp[1].constant);

			return p;
		};
		namespaceHandler.addTemplateClass(spanClass);

		TemplateObject dynClass({ NamespacedIdentifier("dyn"), {}});
		auto dId = dynClass.id;
		dynClass.argList.add(TemplateParameter(dId.id.getChildId("DataType")));

		dynClass.makeClassType = [](const TemplateObject::ConstructData& d)
		{
			ComplexType::Ptr p;

			if (!d.expectTemplateParameterAmount(1))
				return p;

			if (!d.expectType(0))
				return p;

			p = new DynType(d.tp[0].type);

			return p;
		};
		namespaceHandler.addTemplateClass(dynClass);



		auto float4Type = new SpanType(TypeInfo(Types::ID::Float), 4);
		float4Type->setAlias(NamespacedIdentifier("float4"));
		namespaceHandler.registerComplexTypeOrReturnExisting(float4Type);
	}

	void BaseCompiler::executePass(Pass p, BaseScope* scope, ReferenceCountedObject* statement)
    {
		if (scope->getGlobalScope()->getBreakpointHandler().shouldAbort())
			jassertfalse;

		auto st = dynamic_cast<Operations::Statement*>(statement);

		if (isOptimizationPass(p) && passes.isEmpty())
			return;

		setCurrentPass(p);

		if (isOptimizationPass(p))
		{
			for (int i = 0; i < st->getNumChildStatements(); i++)
			{
				auto s = st->getChildStatement(i);

				for (auto o : passes)
					o->reset();

				optimize(s.get(), scope, true);
				
				
				st->removeNoops();
			}

			st->currentPass = p;
		}
		else
			st->process(this, scope);
    }

	void BaseCompiler::executeScopedPass(Pass p, BaseScope* scope, ReferenceCountedObject* statement)
	{
		BaseCompiler::ScopedPassSwitcher sps(this, p);
		executePass(p, scope, statement);
	}

}
}
