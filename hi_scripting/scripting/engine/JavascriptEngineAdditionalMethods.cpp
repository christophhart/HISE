/*  ===========================================================================
*
*   This file is part of HISE.
*   Copyright 2016 Christoph Hart
*
*   HISE is free software: you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation, either version 3 of the License, or
*   (at your option) any later version.
*
*   HISE is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public License
*   along with HISE.  If not, see <http://www.gnu.org/licenses/>.
*
*   Commercial licenses for using HISE in an closed source project are
*   available on request. Please visit the project's website to get more
*   information about commercial licensing:
*
*   http://www.hise.audio/
*
*   HISE is based on the JUCE library,
*   which also must be licenced for commercial applications:
*
*   http://www.juce.com
*
*   ===========================================================================
*/

// This file contains all methods that use forward declared inner classes


//==============================================================================

namespace hise { using namespace juce;


Result HiseJavascriptPreprocessor::process(String& code, const String& externalFile)
{
#if USE_BACKEND
    
    jassert(externalFile.isNotEmpty());
    
    auto hasLocalSwitch = code.startsWith(snex::jit::PreprocessorTokens::on_);
    
    if (!hasLocalSwitch && !this->globalEnabled)
        return Result::ok();

    snex::jit::ExternalPreprocessorDefinition::List empty;
    snex::jit::Preprocessor p(code);

    p.setCurrentFileName(externalFile);

    auto processed = p.processWithResult(definitions);

    if(p.getResult().wasOk())
        code = processed;
    
    deactivatedLines.set(externalFile, p.getDeactivatedLines());
    
    return p.getResult();

#else
    return Result::ok();
#endif
}


bool HiseJavascriptEngine::isJavascriptFunction(const var& v)
{
	if (auto obj = v.getObject())
		return dynamic_cast<WeakCallbackHolder::CallableObject*>(obj) != nullptr;

	return false;
}


bool HiseJavascriptEngine::isInlineFunction(const var& v)
{
	if (auto obj = v.getObject())
	{
		return dynamic_cast<RootObject::InlineFunction::Object*>(obj);
	}

	return false;
}

HiseJavascriptEngine::HiseJavascriptEngine(JavascriptProcessor *p, MainController* mc) : maximumExecutionTime(15.0), root(new RootObject()), unneededScope(new DynamicObject())
{
    
	root->hiseSpecialData.setProcessor(p);

    preprocessor = dynamic_cast<HiseJavascriptPreprocessor*>(mc->getGlobalPreprocessor());
    root->preprocessor = preprocessor;
    
	registerNativeObject(RootObject::ObjectClass::getClassName(), new RootObject::ObjectClass());
	registerNativeObject(RootObject::ArrayClass::getClassName(), new RootObject::ArrayClass());
	registerNativeObject(RootObject::StringClass::getClassName(), new RootObject::StringClass());
	registerApiClass(new RootObject::MathClass());
	registerNativeObject(RootObject::JSONClass::getClassName(), new RootObject::JSONClass());
	registerNativeObject(RootObject::IntegerClass::getClassName(), new RootObject::IntegerClass());
}

bool HiseJavascriptEngine::RootObject::JavascriptNamespace::optimiseFunction(OptimizationPass::OptimizationResult& r, var function, OptimizationPass* p)
{
	if (auto fo = dynamic_cast<InlineFunction::Object*>(function.getObject()))
	{
		if (fo->body != nullptr)
		{
			auto tr = p->executePass(fo->body);
			r.numOptimizedStatements += tr.numOptimizedStatements;
			return true;
		}
	}
	else if (auto fo = dynamic_cast<FunctionObject*>(function.getObject()))
	{
		auto tr = p->executePass(fo->body);
		r.numOptimizedStatements += tr.numOptimizedStatements;
		return true;
	}

	return false;
}

hise::HiseJavascriptEngine::RootObject::OptimizationPass::OptimizationResult HiseJavascriptEngine::RootObject::JavascriptNamespace::runOptimisation(OptimizationPass* p)
{
	OptimizationPass::OptimizationResult r;
	r.passName = p->getPassName();

	for (auto o : inlineFunctions)
	{
		optimiseFunction(r, var(o), p);
	}

	for (auto& co : constObjects)
	{
		if (auto cso = dynamic_cast<ApiClass*>(co.value.getObject()))
		{
			auto fList = cso->getListOfOptimizableFunctions();

			if (fList.isArray())
			{
				for (auto& f : *fList.getArray())
					optimiseFunction(r, f, p);
			}
			else
				jassertfalse;
		}
	}

	return r;
}

HiseJavascriptEngine::RootObject::OptimizationPass::OptimizationResult HiseJavascriptEngine::RootObject::HiseSpecialData::runOptimisation(OptimizationPass* p)
{
	auto r = JavascriptNamespace::runOptimisation(p);

	for (auto api : this->apiClasses)
	{
		auto list = api->getListOfOptimizableFunctions();

		for (auto f : *list.getArray())
			optimiseFunction(r, f, p);
	}

	for (auto& nv : this->root->getProperties())
	{
		optimiseFunction(r, nv.value, p);
	}

	for (auto n : namespaces)
	{
		auto tr = n->runOptimisation(p);
		r.numOptimizedStatements += tr.numOptimizedStatements;
	}

	for (auto c : callbackNEW)
	{
		if (c->statements != nullptr)
		{
			auto tr = p->executePass(c->statements);
			r.numOptimizedStatements += tr.numOptimizedStatements;
		}
	}

	return r;
}



HiseJavascriptEngine::RootObject::RootObject() :
hiseSpecialData(this)
{
#if ENABLE_SCRIPTING_BREAKPOINTS
	callStack.ensureStorageAllocated(128);
#endif

	setMethod("exec", exec);
	setMethod("eval", eval);
	setMethod("trace", trace);
	setMethod("charToInt", charToInt);
	setMethod("parseInt", IntegerClass::parseInt);
	setMethod("parseFloat", IntegerClass::parseFloat);
	setMethod("typeof", typeof_internal);

    // These are not constants so if you're evil you can change them...
    setProperty("AsyncNotification", ApiHelpers::AsyncMagicNumber);
    setProperty("SyncNotification", ApiHelpers::SyncMagicNumber);
}


#if JUCE_MSVC
#pragma warning (push)
#pragma warning (disable : 4702)
#endif


HiseJavascriptEngine::RootObject::Statement::ResultCode HiseJavascriptEngine::RootObject::LockStatement::perform(const Scope& /*s*/, var*) const
{
	if (RegisterName* r = dynamic_cast<RegisterName*>(lockedObj.get()))
	{
		currentLock = &r->rootRegister->getLock(r->indexInRegister);
		return ResultCode::ok;
	}
	else if (ConstReference* cr = dynamic_cast<ConstReference*>(lockedObj.get()))
	{
		var* constObj = cr->ns->constObjects.getVarPointerAt(cr->index);

		if (ApiClass* api = dynamic_cast<ApiClass*>(constObj->getObject()))
		{
			currentLock = &api->apiClassLock;
			return ResultCode::ok;
		}
		else
		{
			currentLock = nullptr;
			location.throwError("Can't lock this object");
			return Statement::ok;
		}
	}
	else
	{
		currentLock = nullptr;
		location.throwError("Can't lock this object");
		return Statement::ok;
	}
}

#if JUCE_MSVC
#pragma warning (pop)
#endif



void HiseJavascriptEngine::RootObject::ArraySubscript::cacheIndex(AssignableObject *instance, const Scope &s) const
{
	if (cachedIndex == -1)
	{
		if (dynamic_cast<LiteralValue*>(index.get()) != nullptr ||
			dynamic_cast<ConstReference*>(index.get()) != nullptr ||
			dynamic_cast<DotOperator*>(index.get()) ||
			dynamic_cast<ApiConstant*>(index.get()))
		{
			if (DotOperator* dot = dynamic_cast<DotOperator*>(index.get()))
			{
				if (dynamic_cast<ConstReference*>(dot->parent.get()) != nullptr)
				{
					if (ConstScriptingObject* cso = dynamic_cast<ConstScriptingObject*>(dot->parent->getResult(s).getObject()))
					{
						int constantIndex = cso->getConstantIndex(dot->child);
						var possibleIndex = cso->getConstantValue(constantIndex);
						if (possibleIndex.isInt() || possibleIndex.isInt64())
						{
							cachedIndex = (int)possibleIndex;
						}
						else location.throwError("[]- access only possible with int values");
					}
					else location.throwError("[]-access using dot operator only valid with const objects as parent");
				}
				else location.throwError("[]-access using dot operator only valid with const objects as parent");
			}
			else
			{
				const var i = index->getResult(s);
				cachedIndex = instance->getCachedIndex(i);

				if (cachedIndex == -1) location.throwError("Property " + i.toString() + " not found");
			}
		}
	}
}


var HiseJavascriptEngine::RootObject::FunctionCall::getResult(const Scope& s) const
{
	try
	{
		if (!initialised)
		{
			initialised = true;

			if (DotOperator* dot = dynamic_cast<DotOperator*> (object.get()))
			{
				parentIsConstReference = dynamic_cast<ConstReference*>(dot->parent.get()) != nullptr;

				if (parentIsConstReference)
				{
					constObject = dynamic_cast<ConstScriptingObject*>(dot->parent->getResult(s).getObject());

					if (constObject != nullptr)
					{
						constObject->getIndexAndNumArgsForFunction(dot->child, functionIndex, numArgs);
						isConstObjectApiFunction = true;

						CHECK_CONDITION_WITH_LOCATION(functionIndex != -1, "function not found");
						CHECK_CONDITION_WITH_LOCATION(numArgs == arguments.size(), "argument amount mismatch: " + String(arguments.size()) + ", Expected: " + String(numArgs));
					}
				}
			}
		}

		if (isConstObjectApiFunction)
		{
			var parameters[5];

			for (int i = 0; i < arguments.size(); i++)
			{
				parameters[i] = arguments[i]->getResult(s);
				HiseJavascriptEngine::checkValidParameter(i, parameters[i], location);
			}
				
#if ENABLE_SCRIPTING_BREAKPOINTS
			if(constObject->wantsCurrentLocation())
				constObject->setCurrentLocation(object->location.externalFile, object->location.getCharIndex());
#endif

			return constObject->callFunction(functionIndex, parameters, numArgs);
		}

		if (DotOperator* dot = dynamic_cast<DotOperator*> (object.get()))
		{
			var thisObject(dot->parent->getResult(s));

			if (ConstScriptingObject* c = dynamic_cast<ConstScriptingObject*>(thisObject.getObject()))
			{
				c->getIndexAndNumArgsForFunction(dot->child, functionIndex, numArgs);

				CHECK_CONDITION_WITH_LOCATION(functionIndex != -1, "function not found");
				CHECK_CONDITION_WITH_LOCATION(numArgs == arguments.size(), "argument amount mismatch: " + String(arguments.size()) + ", Expected: " + String(numArgs));

				var parameters[5];

				for (int i = 0; i < arguments.size(); i++)
					parameters[i] = arguments[i]->getResult(s);

				return c->callFunction(functionIndex, parameters, numArgs);
			}

			if (DynamicObject* dynObj = thisObject.getDynamicObject())
			{
				var property = dynObj->getProperty(dot->child);

				if (auto obj = dynamic_cast<InlineFunction::Object*>(property.getObject()))
				{
					var parameters[5];

					for (int i = 0; i < arguments.size(); i++)
						parameters[i] = arguments[i]->getResult(s);

					return obj->performDynamically(s, parameters, arguments.size());
				}
			}
			if (thisObject.isArray())
			{
				if (auto sf = ArrayClass::getScopedFunction(dot->child))
				{
					s.checkTimeOut(location);
					Array<var> argVars;

					for (auto* a : arguments)
						argVars.add(a->getResult(s));

					const var::NativeFunctionArgs args(thisObject, argVars.begin(), argVars.size());

					return sf(args, s);
				}
			}

			return invokeFunction(s, s.findFunctionCall(location, thisObject, dot->child), thisObject);
		}

		var r = object->getResult(s);

		if (auto obj = dynamic_cast<InlineFunction::Object*>(r.getObject()))
		{
			var parameters[5];

			for (int i = 0; i < arguments.size(); i++)
				parameters[i] = arguments[i]->getResult(s);

			return obj->performDynamically(s, parameters, arguments.size());
		}

		var function(r);
		return invokeFunction(s, function, var(s.scope.get()));
	}
	catch (String& errorMessage)
	{
		throw Error::fromLocation(location, errorMessage);
	}
}

var HiseJavascriptEngine::RootObject::Scope::findFunctionCall(const CodeLocation& location, const var& targetObject, const Identifier& functionName) const
{
	if (DynamicObject* o = targetObject.getDynamicObject())
	{
		if (const var* prop = getPropertyPointer(o, functionName))
			return *prop;

		for (DynamicObject* p = o->getProperty(getPrototypeIdentifier()).getDynamicObject(); p != nullptr;
			p = p->getProperty(getPrototypeIdentifier()).getDynamicObject())
		{
			if (const var* prop = getPropertyPointer(p, functionName))
				return *prop;
		}

		// if there's a class with an overridden DynamicObject::hasMethod, this avoids an error
		if (o->hasMethod(functionName))
			return var();
	}

	if (targetObject.isString())
		if (var* m = findRootClassProperty(StringClass::getClassName(), functionName))
			return *m;

	if (targetObject.isArray())
		if (var* m = findRootClassProperty(ArrayClass::getClassName(), functionName))
			return *m;

	if (var* m = findRootClassProperty(ObjectClass::getClassName(), functionName))
		return *m;

	AudioThreadGuard::Suspender ss(true);

	location.throwError("Unknown function '" + functionName.toString() + "'");
	return var();
}


var HiseJavascriptEngine::callExternalFunctionRaw(var function, const var::NativeFunctionArgs& args)
{
	
	ScopedValueSetter<bool> svs(externalFunctionPending, true);

	prepareTimeout();

	if (auto fo = dynamic_cast<RootObject::FunctionObject*>(function.getObject()))
	{
		return fo->invoke(RootObject::Scope(nullptr, root.get(), root.get()), args);;
	}
	else if (auto ifo = dynamic_cast<RootObject::InlineFunction::Object*>(function.getObject()))
	{
		RootObject::ScopedLocalThisObject sto(*root, args.thisObject);

		auto rv = ifo->performDynamically(RootObject::Scope(nullptr, root.get(), root.get()), const_cast<var*>(args.arguments), args.numArguments);

		return rv;
	}
    
    return var();
}


var HiseJavascriptEngine::callExternalFunction(var function, const var::NativeFunctionArgs& args, Result* result /*= nullptr*/, bool allowMessageThread)
{
#if JUCE_DEBUG
	if (!allowMessageThread)
	{
		auto mc = dynamic_cast<Processor*>(root->hiseSpecialData.processor)->getMainController();
		LockHelpers::noMessageThreadBeyondInitialisation(mc);
	}
#endif

	var returnVal;

	static const Identifier thisIdent("this");

	try
	{
		if(!externalFunctionPending)
			prepareTimeout();

		if (result != nullptr) *result = Result::ok();

		return callExternalFunctionRaw(function, args);
	}
	catch (String &error)
	{
		jassertfalse;
		if (result != nullptr) *result = Result::fail(error);
	}
	catch (RootObject::Error &e)
	{
		static const Identifier func("function");

		if (result != nullptr && root != nullptr)
            *result = Result::fail(root->dumpCallStack(e, func));
	}
	catch (Breakpoint& bp)
	{
		bp.copyLocalScopeToRoot(*root);
		sendBreakpointMessage(bp.index);

		static const Identifier func("function");
		if (result != nullptr && root != nullptr)
            *result = Result::fail(root->dumpCallStack(RootObject::Error::fromBreakpoint(bp), func));
	}

	return returnVal;
}

Array<Identifier> HiseJavascriptEngine::RootObject::HiseSpecialData::hiddenProperties;

bool HiseJavascriptEngine::RootObject::HiseSpecialData::initHiddenProperties = true;

HiseJavascriptEngine::RootObject::HiseSpecialData::HiseSpecialData(RootObject* root_) :
JavascriptNamespace("root"),
root(root_)
{
	if (initHiddenProperties)
	{
		hiddenProperties.addIfNotAlreadyThere(Identifier("exec"));
		hiddenProperties.addIfNotAlreadyThere(Identifier("eval"));
		hiddenProperties.addIfNotAlreadyThere(Identifier("trace"));
		hiddenProperties.addIfNotAlreadyThere(Identifier("charToInt"));
		hiddenProperties.addIfNotAlreadyThere(Identifier("parseInt"));
		hiddenProperties.addIfNotAlreadyThere(Identifier("parseFloat"));
		hiddenProperties.addIfNotAlreadyThere(Identifier("typeof"));
		hiddenProperties.addIfNotAlreadyThere(Identifier("Object"));
		//hiddenProperties.addIfNotAlreadyThere(Identifier("Array"));
		//hiddenProperties.addIfNotAlreadyThere(Identifier("String"));
		hiddenProperties.addIfNotAlreadyThere(Identifier("Math"));
		hiddenProperties.addIfNotAlreadyThere(Identifier("JSON"));
		hiddenProperties.addIfNotAlreadyThere(Identifier("Integer"));
		hiddenProperties.addIfNotAlreadyThere(Identifier("Content"));
		hiddenProperties.addIfNotAlreadyThere(Identifier("SynthParameters"));
		hiddenProperties.addIfNotAlreadyThere(Identifier("Engine"));
		hiddenProperties.addIfNotAlreadyThere(Identifier("Synth"));
		hiddenProperties.addIfNotAlreadyThere(Identifier("Sampler"));
		hiddenProperties.addIfNotAlreadyThere(Identifier("Globals"));
		hiddenProperties.addIfNotAlreadyThere(Identifier("include"));

		initHiddenProperties = false;
	}

	for (int i = 0; i < 32; i++)
	{
		callbackTimes[i] = 0.0;
	}

}

HiseJavascriptEngine::RootObject::HiseSpecialData::~HiseSpecialData()
{
	debugInformation.clear();
}

void HiseJavascriptEngine::RootObject::HiseSpecialData::clear()
{
	clearDebugInformation();
	apiClasses.clear();
	inlineFunctions.clear();
	constObjects.clear();
	callbackNEW.clear();
	globals = nullptr;
}

HiseJavascriptEngine::RootObject::Callback *HiseJavascriptEngine::RootObject::HiseSpecialData::getCallback(const Identifier &callbackId)
{
	for (int i = 0; i < callbackNEW.size(); i++)
	{
		if (callbackNEW[i]->getName() == callbackId)
		{
			return callbackNEW[i].get();
		}
	}

	return nullptr;
}


const HiseJavascriptEngine::RootObject::JavascriptNamespace* HiseJavascriptEngine::RootObject::HiseSpecialData::getNamespace(const Identifier &namespaceId) const
{
	return const_cast<HiseSpecialData*>(this)->getNamespace(namespaceId);
}

HiseJavascriptEngine::RootObject::JavascriptNamespace* HiseJavascriptEngine::RootObject::HiseSpecialData::getNamespace(const Identifier &namespaceId)
{
	static const Identifier r("root");

	if (namespaceId == r) return this;

	for (int i = 0; i < namespaces.size(); i++)
	{
		if (namespaces[i]->id == namespaceId)
		{
			return namespaces[i].get();
		}
	}

	return nullptr;
}


struct ManualGraphicsObject: public DebugableObjectBase
{
	ManualGraphicsObject()
	{};

