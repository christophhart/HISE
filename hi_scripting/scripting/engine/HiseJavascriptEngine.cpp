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

namespace hise {
	using namespace juce;

#define JUCE_JS_OPERATORS(X) \
    X(semicolon,     ";")        X(dot,          ".")       X(comma,        ",") \
    X(openParen,     "(")        X(closeParen,   ")")       X(openBrace,    "{")    X(closeBrace, "}") \
    X(openBracket,   "[")        X(closeBracket, "]")       X(colon,        ":")    X(question,   "?") \
    X(typeEquals,    "===")      X(equals,       "==")      X(assign,       "=") \
    X(typeNotEquals, "!==")      X(notEquals,    "!=")      X(logicalNot,   "!") \
    X(plusEquals,    "+=")       X(plusplus,     "++")      X(plus,         "+") \
    X(minusEquals,   "-=")       X(minusminus,   "--")      X(minus,        "-") \
    X(timesEquals,   "*=")       X(times,        "*")       X(divideEquals, "/=")   X(divide,     "/") \
    X(moduloEquals,  "%=")       X(modulo,       "%")       X(xorEquals,    "^=")   X(bitwiseXor, "^") \
    X(andEquals,     "&=")       X(logicalAnd,   "&&")      X(bitwiseAnd,   "&") \
    X(orEquals,      "|=")       X(logicalOr,    "||")      X(bitwiseOr,    "|") \
    X(leftShiftEquals,    "<<=") X(lessThanOrEqual,  "<=")  X(leftShift,    "<<")   X(lessThan,   "<") \
X(rightShiftUnsigned, ">>>") X(rightShiftEquals, ">>=") X(rightShift,   ">>")   X(greaterThanOrEqual, ">=")  X(greaterThan,  ">") X(preprocessor_, "#") \

#define JUCE_JS_KEYWORDS(X) \
    X(var,      "var")      X(if_,     "if")     X(else_,  "else")   X(do_,       "do")       X(null_,     "null") \
    X(while_,   "while")    X(for_,    "for")    X(break_, "break")  X(continue_, "continue") X(undefined, "undefined") \
    X(function, "function") X(return_, "return") X(true_,  "true")   X(false_,    "false")    X(new_,      "new") \
    X(typeof_,  "typeof")	X(switch_, "switch") X(case_, "case")	 X(default_,  "default")  X(register_var, "reg") \
	X(in, 		"in")		X(inline_, "inline") X(const_, "const")	 X(global_,   "global")	  X(local_,	   "local") \
	X(include_,  "include") X(rLock_,   "readLock") X(wLock_,"writeLock") 	X(extern_, "extern") X(namespace_, "namespace") \
	X(loadJit_, "loadJITModule") X(isDefined_, "isDefined");

namespace TokenTypes
{
#define JUCE_DECLARE_JS_TOKEN(name, str)  static const char* const name = str;
		JUCE_JS_KEYWORDS(JUCE_DECLARE_JS_TOKEN)
		JUCE_JS_OPERATORS(JUCE_DECLARE_JS_TOKEN)
		JUCE_DECLARE_JS_TOKEN(eof, "$eof")
		JUCE_DECLARE_JS_TOKEN(literal, "$literal")
		JUCE_DECLARE_JS_TOKEN(identifier, "$identifier")
}

#if JUCE_MSVC
#pragma warning (push)
#pragma warning (disable: 4702)
#endif



HiseJavascriptEngine::~HiseJavascriptEngine()
{
	abortEverything();

	if (auto content = dynamic_cast<ScriptingApi::Content*>(root->getProperty("Content").getObject()))
	{
		for (int i = 0; i < content->getNumComponents(); i++)
		{
			if (auto c = content->getComponent(i))
				c->preRecompileCallback();
		}
	}

	root->hiseSpecialData.clear();
	root = nullptr;
	breakpointListeners.clear();
}


void HiseJavascriptEngine::setBreakpoints(Array<Breakpoint> &breakpoints)
{
	root->breakpoints.clear();
	root->breakpoints.addArray(breakpoints);
}


void HiseJavascriptEngine::prepareTimeout() const noexcept
{ 
#if USE_BACKEND
	root->timeout = Time::getCurrentTime() + maximumExecutionTime; 
#endif
}




void HiseJavascriptEngine::registerNativeObject(const Identifier& name, DynamicObject* object)
{
	root->setProperty(name, object);
}


void HiseJavascriptEngine::registerGlobalStorge(DynamicObject *globalObject)
{
	registerNativeObject("Globals", globalObject);
	root->hiseSpecialData.globals = globalObject;
}


struct HiseJavascriptEngine::RootObject::CodeLocation
{
	CodeLocation(const String& code, const String &externalFile_) noexcept        : program(code), location(program.getCharPointer()), externalFile(externalFile_) {}
	CodeLocation(const CodeLocation& other) noexcept : program(other.program), location(other.location), externalFile(other.externalFile) {}

