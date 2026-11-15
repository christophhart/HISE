#include "HiseJavascriptEngine.h"
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
    
    auto hasLocalSwitch = code.startsWith(snex::jit::PreprocessorTokens::on_)
                       || code.startsWith(snex::jit::PreprocessorTokens::strict_)
                       || code.startsWith(snex::jit::PreprocessorTokens::warn_)
                       || code.startsWith(snex::jit::PreprocessorTokens::unsafe_);
    
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



#if USE_BACKEND

void HiseJavascriptPreprocessor::setEnableGlobalPreprocessor(bool shouldBeEnabled)
{
	globalEnabled = shouldBeEnabled;
}

void HiseJavascriptPreprocessor::reset()
{
	deactivatedLines.clear();
	definitions.clear();
}

SparseSet<int> HiseJavascriptPreprocessor::getDeactivatedLinesForFile(const String& fileId)
{
	jassert(fileId.isNotEmpty());
	return deactivatedLines[fileId];
}

UriLocation HiseJavascriptPreprocessor::getLocationForPreprocessor(const String& id) const
{
	for (const auto& d: definitions)
	{
		if (d.name == id)
			return d.location;
	}
        
	return {};
}

#endif

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

bool HiseJavascriptEngine::hasBreakpoints(const var& v)
{
	if (auto obj = dynamic_cast<RootObject::LocalScopeCreator*>(v.getObject()))
	{
		return obj->hasBreakpoints;
	}

	return false;
}

HiseJavascriptEngine::ExternalFileData::ExternalFileData(Type t_, const File& f_, const String& name_): t(t_), f(f_), r(Result::ok())
{
	switch (t_)
	{
	case HiseJavascriptEngine::ExternalFileData::Type::RelativeFile:
		scriptName = f.getFileName();
		break;
	case HiseJavascriptEngine::ExternalFileData::Type::AbsoluteFile:
		scriptName = f.getFullPathName();
		break;
	case HiseJavascriptEngine::ExternalFileData::Type::EmbeddedScript:
		scriptName = name_;
		break;
	case HiseJavascriptEngine::ExternalFileData::Type::numTypes:
		break;
	default:
		break;
	}
}

HiseJavascriptEngine::ExternalFileData::ExternalFileData(): f(File()), r(Result::fail("uninitialised"))
{}

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
#if USE_BACKEND
			CallScopeAnalyzer::ScopedHolderSetter svs(p, fo);
#endif
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

#if USE_BACKEND
		CallScopeAnalyzer::ScopedHolderSetter svs(p, c);
#endif
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
	setMethod("isNaN", IntegerClass::isNaN_);
	setMethod("isFinite", IntegerClass::isFinite_);
	setMethod("typeof", typeof_internal);
	setMethod("Rectangle", ApiHelpers::createRectangle);

	// These are not constants so if you're evil you can change them...
	setProperty("AsyncNotification", ApiHelpers::AsyncMagicNumber);
	setProperty("AsyncHiPriorityNotification", ApiHelpers::AsyncHiPriorityMagicNumber);
	setProperty("SyncNotification", ApiHelpers::SyncMagicNumber);
}


#if JUCE_MSVC
#pragma warning (push)
#pragma warning (disable : 4702)
#endif

HiseJavascriptEngine::RootObject::~RootObject()
{
	jassert(!currentlyExecuting);
}