	Identifier getObjectName() const override { return "Graphics"; };
	Identifier getInstanceName() const override { return "g"; };

	bool isWatchable() const override { return false; };

	void getAllFunctionNames(Array<Identifier>& functions) const override
	{
#if USE_BACKEND
		auto gTree = ApiHelpers::getApiTree().getChildWithName("Graphics");

		for (auto c : gTree)
			functions.add(c.getProperty("name", "unknown").toString());  
#endif
	}
};

struct ManualEventObject : public DebugableObjectBase
{
	Identifier getObjectName() const override { return "event"; };

	int getTypeNumber() const override { return 2; };

	Identifier getInstanceName() const override {
		return "event";
	}

	bool isWatchable() const override { return false; };

	int getNumChildElements() const override
	{
		return MouseCallbackComponent::getCallbackPropertyNames().size();
	}

	DebugInformationBase* getChildElement(int index)
	{
		auto id = MouseCallbackComponent::getCallbackPropertyNames()[index];
		return createDebugInformationForChild(id);
	}

	DebugInformationBase* createDebugInformationForChild(const Identifier& id) override
	{
#define ADD_IF(name, type, description) if(id.toString() == name) return createProperty(name, type, description);
		ADD_IF("mouseDownX", "int", "The x - position of the mouse click");
		ADD_IF("mouseDownY", "int", "the y - position of the mouse click");
		ADD_IF("mouseUp", "bool", "true if the mouse was released");
		ADD_IF("x", "int", "the current mouse x - position");
		ADD_IF("y", "int", "the current mouse y - position");
		ADD_IF("clicked", "bool", "true if the mouse is currently clicked");
		ADD_IF("doubleClick", "bool", "true if the mouse is currently double clicked");
		ADD_IF("rightClick", "bool", "true if the mouse is currently right clicked");
		ADD_IF("drag", "bool", "true if the mouse is currently dragged");
		ADD_IF("dragX", "int", "the drag x - delta from the start");
		ADD_IF("dragY", "int", "the drag y - delta from the start");
		ADD_IF("insideDrag", "bool", "true if the mouse is being dragged inside the component");
		ADD_IF("hover", "bool", "true if the mouse is hovering the component");
		ADD_IF("result", "int", "the result of the popup menue");
		ADD_IF("itemText", "String", "the text of the popup menu");
		ADD_IF("shiftDown", "bool", "true if the shift modifier is pressed");
		ADD_IF("cmdDown", "bool", "true if the cmd modifier is pressed");
		ADD_IF("altDown", "bool", "true if the alt modifier is pressed");
		ADD_IF("ctrlDown", "bool", "true if the ctrl modifier is pressed");
#undef ADD_IF

		return nullptr;
	}