	String getCallbackName() const
	{
		if (program.startsWith("function"))
		{
			return program.fromFirstOccurrenceOf("function ", false, false).upToFirstOccurrenceOf("(", false, false);
		}
		else
		{
			if (externalFile.isNotEmpty())
			{
				return "";
			}
			else
			{
				return "onInit";
			}
		}
		
	}

	void fillColumnAndLines(int& col, int& line) const
	{
		col = 1;
		line = 1;

		for (String::CharPointerType i(program.getCharPointer()); i < location && !i.isEmpty(); ++i)
		{
			++col;
			if (*i == '\n') { col = 1; ++line; }
		}
	}

	String getLocationString() const
	{
		int col, line;

		fillColumnAndLines(col, line);

		if (externalFile.isEmpty() || externalFile.contains("()"))
		{
			return "Line " + String(line) + ", column " + String(col);
		}

		else
		{
#if USE_BACKEND

			File f(externalFile);
			const String fileName = f.getFileName();
#else
			const String fileName = externalFile;
#endif

			return fileName + " - Line " + String(line) + ", column " + String(col);

		}
	}

	int getCharIndex() const
	{
		return (int)(location - program.getCharPointer());
	}

	String getEncodedLocationString(const String& processorId, const File& scriptRoot) const
	{
		int charIndex = getCharIndex();

		String l;

		l << processorId << "|";

		if (externalFile.contains("()"))
		{
			l << externalFile;
		}
		else if (!externalFile.isEmpty())
		{
			l << File(externalFile).getRelativePathFrom(scriptRoot);
		}
		
		l << "|" << String(charIndex);
		
		int col = 1, line = 1;

		for (String::CharPointerType i(program.getCharPointer()); i < location && !i.isEmpty(); ++i)
		{
			++col;
			if (*i == '\n') { col = 1; ++line; }
		}

		l << "|" << String(col) << "|" << String(line);

		return "{" + Base64::toBase64(l) + "}";
	}

	struct Helpers
	{
		static int getCharNumberFromBase64String(const String& base64EncodedString)
		{
			auto s = getDecodedString(base64EncodedString);

			auto sa = StringArray::fromTokens(s, "|", "");

			return sa[2].getIntValue();
		}

		static String getProcessorId(const String& base64EncodedString)
		{
			auto s = getDecodedString(base64EncodedString);

			auto sa = StringArray::fromTokens(s, "|", "");

			jassert(sa.size() > 0);

			return sa[0];
		}

		static String getFileName(const String& base64EncodedString)
		{
			auto s = getDecodedString(base64EncodedString);

			auto sa = StringArray::fromTokens(s, "|", "");

			jassert(sa.size() > 1);

			if (sa[1].isEmpty())
				return String();

			if (sa[1].contains("()"))
				return sa[1];

			return "{PROJECT_FOLDER}" + sa[1];
		}

