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

using namespace juce;
USE_ASMJIT_NAMESPACE;



namespace jit
{
WrapBuilder::WrapBuilder(Compiler& c, const Identifier& id, int numChannels_, OpaqueType opaqueType_, bool addParameterClass) :
	TemplateClassBuilder(c, getWrapId(id)),
	WrappedObjectOffset(addParameterClass ? 1 : 0),
	numChannels(numChannels_),
	opaqueType(opaqueType_)
{
	if (addParameterClass)
		addTypeTemplateParameter("ParameterClass");

	init(c, numChannels);
}

WrapBuilder::WrapBuilder(Compiler& c, const Identifier& id, const Identifier& constantArg, int numChannels_, OpaqueType opaqueType_) :
	TemplateClassBuilder(c, getWrapId(id)),
	WrappedObjectOffset(1),
	numChannels(numChannels_),
	opaqueType(opaqueType_)
{
	addIntTemplateParameter(constantArg);
	init(c, numChannels);
}


snex::NamespacedIdentifier WrapBuilder::getWrapId(const Identifier& id)
{
	if (id.toString().contains("::"))
		return NamespacedIdentifier::fromString(id.toString());
	else
		return NamespacedIdentifier("wrap").getChildId(id);
}

void WrapBuilder::init(Compiler& c, int numChannels)
{
	addTypeTemplateParameter("ObjectClass");

	int objIndex = WrappedObjectOffset;

	auto opaqueType_ = opaqueType;

	auto compiler = &c;

	setInitialiseStructFunction([compiler, numChannels, objIndex, opaqueType_](const TemplateObject::ConstructData& cd, StructType* st)
	{
		auto pType = TemplateClassBuilder::Helpers::getSubTypeFromTemplate(st, objIndex);
		st->addMember("obj", TypeInfo(pType, false, false));

		st->setInternalProperty(WrapIds::ObjectIndex, 0);

		st->setInternalProperty(WrapIds::IsNode, true);

		if (opaqueType_ != FullOpaque)
		{
			st->setInternalProperty(WrapIds::IsObjectWrapper, true);

			if (opaqueType_ == GetSelfAsObject)
				st->setInternalProperty(WrapIds::GetSelfAsObject, true);
		}

		for (int i = 1; i < numChannels + 1; i++)
		{
			auto prototypes = Types::ScriptnodeCallbacks::getAllPrototypes(compiler, i);

			for (auto p : prototypes)
				st->addWrappedMemberMethod("obj", p);
		}

		if (opaqueType_ == GetSelfAsObject)
		{
			auto sp = st->getTemplateInstanceId();
			sp.id = sp.id.getChildId("setParameter");

			TemplateObject spo(sp);

			// We need an empty template function instantiation so that
			// the parameter::call function can find it...
			spo.makeFunction = [compiler, st](const TemplateObject::ConstructData& cd)
			{
				FunctionData fd;

				fd.id = cd.id.id;
				fd.templateParameters = cd.tp;
				fd.returnType = TypeInfo(Types::ID::Void);
				fd.addArgs("value", TypeInfo(Types::ID::Double));

				InnerData id(st, OpaqueType::GetObj);

				if (id.resolve())
				{
					TemplateInstance fId(id.st->id.getChildId("setParameter"), id.st->getTemplateInstanceParameters());

					auto r = Result::ok();

					compiler->getNamespaceHandler().createTemplateFunction(fId, cd.tp, r);

					fd.inliner = Inliner::createHighLevelInliner({}, [id](InlineData* b)
						{
							auto d = b->toSyntaxTreeData();

							auto newCall = TemplateClassBuilder::Helpers::createFunctionCall(id.st, d, "setParameter", d->args);

							if (auto f = dynamic_cast<Operations::FunctionCall*>(newCall.get()))
							{
								auto obj = new Operations::MemoryReference(d->location, d->object->clone(d->location), id.getRefType(), id.offset);
								f->setObjectExpression(obj);

								d->target = newCall;
							}
								
							return Result::ok();
						});

					st->addJitCompiledMemberFunction(fd);
				}

					
			};

			spo.argList.add(TemplateParameter(sp.id.getChildId("P"), 0, false));

			spo.functionArgs = []()
			{
				TypeInfo::List args;
				args.add(TypeInfo(Types::ID::Double));
				return args;
			};

			compiler->getNamespaceHandler().addTemplateFunction(spo);
		}

	});

	addFunction(createGetObjectFunction);
	addFunction(createGetWrappedObjectFunction);

#if 0
	addInitFunction([objIndex](const TemplateObject::ConstructData& cd, StructType* st)
	{
		auto obj = dynamic_cast<StructType*>(TemplateClassBuilder::Helpers::getSubTypeFromTemplate(st, objIndex).get());
		Helpers::checkPropertyExists(obj, WrapIds::IsNode, *cd.r);
	});
#endif

	addInitFunction(Helpers::setNumChannelsFromObjectType);

	// Only create a parameter forwarder for self-aware wrappers
	if (opaqueType == GetSelfAsObject)
		addFunction(createSetFunction);

	addFunction(Helpers::constructorFunction);
}

snex::jit::FunctionData WrapBuilder::createSetFunction(StructType* st)
{
	// We just want to forward the setParameter method for self-aware wrappers
	jassert(st->hasInternalProperty(WrapIds::GetSelfAsObject));

	FunctionData sf;
	sf.id = st->id.getChildId("setParameter");
	sf.returnType = TypeInfo(Types::ID::Void, false, false);
	sf.addArgs("value", TypeInfo(Types::ID::Double));
	sf.templateParameters.add(TemplateParameter(sf.id.getChildId("P"), 0, false));

	sf.inliner = Inliner::createHighLevelInliner(sf.id, [st](InlineData* b)
	{
		auto d = b->toSyntaxTreeData();
		auto pType = TemplateClassBuilder::Helpers::getSubTypeFromTemplate(st, 0);

		Symbol sToUse(dynamic_cast<StructType*>(pType.get())->id.getChildId("setParameter"), TypeInfo(Types::ID::Void));

		TemplateInstance tId(sToUse.id, {});
		auto r = Result::ok();

		d->object->currentCompiler->namespaceHandler.createTemplateFunction(tId, d->templateParameters, r);

		if (!r.wasOk())
			return r;

		FunctionClass::Ptr fc = pType->getFunctionClass();
		auto exprCall = new Operations::FunctionCall(d->location, nullptr, sToUse, d->templateParameters);
		auto obj = new Operations::MemoryReference(d->location, d->object, TypeInfo(pType, false, true), 0);

		exprCall->setObjectExpression(obj);

		for (auto a : d->args)
			exprCall->addArgument(a->clone(d->location));

		d->target = exprCall;

		return Result::ok();
	});

	return sf;

}

snex::jit::FunctionData WrapBuilder::createGetObjectFunction(StructType* st)
{
	FunctionData getObjectFunction;
	getObjectFunction.id = st->id.getChildId("getObject");

	if (st->getInternalProperty(WrapIds::GetSelfAsObject, false))
	{
		WeakReference<StructType> safeSC(st);

		getObjectFunction.returnType = TypeInfo(Types::ID::Dynamic);
		getObjectFunction.inliner = Inliner::createHighLevelInliner(getObjectFunction.id, [safeSC](InlineData* b)
			{
				auto d = b->toSyntaxTreeData();
				d->target = new Operations::MemoryReference(d->location, d->object, TypeInfo(safeSC.get(), false, true), 0);
				return Result::ok();
			});

		getObjectFunction.inliner->returnTypeFunction = [safeSC](InlineData* b)
		{
			auto d = dynamic_cast<ReturnTypeInlineData*>(b);
			d->f.returnType = TypeInfo(safeSC.get(), false, true);
			return Result::ok();
		};

		return getObjectFunction;
	}
	else
	{
		InnerData id(st, OpaqueType::GetSelfAsObject);

		if (id.resolve())
		{
			auto pType = id.getRefType();
			auto offset = id.offset;

			getObjectFunction.returnType = pType;
			getObjectFunction.inliner = Inliner::createHighLevelInliner(getObjectFunction.id, [pType, offset](InlineData* b)
				{
					auto d = b->toSyntaxTreeData();
					d->target = new Operations::MemoryReference(d->location, d->object, pType, offset);
					return Result::ok();
				});

			return getObjectFunction;
		}
	}

	return getObjectFunction;
}

snex::jit::FunctionData WrapBuilder::createGetWrappedObjectFunction(StructType* st)
{
	FunctionData getObjectFunction;
	getObjectFunction.id = st->id.getChildId("getWrappedObject");

	InnerData id(st, ForwardToObj);

	if (id.resolve())
	{
		auto pType = id.getRefType();
		auto offset = id.offset;

		getObjectFunction.returnType = pType;
		getObjectFunction.inliner = Inliner::createHighLevelInliner(getObjectFunction.id, [pType, offset](InlineData* b)
		{
			auto d = b->toSyntaxTreeData();
			d->target = new Operations::MemoryReference(d->location, d->object, pType, offset);
			return Result::ok();
		});

	}
	return getObjectFunction;
}

snex::jit::FunctionData WrapBuilder::createGetSelfAsObjectFunction(StructType* st)
{
	auto inner = st;

	TypeInfo pType(inner, false, true);
	FunctionData getObjectFunction;
	getObjectFunction.id = st->id.getChildId("getObject");
	getObjectFunction.returnType = pType;
	getObjectFunction.inliner = Inliner::createHighLevelInliner(getObjectFunction.id, [pType](InlineData* b)
		{
			auto d = b->toSyntaxTreeData();
			d->target = new Operations::MemoryReference(d->location, d->object, pType, 0);
			return Result::ok();
		});

	return getObjectFunction;
}

void WrapBuilder::setInlinerForCallback(Types::ScriptnodeCallbacks::ID cb, CallbackList requiredFunctions, Inliner::InlineType t, const Inliner::Func& inliner)
{
	auto fToReplace = Types::ScriptnodeCallbacks::getPrototype(&c, cb, numChannels);

	auto compiler = &c;
	auto thisNumChannels = numChannels;

	InitialiseStructFunction replacer = [fToReplace, requiredFunctions, t, inliner, compiler, thisNumChannels](const TemplateObject::ConstructData& cd, StructType* st)
	{
		FunctionClass::Ptr fc = st->getFunctionClass();

		Array<FunctionData> matches;

		fc->addMatchingFunctions(matches, fc->getClassName().getChildId(fToReplace.id.getIdentifier()));

		for (auto& m : matches)
		{
			auto copy = FunctionData(m);
			copy.id = st->id.getChildId(fToReplace.id.getIdentifier());

			copy.function = m.function;
			copy.inliner = Inliner::createFromType(fToReplace.id, t, inliner);

			if (!requiredFunctions.isEmpty())
			{
				copy.inliner->precodeGenFunc = [requiredFunctions, st](InlineData* b)
				{
					auto pd = dynamic_cast<PreCodeGenInlineData*>(b);

					Inliner::List requiredInliners;
					InnerData id(st, GetObj);

					if (id.resolve())
					{
						FunctionClass::Ptr fc = id.st->getFunctionClass();

						for (auto rcb : requiredFunctions)
						{
							auto fId = ScriptnodeCallbacks::getIds(fc->getClassName())[rcb];

							auto fToUse = fc->getNonOverloadedFunction(fId);

							if (fToUse.function == nullptr && fToUse.inliner != nullptr)
							{
								PreCodeGenInlineData::Item newItem;
								newItem.functionToCompile = fToUse;
								newItem.objType = id.st;
								newItem.offsetFromRoot = id.offset;

								pd->functionsToCompile.add(newItem);
							}
						}
					}

					return Result::ok();
				};
			}

			auto ok = st->injectInliner(copy);

			if (!ok)
			{
				*cd.r = Result::fail("Can't inject inliner for " + copy.getSignature());
				return;
			}
		}
	};

	addInitFunction(replacer);
}


void WrapBuilder::setEmptyCallback(Types::ScriptnodeCallbacks::ID cb)
{
	setInlinerForCallback(cb, {}, Inliner::HighLevel, WrapLibraryBuilder::Callbacks::empty::noop);

	auto numChannels_ = numChannels;

	addPostFunctionBuilderInitFunction([cb, numChannels_](const TemplateObject::ConstructData& cd, StructType* st)
	{
		auto f = ScriptnodeCallbacks::getPrototype(st->getCompiler(), cb, numChannels_);

		using namespace scriptnode;

		switch (cb)
		{
		case ScriptnodeCallbacks::ProcessFunction: 
			f.function = (void*)prototypes::noop::process<ProcessData<1>>; 
			break;
		case ScriptnodeCallbacks::ProcessFrameFunction: 
			f.function = (void*)prototypes::noop::processFrame<span<float, 1>>; 
			break;
		case ScriptnodeCallbacks::ResetFunction:
			f.function = (void*)prototypes::noop::reset;
			break;
		case ScriptnodeCallbacks::HandleEventFunction:
			f.function = (void*)prototypes::noop::handleHiseEvent;
			break;
		case ScriptnodeCallbacks::PrepareFunction:
			f.function = (void*)prototypes::noop::prepare;
			break;
		}

		st->injectMemberFunctionPointer(f, f.function);
	});
}

juce::Result WrapBuilder::Helpers::constructorInliner(InlineData* b)
{
	using namespace Operations;

	auto d = b->toSyntaxTreeData();
	auto wrapType = TemplateClassBuilder::Helpers::getStructTypeFromInlineData(b);

	auto pId = Identifier("obj");
	auto offset = wrapType->getMemberOffset(pId);

	if (auto childType = dynamic_cast<StructType*>(wrapType->getMemberComplexType(pId).get()))
	{
		if (!childType->hasConstructor())
		{
			d->target = new Noop(d->location);
			return Result::ok();
		}

		auto newCall = TemplateClassBuilder::Helpers::createFunctionCall(childType, d, childType->getConstructorId(), {});

		if (auto fc = as<FunctionCall>(newCall))
		{
			auto obj = new MemoryReference(d->location, d->object, TypeInfo(childType, false, true), (int)offset);
			fc->setObjectExpression(obj);
			d->target = newCall;
			return Result::ok();
		}
	}

	return Result::fail("Can't find obj constructor");
}



juce::Result WrapBuilder::Helpers::addObjReference(SyntaxTreeInlineParser& p)
{
	auto d = p.b->toSyntaxTreeData();
	auto object = d->object;

	auto wrapType = object->getTypeInfo();

	if (auto st = wrapType.getTypedIfComplexType<StructType>())
	{
		auto offset = st->getMemberOffset("obj");
		auto t = st->getMemberTypeInfo("obj");
		p.addExternalExpression("obj", new Operations::MemoryReference(p.originalLocation, object->clone(p.originalLocation), t, (int)offset));

		return Result::ok();
	}

	return Result::fail("not working");
}

bool WrapBuilder::Helpers::checkPropertyExists(StructType* st, const Identifier& id, Result& r)
{
	if (!st->hasInternalProperty(id))
	{
		String s;
		s << st->toString() << "::" << id << " not defined";
		r = Result::fail(s);
		return false;
	}
	
	return true;
}

bool WrapBuilder::Helpers::getInnerType(InnerData& d)
{
	if (d.st == nullptr)
	{
		jassertfalse;
		return false;
	}

	while (d.st->getInternalProperty(WrapIds::IsObjectWrapper, false))
	{
		if ((d.typeToLookFor == GetSelfAsObject) && d.st->getInternalProperty(WrapIds::GetSelfAsObject, false))
			break;

		auto objIndex = (int)d.st->getInternalProperty(WrapIds::ObjectIndex, -1);
		jassert(objIndex != -1);
		auto objId = d.st->getMemberName(objIndex);
		d.offset += (int)d.st->getMemberOffset(objIndex);
		d.st = d.st->getMemberTypeInfo(objId).getTypedComplexType<StructType>();

		if (d.typeToLookFor == GetObj)
			break;
	}

	return true;
}

void WrapBuilder::Helpers::setNumChannelsFromTemplateParameter(const TemplateObject::ConstructData& cd, StructType* st)
{
	auto numChannelsToUse = cd.tp[0].constant;

	if (st->hasInternalProperty(WrapIds::NumChannels))
	{
		auto numAlreadyDefinedChannels = (int)st->getInternalProperty(WrapIds::NumChannels, 0);

		if (numAlreadyDefinedChannels != numChannelsToUse)
		{
			String s;
			s << st->toString() << ": illegal channel wrap amount";
			*cd.r = Result::fail(s);
			return;
		}
	}

	st->setInternalProperty(WrapIds::NumChannels, numChannelsToUse);
}

void WrapBuilder::Helpers::setNumChannelsFromObjectType(const TemplateObject::ConstructData& cd, StructType* st)
{
	InnerData id(st, WrapBuilder::GetObj);

	if (id.resolve())
	{
		auto prop = (int)id.st->getInternalProperty(WrapIds::NumChannels, 0);

		if (prop != 0)
			st->setInternalProperty(WrapIds::NumChannels, prop);
	}
}

WrapBuilder::ExternalFunctionMapData::ExternalFunctionMapData(Compiler& c_, AsmInlineData* d) :
	c(c_),
	acg(d->gen),
	objectType(d->object->getTypeInfo()),
#if SNEX_ASMJIT_BACKEND
	scope(d->object->getScope()),
#else
	scope(nullptr),
#endif
	target(d->target),
	object(d->object)
{
#if SNEX_MIR_BACKEND
	jassertfalse;
#endif

