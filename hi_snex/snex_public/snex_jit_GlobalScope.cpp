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

int DebugHandler::Tokeniser::readNextToken(CodeDocument::Iterator& source)
{
	auto c = source.nextChar();

	if (c == '|')
	{
		while (!source.isEOF() && source.peekNextChar() != '{')
			source.skip();

		return BaseCompiler::MessageType::ValueName;
	}
	if (c == '{')
	{
		source.skipToEndOfLine();
		return BaseCompiler::MessageType::ValueDump;
	}
	if (c == 'P')
	{
		source.skipToEndOfLine(); return BaseCompiler::MessageType::PassMessage;
	}
	if (c == 'W')
	{
		source.skipToEndOfLine(); return BaseCompiler::MessageType::Warning;
	}
	if (c == 'O')
	{
		source.skipToEndOfLine(); return BaseCompiler::MessageType::AsmJitMessage;
	}
	if (c == 'E')
	{
		source.skipToEndOfLine(); return BaseCompiler::MessageType::Error;
	}
	if (c == '-')
	{
		c = source.nextChar();

		source.skipToEndOfLine();

		if (c == '-')
			return BaseCompiler::MessageType::VerboseProcessMessage;
		else
			return BaseCompiler::MessageType::ProcessMessage;
	}

	return BaseCompiler::MessageType::ProcessMessage;
}

juce::CodeEditorComponent::ColourScheme DebugHandler::Tokeniser::getDefaultColourScheme()
{
	CodeEditorComponent::ColourScheme scheme;

	scheme.set("Error", Colour(0xFFCC6666));
	scheme.set("Warning", Colour(0xFFFFFF66));
	scheme.set("Pass", Colour(0xFF66AA66));
	scheme.set("Process", Colours::white);
	scheme.set("VerboseProcess", Colours::lightgrey);
	scheme.set("AsmJit", Colours::lightblue);
	scheme.set("ValueDump", Colours::white);
	scheme.set("ValueName", Colours::lightblue);

	return scheme;
}

GlobalScope::GlobalScope() :
	FunctionClass({}),
	BaseScope({}, nullptr),
	runtimeError(Result::ok()),
	polyHandler(false)
{
	blockType = new DynType(TypeInfo(Types::ID::Float));
	blockType->setAlias(NamespacedIdentifier("block"));

	objectClassesWithJitCallableFunctions.add(new ConsoleFunctions(this));
	
	auto l = getDefaultDefinitions();
	
	ExternalPreprocessorDefinition npv;
	npv.t = ExternalPreprocessorDefinition::Type::Definition;
	npv.name = "NUM_POLYPHONIC_VOICES";
	npv.value = String(1);
	l.add(npv);

	setPreprocessorDefinitions(l);

	addNoInliner("prepare");
	addNoInliner("setExternalData");

	jassert(scopeType == BaseScope::Global);

}


void GlobalScope::registerObjectFunction(FunctionClass* objectClass)
{
	objectClassesWithJitCallableFunctions.add(objectClass);

	if (auto jco = dynamic_cast<JitCallableObject*>(objectClass))
		jco->registerToMemoryPool(this);
	else
		jassertfalse;
}

void GlobalScope::deregisterObject(const NamespacedIdentifier& id)
{
	bool somethingDone = false;

	for (int i = 0; i < objectClassesWithJitCallableFunctions.size(); i++)
	{
		if (objectClassesWithJitCallableFunctions[i]->getClassName() == id)
		{
			functions.remove(i--);
			somethingDone = true;
		}
	}

	if (somethingDone)
	{
		for (auto l : deleteListeners)
		{
			if (l.get() != nullptr)
				l->objectWasDeleted(id);
		}
	}
}