		static String getDecodedString(const String& base64EncodedString)
		{
			MemoryOutputStream mos;
			Base64::convertFromBase64(mos, base64EncodedString.removeCharacters("{}"));
			return String::createStringFromData(mos.getData(), (int)mos.getDataSize());
		}
	};

	String getErrorMessage(const String &message) const
	{
		return message;
		return getLocationString() + ": " + message + "\t" + getEncodedLocationString("", File());
	}

	void throwError(const String& message) const;

	
	String program;
	mutable String externalFile;
	String::CharPointerType location;
	
};

#if JUCE_ENABLE_AUDIO_GUARD
struct HiseJavascriptEngine::RootObject::ScriptAudioThreadGuard: public AudioThreadGuard::Handler
{
public:

	enum IllegalScriptOps
	{
		ObjectCreation = IllegalAudioThreadOps::numIllegalOperationTypes,
		ArrayCreation,
		ArrayResizing,
		ObjectResizing,
		DynamicObjectAccess,
		FunctionCall,
		IllegalApiCall
	};

	ScriptAudioThreadGuard(const CodeLocation& location) :
		loc(location),
		setter(this)
	{
	};

	String getOperationName(int operationType) override
	{
		switch (operationType)
		{
		case ObjectCreation:		return "Object creation";
		case ArrayCreation:			return "non-empty Array creation";
		case ArrayResizing:			return "Array resizing. Call Array.reserve() to make sure there's enough space.";
		case ObjectResizing:		return "Resizing of object.";
		case DynamicObjectAccess:	return "Dynamic object access using []. Try object.member instead";
		case FunctionCall:			return "Non inline function call";
		case IllegalApiCall:		return "Illegal API call";
		default:
			break;
		}

		return Handler::getOperationName(operationType);
	}

	void warn(int operationType) override
	{
		loc.throwError("Illegal operation in audio thread: " + getOperationName(operationType));
	}

private:

	AudioThreadGuard::ScopedHandlerSetter setter;
	CodeLocation loc;
};
#else
struct HiseJavascriptEngine::RootObject::ScriptAudioThreadGuard
{
	ScriptAudioThreadGuard(const CodeLocation& /*location*/) {};
};
#endif

HiseJavascriptEngine::RootObject::Error HiseJavascriptEngine::RootObject::Error::fromLocation(const CodeLocation& location, const String& errorMessage)
{
	Error e;

	e.errorMessage = errorMessage;
	location.fillColumnAndLines(e.columnNumber, e.lineNumber);
	e.charIndex = location.getCharIndex();
	e.externalLocation = location.externalFile;

	return e;
}



void HiseJavascriptEngine::RootObject::CodeLocation::throwError(const String& message) const
{
#if USE_BACKEND

	static const Identifier ui("uninitialised");

	throw Error::fromLocation(*this, message);
#else
	ignoreUnused(message);
	DBG(getErrorMessage(message));
#endif
}



void DebugableObject::Helpers::gotoLocation(ModulatorSynthChain* mainSynthChain, const String& line)
{
	ignoreUnused(mainSynthChain, line);

#if USE_BACKEND
	const String reg = ".*(\\{[^\\s]+\\}).*";

	StringArray matches = RegexFunctions::getFirstMatch(reg, line);

	if (matches.size() == 2)
	{
		auto encodedState = matches[1];

		auto pId = HiseJavascriptEngine::RootObject::CodeLocation::Helpers::getProcessorId(encodedState);
		DebugableObject::Location loc;

		loc.charNumber = HiseJavascriptEngine::RootObject::CodeLocation::Helpers::getCharNumberFromBase64String(encodedState);

		auto fileReference = HiseJavascriptEngine::RootObject::CodeLocation::Helpers::getFileName(encodedState);

		if (fileReference.contains("()"))
		{
			loc.fileName = fileReference;
		}
		else if (fileReference.isNotEmpty())
		{
			loc.fileName = GET_PROJECT_HANDLER(mainSynthChain).getFilePath(fileReference, ProjectHandler::SubDirectories::Scripts);
		}

		auto p = dynamic_cast<JavascriptProcessor*>(ProcessorHelpers::getFirstProcessorWithName(mainSynthChain, pId));

		if (p != nullptr)
		{
			gotoLocation(nullptr, p, loc);
		}
		else
		{
			PresetHandler::showMessageWindow("Can't find location", "The location is not valid", PresetHandler::IconType::Error);
		}
	}
#endif
}

struct HiseJavascriptEngine::RootObject::CallStackEntry
{
	CallStackEntry() :
		functionName(Identifier()),
		location(CodeLocation("", "")),
		processor(nullptr)
	{}

