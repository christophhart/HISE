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
	TemplateObject to({ id, {} });
	to.argList = tp;
	to.description = description;

	auto l = tp;

	auto fCopy = functionBuilders;

	Array<InitialiseStructFunction> initFunctions;

	if(initFunction)
		initFunctions.add(initFunction);

	initFunctions.addArray(additionalInitFunctions);

	to.makeClassType = [l, fCopy, initFunctions](const TemplateObject::ConstructData& cd)
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

		auto st = new StructType(cd.id.id, ip);

		for (const auto& f : initFunctions)
		{
			f(cd, st);

			if (!cd.r->wasOk())
				return ptr;
		}

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
	auto pId = VariadicHelpers::getVariadicMemberIdFromIndex(memberIndex);
	auto offset = parentType->getMemberOffset(pId);
	auto childType = parentType->getMemberComplexType(pId);

	auto obj = new Operations::MemoryReference(d->location, d->object, TypeInfo(childType, false, true), offset);
	dynamic_cast<Operations::FunctionCall*>(newCall.get())->setObjectExpression(obj);
}

snex::jit::TemplateClassBuilder::StatementPtr TemplateClassBuilder::Helpers::createBlock(SyntaxTreeInlineData* d)
{
	auto parentScope = Operations::findParentStatementOfType<Operations::ScopeStatementBase>(d->expression);
	auto blPath = d->expression->currentCompiler->namespaceHandler.createNonExistentIdForLocation(parentScope->getPath(), d->location.getLine());
	return new Operations::StatementBlock(d->location, blPath);
}

snex::jit::TemplateClassBuilder::StatementPtr TemplateClassBuilder::Helpers::createFunctionCall(ComplexType::Ptr converterType, SyntaxTreeInlineData* d, const Identifier& functionId, StatementList originalArgs)
{
	jassert(converterType != nullptr);
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

		for (auto a : originalArgs)
		{
			exprCall->addArgument(a->clone(d->location));
		}
		
		return exprCall;
	}
	else
		return nullptr;
}

ComplexType::Ptr TemplateClassBuilder::Helpers::getSubTypeFromTemplate(StructType* st, int index)
{
	return st->getTemplateInstanceParameters()[index].type.getComplexType();
}

snex::jit::FunctionData TemplateClassBuilder::Helpers::getFunctionFromTargetClass(ComplexType::Ptr targetType, const Identifier& id)
{
	FunctionClass::Ptr fc = targetType->getFunctionClass();
	auto fId = fc->getClassName().getChildId(id);
	return fc->getNonOverloadedFunction(fId);
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

	auto id = targetType->getTemplateInstanceId();
	id.id = id.id.getChildId("setParameter");

	cd.handler->createTemplateFunction(id, { ip[1] }, *cd.r);

	st->addMember("target", TypeInfo(Types::ID::Pointer, true, false), {});
	st->setDefaultValue("target", InitialiserList::makeSingleList(VariableStorage(nullptr, 8)));
}

snex::jit::Operations::Statement::Ptr ParameterBuilder::Helpers::createSetParameterCall(ComplexType::Ptr targetType, SyntaxTreeInlineData* d, Operations::Statement::Ptr input)
{
	StatementList exprArgs;
	exprArgs.add(input);

	auto newCall = TemplateClassBuilder::Helpers::createFunctionCall(targetType, d, "setParameter", exprArgs);

	// We have to manually dereference the member pointer here...
	auto obj = new Operations::MemoryReference(d->location, d->object, TypeInfo(targetType, false, true), 0);
	auto ptr = new Operations::PointerAccess(d->location, obj);

	dynamic_cast<Operations::FunctionCall*>(newCall.get())->setObjectExpression(ptr);

	return newCall;
}