var HiseJavascriptEngine::RootObject::FunctionCall::getResult(const Scope& s) const
{
	auto callWithCallstack = [this](const Scope& s, ConstScriptingObject* c, const Identifier& functionName, int functionIndex, var* parameterList, int numParameters)
	{
#if ENABLE_SCRIPTING_BREAKPOINTS

		bool hasCallableParameter = false;

		for (int i = 0; i < numParameters; i++)
		{
			hasCallableParameter |= dynamic_cast<WeakCallbackHolder::CallableObject*>(parameterList[i].getObject()) != nullptr;
		}

		if (hasCallableParameter)
		{
			String callId;
			callId << c->getObjectName() << "." << functionName;
			RootObject::ScopedCallstackEntry scs(*s.root, Identifier(callId), &location);
			return c->callFunction(functionIndex, parameterList, numParameters);
		}
#endif

		return c->callFunction(functionIndex, parameterList, numParameters);
	};




#if HISE_SHADOW_PARSER_ENABLED
	if (s.isShadowParsing())
	{
		ObjectTypeMap::ScopedCallStack sc(s.currentShadowParser, this);

		if (auto dot = dynamic_cast<DotOperator*> (object.get()))
		{
			auto parentValue = dot->parent->getResult(s);

			dot->parent->resolveTypeDefinitionInScopeTree(s, parentValue, dot->child.toString() + "()", false);

			if(auto lb = s.getShadowParser()->createTypeInfoObject(parentValue, ""))
			{
				Array<var> thisArgs;

				for (auto a : arguments)
					thisArgs.add(a->getResult(s));

				ObjectTypeMap::ScopedLocationSetter sp(*s.getShadowParser(), this);
				logDiagnostics(s, lb->runTests(s.currentShadowParser, dot->child, thisArgs));

				return lb->getReturnType(dot->child, thisArgs);
			}
		}
		else
		{
			auto f = object->getResult(s);

			object->resolveTypeDefinitionInScopeTree(s, f, "function call", false);

			if (auto fo = dynamic_cast<FunctionObject*>(f.getObject()))
			{
				Array<var> args;

				for (auto a : arguments)
					args.add(a->getResult(s));

				auto lv = WeakCallbackHolder::CallableObject::StrictnessLevel::Unset;

				auto rv = fo->invokeShadowParse(s, var::NativeFunctionArgs(var(), args.getRawDataPointer(), args.size()), lv);

				if (rv.second.shouldReport())
					logDiagnostics(s, rv.second);

				return rv.first;
			}
			if (auto ifo = dynamic_cast<InlineFunction::Object*>(f.getObject()))
			{
				Array<var> args;

				for (auto a : arguments)
					args.add(a->getResult(s));

				auto lv = WeakCallbackHolder::CallableObject::StrictnessLevel::Unset;

				auto rv = ifo->invokeShadowParse(s, var::NativeFunctionArgs(var(), args.getRawDataPointer(), args.size()), lv);

				if (rv.second.shouldReport())
					logDiagnostics(s, rv.second);

				return rv.first;
			}
			if (auto nf = dynamic_cast<InbuiltFunction*>(f.getObject()))
			{
				Array<var> args;

				for (auto a : arguments)
					args.add(a->getResult(s));

				logDiagnostics(s, nf->runTests(s.getShadowParser(), nf->getObjectName(), args));
				
				return nf->getReturnType(nf->getObjectName(), { var("") });
			}
			else
			{
				logDiagnostics(s, DiagnosticBase::DiagnosticResult::fail("This expression is not a function: " + object->getProfileName()));
			}
		}
	}
#endif

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
						auto numExpectedArgs = arguments.size();

						constObject->getIndexAndNumArgsForFunction(dot->child, functionIndex, numArgs);
						
						isConstObjectApiFunction = true;
                        
#if ENABLE_SCRIPTING_SAFE_CHECKS
                        types = constObject->getForcedParameterTypes(functionIndex, numArgs);
						
#endif

#if ENABLE_SCRIPTING_BREAKPOINTS
						constFunctionId = dot->child;
#endif

						CHECK_CONDITION_WITH_LOCATION(functionIndex != -1, "function not found");
						CHECK_CONDITION_WITH_LOCATION(numArgs == numExpectedArgs, "argument amount mismatch: " + String(arguments.size()) + ", Expected: " + String(numArgs));
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
                
#if ENABLE_SCRIPTING_SAFE_CHECKS
				logDiagnosticsOrThrow(s, HiseJavascriptEngine::checkValidParameter(i, parameters[i], types[i]));
#endif
			}
				
#if ENABLE_SCRIPTING_BREAKPOINTS
			if(constObject->wantsCurrentLocation())
				constObject->setCurrentLocation(object->location.externalFile, object->location.getCharIndex());