	CallStackEntry(const Identifier &functionName_, const CodeLocation& location_, Processor* processor_) :
		functionName(functionName_),
		location(location_),
		processor(processor_)
	{}

	CallStackEntry(const CallStackEntry& otherEntry) :
		functionName(otherEntry.functionName),
		location(otherEntry.location),
		processor(otherEntry.processor)
	{

	}

	CallStackEntry& operator=(const CallStackEntry& otherEntry)
	{
		functionName = otherEntry.functionName;
		location = otherEntry.location;
		processor = otherEntry.processor;

		return *this;
	}

	CallStackEntry(const Identifier& functionName_) :
		functionName(functionName_),
		location(CodeLocation("", "")),
		processor(nullptr)
	{}

	bool operator== (const CallStackEntry& otherEntry) const
	{
		return functionName == otherEntry.functionName;
	}

	CodeLocation swapLocation(CodeLocation& otherLocation)
	{
		CodeLocation temp = CodeLocation(location);

		location = otherLocation;

		return temp;
	}

	

	WeakReference<Processor> processor;
	Identifier functionName;
	CodeLocation location;
};


struct HiseJavascriptEngine::RootObject::Scope
{
	Scope(const Scope* p, RootObject* r, DynamicObject* s) noexcept : parent(p), root(r), scope(s), currentLoopStatement(nullptr) {}

	mutable var currentIteratorObject;
	mutable int currentIteratorIndex;

	void setCurrentIteratorObject(var &newObject) const
	{
		currentIteratorIndex = 0;
		currentIteratorObject = newObject;
	}

	void incIteratorIndex() const
	{
		currentIteratorIndex++;
	}

	mutable void *currentLoopStatement;

	const Scope* parent;
	ReferenceCountedObjectPtr<RootObject> root;
	DynamicObject::Ptr scope;

	var findFunctionCall(const CodeLocation& location, const var& targetObject, const Identifier& functionName) const;

	var* findRootClassProperty(const Identifier& className, const Identifier& propName) const
	{
		if (DynamicObject* cls = root->getProperty(className).getDynamicObject())
			return getPropertyPointer(cls, propName);

		return nullptr;
	}

	var findSymbolInParentScopes(const Identifier& name) const
	{
		if (const var* v = getPropertyPointer(scope, name))
			return *v;

		return parent != nullptr ? parent->findSymbolInParentScopes(name)
			: var::undefined();
	}



	bool findAndInvokeMethod(const Identifier& function, const var::NativeFunctionArgs& args, var& result) const;

	bool invokeMidiCallback(const Identifier &callbackName, const var::NativeFunctionArgs &args, var &result, DynamicObject*functionScope) const;

	void checkTimeOut(const CodeLocation& location) const
	{
		ignoreUnused(location);

#if USE_BACKEND
		if (Time::getCurrentTime() > root->timeout)
			location.throwError("Execution timed-out");
#endif
	}
};



struct HiseJavascriptEngine::RootObject::Statement
{
	Statement(const CodeLocation& l) noexcept : location(l) {}
	virtual ~Statement() {}

	enum ResultCode  { ok = 0, returnWasHit, breakWasHit, continueWasHit, breakpointWasHit };
	virtual ResultCode perform(const Scope&, var*) const  { return ok; }

	CodeLocation location;