	tp = objectType.getTypedComplexType<StructType>()->getTemplateInstanceParameters();
	argumentRegisters.addArray(d->args);

	// Add them for the template map function
	for (auto& a : argumentRegisters)
		tp.add(TemplateParameter(a->getTypeInfo()));
}

int WrapBuilder::ExternalFunctionMapData::getChannelFromLastArg() const
{
	auto p = tp.getLast().type;

	if (auto st = p.getTypedIfComplexType<StructType>())
	{
		if (st->id.getIdentifier() == Identifier("ProcessData"))
			return st->getTemplateInstanceParameters()[0].constant;
	}

	if (auto sp = p.getTypedIfComplexType<SpanType>())
		return sp->getNumElements();

	return -1;
}

int WrapBuilder::ExternalFunctionMapData::getTemplateConstant(int index) const
{
	jassert(tp[index].constantDefined);
	return tp[index].constant;
}

juce::Result WrapBuilder::ExternalFunctionMapData::insertFunctionPtrToArgReg(void* ptr, int index /*= 0*/)
{
	// Gotta set this before calling anything else...
	jassert(mainFunction != nullptr);

	if (ptr)
	{
		argumentRegisters.insert(index, createPointerArgument(ptr));
		return Result::ok();
	}
	else
		return Result::fail("Can't find function pointer");
}

Result WrapBuilder::ExternalFunctionMapData::emitRemappedFunction(FunctionData& f)
{
	if (mainFunction == nullptr)
		return Result::fail(f.getSignature() + ": unspecified external function pointer");

	f.inliner = nullptr;
	f.function = mainFunction;

	f.args.clear();

	f.functionName = "external " + f.getSignature();

	int i = 1;

	for (auto d : argumentRegisters)
		f.addArgs(Identifier("a" + String(i++)), d->getTypeInfo());

#if SNEX_ASMJIT_BACKEND
	if (!f.isResolved())
		return Result::fail("Can't find function for template parameters " + TemplateParameter::ListOps::toString(tp));
	else
		return acg.emitFunctionCall(target, f, object, argumentRegisters);
#else
	return Result::fail("funky");
#endif
}

snex::jit::FunctionData WrapBuilder::ExternalFunctionMapData::getCallbackFromObject(Types::ScriptnodeCallbacks::ID cb)
{
	Array<TypeInfo> args;

	for (auto a : argumentRegisters)
		args.add(a->getTypeInfo());

	return getCallback(objectType, cb, args);
}

void* WrapBuilder::ExternalFunctionMapData::getWrappedFunctionPtr(Types::ScriptnodeCallbacks::ID cb)
{
	if (auto st = objectType.getTypedComplexType<StructType>())
	{
		auto t = st->getMemberTypeInfo("obj");
		jassert(t.isComplexType());

		// We have to recreate the argument list from the default callbacks because it might want another callback
		auto prototypes = ScriptnodeCallbacks::getAllPrototypes(&c, getChannelFromLastArg());

		Array<TypeInfo> args;

		for (auto a : prototypes[cb].args)
			args.add(a.typeInfo);

		auto f = getCallback(t, cb, args);
		
		if (!f.function && f.inliner != nullptr)
		{
			jassert(f.inliner->inlineType == Inliner::Assembly);

			// This is the case if the wrapper tries to find a valid function pointer that is passed
			// to the external callback, but if fails to do so, because the function is not compiled
			// yet. In order to solve this, 

			jassertfalse;
			return nullptr;
		}
		
		return f.function;
	}

	return nullptr;
}

void WrapBuilder::ExternalFunctionMapData::setExternalFunctionPtrToCall(void* mainFunctionPointer)
{
	mainFunction = mainFunctionPointer;
}

snex::jit::FunctionData WrapBuilder::ExternalFunctionMapData::getCallback(TypeInfo t, Types::ScriptnodeCallbacks::ID cb, const Array<TypeInfo>& args)
{
	Array<FunctionData> matches;

	auto st = t.getTypedComplexType<StructType>();

	FunctionClass::Ptr fc = st->getFunctionClass();

	auto prototype = Types::ScriptnodeCallbacks::getPrototype(&c, cb, getChannelFromLastArg());
	fc->addMatchingFunctions(matches, fc->getClassName().getChildId(prototype.id.getIdentifier()));

	for (auto& m : matches)
	{
		if (m.matchesArgumentTypes(args))
			return m;
	}

	return {};
}

snex::jit::AssemblyRegister::Ptr WrapBuilder::ExternalFunctionMapData::createPointerArgument(void* ptr)
{
#if SNEX_ASMJIT_BACKEND
	AsmCodeGenerator::TemporaryRegister functionPointer(acg, scope, TypeInfo(Types::ID::Pointer, true, false));
	auto fReg = functionPointer.tempReg;
	fReg->createRegister(acg.cc);
	acg.cc.mov(PTR_REG_W(fReg), (uint64_t)ptr);
	return fReg;
#else
	jassertfalse;
	return nullptr;
#endif
}


void WrapBuilder::mapToExternalTemplateFunction(Types::ScriptnodeCallbacks::ID cb, CallbackList requiredFunctions, const std::function<Result(ExternalFunctionMapData&)>& templateMapFunction)
{
	auto nc = numChannels;
	auto& jc = c;

	requiredFunctions.addIfNotAlreadyThere(cb);

	setInlinerForCallback(cb, requiredFunctions, Inliner::InlineType::Assembly, [cb, requiredFunctions, nc, &jc, templateMapFunction](InlineData* b)
	{
		using namespace Operations;

		auto d = b->toAsmInlineData();

		ExternalFunctionMapData mapData(jc, d);

		auto f = mapData.getCallbackFromObject(cb);

		auto r = Result::ok();

		if (templateMapFunction)
		{
			auto r = templateMapFunction(mapData);

			if (!r.wasOk())
				return r;

			return mapData.emitRemappedFunction(f);
		}
		else
			return Result::fail("Can't find map function");
	});
}

}