#endif

			return callWithCallstack(s, constObject, constFunctionId, functionIndex, parameters, numArgs);
		}

		if (DotOperator* dot = dynamic_cast<DotOperator*> (object.get()))
		{
			var thisObject(dot->parent->getResult(s));

			

			if (ConstScriptingObject* c = dynamic_cast<ConstScriptingObject*>(thisObject.getObject()))
			{
				c->getIndexAndNumArgsForFunction(dot->child, functionIndex, numArgs);
                
#if ENABLE_SCRIPTING_SAFE_CHECKS
                types = c->getForcedParameterTypes(functionIndex, numArgs);
#endif

				CHECK_CONDITION_WITH_LOCATION(functionIndex != -1, "function not found");
				CHECK_CONDITION_WITH_LOCATION(numArgs == arguments.size(), "argument amount mismatch: " + String(arguments.size()) + ", Expected: " + String(numArgs));

				var parameters[5];
				bool hasCallableParameter = false;

				for (int i = 0; i < arguments.size(); i++)
                {
                    parameters[i] = arguments[i]->getResult(s);
                    
#if USE_BACKEND && HISE_WARN_UNDEFINED_PARAMETER_CALLS
                    if(parameters[i].isUndefined() || parameters[i].isVoid())
                    {
                        auto p = dynamic_cast<Processor*>(c->getScriptProcessor());
                        
                        auto warn = (bool)GET_HISE_SETTING(p, HiseSettings::Scripting::WarnIfUndefinedParameters);
                        
                        if(warn)
                        {
                            String errorMessage = "Warning: undefined parameter " + String(i);
                            auto e = s.root->fromLocation(location, errorMessage);
							debugError(p, e.toConsoleMessage());
                        }
                        
                        continue;
                    }
#endif
                    
#if ENABLE_SCRIPTING_SAFE_CHECKS
                    logDiagnosticsOrThrow(s, HiseJavascriptEngine::checkValidParameter(i, parameters[i], types[i]));
#endif
                }

				return callWithCallstack(s, c, dot->child, functionIndex, parameters, numArgs);
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

			return invokeFunction(s, s.findFunctionCall(dot->parent->location, thisObject, dot->child), thisObject);
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
		throw s.root->fromLocation(location, errorMessage);
	}
}