	Breakpoint::Reference breakpointReference;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Statement)
};



struct HiseJavascriptEngine::RootObject::Expression : public Statement
{
	Expression(const CodeLocation& l) noexcept : Statement(l) {}

	virtual var getResult(const Scope&) const            { return var::undefined(); }
	virtual void assign(const Scope&, const var&) const  { location.throwError("Cannot assign to this expression!"); }

	ResultCode perform(const Scope& s, var*) const override  { getResult(s); return ok; }
};


void HiseJavascriptEngine::RootObject::addToCallStack(const Identifier& id, const CodeLocation* location)
{
#if ENABLE_SCRIPTING_BREAKPOINTS
	if (enableCallstack)
	{
		callStack.add(CallStackEntry(id, location != nullptr ? *location : CodeLocation("", ""), dynamic_cast<Processor*>(hiseSpecialData.processor)));
	}
#else
	ignoreUnused(id, location);
#endif
}

void HiseJavascriptEngine::RootObject::removeFromCallStack(const Identifier& id)
{
#if ENABLE_SCRIPTING_BREAKPOINTS
	if (enableCallstack)
	{
		callStack.removeAllInstancesOf(CallStackEntry(id));
	}
#else
	ignoreUnused(id);
#endif
}

String HiseJavascriptEngine::RootObject::dumpCallStack(const Error& lastError, const Identifier& rootFunctionName)
{
	if (!enableCallstack)
	{
		auto p = dynamic_cast<Processor*>(hiseSpecialData.processor);

		String callbackName;

		if (auto callback = hiseSpecialData.getCallback(rootFunctionName))
		{
			if (lastError.externalLocation.isEmpty() &&  rootFunctionName != Identifier("onInit"))
			{
				lastError.externalLocation = callback->getDebugName();
				callbackName << callback->getDebugName() << " - ";
			}
		}

		return callbackName << lastError.getLocationString() + ": " + lastError.errorMessage << " " << lastError.getEncodedLocation(p);
	}

	auto p = dynamic_cast<Processor*>(hiseSpecialData.processor);

	const String nl = "\n";
	String s;
	s << lastError.errorMessage << " " << lastError.getEncodedLocation(p);
	s << nl;

	Error thisError = lastError;

	bool callbackFound = false;

	for (int i = callStack.size() - 1; i >= 0; i--)
	{	
		auto entry = callStack.getReference(i);

		if (auto callback = hiseSpecialData.getCallback(entry.functionName))
		{
			thisError.externalLocation = callback->getDebugName();
			callbackFound = true; // skip the last line because it's the callback
		}

		s << ":\t\t\t" << entry.functionName << "() - " << thisError.toString(p) << nl;

		thisError = Error::fromLocation(entry.location, "");

		
	}

	if (!callbackFound)
	{
		s << ":\t\t\t" << rootFunctionName << "() - " << thisError.toString(p) << nl;
	}

	//CallStackEntry lastEntry(rootFunctionName, lastLocation, dynamic_cast<Processor*>(hiseSpecialData.processor));

	//s << ":\t\t\t" << lastEntry.toString() << nl;

	callStack.clearQuick();

	return s;
}

var HiseJavascriptEngine::RootObject::typeof_internal(Args a)
{
	var v(get(a, 0));

	if (v.isVoid())                      return "void";
	if (v.isString())                    return "string";
	if (isNumeric(v))                   return "number";
	if (isFunction(v) || v.isMethod())  return "function";
	if (v.isObject())                    return "object";

	return "undefined";
}

var HiseJavascriptEngine::RootObject::exec(Args a)
{
	if (RootObject* root = dynamic_cast<RootObject*> (a.thisObject.getObject()))
		root->execute(getString(a, 0), false);

	return var::undefined();
}

var HiseJavascriptEngine::RootObject::eval(Args a)
{
	if (RootObject* root = dynamic_cast<RootObject*> (a.thisObject.getObject()))
		return root->evaluate(getString(a, 0));

	return var::undefined();
}

