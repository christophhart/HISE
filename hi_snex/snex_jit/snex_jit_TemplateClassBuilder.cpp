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

TemplateClassBuilder::TemplateClassBuilder(Compiler& compiler, const NamespacedIdentifier& id_) :
	id(id_),
	c(compiler)
{
	addInitFunction([&compiler](const TemplateObject::ConstructData& cd, StructType* st)
	{
		st->setCompiler(compiler);
	});
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

void TemplateClassBuilder::addIntTemplateParameterWithDefault(const Identifier& templateId, int defaultValue)
{
	tp.add(TemplateParameter(id.getChildId(templateId), defaultValue, true));
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

	Array<InitialiseStructFunction> postbF;

	postbF.addArray(postFunctionBuilderFunctions);

	to.makeClassType = [l, fCopy, initFunctions, postbF](const TemplateObject::ConstructData& cd2)
	{
		ComplexType::Ptr ptr;

		TemplateObject::ConstructData toUse = cd2;

		if (!l.getLast().isVariadic() && toUse.tp.size() < l.size())
		{
			int numDefined = toUse.tp.size();
			int numExpected = l.size();

			for (int i = numDefined; i < numExpected; i++)
			{
				auto argumentFromTemplateBuilder = l[i];

				if (argumentFromTemplateBuilder.t == TemplateParameter::IntegerTemplateArgument)
				{
					jassert(argumentFromTemplateBuilder.constantDefined);
					toUse.tp.set(i, TemplateParameter(argumentFromTemplateBuilder.constant));
				}
				else
				{
					jassert(argumentFromTemplateBuilder.type.isValid());
					toUse.tp.set(i, TemplateParameter(argumentFromTemplateBuilder.type));
				}
			}
		}

		for (int i = 0; i < l.size(); i++)
		{
			if (l[i].t == TemplateParameter::IntegerTemplateArgument)
			{
				if (!toUse.expectIsNumber(i))
					return ptr;
			}
			else
			{
				if (!toUse.expectType(i))
					return ptr;
			}
		}

		auto ip = TemplateParameter::ListOps::merge(l, toUse.tp, *toUse.r);

		if (!toUse.r->wasOk())
			return ptr;

		ScopedPointer<StructType> st = new StructType(toUse.id.id, ip);

		for (const auto& f : initFunctions)
		{
			f(toUse, st);

			if (!toUse.r->wasOk())
				return ptr;
		}

		for (const auto& f : fCopy)
		{
			auto fData = f(st);

			if(fData.id.isValid())
				st->addJitCompiledMemberFunction(fData.withParent(st->id));
		}

		for (const auto& f : postbF)
		{
			f(toUse, st);

			if (!toUse.r->wasOk())
				return ptr;
		}

		st->finaliseExternalDefinition();

		ptr = st.release();

		return ptr;
	};

	return to;
}

void TemplateClassBuilder::Helpers::addChildObjectPtr(StatementPtr newCall, SyntaxTreeInlineData* d, StructType* parentType, int memberIndex)
{
	auto pId = VariadicHelpers::getVariadicMemberIdFromIndex(memberIndex);
	auto offset = parentType->getMemberOffset(pId);
	auto childType = parentType->getMemberComplexType(pId);

	if (auto fc = dynamic_cast<Operations::FunctionCall*>(newCall.get()))
	{
		auto obj = new Operations::MemoryReference(d->location, d->object->clone(d->location), TypeInfo(childType, false, true), (int)offset);
		fc->setObjectExpression(obj);
	}
}

snex::jit::TemplateClassBuilder::StatementPtr TemplateClassBuilder::Helpers::createBlock(SyntaxTreeInlineData* d)
{
	auto parentScope = Operations::findParentStatementOfType<Operations::ScopeStatementBase>(d->expression.get());
	auto blPath = d->expression->currentCompiler->namespaceHandler.createNonExistentIdForLocation(parentScope->getPath(), Random::getSystemRandom().nextInt({0, 99999}));
	return new Operations::StatementBlock(d->location, blPath);
}

snex::jit::StructType* TemplateClassBuilder::Helpers::getStructTypeFromInlineData(InlineData* b)
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

int TemplateClassBuilder::Helpers::getTemplateConstant(StructType* st, int index, Result& r)
{
	auto tp = st->getTemplateInstanceParameters()[index];

	if (tp.constantDefined)
		return tp.constant;

	r = Result::fail("Expected template constant at index " + String(index));
	return -1;
}

snex::jit::TemplateClassBuilder::StatementPtr TemplateClassBuilder::Helpers::createFunctionCall(ComplexType::Ptr converterType, SyntaxTreeInlineData* d, const Identifier& functionId, StatementList originalArgs)
{
	if (functionId.isNull())
	{
		return new Operations::Noop(d->location);
	}

	jassert(converterType != nullptr);
	auto f = getFunctionFromTargetClass(converterType, functionId);

	TemplateParameter::List tpToUse;

	if (TemplateParameter::ListOps::isArgument(f.templateParameters))
	{
		auto r = Result::ok();
		tpToUse = TemplateParameter::ListOps::merge(f.templateParameters, d->templateParameters, r);

		if (d->templateParameters.isEmpty() && !r.wasOk())
		{
			// It might be possible that it's trying to call a template without the specification...
			auto cb = ScriptnodeCallbacks::getCallbackId(f.id);

			if (cb == ScriptnodeCallbacks::ProcessFrameFunction)
			{

			}
		}

		d->location.test(r);
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

	Array<FunctionData> matches;

	fc->addMatchingFunctions(matches, fId);

	// Prefer instantiated functions over their template definitions
	for (auto m : matches)
	{
		if (!TemplateParameter::ListOps::isArgument(m.templateParameters))
			return m;
	}

	return matches.getFirst();
	
}


void TemplateClassBuilder::Helpers::redirectProcessCallbacksToFixChannel(const TemplateObject::ConstructData& cd, StructType* st)
{
	jassert(st->hasInternalProperty(WrapIds::NumChannels));

	if (int numChannel = st->getInternalProperty(WrapIds::NumChannels, 0))
	{
		TemplateInstance tId(NamespacedIdentifier("ProcessData"), {});

		TemplateParameter::List tp;
		tp.add(TemplateParameter(numChannel));

		auto r = Result::ok();

		TypeInfo pd(cd.handler->createTemplateInstantiation(tId, tp, r), false, true);
		TypeInfo pfd(cd.handler->registerComplexTypeOrReturnExisting(new SpanType(TypeInfo(Types::ID::Float), numChannel)), false, true);

		*cd.r = st->redirectAllOverloadedMembers("process", { pd });

		if (!cd.r->wasOk())
			return;

		*cd.r = st->redirectAllOverloadedMembers("processFrame", { pfd });

		if (!cd.r->wasOk())
			return;
		
#if 0
		for (auto cf : callbacks)
		{
			TypeInfo::List pd, pfd;
			pd.addArray(processDatas);
			pfd.addArray(processFrameDatas);

			addFunction([cf, pfd, pd](StructType* st)
				{
					auto nc = (int)st->getInternalProperty("NumChannels", 0);

					jassert(nc != 0);

					auto cId = ScriptnodeCallbacks::getCallbackId(cf.id);

					if (cId == ScriptnodeCallbacks::ProcessFunction)
					{
						auto maxChannelAmount = cf.args[0].typeInfo.getTypedComplexType<StructType>()->getTemplateInstanceParameters()[0].constant;

						if (maxChannelAmount != nc)
							cf.args.getReference(0).typeInfo = pd[nc - 1];
					}
					if (cId == ScriptnodeCallbacks::ProcessFrameFunction)
					{
						auto maxChannelAmount = cf.args[0].typeInfo.getTypedComplexType<SpanType>()->getNumElements();

						if (maxChannelAmount != nc)
							cf.args.getReference(0).typeInfo = pfd[nc - 1];
					}

					return cf;
				});
		}
#endif
	}
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

		WrapBuilder::InnerData id(st->getMemberComplexType(pId).get(), WrapBuilder::GetSelfAsObject);
		id.offset = (int)st->getMemberOffset(pId);

		if (id.resolve())
		{
			auto base = dynamic_cast<Operations::Expression*>(d->object.get());
			d->target = new Operations::MemoryReference(d->location, base, id.getRefType(), id.offset);
		}

		return id.getResult();
	});

	getF.inliner->returnTypeFunction = [st](InlineData* b)
	{
		auto rt = dynamic_cast<ReturnTypeInlineData*>(b);
		auto pId = getVariadicMemberIdFromIndex(rt->templateParameters.getFirst().constant);
		auto t = st->getMemberComplexType(pId);

		WrapBuilder::InnerData id(t.get(), WrapBuilder::GetSelfAsObject);
		
		if (id.resolve())
			rt->f.returnType = id.getRefType();

		return id.getResult();
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

		Identifier thisId = functionId;

		if (functionId == FunctionClass::getSpecialSymbol(st->id, FunctionClass::Constructor))
		{
			FunctionClass::Ptr fc = childParameter->getFunctionClass();
			auto f = fc->getSpecialFunction(FunctionClass::Constructor);
			thisId = f.id.getIdentifier();
		}

		auto newCall = Helpers::createFunctionCall(childParameter, d, thisId, processedArgs);

		

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




}
}