void GlobalScope::registerFunctionsToNamespaceHandler(NamespaceHandler& handler)
{
	NamespaceHandler::ScopedNamespaceSetter sns(handler, NamespacedIdentifier());

	blockType = handler.registerComplexTypeOrReturnExisting(blockType);

	jassert(dynamic_cast<DynType*>(blockType.get()) != nullptr);

	addFunctionClass(new MathFunctions(false, blockType));

    NamespaceHandler::SymbolDebugInfo di;
    
	for (auto of : objectClassesWithJitCallableFunctions)
	{
		handler.addSymbol(of->getClassName(), TypeInfo(Types::ID::Pointer, true), NamespaceHandler::StaticFunctionClass, di);
	}

	for (auto rc : registeredClasses)
	{
		handler.addSymbol(rc->getClassName(), TypeInfo(Types::ID::Pointer, true), NamespaceHandler::StaticFunctionClass, di);
	}
}

bool GlobalScope::hasFunction(const NamespacedIdentifier& symbol) const
{
	for (auto of : objectClassesWithJitCallableFunctions)
	{
		if (of->hasFunction(symbol))
			return true;
	}

	for (auto rc : registeredClasses)
	{
		if (rc->hasFunction(symbol))
			return true;
	}

	return false;
}

void GlobalScope::addMatchingFunctions(Array<FunctionData>& matches, const NamespacedIdentifier& symbol) const
{
	FunctionClass::addMatchingFunctions(matches, symbol);

	for (auto of : objectClassesWithJitCallableFunctions)
		of->addMatchingFunctions(matches, symbol);
}

void GlobalScope::addObjectDeleteListener(ObjectDeleteListener* l)
{
	deleteListeners.addIfNotAlreadyThere(l);
}

void GlobalScope::removeObjectDeleteListener(ObjectDeleteListener* l)
{
	deleteListeners.removeAllInstancesOf(l);
}

snex::jit::FunctionClass::Map GlobalScope::getMap()
{
	if (currentMap.isEmpty())
	{
		currentMap = FunctionClass::getMap();

		for (auto f : objectClassesWithJitCallableFunctions)
			currentMap.addArray(f->getMap());
	}

	return currentMap;
}

void GlobalScope::sendBlinkMessage(int lineNumber)
{
	for (auto dh : debugHandlers)
	{
		if (dh != nullptr)
			dh->blink(lineNumber);
	}
}

void GlobalScope::logMessage(const String& message)
{
	

	if (MessageManager::getInstance()->isThisTheMessageThread())
	{
		for (auto dh : debugHandlers)
		{
			if (dh != nullptr)
				dh->logMessage(BaseCompiler::AsmJitMessage, message);
		}
	}
	else
	{
		{
			SimpleReadWriteLock::ScopedReadLock sl(messageLock);
			pendingMessages.add(message);
		}
		
		triggerAsyncUpdate();
	}
}

void GlobalScope::addOptimization(const juce::String& passId)
{
	optimizationPasses.addIfNotAlreadyThere(passId);

	if (passId == OptimizationIds::Inlining)
	{
		removeFunctionClass(NamespacedIdentifier("Math"));
		addFunctionClass(new MathFunctions(true, blockType));
	}
}

void GlobalScope::clearOptimizations()
{
	optimizationPasses.clear();

	removeFunctionClass(NamespacedIdentifier("Math"));

	addFunctionClass(new MathFunctions(false, blockType));
}

bool GlobalScope::checkRuntimeErrorAfterExecution()
{
	if (!currentRuntimeError.wasOk() && isRuntimeErrorCheckEnabled())
	{
		auto m = currentRuntimeError.toString();

		runtimeError = Result::fail(m);

		for (auto& dh : debugHandlers)
		{
			if(dh.get() != nullptr)
				dh->logMessage(BaseCompiler::Error, m);
		}
			
		currentRuntimeError = {};

		return true;
	}

	runtimeError = Result::ok();

	return false;
}