Result HiseJavascriptEngine::execute(const String& javascriptCode, bool allowConstDeclarations/*=true*/)
{
#if JUCE_DEBUG
	auto mc = dynamic_cast<Processor*>(root->hiseSpecialData.processor)->getMainController();
	LockHelpers::noMessageThreadBeyondInitialisation(mc);
#endif

	static const Identifier onInit("onInit");

	try
	{
		prepareTimeout();
		root->execute(javascriptCode, allowConstDeclarations);

		
	}
	catch (String &error)
	{
		jassertfalse;
		return Result::fail(error);
	}
	catch (RootObject::Error &e)
	{
#if USE_FRONTEND
		DBG(e.errorMessage);
		return Result::fail(e.errorMessage);
#endif

		return Result::fail(root->dumpCallStack(e, onInit));
	}
	catch (Breakpoint& bp)
	{
		if (bp.localScope != nullptr)
			bp.copyLocalScopeToRoot(*root);

		sendBreakpointMessage(bp.index);
		return Result::fail(root->dumpCallStack(RootObject::Error::fromBreakpoint(bp), onInit));
	}

	return Result::ok();
}

var HiseJavascriptEngine::evaluate(const String& code, Result* result)
{
#if JUCE_DEBUG
	auto mc = dynamic_cast<Processor*>(root->hiseSpecialData.processor)->getMainController();
	LockHelpers::noMessageThreadBeyondInitialisation(mc);
#endif

	static const Identifier ext("eval");

	try
	{
		prepareTimeout();
		if (result != nullptr) *result = Result::ok();
		return root->evaluate(code);
	}
	catch (String& error)
	{
		jassertfalse;

		if (result != nullptr) *result = Result::fail(error);
	}
	catch (RootObject::Error& e)
	{
		if (result != nullptr) *result = Result::fail(root->dumpCallStack(e, ext));
	}
	catch (Breakpoint& bp)
	{
		if (result != nullptr) *result = Result::fail(root->dumpCallStack(RootObject::Error::fromBreakpoint(bp), ext));
	}

	return var::undefined();
}


const NamedValueSet& HiseJavascriptEngine::getRootObjectProperties() const noexcept
{
	return root->getProperties();
}

DynamicObject * HiseJavascriptEngine::getRootObject()
{
	return dynamic_cast<DynamicObject*>(root.get());
}

void HiseJavascriptEngine::setCallStackEnabled(bool shouldBeEnabled)
{
	root->setCallStackEnabled(shouldBeEnabled);
}

void HiseJavascriptEngine::registerApiClass(ApiClass *apiClass)
{
	root->hiseSpecialData.apiClasses.add(apiClass);
	root->hiseSpecialData.apiIds.add(apiClass->getObjectName());
}

#if 0
const ApiClassBase* HiseJavascriptEngine::getApiClass(const Identifier &className) const
{
	const int index = root->hiseSpecialData.apiIds.indexOf(className);

	if (index != -1)
	{
		return root->hiseSpecialData.apiClasses[index];
	}

	return nullptr;
}
#endif

ApiClass::Constant ApiClass::Constant::null;

#if JUCE_MSVC
#pragma warning (pop)
#endif



var HiseJavascriptEngine::callFunction(const Identifier& function, const var::NativeFunctionArgs& args, Result* result)
{
#if JUCE_DEBUG
	auto mc = dynamic_cast<Processor*>(root->hiseSpecialData.processor)->getMainController();
	LockHelpers::noMessageThreadBeyondInitialisation(mc);
#endif

	var returnVal(var::undefined());

	try
	{
		prepareTimeout();
		if (result != nullptr) *result = Result::ok();
		RootObject::Scope(nullptr, root, root).findAndInvokeMethod(function, args, returnVal);
	}
	catch (String &error)
	{
		jassertfalse;
		if (result != nullptr) *result = Result::fail(error);
	}
	catch (RootObject::Error &e)
	{
		if (result != nullptr) *result = Result::fail(root->dumpCallStack(e, function));
	}
	catch (Breakpoint& bp)
	{
		if (result != nullptr) *result = Result::fail(root->dumpCallStack(RootObject::Error::fromBreakpoint(bp), function));
	}

	return returnVal;
}