snex::jit::FunctionData ParameterBuilder::Helpers::connectFunction(StructType* st)
{
	FunctionData cFunc;

	auto targetType = TemplateClassBuilder::Helpers::getSubTypeFromTemplate(st, 0);

	cFunc.id = st->id.getChildId("connect");
	cFunc.returnType = TypeInfo(Types::ID::Void, false, false);
	cFunc.templateParameters.add(TemplateParameter(cFunc.id.getChildId("Index"), 0, false));
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

bool ParameterBuilder::Helpers::isParameterClass(const TypeInfo& type)
{
	return type.getTypedComplexType<StructType>()->id.getParent() == NamespacedIdentifier("parameter");
}

snex::jit::FunctionData TemplateClassBuilder::VariadicHelpers::getFunction(StructType* st)
{
	FunctionData getF;
	getF.id = st->id.getChildId("get");

	getF.templateParameters.add(TemplateParameter(getF.id.getChildId("Index")));

	getF.inliner = Inliner::createHighLevelInliner(getF.id, [st](InlineData* b)
	{
		auto d = b->toSyntaxTreeData();

		auto pId = getVariadicMemberIdFromIndex(d->templateParameters.getFirst().constant);

		auto offset = st->getMemberOffset(pId);
		auto type = TypeInfo(st->getMemberComplexType(pId), false, true);
		auto base = dynamic_cast<Operations::Expression*>(d->object.get());

		d->target = new Operations::MemoryReference(d->location, base, type, offset);
		return Result::ok();
	});

	getF.inliner->returnTypeFunction = [st](InlineData* b)
	{
		auto rt = dynamic_cast<ReturnTypeInlineData*>(b);
		auto pId = getVariadicMemberIdFromIndex(rt->templateParameters.getFirst().constant);
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

	auto bl = Helpers::createBlock(d);

	Operations::Statement::List processedArgs;

	for (auto arg : d->args)
	{
		if (auto f = Operations::as<Operations::FunctionCall>(arg))
		{
			auto bPath = Operations::as<Operations::StatementBlock>(bl)->getPath();

			auto index = d->args.indexOf(arg);

			String s = "arg" + String(index + 1);

			auto vId = Symbol(bPath.getChildId(Identifier(s)), arg->getTypeInfo());

			auto v = new Operations::VariableReference(d->location, vId);

			auto as = new Operations::Assignment(d->location, v, JitTokens::assign_, arg->clone(d->location), true);

			bl->addStatement(as, false);

			processedArgs.add(v->clone(d->location));
		}
		else
		{
			processedArgs.add(arg->clone(d->location));
		}
	}


	for (int i = offset; i < pList.size(); i++)
	{
		auto childParameter = pList[i].type.getComplexType();
		auto newCall = Helpers::createFunctionCall(childParameter, d, functionId, processedArgs);

		if (newCall == nullptr)
		{
			String s;

			s << childParameter->toString() << " does not have a method " << functionId;

			d->location.throwError(s);
		}

		

		Helpers::addChildObjectPtr(newCall, d, st, i-offset);
		bl->addStatement(newCall);
	}

	return bl;
}

WrapBuilder::WrapBuilder(Compiler& c, const Identifier& id, int numChannels):
	TemplateClassBuilder(c, NamespacedIdentifier("wrap").getChildId(id))
{
	addTypeTemplateParameter("ObjectClass");
	
	

	setInitialiseStructFunction([&c, numChannels](const TemplateObject::ConstructData& cd, StructType* st)
	{
		auto pType = TemplateClassBuilder::Helpers::getSubTypeFromTemplate(st, 0);
		st->addMember("obj", TypeInfo(pType, false, false));

		auto prototypes = Types::ScriptnodeCallbacks::getAllPrototypes(c, numChannels);

		for (auto p : prototypes)
			st->addWrappedMemberMethod("obj", p);

		TemplateObject to(st->getTemplateInstanceId());
		
		to.argList.add(TemplateParameter(to.id.id.getChildId("P"), 0, false));
		to.makeFunction = [st](const TemplateObject::ConstructData& cd_)
		{
			FunctionData f;
			f.id = cd_.id.id;
			f.addArgs("newValue", TypeInfo(Types::ID::Double));
			f.returnType = TypeInfo(Types::ID::Void);
			f.templateParameters.addArray(cd_.tp);

			f.inliner = Inliner::createHighLevelInliner(f.id, [](InlineData* b)
			{
				using namespace Operations;
				auto d = b->toSyntaxTreeData();
				auto st = d->object->getTypeInfo().getTypedComplexType<StructType>();
				auto objType = st->getMemberTypeInfo("obj").getTypedComplexType<StructType>();

				Symbol fSymbol(objType->id.getChildId("setParameter"), TypeInfo(Types::ID::Void));

				auto r = Result::ok();
				auto& h = d->object->currentScope->getNamespaceHandler();
				
				auto id = objType->getTemplateInstanceId().getChildIdWithSameTemplateParameters("setParameter");

				h.createTemplateFunction(id, d->templateParameters, r);

				if (r.failed())
					return r;

				auto newCall = new FunctionCall(d->location, nullptr, fSymbol, d->templateParameters);
				auto ref = new MemoryReference(d->location, d->object->clone(d->location), TypeInfo(objType), 0);
				newCall->setObjectExpression(ref);
				newCall->addArgument(d->args[0]->clone(d->location));
				d->target = newCall;

				return Result::ok();
			});

			st->addJitCompiledMemberFunction(f);
		};

		to.functionArgs = []()
		{
			TypeInfo::List callParameters;
			callParameters.add(TypeInfo(Types::ID::Double));
			return callParameters;
		};

		cd.handler->addTemplateFunction(to);
	});

}



ContainerNodeBuilder::ContainerNodeBuilder(Compiler& c, const Identifier& id, int numChannels_) :
	TemplateClassBuilder(c, NamespacedIdentifier("container").getChildId(id)),
	numChannels(numChannels_)
{
	addTypeTemplateParameter("ParameterClass");
	addVariadicTypeTemplateParameter("ProcessorTypes");
	addFunction(TemplateClassBuilder::VariadicHelpers::getFunction);

	setInitialiseStructFunction([](const TemplateObject::ConstructData& cd, StructType* st)
	{
		auto pType = TemplateClassBuilder::Helpers::getSubTypeFromTemplate(st, 0);

		if (!ParameterBuilder::Helpers::isParameterClass(TypeInfo(pType)))
		{
			String s;
			s << "Expected parameter class at Index 0: " << pType->toString();
			*cd.r = Result::fail(s);
		}

		st->addMember("parameters", TypeInfo(pType, false, false));
	});

	addInitFunction(TemplateClassBuilder::VariadicHelpers::initVariadicMembers<1>);

	addFunction(Helpers::getParameterFunction);
	addFunction(Helpers::setParameterFunction);

	callbacks = snex::Types::ScriptnodeCallbacks::getAllPrototypes(c, numChannels);

	for (auto& cf : callbacks)
		cf.inliner = Inliner::createHighLevelInliner(cf.id, Helpers::defaultForwardInliner);
}

void ContainerNodeBuilder::addHighLevelInliner(const Identifier& functionId, const Inliner::Func& inliner)
{
	jassert(isScriptnodeCallback(functionId));

	for (auto& cf : callbacks)
	{
		if (cf.id.id == functionId)
		{
			cf.inliner = Inliner::createHighLevelInliner(id.getChildId(functionId), inliner);
			break;
		}
	}
}

void ContainerNodeBuilder::addAsmInliner(const Identifier& functionId, const Inliner::Func& inliner)
{
	jassert(isScriptnodeCallback(functionId));

	for (auto& cf : callbacks)
	{
		if (cf.id.id == functionId)
		{
			cf.inliner = Inliner::createAsmInliner(id.getChildId(functionId), inliner);
			break;
		}
	}
}

void ContainerNodeBuilder::deactivateCallback(const Identifier& id)
{
	addHighLevelInliner(id, [](InlineData* b)
	{
		auto d = b->toSyntaxTreeData();
		d->target = new Operations::Noop(d->location);
		return Result::ok();
	});
}

void ContainerNodeBuilder::flush()
{
	description << "\n#### Template Parameters:\n";
	description << "- **ParameterClass**: a class template (parameter::xxx) that defines the parameter connections\n";
	description << "- **ProcessorTypes**: a dynamic list of nodes that are processed.\n";



	/** TODO
	
		- fix get<0>() offset
		- make setParameter() && connect work.
		- add integer template parameter to connect
	*/

	jassert(numChannels != -1);

#if 0
	addFunction([](StructType* st)
	{
		FunctionData cData;
		cData.id = st->id.getChildId("connect");
		cData.returnType = TypeInfo(Types::ID::Void);
		cData.addArgs("obj", TypeInfo());
		cData.templateParameters.add(TemplateParameter(cData.id.getChildId("P"), 0, false));

		auto parameterClass = TemplateClassBuilder::Helpers::getStructTypeFromTemplate(st, 0);

		auto il = TemplateClassBuilder::Helpers::getFunctionFromTargetClass(parameterClass, "connect").inliner;

		cData.inliner = Inliner::createAsmInliner(cData.id, [il](InlineData* b)
		{
			auto d = b->toAsmInlineData();
			auto st = Helpers::getStructTypeFromInlineData(b);
			auto pClass = TemplateClassBuilder::Helpers::getStructTypeFromTemplate(st, 0);

			AsmInlineData copy(*d);

			copy.templateParameters = {};

			return il->process(&copy);
		});

		return cData;
	});

	addFunction([](StructType* st)
	{
		FunctionData pFunc;
		pFunc.id = st->id.getChildId("setParameter");
		pFunc.returnType = TypeInfo(Types::ID::Void);
		pFunc.addArgs("value", TypeInfo(Types::ID::Double));
		pFunc.templateParameters.add(TemplateParameter(pFunc.id.getChildId("P"), 0, false));

		auto parameterClass = TemplateClassBuilder::Helpers::getStructTypeFromTemplate(st, 0);
		auto il = TemplateClassBuilder::Helpers::getFunctionFromTargetClass(parameterClass, "setParameter").inliner;

		pFunc.inliner = Inliner::createHighLevelInliner(pFunc.id, [il](InlineData* b)
		{

			jassertfalse;
			return Result::ok();
		});

		return pFunc;
	});
#endif


	for (auto cf : callbacks)
	{
		addFunction([cf](StructType* st)
		{
			return cf;
		});
	}

	TemplateClassBuilder::flush();
}

bool ContainerNodeBuilder::isScriptnodeCallback(const Identifier& id) const
{
	for (auto f : callbacks)
	{
		if (f.id.id == id)
			return true;
	}

	return false;
}

juce::Result ContainerNodeBuilder::Helpers::defaultForwardInliner(InlineData* b)
{
	auto d = b->toSyntaxTreeData();
	auto st = getStructTypeFromInlineData(b);
	auto id = getFunctionIdFromInlineData(b);

	constexpr int ParameterOffset = 1;

	d->target = TemplateClassBuilder::VariadicHelpers::callEachMember(d, st, id, ParameterOffset);
	return Result::ok();
}

snex::jit::StructType* ContainerNodeBuilder::Helpers::getStructTypeFromInlineData(InlineData* b)
{
	if (b->isHighlevel())
	{
		auto d = b->toSyntaxTreeData();
		return d->object->getTypeInfo().getTypedComplexType<StructType>();
	}
	else
	{
		auto d = b->toAsmInlineData();
		return d->object->getTypeInfo().getTypedComplexType<StructType>();
	}

	
}

juce::Identifier ContainerNodeBuilder::Helpers::getFunctionIdFromInlineData(InlineData* b)
{
	auto d = b->toSyntaxTreeData();
	return Operations::as<Operations::FunctionCall>(d->expression)->function.id.id;
}

snex::jit::FunctionData ContainerNodeBuilder::Helpers::getParameterFunction(StructType* st)
{
	FunctionData gf;
	gf.id = st->id.getChildId("getParameter");
	gf.returnType = TypeInfo(Types::ID::Dynamic);
	gf.templateParameters.add(TemplateParameter(gf.id.getChildId("Index"), 0, false));

	gf.inliner = Inliner::createHighLevelInliner(gf.id, [st](InlineData* b)
	{
		auto d = b->toSyntaxTreeData();

		int index;
		StructType* parameterType;

		ParameterBuilder::Helpers::forwardToListElements(st, d->templateParameters, &parameterType, index);

		int offset = ParameterBuilder::Helpers::getParameterListOffset(st, index);

		jassert(ParameterBuilder::Helpers::isParameterClass(TypeInfo(parameterType, false, true)));

		

		d->target = new Operations::MemoryReference(d->location, d->object, TypeInfo(parameterType, false, true), offset);

		return Result::ok();
	});

	gf.inliner->returnTypeFunction = [st](InlineData* b)
	{
		auto rt = dynamic_cast<ReturnTypeInlineData*>(b);

		int index;
		StructType* parameterType;

		ParameterBuilder::Helpers::forwardToListElements(st, rt->templateParameters, &parameterType, index);

		jassert(ParameterBuilder::Helpers::isParameterClass(TypeInfo(parameterType, false, true)));

		rt->f.returnType = TypeInfo(parameterType, false, true);
		return Result::ok();
	};

	return gf;
}

snex::jit::FunctionData ContainerNodeBuilder::Helpers::setParameterFunction(StructType* st)
{
	FunctionData sf;
	sf.id = st->id.getChildId("setParameter");
	sf.returnType = TypeInfo(Types::ID::Void, false, false);
	sf.addArgs("value", TypeInfo(Types::ID::Double));
	sf.templateParameters.add(TemplateParameter(sf.id.getChildId("P"), 0, false));

	sf.inliner = Inliner::createHighLevelInliner(sf.id, [st](InlineData* b)
	{
		auto d = b->toSyntaxTreeData();

		int index;
		StructType* parameterType;

		ParameterBuilder::Helpers::forwardToListElements(st, d->templateParameters, &parameterType, index);

		int offset = ParameterBuilder::Helpers::getParameterListOffset(st, index);

		jassert(ParameterBuilder::Helpers::isParameterClass(TypeInfo(parameterType, false, true)));

		auto newCall = TemplateClassBuilder::Helpers::createFunctionCall(parameterType, d, "call", d->args);

		auto obj = new Operations::MemoryReference(d->location, d->object, TypeInfo(parameterType, false, true), offset);

		dynamic_cast<Operations::FunctionCall*>(newCall.get())->setObjectExpression(obj);

		d->target = newCall;

		return Result::ok();
	});

	return sf;
}

}
}