void GlobalScope::setPreprocessorDefinitions(var d, bool clearExisting)
{
	if(clearExisting)
		preprocessorDefinitions.clear();

	if (auto obj = d.getDynamicObject())
	{
		for (auto& o : obj->getProperties())
		{
			ExternalPreprocessorDefinition d;
			d.name = o.name.toString();
			d.value = o.value.toString();
			d.t = ExternalPreprocessorDefinition::Type::Definition;

			preprocessorDefinitions.addIfNotAlreadyThere(d);
		}
	}
}

void GlobalScope::setPreprocessorDefinitions(const ExternalPreprocessorDefinition::List& d, bool clearExisting)
{
	if(clearExisting)
		preprocessorDefinitions.clear();

	for (auto& a : d)
		preprocessorDefinitions.addIfNotAlreadyThere(a);

}

ExternalPreprocessorDefinition::List GlobalScope::getDefaultDefinitions()
{
	ExternalPreprocessorDefinition::List defaultMacros;

	{
		ExternalPreprocessorDefinition declareNode;
		declareNode.name = "DECLARE_NODE(className)";
		declareNode.value = "__internal_property(\"IsNode\", 1); __internal_property(\"NodeId\", className);";
		declareNode.t = ExternalPreprocessorDefinition::Type::Macro;
		declareNode.description = "Use this macro inside a class to make it a valid node. The `className` must be the exact same ID as the class and you need to define a `template <int P> void setParameter(double v)` method.  ";

		defaultMacros.add(declareNode);
	}

	{
		ExternalPreprocessorDefinition son;
		son.name = "SNEX_NODE(className)";
		son.value = "__internal_property(\"NodeId\", className);";
		son.t = ExternalPreprocessorDefinition::Type::Macro;
		defaultMacros.add(son);
	}

	{
		ExternalPreprocessorDefinition dpe;
		dpe.name = "DECLARE_PARAMETER_EXPRESSION(name, expression)";
		dpe.value = "struct name { static double op(double input) { return expression; }};";
		dpe.t = ExternalPreprocessorDefinition::Type::Macro;

		defaultMacros.add(dpe);
	}

	{
		// #define SNEX_METADATA_ID(dsp1_t);
		ExternalPreprocessorDefinition declareId;
		declareId.name = "SNEX_METADATA_ID(className)";
		declareId.value = "__internal_property(\"NodeId\", className);";
		declareId.t = ExternalPreprocessorDefinition::Type::Macro;

		defaultMacros.add(declareId);
	}

	{
		// #define SNEX_METADATA_NUM_CHANNELS(numChannels);
		ExternalPreprocessorDefinition nc;
		nc.name = "SNEX_METADATA_NUM_CHANNELS(numChannels)";
		nc.value = "static const int NumChannels = numChannels;";
		nc.t = ExternalPreprocessorDefinition::Type::Macro;

		defaultMacros.add(nc);
	}
	
	
	{
		// #define SNEX_METADATA_ENCODED_PARAMETERS(NumElements) static const span<unsigned int, NumElements> encodedParameters =
		ExternalPreprocessorDefinition pd;
		pd.name = "SNEX_METADATA_ENCODED_PARAMETERS(NumElements)";
		pd.value = "const span<int, NumElements> encodedParameters ="; 
		pd.t = ExternalPreprocessorDefinition::Type::Macro;
		defaultMacros.add(pd);
	}
	
	{
		// #define MIN_MAX(minValue, maxValue) static const double min = minValue; static const double max = maxValue;
		ExternalPreprocessorDefinition mm;
		mm.t = ExternalPreprocessorDefinition::Type::Macro;
		mm.name = "MIN_MAX(minValue, maxValue";
		mm.value = "static const double min = minValue; static const double max = maxValue;";
		mm.description = "used by DECLARE_PARAMETER_RANGE";

		defaultMacros.add(mm);
	}
	{
		// #define RANGE_FUNCTION(id) static double id(double input) { return ranges::id(min, max, input); }

		ExternalPreprocessorDefinition rf;
		rf.t = ExternalPreprocessorDefinition::Type::Macro;
		rf.name = "RANGE_FUNCTION(id)";
		rf.value = "static double id(double input) { return ranges::id(min, max, input); }";

		defaultMacros.add(rf);
	}

	{
		// #define RANGE_FUNCTION_3(id) static double id(double input) { return ranges::id(min, max, skew, input); }

		ExternalPreprocessorDefinition rfs;
		rfs.t = ExternalPreprocessorDefinition::Type::Macro;
		rfs.name = "RANGE_FUNCTION_3(id, functionId, thirdParameter)";
		rfs.value = "static double id(double input) { return ranges::functionId(min, max, thirdParameter, input); }";

		defaultMacros.add(rfs);
	}

	{
		// #define DECLARE_PARAMETER_RANGE(name, minValue, maxValue) 
		// struct name { MIN_MAX(minValue, maxValue) RANGE_FUNCTION(to0To1); RANGE_FUNCTION(from0To1) };
		ExternalPreprocessorDefinition dpr;
		dpr.t = ExternalPreprocessorDefinition::Type::Macro;
		dpr.name = "DECLARE_PARAMETER_RANGE(name, minValue, maxValue)";
		dpr.value = "struct name { MIN_MAX(minValue, maxValue) RANGE_FUNCTION(to0To1); RANGE_FUNCTION(from0To1) };";
		
		defaultMacros.add(dpr);
	}

	{
		/* #define DECLARE_PARAMETER_RANGE_SKEW(name, min, max, skew) struct name {\
		static constexpr double to0To1(double input) {  return RANGE_BASE::to0To1Skew(min, max, skew, input); }\
		static constexpr double from0To1(double input){ return RANGE_BASE::from0To1Skew(min, max, skew, input);} */
	
		ExternalPreprocessorDefinition dprs;
		dprs.t = ExternalPreprocessorDefinition::Type::Macro;
		dprs.name = "DECLARE_PARAMETER_RANGE_SKEW(name, minValue, maxValue, skewValue)";
		dprs.value = "struct name { MIN_MAX(minValue, maxValue) static const double skew = skewValue; RANGE_FUNCTION_3(to0To1, to0To1Skew, skew); RANGE_FUNCTION_3(from0To1, from0To1Skew, skew) };";

		defaultMacros.add(dprs);
	}

	{
		// #define DECLARE_PARAMETER_RANGE_STEP(name, min, max, step) struct name

		ExternalPreprocessorDefinition dprs;
		dprs.t = ExternalPreprocessorDefinition::Type::Macro;
		dprs.name = "DECLARE_PARAMETER_RANGE_STEP(name, minValue, maxValue, stepValue)";
		dprs.value = "struct name { MIN_MAX(minValue, maxValue) static const double step = stepValue; RANGE_FUNCTION_3(to0To1, to0To1Step, step); RANGE_FUNCTION_3(from0To1, from0To1Step, step) };";

		defaultMacros.add(dprs);
	}

	return defaultMacros;
}

void GlobalScope::clearDebugMessages()
{
	if (MessageManager::getInstance()->isThisTheMessageThread())
	{
		for (auto dh : debugHandlers)
		{
			if (dh != nullptr)
				dh->clearLogger();
		}
	}
	else
	{
		{
			hise::SimpleReadWriteLock::ScopedWriteLock sl(messageLock);
			pendingMessages.clearQuick();
		}

		clearNext = true;
		triggerAsyncUpdate();
	}
}

void GlobalScope::handleAsyncUpdate()
{
	if (clearNext)
	{
		for (auto dh : debugHandlers)
		{
			if (dh != nullptr)
				dh->clearLogger();
		}

		clearNext = false;
		return;
	}

	Array<String> thisTime;

	{
		hise::SimpleReadWriteLock::ScopedWriteLock sl(messageLock);
		std::swap(pendingMessages, thisTime);
	}

	for (const auto& t : thisTime)
	{
		for (auto dh : debugHandlers)
		{
			if (dh != nullptr)
				dh->logMessage(BaseCompiler::AsmJitMessage, t);
		}
	}
}

}
}
