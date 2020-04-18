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
using namespace asmjit;

TemplateClassBuilder::TemplateClassBuilder(Compiler& compiler, const NamespacedIdentifier& id_) :
	id(id_),
	c(compiler)
{
}

void TemplateClassBuilder::flush()
{
	c.addTemplateClass(createTemplateObject());
}

void TemplateClassBuilder::addVariadicTypeTemplateParameter(const Identifier vTypeId)
{
	tp.add(TemplateParameter(id.getChildId(vTypeId), TypeInfo(), jit::TemplateParameter::Variadic));
}

void TemplateClassBuilder::addIntTemplateParameter(const Identifier& templateId)
{
	tp.add(TemplateParameter(id.getChildId(templateId), 0, false));
}

void TemplateClassBuilder::addTypeTemplateParameter(const Identifier& templateId)
{
	tp.add(TemplateParameter(id.getChildId(templateId), TypeInfo()));
}

void TemplateClassBuilder::addFunction(const FunctionBuilder& f)
{
	functionBuilders.add(f);
}

snex::jit::TemplateObject TemplateClassBuilder::createTemplateObject()
{
	TemplateObject to;
	to.id = id;
	to.argList = tp;

	auto l = tp;

	auto fCopy = functionBuilders;
	auto sCopy = initFunction;

	to.makeClassType = [l, fCopy, sCopy](const TemplateObject::ConstructData& cd)
	{
		ComplexType::Ptr ptr;

		if (!l.getLast().isVariadic() && !cd.expectTemplateParameterAmount(l.size()))
			return ptr;

		for (int i = 0; i < l.size(); i++)
		{
			if (l[i].t == TemplateParameter::IntegerTemplateArgument)
			{
				if (!cd.expectIsNumber(i))
					return ptr;
			}
			else
			{
				if (!cd.expectIsComplexType(i))
					return ptr;
			}
		}

		auto ip = TemplateParameter::ListOps::merge(l, cd.tp, *cd.r);

		if (!cd.r->wasOk())
			return ptr;

		auto st = new StructType(cd.id, ip);

		if(sCopy)
			sCopy(cd, st);

		for (const auto& f : fCopy)
		{
			auto fData = f(st);
			st->addJitCompiledMemberFunction(fData);
		}

		st->finaliseExternalDefinition();

		ptr = st;

		return ptr;
	};

	return to;
}

void TemplateClassBuilder::Helpers::addChildObjectPtr(StatementPtr newCall, SyntaxTreeInlineData* d, StructType* parentType, int memberIndex)
{
	auto pId = getMemberIdFromIndex(memberIndex);
	auto offset = parentType->getMemberOffset(pId);
	auto childType = parentType->getMemberComplexType(pId);

	auto obj = new Operations::MemoryReference(d->location, d->object, TypeInfo(childType, false, true), offset);
	dynamic_cast<Operations::FunctionCall*>(newCall.get())->setObjectExpression(obj);
}

snex::jit::TemplateClassBuilder::StatementPtr TemplateClassBuilder::Helpers::createBlock(SyntaxTreeInlineData* d)
{
	auto parentScope = Operations::findParentStatementOfType<Operations::ScopeStatementBase>(d->expression);
	auto blPath = d->location.createAnonymousScopeId(parentScope->getPath());
	return new Operations::StatementBlock(d->location, blPath);
}

juce::Identifier TemplateClassBuilder::Helpers::getMemberIdFromIndex(int index)
{
	String p = "_p" + String(index + 1);
	return Identifier(p);
}

snex::jit::TemplateClassBuilder::StatementPtr TemplateClassBuilder::Helpers::createFunctionCall(StructType* converterType, SyntaxTreeInlineData* d, const Identifier& functionId, StatementPtr input)
{
	auto f = getFunctionFromTargetClass(converterType, functionId);

	TemplateParameter::List tpToUse;

	if (TemplateParameter::ListOps::isArgument(f.templateParameters))
	{
		auto r = Result::ok();
		tpToUse = TemplateParameter::ListOps::merge(f.templateParameters, d->templateParameters, r);

		if (r.failed())
		{
			jassertfalse;
			return nullptr;
		}
	}
		
	else
		tpToUse = f.templateParameters;

	if (f.id.isValid())
	{
		auto exprCall = new Operations::FunctionCall(d->location, nullptr, { f.id, f.returnType }, tpToUse);
		exprCall->addArgument(input->clone(d->location));
		return exprCall;
	}
	else
		return nullptr;
}

snex::jit::StructType* TemplateClassBuilder::Helpers::getStructTypeFromTemplate(StructType* st, int index)
{
	return st->getTemplateInstanceParameters()[index].type.getTypedComplexType<StructType>();
}

snex::jit::FunctionData TemplateClassBuilder::Helpers::getFunctionFromTargetClass(StructType* targetType, const Identifier& id)
{
	FunctionClass::Ptr fc = targetType->getFunctionClass();
	auto fId = targetType->id.getChildId(id);
	return fc->getNonOverloadedFunction(fId);
}