	void getAllConstants(Array<Identifier>& ids) const override
	{
		StringArray eventProperties = MouseCallbackComponent::getCallbackPropertyNames();

		for (auto e : eventProperties)
			ids.add(e);
	}

	DebugInformationBase* createProperty(const String& id, const String& type, const String& description)
	{
		auto s = new SettableDebugInfo();
		s->dataType = type;
		s->typeValue = 2;
		s->name = "event." + id;
		s->codeToInsert = s->name;
		s->description.append("\n" + description, GLOBAL_BOLD_FONT());
		s->category = "Event Callback property";
		
		return s;
	}

	

	
};



void HiseJavascriptEngine::RootObject::HiseSpecialData::createDebugInformation(DynamicObject *rootObject)
{
	ScopedLock sl(debugLock);

	debugInformation.clear();

	WeakReference<HiseSpecialData> safeThis(this);

	for (int i = 0; i < constObjects.size(); i++)
	{
		auto vf = [safeThis, i]()
		{
			if (safeThis == nullptr)
				return var();

			if (auto v = safeThis->constObjects.getVarPointerAt(i))
				return *v;

			return var();
		};

		auto cid = constObjects.getName(i);

		debugInformation.add(new LambdaValueInformation(vf, cid, Identifier(), DebugInformation::Type::Constant, constLocations[i], comments[cid].toString()));
	}

	const int numRegisters = varRegister.getNumUsedRegisters();

	for (int i = 0; i < numRegisters; i++)
	{
		auto vf = [safeThis, i]()
		{
			if (safeThis == nullptr)
				return var();

			if (auto v = safeThis->varRegister.getVarPointer(i))
				return *v;

			return var();
		};

		auto rid = varRegister.getRegisterId(i);

		debugInformation.add(new LambdaValueInformation(vf, rid, Identifier(), DebugInformation::Type::RegisterVariable, registerLocations[i], comments[rid].toString()));
	}
	
	for (int i = 0; i < apiClasses.size(); i++)
	{
		debugInformation.add(new DebugableObjectInformation(apiClasses[i].get(), apiClasses[i]->getObjectName(), DebugableObjectInformation::Type::ApiClass));
		
	}

	if (auto globalObject = rootObject->getProperty("Globals").getDynamicObject())
	{
		for (int i = 0; i < globalObject->getProperties().size(); i++)
			debugInformation.add(new DynamicObjectDebugInformation(globalObject, globalObject->getProperties().getName(i), DebugInformation::Type::Globals));
	}

	for (int i = 0; i < rootObject->getProperties().size(); i++)
	{
		const Identifier propertyId = rootObject->getProperties().getName(i);
		if (hiddenProperties.contains(propertyId)) continue;

		auto t = rootObject->getProperty(propertyId).isMethod() ? DebugInformation::Type::Variables : DebugInformation::Type::ExternalFunction;

		debugInformation.add(new DynamicObjectDebugInformation(rootObject, propertyId, t));
	}

	for (int i = 0; i < namespaces.size(); i++)
	{
		auto ns = namespaces[i].get();

		debugInformation.add(new DebugableObjectInformation(ns, ns->id, DebugInformation::Type::Namespace));

		const int numNamespaceObjects = ns->getNumDebugObjects();

		for (int j = 0; j < numNamespaceObjects; j++)
		{
			debugInformation.add(ns->createDebugInformation(j));
		}
	}

	for (int i = 0; i < inlineFunctions.size(); i++)
	{
		InlineFunction::Object *o = dynamic_cast<InlineFunction::Object*>(inlineFunctions.getUnchecked(i).get());

		auto inlineDebugInfo = new DebugableObjectInformation(o, o->getObjectName(), DebugInformation::Type::InlineFunction);

		debugInformation.add(inlineDebugInfo);
	}

	for (int i = 0; i < callbackNEW.size(); i++)
	{
		if (!callbackNEW[i]->isDefined()) continue;

		debugInformation.add(new DebugableObjectInformation(callbackNEW[i].get(), callbackNEW[i]->getName(), DebugInformation::Type::Callback));
	}


	// Artificially create g object
	{
		debugInformation.add(ManualDebugObject::create<ManualGraphicsObject>());
		debugInformation.add(ManualDebugObject::create<ManualEventObject>());
	}
}


DebugInformation* HiseJavascriptEngine::RootObject::JavascriptNamespace::createDebugInformation(int index)
{
	int prevLimit = 0;
	int upperLimit = varRegister.getNumUsedRegisters();

	WeakReference<JavascriptNamespace> safeThis(const_cast<JavascriptNamespace*>(this));

	if (index < upperLimit)
	{
		auto vf = [safeThis, index]()
		{
			if (safeThis != nullptr)
			{
				if (auto v = safeThis->varRegister.getVarPointer(index))
					return *v;
			}

			return var();
		};

		auto rid = varRegister.getRegisterId(index);

		DebugInformation* di = new LambdaValueInformation(vf, rid, id, DebugInformation::Type::RegisterVariable, registerLocations[index], comments[rid].toString());
		return di;
	}

	prevLimit = upperLimit;
	upperLimit += inlineFunctions.size();

	if (index < upperLimit)
	{
		const int inlineIndex = index - prevLimit;

		InlineFunction::Object *o = dynamic_cast<InlineFunction::Object*>(inlineFunctions.getUnchecked(inlineIndex).get());

		return new DebugableObjectInformation(o, o->name, DebugInformation::Type::InlineFunction, id, o->getComment());
	}

	prevLimit = upperLimit;
	upperLimit += constObjects.size();

	if (index < upperLimit)
	{
		const int constIndex = index - prevLimit;

		auto vf = [safeThis, constIndex]()
		{
			if (safeThis != nullptr)
			{
				if (auto v = safeThis->constObjects.getVarPointerAt(constIndex))
					return *v;
			}

			return var();
		};

		auto cid = constObjects.getName(constIndex);

		DebugInformation* di = new LambdaValueInformation(vf, 
													      cid, 
														  id, 
														  DebugInformation::Type::Constant, constLocations[constIndex],
														  comments[cid].toString());
	
		return di;
	}

	return nullptr;
}


DynamicObject* HiseJavascriptEngine::RootObject::HiseSpecialData::getInlineFunction(const Identifier &inlineFunctionId)
{
	const String idAsString = inlineFunctionId.toString();

	if (idAsString.contains("."))
	{
		Identifier ns(idAsString.upToFirstOccurrenceOf(".", false, false));
		Identifier fn(idAsString.fromFirstOccurrenceOf(".", false, false));

		JavascriptNamespace* n = getNamespace(Identifier(ns));

		if (n != nullptr)
		{
			for (int i = 0; i < n->inlineFunctions.size(); i++)
			{
				if (dynamic_cast<InlineFunction::Object*>(n->inlineFunctions[i].get())->name == fn)
				{
					return n->inlineFunctions[i].get();
				}
			}
		}
	}
	else
	{
		for (int i = 0; i < inlineFunctions.size(); i++)
		{
			if (dynamic_cast<InlineFunction::Object*>(inlineFunctions[i].get())->name == inlineFunctionId)
			{
				return inlineFunctions[i].get();
			}
		}
	}

	return nullptr;
}


void HiseJavascriptEngine::RootObject::HiseSpecialData::throwExistingDefinition(const Identifier &name, VariableStorageType type, CodeLocation &l)
{
	String typeName;

	switch (type)
	{
	case HiseJavascriptEngine::RootObject::HiseSpecialData::VariableStorageType::Undeclared: typeName = "undeclared";
		break;
	case HiseJavascriptEngine::RootObject::HiseSpecialData::VariableStorageType::LocalScope: typeName = "local variable";
		break;
	case HiseJavascriptEngine::RootObject::HiseSpecialData::VariableStorageType::RootScope: typeName = "variable";
		break;
	case HiseJavascriptEngine::RootObject::HiseSpecialData::VariableStorageType::Register: typeName = "register variable";
		break;
	case HiseJavascriptEngine::RootObject::HiseSpecialData::VariableStorageType::ConstVariables: typeName = "const variable";
		break;
	case HiseJavascriptEngine::RootObject::HiseSpecialData::VariableStorageType::Globals: typeName = "global variable";
		break;
	default:
		break;
	}

	l.throwError("Identifier " + name.toString() + " is already defined as " + typeName);
}

void HiseJavascriptEngine::RootObject::HiseSpecialData::checkIfExistsInOtherStorage(VariableStorageType thisType, const Identifier &name, CodeLocation& l)
{
	VariableStorageType type = getExistingVariableStorage(name);

	switch (thisType)
	{
	case HiseJavascriptEngine::RootObject::HiseSpecialData::VariableStorageType::Undeclared:
		break;

	case HiseJavascriptEngine::RootObject::HiseSpecialData::VariableStorageType::LocalScope:
		break;

	case HiseJavascriptEngine::RootObject::HiseSpecialData::VariableStorageType::RootScope:

		if (type == VariableStorageType::ConstVariables || type == VariableStorageType::Globals || type == VariableStorageType::Register)
			throwExistingDefinition(name, type, l);
		break;

	case HiseJavascriptEngine::RootObject::HiseSpecialData::VariableStorageType::Register:

		if (type == VariableStorageType::ConstVariables || type == VariableStorageType::Globals || type == VariableStorageType::RootScope)
			throwExistingDefinition(name, type, l);
		break;

	case HiseJavascriptEngine::RootObject::HiseSpecialData::VariableStorageType::ConstVariables:

		if (type == VariableStorageType::Register || type == VariableStorageType::Globals || type == VariableStorageType::RootScope)
			throwExistingDefinition(name, type, l);
		break;

	case HiseJavascriptEngine::RootObject::HiseSpecialData::VariableStorageType::Globals:

		if (type == VariableStorageType::RootScope || type == VariableStorageType::ConstVariables || type == VariableStorageType::Register)
			throwExistingDefinition(name, type, l);
		break;
	default:
		break;
	}
}

HiseJavascriptEngine::RootObject::HiseSpecialData::VariableStorageType HiseJavascriptEngine::RootObject::HiseSpecialData::getExistingVariableStorage(const Identifier &name)
{
	if (constObjects.contains(name)) return VariableStorageType::ConstVariables;
	else if (varRegister.getRegisterIndex(name) != -1) return VariableStorageType::Register;
	else if (globals->getProperties().contains(name)) return VariableStorageType::Globals;
	else if (root->getProperties().contains(name)) return VariableStorageType::RootScope;
	else return VariableStorageType::Undeclared;
}


var HiseJavascriptEngine::getInlineFunction(const Identifier& id)
{
	if (auto r = dynamic_cast<HiseJavascriptEngine::RootObject*>(getRootObject()))
	{
		return var(r->hiseSpecialData.getInlineFunction(id));
	}

	return {};
}


juce::StringArray HiseJavascriptEngine::getInlineFunctionNames(int numArgs /*= -1*/)
{
	if (auto r = dynamic_cast<HiseJavascriptEngine::RootObject*>(getRootObject()))
	{
		StringArray list;

		auto addAll = [numArgs](RootObject::JavascriptNamespace* ns, StringArray& sa)
		{
			String prefix = ns->id == Identifier("root") ? "" : ns->id.toString() + ".";

			for (auto f_ : ns->inlineFunctions)
			{
				if (auto f = dynamic_cast<HiseJavascriptEngine::RootObject::InlineFunction::Object*>(f_))
				{
					if (numArgs != -1 && f->parameterNames.size() != numArgs)
						continue;

					sa.add(prefix + f->name.toString());
				}
			}
		};

		addAll(&r->hiseSpecialData, list);

		for (auto ns : r->hiseSpecialData.namespaces)
		{
			addAll(ns, list);
		}
		
		return list;
	}

	return {};
}



var HiseJavascriptEngine::executeInlineFunction(var inlineFunction, var* arguments, Result* result, int numArgs)
{
#if JUCE_DEBUG
	auto mc = dynamic_cast<Processor*>(root->hiseSpecialData.processor)->getMainController();
	LockHelpers::noMessageThreadBeyondInitialisation(mc);
#endif

	auto f = static_cast<HiseJavascriptEngine::RootObject::InlineFunction::Object*>(inlineFunction.getObject());

	if (f == nullptr)
	{
		if (result != nullptr)
			*result = Result::fail("No valid function");

		return {};
	};

	if (numArgs == -1)
		numArgs = f->parameterNames.size();

	if (f->parameterNames.size() != numArgs)
	{
		if(result != nullptr)
			*result = Result::fail("Argument amount mismatch.");

		return {};
	}

	auto rootObj = getRootObject();
	auto s = HiseJavascriptEngine::RootObject::Scope(nullptr, static_cast<HiseJavascriptEngine::RootObject*>(rootObj), rootObj);

	try
	{
		prepareTimeout();
		if (result != nullptr) *result = Result::ok();
		return f->performDynamically(s, arguments, numArgs);
	}
	catch (String &error)
	{
		jassertfalse;
		*result = Result::fail(error);
	}
	catch (RootObject::Error &e)
	{
		if (result != nullptr) *result = Result::fail(root->dumpCallStack(e, f->name));

		f->cleanUpAfterExecution();
	}
	catch (Breakpoint& bp)
	{
		if(bp.localScope == nullptr)
			bp.localScope = f->createDynamicObjectForBreakpoint().getDynamicObject();

		bp.copyLocalScopeToRoot(*root);

		sendBreakpointMessage(bp.index);
		
		if (result != nullptr) *result = Result::fail(root->dumpCallStack(RootObject::Error::fromBreakpoint(bp), f->name));

		f->cleanUpAfterExecution();
	}

	return var();
}

juce::String::CharPointerType HiseJavascriptEngine::RootObject::Callback::getProgramPtr() const
{
	return statements->location.program.getCharPointer();
}

var HiseJavascriptEngine::executeCallback(int callbackIndex, Result *result)
{
#if JUCE_DEBUG
	auto mc = dynamic_cast<Processor*>(root->hiseSpecialData.processor)->getMainController();
	LockHelpers::noMessageThreadBeyondInitialisation(mc);
#endif

	RootObject::Callback *c = root->hiseSpecialData.callbackNEW[callbackIndex].get();
	

	// You need to register the callback correctly...
	jassert(c != nullptr);

	if (c != nullptr && c->isDefined())
	{
		try
		{
			prepareTimeout();

			var returnVal = c->perform(root.get());

			if (result != nullptr) *result = Result::ok();

			c->cleanLocalProperties();

			return returnVal;
		}
		catch (String &error)
		{
			jassertfalse;
			if (result != nullptr) *result = Result::fail(error);
		}
		catch (RootObject::Error &e)
		{
			AudioThreadGuard::Suspender suspender;
			ignoreUnused(suspender);

			if (result != nullptr) *result = Result::fail(root->dumpCallStack(e, c->getName()));
		}
		catch (Breakpoint& bp)
		{
			if(bp.localScope == nullptr)
				bp.localScope = c->createDynamicObjectForBreakpoint().getDynamicObject();

			bp.copyLocalScopeToRoot(*root);

			sendBreakpointMessage(bp.index);
			
			if (result != nullptr) *result = Result::fail(root->dumpCallStack(RootObject::Error::fromBreakpoint(bp), c->getName()));
		}
	}

	c->cleanLocalProperties();

	return var();
}


void HiseJavascriptEngine::RootObject::Callback::setStatements(BlockStatement *s) noexcept
{
	statements = s;
	isCallbackDefined = s->statements.size() != 0;
}


var HiseJavascriptEngine::RootObject::Callback::perform(RootObject *root)
{
	RootObject::Scope s(nullptr, root, root);

	var returnValue = var::undefined();

#if USE_BACKEND
	const double pre = Time::getMillisecondCounterHiRes();


	root->addToCallStack(callbackName, nullptr);


    LocalScopeCreator::ScopedSetter svs(root, this);

	statements->perform(s, &returnValue);

	root->removeFromCallStack(callbackName);

	const double post = Time::getMillisecondCounterHiRes();
	lastExecutionTime = post - pre;
#else
	statements->perform(s, &returnValue);
#endif

	return returnValue;
}

AttributedString DynamicObjectDebugInformation::getDescription() const
{
	return AttributedString();
}

void ScriptingObject::logErrorAndContinue(const String &errorMessage) const
{
#if USE_BACKEND
    
    auto mc = getScriptProcessor()->getMainController_();
    auto chain = const_cast<ModulatorSynthChain*>(mc->getMainSynthChain());
    
    debugError(chain, errorMessage);
    
#else
    ignoreUnused(errorMessage);
    DBG(errorMessage);
#endif
}
    
void ScriptingObject::reportScriptError(const String &errorMessage) const
{
#if USE_BACKEND

	if (CompileExporter::isExportingFromCommandLine())
		throw CommandLineException(errorMessage);
	else
		throw errorMessage;
#else
	
#if JUCE_DEBUG
	DBG(errorMessage);
#else
	ignoreUnused(errorMessage);
#endif

#endif
}




String JavascriptProcessor::Helpers::stripUnusedNamespaces(const String &code, int& counter)
{
	jassertfalse;

	HiseJavascriptEngine::RootObject::ExpressionTreeBuilder it(code, "", nullptr);

	try
	{
		String returnString = it.removeUnneededNamespaces(counter);
		return returnString;
	}
	catch (String &e)
	{
		Logger::getCurrentLogger()->writeToLog(e);
		return code;
	}
}

String JavascriptProcessor::Helpers::uglify(const String& prettyCode)
{
	jassertfalse;

	HiseJavascriptEngine::RootObject::ExpressionTreeBuilder it(prettyCode, "", nullptr);

	try
	{
		String returnString = it.uglify();
		return returnString;
	}
	catch (String &e)
	{
		Logger::getCurrentLogger()->writeToLog(e);
		return prettyCode;
	}
}



} // namespace hise