int HiseJavascriptEngine::registerCallbackName(const Identifier &callbackName, int numArgs, double bufferTime)
{
	// Can't register a callback twice...
	jassert(root->hiseSpecialData.getCallback(callbackName) == nullptr);

	root->hiseSpecialData.callbackNEW.add(new RootObject::Callback(callbackName, numArgs, bufferTime));

	return 1;
}


void HiseJavascriptEngine::setCallbackParameter(int callbackIndex, int parameterIndex, const var& newValue)
{
	root->hiseSpecialData.callbackNEW[callbackIndex]->setParameterValue(parameterIndex, newValue);
}

DebugInformationBase* HiseJavascriptEngine::getDebugInformation(int index)
{
	return root->hiseSpecialData.getDebugInformation(index);
}


#if 0
var HiseJavascriptEngine::getDebugObject(const Identifier &id) const
{
	String idAsString = id.toString();

	if (idAsString.contains("(") || idAsString.contains("["))
	{
		Result r = Result::ok();

		var ev = const_cast<HiseJavascriptEngine*>(this)->evaluate(idAsString, &r);

		if (!r.failed() && !ev.isVoid())
			return ev;
	}

	if (idAsString.containsChar('.'))
	{
		StringArray sa = StringArray::fromTokens(idAsString, ".", "");

		var v = getScriptVariableFromRootNamespace(Identifier(sa[0]));

		if (auto dyn = dynamic_cast<const DynamicObject*>(v.getObject()))
		{
			return dyn->getProperty(Identifier(sa[1]));
		}
		else if (auto api = dynamic_cast<const ApiClass*>(v.getObject()))
		{
			const int index = api->getConstantIndex(Identifier(sa[1]));
			return api->getConstantValue(index);
		}
		else if (dynamic_cast<const RootObject::JavascriptNamespace*>(v.getObject()) != nullptr)
		{
			Result r = Result::ok();

			var ev = const_cast<HiseJavascriptEngine*>(this)->evaluate(idAsString, &r);

			if (!r.failed() && !ev.isVoid())
				return ev;
		}
		
		return var();
	}
	else
	{
		return getScriptVariableFromRootNamespace(id);
	}
}
#endif


var HiseJavascriptEngine::getScriptVariableFromRootNamespace(const Identifier & id) const
{
	var v = root->getProperty(id);

	if (!v.isVoid())
		return v;

	v = root->hiseSpecialData.constObjects[id];
	if (!v.isVoid())
		return v;

	v = root->hiseSpecialData.getNamespace(id);
	if (v.getObject() != nullptr)
		return v;

	int registerIndex = root->hiseSpecialData.varRegister.getRegisterIndex(id);
	if (registerIndex != -1)
	{
		v = root->hiseSpecialData.varRegister.getFromRegister(registerIndex);

		if (!v.isVoid())
			return v;
	}

	DynamicObject* globals = root->hiseSpecialData.globals;
	if (globals != nullptr)
	{
		v = globals->getProperty(id);

		if (!v.isVoid())
			return v;
	}

	return var();
}

int HiseJavascriptEngine::getNumIncludedFiles() const
{
	return root->hiseSpecialData.includedFiles.size();
}

File HiseJavascriptEngine::getIncludedFile(int fileIndex) const
{
	return root->hiseSpecialData.includedFiles[fileIndex]->f;
}


Result HiseJavascriptEngine::getIncludedFileResult(int fileIndex) const
{
	return root->hiseSpecialData.includedFiles[fileIndex]->r;
}

int HiseJavascriptEngine::getNumDebugObjects() const
{
	return root->hiseSpecialData.getNumDebugObjects();
}