snex::jit::ParameterBuilder ParameterBuilder::Helpers::createWithTP(Compiler& c, const Identifier& n)
{
	ParameterBuilder b(c, n);

	b.addTypeTemplateParameter("T");
	b.addIntTemplateParameter("P");

	return b;
}

snex::jit::FunctionData ParameterBuilder::Helpers::createCallPrototype(StructType* st, const Inliner::Func& highlevelFunc)
{
	FunctionData pFunc;

	pFunc.id = st->id.getChildId("call");
	pFunc.templateParameters.add(TemplateParameter(pFunc.id.getChildId("P_"), 0, false));
	pFunc.addArgs("value", TypeInfo(Types::ID::Double, false, false));
	pFunc.returnType = TypeInfo(Types::ID::Void, false, false);
	pFunc.inliner = Inliner::createHighLevelInliner(pFunc.id, highlevelFunc);

	return pFunc;
}

void ParameterBuilder::Helpers::initSingleParameterStruct(const TemplateObject::ConstructData& cd, StructType* st)
{
	auto ip = st->getTemplateInstanceParameters();

	auto targetType = ip[0].type.getTypedComplexType<StructType>();

	cd.handler->createTemplateFunction(targetType->id.getChildId("setParameter"), { ip[1] }, *cd.r);

	st->addMember("target", TypeInfo(Types::ID::Pointer, true, false));
	st->setDefaultValue("target", InitialiserList::makeSingleList(VariableStorage(nullptr, 8)));
}

snex::jit::Operations::Statement::Ptr ParameterBuilder::Helpers::createSetParameterCall(StructType* targetType, SyntaxTreeInlineData* d, Operations::Statement::Ptr input)
{
	auto newCall = TemplateClassBuilder::Helpers::createFunctionCall(targetType, d, "setParameter", input);

	// We have to manually dereference the member pointer here...
	auto obj = new Operations::MemoryReference(d->location, d->object, TypeInfo(targetType, false, true), 0);
	auto ptr = new Operations::PointerAccess(d->location, obj);

	dynamic_cast<Operations::FunctionCall*>(newCall.get())->setObjectExpression(ptr);

	return newCall;
}

snex::jit::FunctionData ParameterBuilder::Helpers::connectFunction(StructType* st)
{
	FunctionData cFunc;

	auto targetType = TemplateClassBuilder::Helpers::getStructTypeFromTemplate(st, 0);

	cFunc.id = st->id.getChildId("connect");
	cFunc.returnType = TypeInfo(Types::ID::Void, false, false);
	cFunc.addArgs("target", TypeInfo(targetType, false, true));

	cFunc.inliner = Inliner::createAsmInliner(cFunc.id, [targetType](InlineData* b)
	{
		auto d = b->toAsmInlineData();
		auto& cc = d->gen.cc;
		auto target = d->args[0];
		auto obj = d->object;

		auto mem = target->getAsMemoryLocation();

		if (mem.isNone())
			jassertfalse;

		auto tmp = cc.newGpq();
		cc.lea(tmp, mem);
		obj->loadMemoryIntoRegister(cc);
		cc.mov(x86::ptr(PTR_REG_R(obj)), tmp);
		return Result::ok();
	});

	return cFunc;
}

snex::jit::FunctionData TemplateClassBuilder::VariadicHelpers::getFunction(StructType* st)
{
	FunctionData getF;
	getF.id = st->id.getChildId("get");

	getF.templateParameters.add(TemplateParameter(getF.id.getChildId("Index")));

	getF.inliner = Inliner::createHighLevelInliner(getF.id, [st](InlineData* b)
	{
		auto d = b->toSyntaxTreeData();

		auto pId = Helpers::getMemberIdFromIndex(d->templateParameters.getFirst().constant);

		auto offset = st->getMemberOffset(pId);
		auto type = TypeInfo(st->getMemberComplexType(pId), false, true);
		auto base = dynamic_cast<Operations::Expression*>(d->object.get());

		d->target = new Operations::MemoryReference(d->location, base, type, offset);
		return Result::ok();
	});

	getF.inliner->returnTypeFunction = [st](InlineData* b)
	{
		auto rt = dynamic_cast<ReturnTypeInlineData*>(b);
		auto pId = Helpers::getMemberIdFromIndex(rt->templateParameters.getFirst().constant);
		auto t = st->getMemberComplexType(pId);

		if (t == nullptr)
			return Result::fail("Can't deduce type");

		rt->f.returnType = TypeInfo(t, false, true);

		return Result::ok();
	};

	return getF;
}

snex::jit::TemplateClassBuilder::StatementPtr TemplateClassBuilder::VariadicHelpers::callEachMember(SyntaxTreeInlineData* d, StructType* st, const Identifier& functionId, int offset/*=0*/)
{
	auto pList = st->getTemplateInstanceParameters();
	auto input = d->args[0]->clone(d->location);

	auto bl = Helpers::createBlock(d);

	for (int i = offset; i < pList.size(); i++)
	{
		auto childParameter = pList[i].type.getTypedComplexType<StructType>();
		auto newCall = Helpers::createFunctionCall(childParameter, d, functionId, input);
		Helpers::addChildObjectPtr(newCall, d, st, i);
		bl->addStatement(newCall);
	}

	return bl;
}

}
}