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


namespace snex 
{

using namespace juce;
USE_ASMJIT_NAMESPACE;

namespace jit
{

ParameterBuilder::ParameterBuilder(Compiler& c, const Identifier& id) :
	TemplateClassBuilder(c, NamespacedIdentifier("parameter").getChildId(id))
{
	initFunction = Helpers::initSingleParameterStruct;
}

snex::jit::ParameterBuilder ParameterBuilder::Helpers::createWithTP(Compiler& c, const Identifier& n)
{
	ParameterBuilder b(c, n);

	b.setConnectFunction();
	b.addTypeTemplateParameter("NodeType");
	b.addIntTemplateParameter("ParameterIndex");

	return b;
}

snex::jit::FunctionData ParameterBuilder::Helpers::createCallPrototype(StructType* st, const Inliner::Func& highlevelFunc)
{
	FunctionData pFunc;

	pFunc.id = st->id.getChildId("call");
	//pFunc.templateParameters.add(TemplateParameter(pFunc.id.getChildId("P_"), 0, false));
	pFunc.addArgs("value", TypeInfo(Types::ID::Double, false, false));
	pFunc.returnType = TypeInfo(Types::ID::Void, false, false);
	pFunc.inliner = Inliner::createHighLevelInliner(pFunc.id, highlevelFunc);

	return pFunc;
}

void ParameterBuilder::Helpers::initSingleParameterStruct(const TemplateObject::ConstructData& cd, StructType* st)
{
	auto ip = st->getTemplateInstanceParameters();

	WrapBuilder::InnerData resolver(ip[0].type.getTypedComplexType<StructType>(), WrapBuilder::GetSelfAsObject);

	if (resolver.resolve())
	{
		auto id = resolver.st->getTemplateInstanceId();
		id.id = id.id.getChildId("setParameter");

		cd.handler->createTemplateFunction(id, { ip[1] }, *cd.r);

		// Don't use the actual type, but a opaque pointer
		// or the initialisation will try to init the pointer with
		// the default values of the target class...
		st->addMember("target", TypeInfo(Types::ID::Pointer, true), {});
		st->setDefaultValue("target", InitialiserList::makeSingleList(VariableStorage(nullptr, 8)));
	}

	
}

snex::jit::Operations::Statement::Ptr ParameterBuilder::Helpers::createSetParameterCall(ComplexType::Ptr targetType, int parameterIndex, SyntaxTreeInlineData* d, Operations::Statement::Ptr input)
{
	StatementList exprArgs;
	exprArgs.add(input);

	if (auto newCall = TemplateClassBuilder::Helpers::createFunctionCall(targetType, d, "setParameter", exprArgs))
	{
		using namespace Operations;

		// We have to manually dereference the member pointer here...
		auto obj = new MemoryReference(d->location, d->object, TypeInfo(targetType, false, true), 0);
		auto ptr = new PointerAccess(d->location, obj);

		auto asFunction = as<FunctionCall>(newCall);

		asFunction->function.templateParameters.getReference(0).constant = parameterIndex;
		asFunction->setObjectExpression(ptr);

		return newCall;
	}

	return nullptr;
}

snex::jit::FunctionData ParameterBuilder::Helpers::connectFunction(StructType* st)
{
	FunctionData cFunc;

	auto targetType = TemplateClassBuilder::Helpers::getSubTypeFromTemplate(st, 0);

	WrapBuilder::InnerData id(dynamic_cast<StructType*>(targetType.get()), WrapBuilder::OpaqueType::GetSelfAsObject);

	if (id.resolve())
	{
		cFunc.id = st->id.getChildId("connect");
		cFunc.returnType = TypeInfo(Types::ID::Void, false, false);
		cFunc.templateParameters.add(TemplateParameter(cFunc.id.getChildId("Index"), 0, false));
		cFunc.addArgs("target", TypeInfo(Types::ID::Dynamic));

#if SNEX_ASMJIT_BACKEND
		cFunc.inliner = Inliner::createAsmInliner(cFunc.id, [](InlineData* b)
		{
			auto d = b->toAsmInlineData();

			if (d->templateParameters.size() == 0)
				return Result::fail("Missing template parameter for connect()");

			auto& cc = d->gen.cc;
			auto target = d->args[0];
			auto obj = d->object;

			auto mem = target->getMemoryLocationForReference();

			if (mem.isNone())
				jassertfalse;

			auto tmp = cc.newGpq();
			cc.lea(tmp, mem);
			obj->loadMemoryIntoRegister(cc);
			cc.mov(x86::ptr(PTR_REG_R(obj)), tmp);
			return Result::ok();
		});
#endif
	}

	return cFunc;
}

bool ParameterBuilder::Helpers::isParameterClass(const TypeInfo& type)
{
	return type.getTypedComplexType<StructType>()->id.getParent() == NamespacedIdentifier("parameter");
}

}

namespace Types {

Result ParameterLibraryBuilder::registerTypes()
{
	using PH = ParameterBuilder::Helpers;
	using TCH = TemplateClassBuilder::Helpers;

	auto empty = new StructType(NamespacedIdentifier("parameter").getChildId("empty"));

	auto emptyCallFunction = PH::createCallPrototype(empty, [](InlineData* b)
		{
			auto d = b->toSyntaxTreeData();

			d->target = new Operations::Noop(d->location);

			return Result::ok();
		});

	empty->addJitCompiledMemberFunction(emptyCallFunction);
	empty->finaliseAlignment();

	c.getNamespaceHandler().registerComplexTypeOrReturnExisting(empty);

	auto plain = PH::createWithTP(c, "plain");
	plain.setDescription("A plain parameter connection to a certain parameter index of a given node type. The value will be passed to the target without any conversion.");
	plain.addFunction([](StructType* st)
		{
			return PH::createCallPrototype(st, [st](InlineData* b)
				{
					auto d = b->toSyntaxTreeData();
					auto input = d->args[0]->clone(d->location);

					

					auto targetType = TCH::getSubTypeFromTemplate(st, 0);

					auto r = Result::ok();
					auto parameterType = TCH::getTemplateConstant(st, 1, r);
					d->location.test(r);

					WrapBuilder::InnerData id(targetType.get(), WrapBuilder::OpaqueType::GetSelfAsObject);

					if (id.resolve())
					{
						d->target = PH::createSetParameterCall(id.st, parameterType, d, input);
					}

					return id.getResult();
				});
		});

	plain.flush();

	auto expr = PH::createWithTP(c, "expression");
	expr.setDescription("A parameter with an expression that is evaluated before sending the value to the destination.  \nThe expression class must have a function `static double ExpressionClass::op(double input);`");
	expr.addTypeTemplateParameter("ExpressionClass");
	expr.addFunction([](StructType* st)
		{
			return PH::createCallPrototype(st, [st](InlineData* b)
				{
					auto d = b->toSyntaxTreeData();
					auto exprType = TCH::getSubTypeFromTemplate(st, 2);
					auto exprCall = TCH::createFunctionCall(exprType, d, "op", d->args);
					auto targetType = TCH::getSubTypeFromTemplate(st, 0);

					auto r = Result::ok();
					auto parameterIndex = TCH::getTemplateConstant(st, 1, r);

					d->target = PH::createSetParameterCall(targetType, parameterIndex, d, exprCall);

					return r;
				});
		});
	expr.flush();

	auto from0To1 = PH::createWithTP(c, "from0To1");
	from0To1.addTypeTemplateParameter("RangeClass");
	from0To1.setDescription("A parameter that converts a normalised value to a given range before sending it to the destination. The RangeClass must have a `static double from0To1(double input);` method.");

	from0To1.addFunction([](StructType* st)
		{
			return PH::createCallPrototype(st, [st](InlineData* b)
				{
					auto d = b->toSyntaxTreeData();
					auto rangeType = TCH::getSubTypeFromTemplate(st, 2);
					auto rangeCall = TCH::createFunctionCall(rangeType, d, "from0To1", d->args);

					if (rangeCall == nullptr)
						return Result::fail("from0To1 not found");

					auto targetType = TCH::getSubTypeFromTemplate(st, 0);

					auto r = Result::ok();

					auto parameterIndex = TCH::getTemplateConstant(st, 1, r);
					d->target = PH::createSetParameterCall(targetType, parameterIndex, d, rangeCall);

					return r;
				});
		});
	from0To1.flush();

	auto to0To1 = PH::createWithTP(c, "to0To1");
	to0To1.addTypeTemplateParameter("RangeClass");
	to0To1.setDescription("A parameter connection that sends a normalised value to the target. The input value will be scaled based on the Range class which needs a `static double to0To1(double input);` method.");
	to0To1.addFunction([](StructType* st)
		{
			return PH::createCallPrototype(st, [st](InlineData* b)
				{
					auto d = b->toSyntaxTreeData();
					auto input = d->args[0]->clone(d->location);

					auto rangeType = TCH::getSubTypeFromTemplate(st, 2);
					auto rangeCall = TCH::createFunctionCall(rangeType, d, "to0To1", d->args);
					auto targetType = TCH::getSubTypeFromTemplate(st, 0);

					auto r = Result::ok();
					auto parameterIndex = TCH::getTemplateConstant(st, 1, r);
					d->location.test(r);
					d->target = PH::createSetParameterCall(targetType, parameterIndex, d, rangeCall);

					return Result::ok();
				});
		});
	to0To1.flush();

	ParameterBuilder chainP(c, "chain");

	chainP.addTypeTemplateParameter("InputRange");
	chainP.setDescription("A parameter connection to multiple targets. The `Parameters` argument can be a list of other parameter classes.  \nThe input value will be normalised using the `InputRange` class, so you most probably want to scale the values back using `parameter::from0To1` (or a custom scaling using `parameter::expression`).");
	chainP.addVariadicTypeTemplateParameter("Parameters");
	chainP.addFunction(TemplateClassBuilder::VariadicHelpers::getFunction);
	chainP.setInitialiseStructFunction(TemplateClassBuilder::VariadicHelpers::initVariadicMembers<1>);

	chainP.setConnectFunction([](StructType* st)
		{
			FunctionData cFunc;

			cFunc.id = st->id.getChildId("connect");
			cFunc.returnType = TypeInfo(Types::ID::Void, false, false);
			cFunc.templateParameters.add(TemplateParameter(cFunc.id.getChildId("Index"), 0, false));
			cFunc.addArgs("target", TypeInfo(Types::ID::Dynamic, false, true));

			cFunc.inliner = Inliner::createHighLevelInliner(cFunc.id, [st](InlineData* b)
				{
					auto d = b->toSyntaxTreeData();



					int parameterIndex = d->templateParameters.getFirst().constant;
					auto targetType = TCH::getSubTypeFromTemplate(st, parameterIndex + 1);
					auto newCall = TCH::createFunctionCall(targetType, d, "connect", d->args);
					TCH::addChildObjectPtr(newCall, d, st, parameterIndex);

					d->target = newCall;
					return Result::ok();
				});

			return cFunc;
		});


	chainP.addFunction([](StructType* st)
	{
		return PH::createCallPrototype(st, [st](InlineData* b)
		{
			auto d = b->toSyntaxTreeData();

			auto rangeType = TCH::getSubTypeFromTemplate(st, 0);
			auto rangeCall = TCH::createFunctionCall(rangeType, d, "to0To1", d->args);

			if (rangeCall == nullptr)
				d->location.throwError("Can't find function " + rangeType->toString() + "::to0To1(double)");

			d->args.set(0, rangeCall);
			d->target = TemplateClassBuilder::VariadicHelpers::callEachMember(d, st, "call", 1);

			return Result::ok();
		});
	});
	chainP.flush();

	ParameterBuilder listP(c, "list");

	listP.addVariadicTypeTemplateParameter("Parameters");
	listP.addFunction(TemplateClassBuilder::VariadicHelpers::getFunction);
	listP.setInitialiseStructFunction(TemplateClassBuilder::VariadicHelpers::initVariadicMembers<0>);
	listP.addFunction([](StructType* st)
		{
			return PH::createCallPrototype(st, [st](InlineData* b)
				{
					auto d = b->toSyntaxTreeData();

					auto value = d->templateParameters.getFirst().constant;
					auto targetType = TCH::getSubTypeFromTemplate(st, value);
					auto newCall = TCH::createFunctionCall(targetType, d, "call", d->args);
					TCH::addChildObjectPtr(newCall, d, st, value);

					d->target = newCall;

					return Result::ok();
				});
		});
	listP.flush();

	return Result::ok();
}

}



}