namespace Types {




snex::jit::FunctionData WrapLibraryBuilder::createInitConstructor(StructType* st)
{
	// Only call this for wrappers that are self aware
	jassert(st->getInternalProperty(WrapIds::GetSelfAsObject, false));

	FunctionData f;

	f.id = st->id.getChildId(FunctionClass::getSpecialSymbol(st->id, FunctionClass::Constructor));
	f.returnType = TypeInfo(Types::ID::Void);

	f.inliner = Inliner::createHighLevelInliner(f.id, [st](InlineData* b)
		{
			auto d = b->toSyntaxTreeData();
			auto ic = st->getMemberComplexType(Identifier("initialiser"));

			FunctionClass::Ptr fc = ic->getFunctionClass();

			auto icf = fc->getSpecialFunction(FunctionClass::Constructor);

			auto nc = new Operations::FunctionCall(d->location, nullptr, Symbol(icf.id, TypeInfo(Types::ID::Void)), icf.templateParameters);

			auto initRef = new Operations::MemoryReference(d->location, d->object, TypeInfo(ic, false), (int)st->getMemberOffset(1));

			WrapBuilder::InnerData id(st, WrapBuilder::OpaqueType::GetObj);

			if (id.resolve())
			{
				auto objRef = new Operations::MemoryReference(d->location, d->object, id.getRefType(), id.offset);

				nc->setObjectExpression(initRef);
				nc->addArgument(objRef);

				if (icf.canBeInlined(true))
				{
					SyntaxTreeInlineData sd(nc, {}, icf);
					sd.object = initRef->clone(d->location);
					sd.path = d->path;
					sd.templateParameters = d->templateParameters;
					auto r = icf.inlineFunction(&sd);

					if (!r.wasOk())
						return r;

					d->target = sd.target;
				}
				else
					d->target = nc;
			}

			return id.getResult();
		});

	return f;
}

void WrapLibraryBuilder::createDefaultInitialiser(const TemplateObject::ConstructData& cd, StructType* st)
{
	auto ic = TemplateClassBuilder::Helpers::getSubTypeFromTemplate(st, 1);

#if 0
	
	auto objType = TemplateClassBuilder::Helpers::getSubTypeFromTemplate(st, 0);
	auto ic_st = dynamic_cast<StructType*>(ic.get());
	FunctionClass::Ptr fc = ic_st->getFunctionClass();
	TypeInfo expected(objType, false, true);
	auto con = fc->getConstructor({ expected });

	if (!con.id.isValid())
	{
		String s;
		s << "constructor of initialiser class " << ic->toString() << " doesn't match ";
		s << expected.toString();
		*cd.r = Result::fail(s);
	}
#endif

	st->addMember("initialiser", TypeInfo(ic, false, false));
	InitialiserList::Ptr di = new InitialiserList();
	di->addChild(new InitialiserList::MemberPointer(st, "obj"));
	st->setDefaultValue("initialiser", di);
}

Result WrapLibraryBuilder::registerTypes()
{
	{
		WrapBuilder init(c, "init", numChannels, WrapBuilder::GetSelfAsObject);
		init.addTypeTemplateParameter("InitialiserClass");

		init.addInitFunction(createDefaultInitialiser);
		init.addFunction(createInitConstructor);

		init.flush();

		WrapBuilder data(c, "data", numChannels, WrapBuilder::GetSelfAsObject);
		data.addTypeTemplateParameter("DataHandler");
		//data.addInitFunction(createDefaultInitialiser);

		data.addInitFunction([](const TemplateObject::ConstructData& cd, StructType* st)
		{
			auto dataHandlerClass = dynamic_cast<StructType*>(TemplateClassBuilder::Helpers::getSubTypeFromTemplate(st, 1).get());

			

			st->addMember("initialiser", TypeInfo(dataHandlerClass));

			auto l = dataHandlerClass->makeDefaultInitialiserList();

			st->setDefaultValue("initialiser", l);

			if (!WrapBuilder::Helpers::checkPropertyExists(dataHandlerClass, "NumAudioFiles", *cd.r))
				return;

			if (!WrapBuilder::Helpers::checkPropertyExists(dataHandlerClass, "NumTables", *cd.r))
				return;

			if (!WrapBuilder::Helpers::checkPropertyExists(dataHandlerClass, "NumSliderPacks", *cd.r))
				return;
		});

		//data.addFunction(createInitConstructor);

		auto ed = c.getComplexType(NamespacedIdentifier("ExternalData"));

		data.addFunction([ed](StructType* st)
		{
			FunctionData se;

			se.id = st->id.getChildId("setExternalData");

			se.addArgs("d", TypeInfo(ed, true, true));
			se.addArgs("index", TypeInfo(Types::ID::Integer));
			se.returnType = TypeInfo(Types::ID::Void);
			se.setDefaultParameter("index", VariableStorage(0));

			se.inliner = Inliner::createHighLevelInliner(se.id, [st](InlineData* b)
			{
				auto d = b->toSyntaxTreeData();
				auto ic = st->getMemberComplexType(Identifier("initialiser"));



				FunctionClass::Ptr fc = ic->getFunctionClass();

				auto ef = fc->getNonOverloadedFunction(fc->getClassName().getChildId("setExternalData"));

				auto nc = new Operations::FunctionCall(d->location, nullptr, Symbol(ef.id, TypeInfo(Types::ID::Void)), ef.templateParameters);

				//nc->setAllowInlining(false);

				auto ioffset = st->getMemberOffset("initialiser");

				auto initRef = new Operations::MemoryReference(d->location, d->object, TypeInfo(ic, false), (int)ioffset);


				WrapBuilder::InnerData id(st->getMemberComplexType("obj").get(), WrapBuilder::GetSelfAsObject);

				if (id.resolve())
				{
					auto objRef = new Operations::MemoryReference(d->location, d->object, id.getRefType(), id.offset);

					nc->setObjectExpression(initRef);
					nc->addArgument(objRef);
					nc->addArgument(d->args[0]->clone(d->location));
					nc->addArgument(d->args[1]->clone(d->location));

					d->target = nc;
				}

				return id.getResult();
			});

			return se;
		});

		data.flush();

	}

	WrapBuilder event(c, "event", numChannels, WrapBuilder::ForwardToObj);
	event.mapToExternalTemplateFunction(ScriptnodeCallbacks::ProcessFunction, { ScriptnodeCallbacks::HandleEventFunction }, Callbacks::wrap_event::process);
	event.flush();

	WrapBuilder os(c, "fix", "NumChannels", numChannels, WrapBuilder::ForwardToObj);

	os.addInitFunction(WrapBuilder::Helpers::setNumChannelsFromTemplateParameter);
	os.addInitFunction(TemplateClassBuilder::Helpers::redirectProcessCallbacksToFixChannel);
	os.setInlinerForCallback(ScriptnodeCallbacks::ProcessFunction, {}, Inliner::HighLevel, Callbacks::fix::process);
	os.setInlinerForCallback(ScriptnodeCallbacks::ProcessFrameFunction, {}, Inliner::HighLevel, Callbacks::fix::processFrame);
	
	os.flush();

	WrapBuilder fb(c, "fix_block", "BlockSize", numChannels, WrapBuilder::ForwardToObj);
	fb.mapToExternalTemplateFunction(ScriptnodeCallbacks::PrepareFunction, {}, Callbacks::fix_block::prepare);
	fb.mapToExternalTemplateFunction(ScriptnodeCallbacks::ProcessFunction, {}, Callbacks::fix_block::process);
	fb.flush();


	WrapBuilder frame(c, "frame", "NumChannels", numChannels, WrapBuilder::ForwardToObj);

	frame.addInitFunction(WrapBuilder::Helpers::setNumChannelsFromTemplateParameter);
	frame.addInitFunction(TemplateClassBuilder::Helpers::redirectProcessCallbacksToFixChannel);

	frame.setInlinerForCallback(ScriptnodeCallbacks::ProcessFunction, {}, Inliner::HighLevel, Callbacks::frame::process);
	frame.mapToExternalTemplateFunction(ScriptnodeCallbacks::PrepareFunction, {}, Callbacks::frame::prepare);

	frame.flush();

	

	WrapBuilder mod(c, "mod", numChannels, WrapBuilder::GetSelfAsObject, true);
	mod.addInitFunction([](const TemplateObject::ConstructData& cd, StructType* st)
	{
		if (!ParameterBuilder::Helpers::isParameterClass(cd.tp[0].type))
		{
			*cd.r = Result::fail("Expected parameter class as first template parameter");
			return;
		}

		st->addMember("p", cd.tp[0].type);
	});

	mod.addFunction(Callbacks::mod::checkModValue);
	mod.addFunction(Callbacks::mod::getParameter);

	mod.setInlinerForCallback(ScriptnodeCallbacks::ProcessFunction, {},Inliner::HighLevel, Callbacks::mod::process);
	mod.setInlinerForCallback(ScriptnodeCallbacks::ProcessFrameFunction, {}, Inliner::HighLevel, Callbacks::mod::processFrame);

	mod.flush();


	WrapBuilder nWrapper(c, "node", 2, WrapBuilder::OpaqueType::GetSelfAsObject);

	nWrapper.addInitFunction([](const TemplateObject::ConstructData& d, StructType* st)
	{
		WrapBuilder::InnerData id(st, WrapBuilder::ForwardToObj);

		if (id.resolve())
		{
			auto metaClass = id.st->id.getChildId("metadata");

			if (auto mc = dynamic_cast<StructType*>(d.handler->getComplexType(metaClass).get()))
			{
				WrapBuilder::Helpers::checkPropertyExists(mc, WrapIds::NodeId, *d.r);
				WrapBuilder::Helpers::checkPropertyExists(mc, WrapIds::NumChannels, *d.r);

				
				auto id = mc->getInternalProperty(WrapIds::NodeId, "").toString();
				auto numChannels = mc->getInternalProperty(WrapIds::NumChannels, 0).toString();
				
				st->setInternalProperty(WrapIds::NodeId, id);
				st->setInternalProperty(WrapIds::NumChannels, numChannels);
			}
		}
	});

	nWrapper.addInitFunction(TemplateClassBuilder::Helpers::redirectProcessCallbacksToFixChannel);

	nWrapper.addFunction([](StructType* st)
	{
		auto compiler = st->getCompiler();
		jassert(compiler != nullptr);

		auto c = ScriptnodeCallbacks::getPrototype(compiler, ScriptnodeCallbacks::SetExternalDataFunction, 2);

		c.inliner = Inliner::createHighLevelInliner({}, [](InlineData* b)
		{
			cppgen::Base c;

			c << "this->obj.setExternalData(b, index);";

			return SyntaxTreeInlineParser(b, { "b", "index" }, c).flush();
		});

		return c;
	});

	nWrapper.flush();

	registerCoreTemplates();

	return Result::ok();
}


void WrapLibraryBuilder::registerCoreTemplates()
{
	WrapBuilder coreMidi(c, "core::midi", numChannels, WrapBuilder::FullOpaque);

	coreMidi.addInitFunction([](const TemplateObject::ConstructData& cd, StructType* st)
		{
			auto mv = cd.handler->getComplexType(NamespacedIdentifier("ModValue"));
			st->addMember("v", TypeInfo(mv));
		});

	coreMidi.addFunction([](StructType* st)
		{
			auto c_ = st->getCompiler();
			auto f = ScriptnodeCallbacks::getPrototype(c_, ScriptnodeCallbacks::HandleModulation, 2);
			f.id = st->id.getChildId(f.id.getIdentifier());
			f.inliner = Inliner::createHighLevelInliner({}, Callbacks::core_midi::handleModulation);
			return f;
		});

	coreMidi.setEmptyCallback(ScriptnodeCallbacks::ProcessFunction);
	coreMidi.setEmptyCallback(ScriptnodeCallbacks::ProcessFrameFunction);
	coreMidi.setEmptyCallback(ScriptnodeCallbacks::ResetFunction);
	coreMidi.setInlinerForCallback(ScriptnodeCallbacks::PrepareFunction, {}, Inliner::HighLevel, Callbacks::core_midi::prepare);
	coreMidi.setInlinerForCallback(ScriptnodeCallbacks::HandleEventFunction, {}, Inliner::HighLevel, Callbacks::core_midi::handleHiseEvent);

	coreMidi.flush();

}

juce::Result WrapLibraryBuilder::Callbacks::fix_block::prepare(WrapBuilder::ExternalFunctionMapData& mapData)
{
	int blockSize = mapData.getTemplateConstant(0);

	void* data = nullptr;

#define INSERT(b) if(blockSize == b) data = (void*)scriptnode::wrap::static_functions::fix_block<b>::prepare;
	INSERT(16);
	INSERT(32);
	INSERT(64);
	INSERT(128);
	INSERT(256);
	INSERT(512);
#undef INSERT

	mapData.setExternalFunctionPtrToCall(data);

	return mapData.insertFunctionPtrToArgReg(mapData.getWrappedFunctionPtr(ScriptnodeCallbacks::PrepareFunction));
}

juce::Result WrapLibraryBuilder::Callbacks::fix_block::process(WrapBuilder::ExternalFunctionMapData& mapData)
{
	struct Key
	{
		Key(int b, int c) : blocksize(b), channelAmount(c) { }
		String toString() const { return String(blocksize << 16 | channelAmount); }
		int blocksize; int channelAmount;
	};

	HashMap<String, void*> map;

#define INSERT(b, c) map.set(Key(b, c).toString(), (void*)scriptnode::wrap::static_functions::fix_block<b>::process<Types::ProcessData<c>>);
	INSERT(16, 1);  INSERT(16, 2);
	INSERT(32, 1);  INSERT(32, 2);
	INSERT(64, 1);  INSERT(64, 2);
	INSERT(128, 1); INSERT(128, 2);
	INSERT(256, 1); INSERT(256, 2);
	INSERT(512, 1); INSERT(512, 2);
#undef INSERT

	auto channelAmount = mapData.getChannelFromLastArg();
	auto blockSize = mapData.getTemplateConstant(0);
	auto ptr = map[Key(blockSize, channelAmount).toString()];

	mapData.setExternalFunctionPtrToCall(ptr);

	return mapData.insertFunctionPtrToArgReg(mapData.getWrappedFunctionPtr(ScriptnodeCallbacks::ProcessFunction));
}

juce::Result WrapLibraryBuilder::Callbacks::wrap_event::process(WrapBuilder::ExternalFunctionMapData& mapData)
{
	int numChannels = mapData.getChannelFromLastArg();

	jassert(numChannels > 0 && numChannels <= 2);

	void* processFunctions[2];

	processFunctions[0] = (void*)scriptnode::wrap::static_functions::event::template process<ProcessData<1>>;
	processFunctions[1] = (void*)scriptnode::wrap::static_functions::event::template process<ProcessData<2>>;

	mapData.setExternalFunctionPtrToCall(processFunctions[numChannels - 1]);

	auto r = mapData.insertFunctionPtrToArgReg(mapData.getWrappedFunctionPtr(ScriptnodeCallbacks::ProcessFunction));

	if (!r.wasOk())
		return r;

	return mapData.insertFunctionPtrToArgReg(mapData.getWrappedFunctionPtr(ScriptnodeCallbacks::HandleEventFunction), 1);
}

juce::Result WrapLibraryBuilder::Callbacks::fix::process(InlineData* b)
{
	auto f = getFunction(b, "process");
	return createFunctionCall(b, f);
}



juce::Result WrapLibraryBuilder::Callbacks::fix::processFrame(InlineData* b)
{
	auto f = getFunction(b, "processFrame");
	return createFunctionCall(b, f);
}

juce::Result WrapLibraryBuilder::Callbacks::fix::createFunctionCall(InlineData* b, FunctionData& f)
{
	auto d = b->toSyntaxTreeData();

	f.templateParameters = createTemplateInstance(d->object, f);
	auto st = d->object->getTypeInfo().getTypedComplexType<StructType>();
	auto wrappedType = TemplateClassBuilder::Helpers::getSubTypeFromTemplate(st, 1);

	auto newCall = new Operations::FunctionCall(d->location, nullptr, Symbol(f.id, f.returnType), f.templateParameters);

	newCall->addArgument(d->args[0]->clone(d->location));
	auto newObj = new Operations::MemoryReference(d->location, d->object->clone(d->location), TypeInfo(wrappedType, false, true), 0);
	newCall->setObjectExpression(newObj);

	d->target = newCall;

	return Result::ok();
}

snex::jit::FunctionData WrapLibraryBuilder::Callbacks::fix::getFunction(InlineData* b, const Identifier& id)
{
	auto d = b->toSyntaxTreeData();

	auto st = d->object->getTypeInfo().getTypedComplexType<StructType>();
	auto numChannels = st->getInternalProperty("NumChannels", 0);
	auto wrappedType = TemplateClassBuilder::Helpers::getSubTypeFromTemplate(st, 1);

	FunctionClass::Ptr fc = wrappedType->getFunctionClass();

	Array<FunctionData> matches;

	fc->addMatchingFunctions(matches, fc->getClassName().getChildId(id));

	return matches[0];
}

juce::Result WrapLibraryBuilder::Callbacks::frame::process(InlineData* b)
{
	using namespace cppgen;
	Base c(Base::OutputType::WrapInBlock);

	c << "auto frameData = data.toFrameData();";
	c << "while(frameData.next())";
	c << "    this->obj.processFrame(frameData.toSpan());";

	SyntaxTreeInlineParser p(b, {"data"}, c);

	return p.flush();
}

juce::Result WrapLibraryBuilder::Callbacks::frame::prepare(WrapBuilder::ExternalFunctionMapData& mapData)
{
	int numChannels = mapData.getTemplateConstant(0);

	span<void*, NUM_MAX_CHANNELS> data = { nullptr };

#define INSERT(b) data[b] = (void*)scriptnode::wrap::static_functions::frame<b>::prepare;
	INSERT(1);
	INSERT(2);
	INSERT(4);
	INSERT(8);
#undef INSERT

	mapData.setExternalFunctionPtrToCall(data[numChannels]);

	return mapData.insertFunctionPtrToArgReg(mapData.getWrappedFunctionPtr(ScriptnodeCallbacks::PrepareFunction));
}

snex::jit::FunctionData WrapLibraryBuilder::Callbacks::mod::checkModValue(StructType* st)
{
	FunctionData cmv;
	cmv.id = st->id.getChildId("checkModValue");
	cmv.returnType = TypeInfo(Types::ID::Void);

	cmv.inliner = Inliner::createHighLevelInliner({}, [](InlineData* b)
	{
		auto st = b->toSyntaxTreeData()->object->getTypeInfo().getComplexType();
		WrapBuilder::InnerData inner(st.get(), WrapBuilder::ForwardToObj);

		if (inner.resolve())
		{
			FunctionClass::Ptr fc = inner.st->getFunctionClass();

			auto fId = fc->getClassName().getChildId("handleModulation");
			auto ok = fc->hasFunction(fId);

			if (!ok)
				return Result::fail("missing function " + fId.toString());
		}

		using namespace cppgen;

		Base c(Base::OutputType::WrapInBlock);

		c << "double mv = 0.0;";
		c << "if(this->obj.handleModulation(mv))";
		c << "    this->getParameter().call(mv);";

		SyntaxTreeInlineParser p(b, {}, c);
		WrapBuilder::Helpers::addObjReference(p);
		return p.flush();
	});

	return cmv;
}

snex::jit::FunctionData WrapLibraryBuilder::Callbacks::mod::getParameter(StructType* st)
{
	FunctionData cf;
	cf.id = st->id.getChildId("getParameter");
	auto t = st->getMemberTypeInfo("p").withModifiers(false, true);
	cf.returnType = t;

	cf.inliner = Inliner::createHighLevelInliner({}, [t](InlineData* b)
	{
		auto d = b->toSyntaxTreeData();
		auto wClass = d->object->getTypeInfo().getTypedComplexType<StructType>();
		auto offset = wClass->getMemberOffset("p");
		d->target = new Operations::MemoryReference(d->location, d->object->clone(d->location), t, (int)offset);
		return Result::ok();
	});

	return cf;
}

juce::Result WrapLibraryBuilder::Callbacks::mod::process(InlineData* b)
{
	cppgen::Base c;
	c << "this->obj.process(data);";
	c << "this->checkModValue();";

	return SyntaxTreeInlineParser(b, { "data" }, c).flush();
}

juce::Result WrapLibraryBuilder::Callbacks::mod::processFrame(InlineData* b)
{
	cppgen::Base c;
	c << "this->obj.processFrame(data);";
	c << "this->checkModValue();";
	
	return SyntaxTreeInlineParser(b, { "data" }, c).flush();
}

juce::Result WrapLibraryBuilder::Callbacks::empty::noop(InlineData* b)
{
	auto d = b->toSyntaxTreeData();

	d->target = new Operations::Noop(d->location);

	return Result::ok();
}

juce::Result WrapLibraryBuilder::Callbacks::core_midi::prepare(InlineData* b)
{
	cppgen::Base c;
	c << "this->obj.prepare(ps);";

	return SyntaxTreeInlineParser(b, { "ps" }, c).flush();
}

juce::Result WrapLibraryBuilder::Callbacks::core_midi::handleHiseEvent(InlineData* b)
{
	cppgen::Base c;


	c << "double thisModValue = 0.0;";
	c << "auto thisChanged = this->obj.getMidiValue(e, thisModValue);";
	c << "if (thisChanged)";
	c << "   this->v.setModValueIfChanged(thisModValue);";

	return SyntaxTreeInlineParser(b, { "e" }, c).flush();
}

juce::Result WrapLibraryBuilder::Callbacks::core_midi::handleModulation(InlineData* b)
{
	cppgen::Base c;
	c << "return this->v.getChangedValue(value);";
	return SyntaxTreeInlineParser(b, { "value" }, c).flush();
}

}


}