var HiseJavascriptEngine::RootObject::FunctionObject::invoke(const Scope& s, const var::NativeFunctionArgs& args) const
{
	WARN_IF_AUDIO_THREAD(true, ScriptAudioThreadGuard::FunctionCall);

	DynamicObject::Ptr functionRoot(new DynamicObject());

	static const Identifier thisIdent("this");
	functionRoot->setProperty(thisIdent, args.thisObject);

	for (int i = 0; i < parameters.size(); ++i)
		functionRoot->setProperty(parameters.getReference(i),
			i < args.numArguments ? args.arguments[i] : var::undefined());

	if (!capturedLocals.isEmpty())
	{
		for (const auto& c : capturedLocalValues)
			functionRoot->setProperty(c.name, c.value);
	}

	var result;

#if ENABLE_SCRIPTING_BREAKPOINTS

	LocalScopeCreator::ScopedSetter sls(s.root, const_cast<FunctionObject*>(this));

	{
		SimpleReadWriteLock::ScopedWriteLock sl(debugScopeLock);
		lastScope = functionRoot;
	}
#endif

	jassert(s.root->threadAwareLocalScopeCreator.get()->getRootBlockStatement() == body.get());

	body->perform(Scope(&s, s.root.get(), functionRoot.get()), &result);

#if ENABLE_SCRIPTING_SAFE_CHECKS
	if (enableCycleCheck)
		lastScopeForCycleCheck = var(functionRoot.get());
#endif

	functionRoot->removeProperty("this");



	return result;
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

	location.throwError(*root, "Unknown function '" + functionName.toString() + "'");
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


var HiseJavascriptEngine::callExternalFunction(var function, const var::NativeFunctionArgs& args, UriError* result /*= nullptr*/, bool allowMessageThread)
{
#if JUCE_DEBUG
	if (!allowMessageThread)
	{
		auto mc = dynamic_cast<Processor*>(root->hiseSpecialData.processor)->getMainController();
		LockHelpers::noMessageThreadBeyondInitialisation(mc);
	}
#endif

	var returnVal;

	auto wrapError = [&](const UriError& e)
	{
		if (auto c = dynamic_cast<DebugableObject*>(function.getObject()))
			return e.withBaseCallstack({ c->getObjectName().toString(), c->getLocation() });

		return e;
	};

	static const Identifier thisIdent("this");

	try
	{
		if(!externalFunctionPending)
			prepareTimeout();

		if (result != nullptr) *result = UriError::ok();

		return callExternalFunctionRaw(function, args);
	}
	catch (String &error)
	{
		jassertfalse;
		if (result != nullptr)
			*result = wrapError(UriError::fail(error));
	}
	catch (UriError &e)
	{
		if (result != nullptr)
			*result = wrapError(e);
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
		hiddenProperties.addIfNotAlreadyThere(Identifier("Rectangle"));
		hiddenProperties.addIfNotAlreadyThere(Identifier("parseFloat"));
		hiddenProperties.addIfNotAlreadyThere(Identifier("typeof"));
		hiddenProperties.addIfNotAlreadyThere(Identifier("Object"));
		//hiddenProperties.addIfNotAlreadyThere(Identifier("Array"));
		//hiddenProperties.addIfNotAlreadyThere(Identifier("String"));
		hiddenProperties.addIfNotAlreadyThere(Identifier("Math"));
		hiddenProperties.addIfNotAlreadyThere(Identifier("Threads"));
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

HiseJavascriptEngine::RootObject::HiseSpecialData* HiseJavascriptEngine::RootObject::HiseSpecialData::createShadowParseData(DiagnosticBase::ShadowParserBase* shadowParser)
{
	auto hd = new HiseSpecialData(root);
	hd->setProcessor(processor);

	hd->globals = new DynamicObject();

	for (auto d : globals->getProperties())
		hd->globals->setProperty(d.name, d.value);

	for (auto ac : apiClasses)
		hd->apiClasses.add(ac);

	for (auto ac : apiIds)
		hd->apiIds.add(ac);

	for (auto c : callbackNEW)
	{
		hd->callbackIds.add(c->getName());
		hd->callbackNEW.add(new Callback(c->getName(), c->getNumArgs(), 0.0));
	}
	
	hd->currentShadowParser = shadowParser;
	return hd;
}

void HiseJavascriptEngine::RootObject::HiseSpecialData::shadowParseCallback(DiagnosticBase::ShadowParserBase* shadowParser, const Identifier& callbackId)
{
	for (auto cb : callbackNEW)
	{
		if (cb->getName() == callbackId)
		{
			Array<var> args;

			for (int i = 0; i < cb->getNumArgs(); i++)
				args.add(var(1));

			cb->invokeShadowParse(dynamic_cast<ObjectTypeMap*>(shadowParser)->getCurrentScope(), var::NativeFunctionArgs(var(), args.getRawDataPointer(), args.size()));

			break;
		}
	}
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
		ADD_IF("drag", "bool", "true if the mouse is currently dragged or clicked");
		ADD_IF("isDragOnly", "bool", "true if the mouse is currently dragged only (false on clicked)");
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


String HiseJavascriptEngine::RootObject::HiseSpecialData::throwExistingDefinition(const Identifier &name, VariableStorageType type)
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

	String msg;
	msg << "Identifier " << name.toString() << " is already defined as " << typeName;
	return msg;
}


String HiseJavascriptEngine::RootObject::HiseSpecialData::checkIfExistsInOtherStorage(VariableStorageType thisType, const Identifier &name)
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
			return throwExistingDefinition(name, type);
		break;

	case HiseJavascriptEngine::RootObject::HiseSpecialData::VariableStorageType::Register:

		if (type == VariableStorageType::ConstVariables || type == VariableStorageType::Globals || type == VariableStorageType::RootScope)
			return throwExistingDefinition(name, type);
		break;

	case HiseJavascriptEngine::RootObject::HiseSpecialData::VariableStorageType::ConstVariables:

		if (type == VariableStorageType::Register || type == VariableStorageType::Globals || type == VariableStorageType::RootScope)
			return throwExistingDefinition(name, type);
		break;

	case HiseJavascriptEngine::RootObject::HiseSpecialData::VariableStorageType::Globals:

		if (type == VariableStorageType::RootScope || type == VariableStorageType::ConstVariables || type == VariableStorageType::Register)
			return throwExistingDefinition(name, type);
		break;
	default:
		break;
	}

	return {};
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



var HiseJavascriptEngine::executeInlineFunction(var inlineFunction, var* arguments, UriError* result, int numArgs)
{
	RootObject::ScopedWaker sw(*root);
	
#if JUCE_DEBUG
	auto mc = dynamic_cast<Processor*>(root->hiseSpecialData.processor)->getMainController();
	LockHelpers::noMessageThreadBeyondInitialisation(mc);
#endif

	auto f = static_cast<HiseJavascriptEngine::RootObject::InlineFunction::Object*>(inlineFunction.getObject());

	if (f == nullptr)
	{
		if (result != nullptr)
			*result = UriError::fail("No valid function");

		return {};
	};

	if (numArgs == -1)
		numArgs = f->parameterNames.size();

	if (f->parameterNames.size() != numArgs)
	{
		if(result != nullptr)
			*result = UriError::fail("Argument amount mismatch.");

		return {};
	}

	auto rootObj = getRootObject();
	auto s = HiseJavascriptEngine::RootObject::Scope(nullptr, static_cast<HiseJavascriptEngine::RootObject*>(rootObj), rootObj);

	try
	{
		prepareTimeout();

		if (result != nullptr) 
			*result = UriError::ok();

		return f->performDynamically(s, arguments, numArgs);
	}
	catch (String &error)
	{
		jassertfalse;
		if (result != nullptr)
		{
			*result = UriError::fail(error);
			root->attachCallstack(*result);
		}
	}
	catch (UriError &e)
	{
		if (result != nullptr)
			*result = e;

		f->cleanUpAfterExecution();
	}

	return var();
}

juce::String::CharPointerType HiseJavascriptEngine::RootObject::Callback::getProgramPtr() const
{
	return statements->location.program.getCharPointer();
}

var HiseJavascriptEngine::executeCallback(int callbackIndex, UriError *result)
{
	RootObject::ScopedWaker sw(*root);

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

			if (result != nullptr) *result = UriError::ok();

			c->cleanLocalProperties();

			return returnVal;
		}
		catch (String &error)
		{
			jassertfalse;
			if (result != nullptr)
			{
				*result = UriError::fail(error);
				root->attachCallstack(*result);
			}
				
		}
		catch (UriError &e)
		{
			AudioThreadGuard::Suspender suspender;
			ignoreUnused(suspender);

			if (result != nullptr)
				*result = e.withProcessorId(root->hiseSpecialData.processor);
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

bool HiseJavascriptEngine::RootObject::Callback::isDefined() const noexcept
{ return isCallbackDefined; }

Identifier HiseJavascriptEngine::RootObject::Callback::getObjectName() const
{ return getName(); }

const Identifier& HiseJavascriptEngine::RootObject::Callback::getName() const
{ return callbackName; }

int HiseJavascriptEngine::RootObject::Callback::getNumArgs() const
{ return numArgs; }

String HiseJavascriptEngine::RootObject::Callback::getDebugDataType() const
{ return "Callback"; }

String HiseJavascriptEngine::RootObject::Callback::getDebugName() const
{ return callbackName.toString() + "()"; }

HiseJavascriptEngine::RootObject::LocalScopeCreator::ScopeData HiseJavascriptEngine::RootObject::Callback::createScope(RootObject* r)
{
	using LSC = HiseJavascriptEngine::RootObject::LocalScopeCreator;
	
	LSC::ScopeData sd;

	for (int i = 0; i < numArgs; i++)
	{
		LSC::Item item;
		item.id = parameters[i];
		item.value = parameterValues[i];
		item.type = LSC::DataType::Parameter;
		item.source = this;

		sd.push_back(item);
	}

	for (auto& nv : localProperties)
	{
		LSC::Item item;
		item.id = nv.name;
		item.value = nv.value;
		item.type = LSC::DataType::LocalVar;
		item.source = this;

		sd.push_back(item);
	}

	return sd;
}

int HiseJavascriptEngine::RootObject::Callback::getNumChildElements() const
{
	return getNumArgs() + localProperties.size();
}

void HiseJavascriptEngine::RootObject::Callback::setParameterValue(int parameterIndex, const var& newValue)
{
	parameterValues[parameterIndex] = newValue;
}

var* HiseJavascriptEngine::RootObject::Callback::getVarPointer(const Identifier& id)
{
	for (int i = 0; i < 4; i++)
	{
		if (id == parameters[i]) return &parameterValues[i];
	}

	return nullptr;
}

String HiseJavascriptEngine::RootObject::Callback::getDebugValue() const
{
	const double percentage = lastExecutionTime / bufferTime * 100.0;
	return String(percentage, 2) + "%";
}

var HiseJavascriptEngine::RootObject::Callback::createDynamicObjectForBreakpoint()
{
	auto object = new DynamicObject();
	auto arguments = new DynamicObject();

	for (int i = 0; i < numArgs; i++)
		arguments->setProperty(parameters[i], parameterValues[i]);

	auto locals = new DynamicObject();

	for (int i = 0; i < localProperties.size(); i++)
		locals->setProperty(localProperties.getName(i), localProperties.getValueAt(i));

	object->setProperty("args", var(arguments));
	object->setProperty("locals", var(locals));

	return var(object);
}

var HiseJavascriptEngine::RootObject::Callback::invokeShadowParse(const Scope& s, const var::NativeFunctionArgs& args)
{
	if (statements == nullptr)
		return var();

	var rv;

	if (auto st = s.getShadowParser())
	{
		ObjectTypeMap::TypeScope ts(*st, statements->getBlockCharIndexRange(), st->getStrictnessForCallback(callbackName), WeakCallbackHolder::CallableObject::CallScope::Safe);


		ObjectTypeMap::ScopedCallStack sc(st, statements, callbackName.toString());

		auto obj = ts.getCurrentScopeObject();

		if (args.thisObject.isObject())
			obj->setProperty("this", args.thisObject);

		Array<var> thisArgs;

		for (int i = 0; i < getNumArgs(); i++)
		{
			auto id = parameters[i];

			if (isPositiveAndBelow(i, args.numArguments))
				obj->setProperty(id, args.arguments[i]);
		}

		statements.get()->perform(*ts.getCurrentScope(), &rv);
	}

	return rv;

	return var();
}

void HiseJavascriptEngine::RootObject::Callback::doubleClickCallback(const MouseEvent& mouseEvent, Component* component)
{
	DBG("JUMP");
}

void HiseJavascriptEngine::RootObject::Callback::cleanLocalProperties()
{
	// Only clear the local properties when the breakpoints are disabled
	// to allow the inspection of local variables
#if !ENABLE_SCRIPTING_BREAKPOINTS
				if (!localProperties.isEmpty())
				{
					for (int i = 0; i < localProperties.size(); i++)
						*localProperties.getVarPointerAt(i) = var();
				}

				for (int i = 0; i < numArgs; i++)
					parameterValues[i] = var();
#endif
}


var HiseJavascriptEngine::RootObject::Callback::perform(RootObject *root)
{
	RootObject::Scope s(nullptr, root, root);

	var returnValue = var::undefined();

#if USE_BACKEND
	const double pre = Time::getMillisecondCounterHiRes();

	RootObject::ScopedCallstackEntry sc(*root, callbackName, nullptr);
    LocalScopeCreator::ScopedSetter svs(root, this);

	jassert(root->threadAwareLocalScopeCreator.get()->getRootBlockStatement() == statements.get());
	statements->perform(s, &returnValue);

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

LambdaValueInformation::LambdaValueInformation(const ValueFunction& f, const Identifier& id_,
	const Identifier& namespaceId_, Type t, UriLocation location_, const String& comment_):
	DebugInformation(t),
	vf(f),
	namespaceId(namespaceId_),
	id(id_),
	location(location_)
{
	cachedValue = f();
	DebugableObjectBase::updateLocation(location, cachedValue);

	if (comment_.isNotEmpty())
		comment.append(comment_, GLOBAL_FONT(), Colours::white);;
}

UriLocation LambdaValueInformation::getLocation() const
{
	return location;
}

AttributedString LambdaValueInformation::getDescription() const
{
	return comment;
}

String LambdaValueInformation::getTextForDataType() const
{ return getVarType(getCachedValueFunction(false)); }

String LambdaValueInformation::getTextForName() const
{ 
	return namespaceId.isNull() ? id.toString() :
		       namespaceId.toString() + "." + id.toString(); 
}

int LambdaValueInformation::getNumChildElements() const
{
	auto value = getCachedValueFunction(false);

	if (auto obj = getDebugableObject(value))
	{
		auto customSize = obj->getNumChildElements();

		if (customSize != -1)
			return customSize;
	}

	if (value.isBuffer())
	{
		auto s = value.getBuffer()->size;

		if (isPositiveAndBelow(s, 513))
			return s;

		return 0;
	}
		
	if (auto dyn = value.getDynamicObject())
		return dyn->getProperties().size();

	if (auto ar = value.getArray())
		return jmin<int>(128, ar->size());

	return 0;
}

DebugInformation::Ptr LambdaValueInformation::getChildElement(int index)
{
	auto value = getCachedValueFunction(false);

	if (auto obj = getDebugableObject(value))
	{
		auto numCustom = obj->getNumChildElements();

		if (isPositiveAndBelow(index, numCustom))
			return obj->getChildElement(index);
	}

	WeakReference<LambdaValueInformation> safeThis(this);

	if (value.isBuffer())
	{
		auto actualValueFunction = [index, safeThis]()
		{
			if (safeThis == nullptr)
				return var();

			if (auto b = safeThis->getCachedValueFunction(false).getBuffer())
			{
				if (isPositiveAndBelow(index, b->size))
					return var(b->getSample(index));
			}

			return var(0.0f);
		};

		String cid = "%PARENT%[" + String(index) + "]";

		return new LambdaValueInformation(actualValueFunction, Identifier(cid), namespaceId, (Type)getType(), location);
	}
	else if (auto dyn = value.getDynamicObject())
	{
		String cid;

		const NamedValueSet& s = dyn->getProperties();

		if (isPositiveAndBelow(index, s.size()))
		{
			auto mid = s.getName(index);
			cid << id << "." << mid;

			auto cf = [safeThis, mid]()
			{
				if (safeThis == nullptr)
					return var();

				auto v = safeThis->getCachedValueFunction(false);
				return v.getProperty(mid, {});
			};

			return new LambdaValueInformation(cf, Identifier(cid), namespaceId, (Type)getType(), location);
		}
	}
	else if (auto ar = value.getArray())
	{
		String cid;
		cid << id << "[" << String(index) << "]";

		auto cf = [index, safeThis]()
		{
			if (safeThis == nullptr)
				return var();

			auto a = safeThis->getCachedValueFunction(false);

			if (auto ar = a.getArray())
				return (*ar)[index];

			return var();
		};

		return new LambdaValueInformation(cf, Identifier(cid), namespaceId, (Type)getType(), location);
	}

	return new DebugInformationBase();
}

var LambdaValueInformation::getCachedValueFunction(bool forceLookup) const
{
	if (forceLookup || cachedValue.isUndefined())
		cachedValue = vf();

	return cachedValue;
}

bool LambdaValueInformation::isAutocompleteable() const
{
	if (customAutoComplete)
		return autocompleteable;

	auto v = getCachedValueFunction(false);

	if (v.isObject())
		return true;
        
	return false;
}

void LambdaValueInformation::setAutocompleteable(bool shouldBe)
{
	customAutoComplete = true;
	autocompleteable = shouldBe;
}

var LambdaValueInformation::getVariantCopy() const
{ return var(getCachedValueFunction(false)); }

String LambdaValueInformation::getTextForValue() const
{
	auto v = getCachedValueFunction(true);
	return getVarValue(v); 
}

DebugableObjectBase* LambdaValueInformation::getObject()
{ return getDebugableObject(getCachedValueFunction(false)); }

DebugableObjectInformation::DebugableObjectInformation(DebugableObjectBase* object_, const Identifier& id_, Type t,
	const Identifier& namespaceId_, const String& comment_):
	DebugInformation(t),
	object(object_),
	id(id_),
	namespaceId(namespaceId_)
{
	if (comment_.isNotEmpty())
	{
		comment.append(comment_, GLOBAL_FONT(), Colours::white);
	}
}

String DebugableObjectInformation::getTextForDataType() const
{ return object != nullptr ? object->getDebugDataType() : ""; }

String DebugableObjectInformation::getTextForName() const
{ 
	if (object == nullptr)
		return "";

	return namespaceId.isNull() ? object->getDebugName() :
		       namespaceId.toString() + "." + object->getDebugName(); 
}

String DebugableObjectInformation::getTextForValue() const
{ return object != nullptr ? object->getDebugValue() : ""; }

AttributedString DebugableObjectInformation::getDescription() const
{ return comment; }

bool DebugableObjectInformation::isWatchable() const
{ return object != nullptr ? object->isWatchable() : false; }

int DebugableObjectInformation::getNumChildElements() const
{
	if (object != nullptr)
	{
		auto o = object->getNumChildElements();

		if (o != -1)
			return o;

			
	}

	return 0;
}

DebugInformationBase::Ptr DebugableObjectInformation::getChildElement(int index)
{
	if (object != nullptr)
		return object->getChildElement(index);
			
	return nullptr;
}

DebugableObjectBase* DebugableObjectInformation::getObject()
{ return object.get(); }

const DebugableObjectBase* DebugableObjectInformation::getObject() const
{ return object.get(); }

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

ConstScriptingObject::ConstScriptingObject(ProcessorWithScriptingContent* p, int numConstants):
	ScriptingObject(p),
	ApiClass(numConstants)
{

}

Identifier ConstScriptingObject::getInstanceName() const
{ return name.isValid() ? name : getObjectName(); }

bool ConstScriptingObject::objectDeleted() const
{ return false; }

bool ConstScriptingObject::objectExists() const
{ return false; }

bool ConstScriptingObject::addLocationForFunctionCall(const Identifier& id,
	const UriLocation& location)
{
	ignoreUnused(id, location);
	return false;
}

bool ConstScriptingObject::checkValidObject() const
{
	if (!objectExists())
	{
		reportScriptError(getObjectName().toString() + " " + getInstanceName() + " does not exist.");
		RETURN_IF_NO_THROW(false)
	}

	if (objectDeleted())
	{
		reportScriptError(getObjectName().toString() + " " + getInstanceName() + " was deleted");
		RETURN_IF_NO_THROW(false)
	}

	return true;
}

void ConstScriptingObject::setName(const Identifier& name_) noexcept
{ name = name_; }

void ScriptingObject::reportScriptError(const String &errorMessage) const
{
#if USE_BACKEND
	if (CompileExporter::isExportingFromCommandLine())
    {
        std::cout << errorMessage;
    }
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




void HiseJavascriptEngine::attachCallstack(UriError& e) const
{
	root->attachCallstack(e);
}





#if USE_BACKEND

// ==============================================================================
// HiseJavascriptEngine::shadowParse implementation

mcl::TokenCollection::Token* HiseJavascriptEngine::createAutocompleteTokenFromInlineFunction(const var& v)
{
    return RootObject::InlineFunction::Object::createAutocompleteTokenFromString(v);
}



#endif // USE_BACKEND

} // namespace hise