void HiseJavascriptEngine::clearDebugInformation()
{
	root->hiseSpecialData.clearDebugInformation();
}

void HiseJavascriptEngine::rebuildDebugInformation()
{
	root->hiseSpecialData.clearDebugInformation();

	root->hiseSpecialData.createDebugInformation(root);
}

var HiseJavascriptEngine::executeWithoutAllocation(const Identifier &function, const var::NativeFunctionArgs& args, Result* result /*= nullptr*/, DynamicObject *scopeToUse)
{
#if JUCE_DEBUG
	auto mc = dynamic_cast<Processor*>(root->hiseSpecialData.processor)->getMainController();
	LockHelpers::noMessageThreadBeyondInitialisation(mc);
#endif

	var returnVal(var::undefined());

	try
	{
		prepareTimeout();
		if (result != nullptr) *result = Result::ok();
		RootObject::Scope(nullptr, root, root).invokeMidiCallback(function, args, returnVal, (scopeToUse != nullptr ? scopeToUse : unneededScope.get()));
	}
	catch (String& error)
	{
		if (result != nullptr) *result = Result::fail(error);
	}

	return returnVal;
}

void HiseJavascriptEngine::checkValidParameter(int index, const var& valueToTest, const RootObject::CodeLocation& location)
{
#if ENABLE_SCRIPTING_SAFE_CHECKS

	if (valueToTest.isUndefined() || valueToTest.isVoid())
	{
		location.throwError("API call with undefined parameter " + String(index));
	}
#else
    ignoreUnused(location, index, valueToTest);
#endif
}

void HiseJavascriptEngine::extendTimeout(int milliSeconds)
{
	auto newTimeout = root->timeout.toMilliseconds() + milliSeconds;

	root->timeout = Time(newTimeout);
}

void HiseJavascriptEngine::abortEverything()
{
	if(root != nullptr)
		root->timeout = Time(0);
}

hise::DebugableObjectBase* HiseJavascriptEngine::getDebugObject(const String& token)
{
	if (token.isEmpty())
		return nullptr;

	if (auto obj = ApiProviderBase::getDebugObject(token))
		return obj;

	auto r = root->evaluate(token);

	if (r.isArray())
		return ApiProviderBase::getDebugObject("Array");
	if (r.isString())
		return ApiProviderBase::getDebugObject("String");

		

	if(auto s = dynamic_cast<DebugableObjectBase*>(r.getObject()))
		return s;

	return nullptr;
}

juce::String HiseJavascriptEngine::getHoverString(const String& token)
{
	try
	{
		auto value = root->evaluate(token).toString();

		if (token != value)
			return token + ": " + value;

		return "";
	}
	catch (String& error)
	{
		return "";
	}
	catch (RootObject::Error& e)
	{
		return "";
	}
}

HiseJavascriptEngine::RootObject::Callback::Callback(const Identifier &id, int numArgs_, double bufferTime_) :
callbackName(id),
bufferTime(bufferTime_),
numArgs(numArgs_)
{
	for (int i = 0; i < 4; i++)
	{
		parameters[i] = Identifier::null;
		parameterValues[i] = var::undefined();
	}
}



#if INCLUDE_NATIVE_JIT
NativeJITScope* HiseJavascriptEngine::RootObject::HiseSpecialData::getNativeJITScope(const Identifier& id)
{
	for (int i = 0; i < jitScopes.size(); i++)
	{
		if (jitScopes[i]->getName() == id)
		{
			return jitScopes[i];
		}
	}

	return nullptr;
}

NativeJITCompiler* HiseJavascriptEngine::RootObject::HiseSpecialData::getNativeCompiler(const Identifier& id)
{
	for (int i = 0; i < jitModules.size(); i++)
	{
		if (jitModules[i]->getModuleName() == id)
		{
			return jitModules[i];
		}
	}

	return nullptr;
}
#endif

} // namespace hise