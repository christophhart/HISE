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
	X(include_,  "include") X(extern_, "extern") X(namespace_, "namespace") \
	X(isDefined_, "isDefined");

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

HiseJavascriptEngine::TimeoutExtender::TimeoutExtender(HiseJavascriptEngine* e):
	engine(e)
{
	start = Time::getMillisecondCounter();
}

HiseJavascriptEngine::TimeoutExtender::~TimeoutExtender()
{
#if USE_BACKEND
	if (engine != nullptr)
	{
		auto delta = Time::getMillisecondCounter() - start;
		engine->extendTimeout(delta);
	}
#endif
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

	String getCallbackName(bool returnExternalFileName=false) const
	{
		if (program.startsWith("function"))
		{
			return program.fromFirstOccurrenceOf("function ", false, false).upToFirstOccurrenceOf("(", false, false);
		}
		else
		{
			if (externalFile.isNotEmpty())
			{
				if (returnExternalFileName)
					return externalFile.replaceCharacter('\\', '/').fromLastOccurrenceOf("/", false, false);
				else
					return {};
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



bool DebugableObject::Helpers::gotoLocation(ModulatorSynthChain* mainSynthChain, const String& line)
{
	ignoreUnused(mainSynthChain, line);

#if USE_BACKEND
	const String reg = ".*(\\{[^\\s]+\\}).*";

	StringArray matches = RegexFunctions::getFirstMatch(reg, line);

	if (matches.size() == 2)
	{
		auto encodedState = matches[1];

		if (encodedState.startsWith("{GLSL"))
		{
			auto s = StringArray::fromTokens(encodedState.removeCharacters("{}"), ":", "");
			s.removeEmptyStrings();

			auto lineNumber = line.fromFirstOccurrenceOf("(", false, false).getIntValue() - 1;

			if (auto p = dynamic_cast<JavascriptProcessor*>(ProcessorHelpers::getFirstProcessorWithName(mainSynthChain, s[1])))
			{
				for (int i = 0; i < p->getNumWatchedFiles(); i++)
				{
					if (p->getWatchedFile(i).getFileNameWithoutExtension() == s[2])
					{
						CodeDocument::Position pos(p->getWatchedFileDocument(i), lineNumber, 0);
						DebugableObject::Location loc;
						loc.charNumber = pos.getPosition();
						loc.fileName = p->getWatchedFile(i).getFullPathName();
						return gotoLocation(nullptr, p, loc);
					}
				}
			}
		}
		else
		{
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
				return gotoLocation(nullptr, p, loc);
			}
			else
			{
				PresetHandler::showMessageWindow("Can't find location", "The location is not valid", PresetHandler::IconType::Error);
			}
		}
	}
#endif
    
    return false;
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
		if (const var* v = getPropertyPointer(scope.get(), name))
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
	using Ptr = ScopedPointer<Statement>;

	Statement(const CodeLocation& l) noexcept : location(l) {}
	virtual ~Statement() {}

	enum ResultCode  { ok = 0, returnWasHit, breakWasHit, continueWasHit, breakpointWasHit };
	virtual ResultCode perform(const Scope&, var*) const  { return ok; }

	virtual bool isConstant() const { return false; }

	CodeLocation location;
	
	/** Return nullptr if there is no child, otherwise a reference to the child statement. 
		This makes the syntax tree iteratable for optimisations.
	*/
	virtual Statement* getChildStatement(int index) { return nullptr; };
	
	/** Helper function to quickly replace expression children that are optimized away. 
	
		Use inside replaceChildStatement with each childStatement
	*/
	template <typename T> static bool swapIf(Ptr& newChild, Statement* childToReplace, ScopedPointer<T>& currentChild)
	{
		if (childToReplace == currentChild.get())
		{
			auto nc = newChild.release();
			auto oc = currentChild.release();

			newChild = dynamic_cast<Statement*>(oc);
			currentChild = dynamic_cast<T*>(nc);

			return true;
		}

		return false;
	}

	template <typename T> static bool swapIfArrayElement(Ptr& newChild, Statement* childToReplace, OwnedArray<T>& arrayToSwap)
	{
		auto idx = arrayToSwap.indexOf(dynamic_cast<T*>(childToReplace));

		if (idx != -1)
		{
			arrayToSwap.set(idx, dynamic_cast<T*>(newChild.release()), true);
			return true;
		}

		return false;
	}

	virtual bool replaceChildStatement(Ptr& newChild, Statement* oldChild) { return false; };

	Breakpoint::Reference breakpointReference;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Statement)
};

struct HiseJavascriptEngine::RootObject::Expression : public Statement
{
	Expression(const CodeLocation& l) noexcept : Statement(l) {}

	virtual var getResult(const Scope&) const            { return var::undefined(); }
	virtual void assign(const Scope&, const var&) const  { location.throwError("Cannot assign to this expression!"); }

	virtual Identifier getVariableName() const { return {}; }

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

Result HiseJavascriptEngine::execute(const String& javascriptCode, bool allowConstDeclarations/*=true*/, const Identifier& callbackId)
{
#if JUCE_DEBUG
	auto mc = dynamic_cast<Processor*>(root->hiseSpecialData.processor)->getMainController();
	LockHelpers::noMessageThreadBeyondInitialisation(mc);
#endif

	static const Identifier onInit("onInit");

	Identifier callbackIdTouse = callbackId;
	if (callbackIdTouse.isNull())
		callbackIdTouse = onInit;

	try
	{
		prepareTimeout();

        
#if USE_BACKEND
        
        auto copy = javascriptCode;

        String pid = dynamic_cast<Processor*>(root->hiseSpecialData.processor)->getId();
        pid << "." << callbackId.toString();
        
        auto ok = preprocessor->process(copy, pid);
        
        if (!ok.wasOk())
        {
            RootObject::CodeLocation loc(javascriptCode, callbackId.toString() + "()");
            loc.location = loc.program.getCharPointer() + ok.getErrorMessage().getIntValue();
            loc.throwError(ok.getErrorMessage().fromFirstOccurrenceOf(":", false, false));
        }
#else
        auto& copy = javascriptCode;
#endif

		root->execute(copy, allowConstDeclarations);
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
		if(e.externalLocation.isEmpty())
			e.externalLocation = callbackIdTouse.toString() + "()";

		return Result::fail(root->dumpCallStack(e, callbackIdTouse));
	}
	catch (Breakpoint& bp)
	{
		if (bp.localScope != nullptr)
			bp.copyLocalScopeToRoot(*root);

		sendBreakpointMessage(bp.index);
		return Result::fail(root->dumpCallStack(RootObject::Error::fromBreakpoint(bp), callbackIdTouse));
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
		RootObject::Scope(nullptr, root.get(), root.get()).findAndInvokeMethod(function, args, returnVal);
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

DebugInformationBase::Ptr HiseJavascriptEngine::getDebugInformation(int index)
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
	
	if (auto globals = root->hiseSpecialData.globals)
	{
		v = globals->getProperty(id);

		if (!v.isVoid())
			return v;
	}

	return var();
}

void HiseJavascriptEngine::addShaderFile(const File& f)
{
	root->hiseSpecialData.includedFiles.add(new ExternalFileData(ExternalFileData::Type::EmbeddedScript, f, f.getFileName()));
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

	root->hiseSpecialData.createDebugInformation(root.get());
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
		RootObject::Scope(nullptr, root.get(), root.get()).invokeMidiCallback(function, args, returnVal, (scopeToUse != nullptr ? scopeToUse : unneededScope.get()));
	}
	catch (String& error)
	{
		if (result != nullptr) *result = Result::fail(error);
	}

	return returnVal;
}

void HiseJavascriptEngine::sendBreakpointMessage(int breakpointIndex)
{
	for (int i = 0; i < breakpointListeners.size(); i++)
	{
		if (breakpointListeners[i].get() != nullptr)
		{
			breakpointListeners[i]->breakpointWasHit(breakpointIndex);
		}
	}
}

void HiseJavascriptEngine::checkValidParameter(int index, const var& valueToTest, const RootObject::CodeLocation& location, VarTypeChecker::VarTypes expectedType)
{
#if ENABLE_SCRIPTING_SAFE_CHECKS

	if (valueToTest.isUndefined() || valueToTest.isVoid())
	{
		location.throwError("API call with undefined parameter " + String(index));
	}
    
    if(expectedType != VarTypeChecker::Undefined)
    {
        auto ok = VarTypeChecker::checkType(valueToTest, expectedType, false);
        
        if(ok.failed())
        {
            location.throwError(ok.getErrorMessage());
        }
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
    preCompileListeners.sendMessage(sendNotificationSync, true);
    
	if(root != nullptr)
		root->timeout = Time(0);
}

hise::DebugableObjectBase* HiseJavascriptEngine::getDebugObject(const String& token)
{
	if (token.isEmpty())
		return nullptr;

	if (auto obj = ApiProviderBase::getDebugObject(token))
		return obj;

	try
	{
		auto r = root->evaluate(token);

		if (r.isArray())
			return ApiProviderBase::getDebugObject("Array");
		if (r.isString())
			return ApiProviderBase::getDebugObject("String");
		
		if (auto s = dynamic_cast<DebugableObjectBase*>(r.getObject()))
			return s;

		if (auto dyn = r.getDynamicObject())
		{
			return new DynamicDebugableObjectWrapper(dyn, token, token);
		}
	}
	catch (...)
	{
		// we don't care about compile fails here
		return nullptr;
	}

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
	catch (String& )
	{
		return "";
	}
	catch (RootObject::Error& )
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

hise::DebugInformation* HiseJavascriptEngine::RootObject::Callback::getChildElement(int index)
{
	WeakReference<Callback> safeThis(this);

	if (index < getNumArgs())
	{
		auto vf = [safeThis, index]()
		{
			if (safeThis != nullptr)
				return safeThis->parameterValues[index];

			return var();
		};

		String mid = "%PARENT%." + parameters[index].toString();

		return new LambdaValueInformation(vf, mid, {}, DebugInformation::Type::Callback, getLocation());
	}
	else
	{
	index -= numArgs;

	auto id = safeThis->localProperties.getName(index);

	auto vf = [safeThis, id]()
	{
		if (safeThis != nullptr)
			return safeThis->localProperties[id];

		return var();
	};

	String mid = "%PARENT%." + id.toString();

	return new LambdaValueInformation(vf, mid, {}, DebugInformation::Type::Callback, getLocation());
	}

}

struct IncludeFileToken : public mcl::TokenCollection::Token
{
	IncludeFileToken(const File& root_, const File& f):
		Token(""),
		sf(f),
		root(root_)
	{
		markdownDescription << "`" << sf.getFullPathName() << "`";
		
		tokenContent << "include(" << sf.getRelativePathFrom(root).replaceCharacter('\\', '/').quoted() << ");";

		priority = 100;
		c = Colours::magenta;
	}

	

	File root;
	File sf;
};

struct TokenWithDot : public mcl::TokenCollection::Token
{
	TokenWithDot(const String& token, const String& classId_) :
		Token(token),
		classId(classId_)
	{
		
	};

	String getCodeToInsert(const String& input) const override
	{
		if (classId.isEmpty())
			return Token::getCodeToInsert(input);

		if (input.containsChar('.'))
			return tokenContent.fromLastOccurrenceOf(".", false, false);
		else
			return tokenContent;
	}

	bool matches(const String& input, const String& previousToken, int lineNumber) const override
	{
		if (classId.isEmpty())
		{
			if (previousToken.isNotEmpty())
				return false;

			return Token::matches(input, previousToken, lineNumber);
		}

#if 0
		if (previousToken.isNotEmpty() && !previousToken.startsWith(classId))
			return false;
#endif

		return matchesInput(previousToken + input, tokenContent);
	}

    static bool hasCallbackArgument(const ValueTree& method)
    {
        auto s = method["name"].toString();

        auto isArrayFunction = [](const String& s)
        {
            return s == "find" ||
                   s == "filter" ||
                   s == "map" ||
                   s == "some";
        };
        
        return s.contains("Callback") || s.contains("setPaintRoutine") || s.contains("setErrorFunction") || s.contains("setOn") || isArrayFunction(s);
    }
    
    static String getContent(const ValueTree& method, const Identifier objectId)
    {
        String s;
        s << objectId << "." << method["name"].toString();
        
        if (hasCallbackArgument(method))
        {
            auto args = method["arguments"].toString();
            static const String body = "\n{\n\t \n}";

            auto replaceArgs = [&](const String& varName, const String& newArgs)
            {
                if (args.contains(varName))
                {
                    String newF;
                    newF << "function(" << newArgs << ")" << body;

                    args = args.replace("var " + varName, newF);
                }
            };

            auto replaceFCallback = [&](const String& methodName, const String& newArgs)
            {
                if (s.contains(methodName))
                {
                    String func;
                    func << "function(" << newArgs << ")" << body;
                    args = args.replace("var f", func);
                }
            };

            replaceArgs("timerCallback", "");
            replaceArgs("paintFunction", "g");
            replaceArgs("mouseCallbackFunction", "event");
            replaceArgs("loadingCallback", "isPreloading");
            replaceArgs("loadCallback", "obj");
            replaceArgs("saveCallback", "");
            replaceArgs("loadingFunction", ""); // this is peak code quality right here...
            replaceArgs("displayFunction", "displayValue");
            replaceArgs("contentFunction", "changedIndex");
            replaceArgs("testFunction", "currentValue, index, arr");
			replaceArgs("errorCallback", "state, message");

            replaceArgs("playbackCallback", "timestamp, playState");
            replaceArgs("updateCallback", "index, value");
            replaceArgs("presetPreCallback", "presetData");
            replaceArgs("presetPostCallback", "presetFile");
            replaceArgs("newProcessFunction", "fftData, startIndex");
            replaceArgs("backgroundTaskFunction", "thread");
            replaceArgs("newFinishCallback", "isFinished, wasCancelled");

            replaceFCallback("setOnBeatChange", "beatIndex, isNewBar");
            replaceFCallback("setOnSignatureChange", "nom, denom");
            replaceFCallback("setOnTempoChange", "newTempo");
            replaceFCallback("setOnTransportChange", "isPlaying");
            
            s << args;
            s << ";";
        }
        else
        {
            s << method["arguments"].toString().replace("var callback", "function()\n{\t \n}");
        }

        return s;
    }
    
	String classId;
};

struct ObjectConstantToken : public TokenWithDot
{
	ObjectConstantToken(DebugInformation::Ptr parent, const Identifier& id, const var& value):
		TokenWithDot(parent->getTextForName() + "." + id.toString(), parent->getTextForName())
	{
		priority = 99;
		c = Colour(0xFF88EECC);
		markdownDescription << "Constant value: `" << value.toString() << "`";
	}
};

struct HiseJavascriptEngine::TokenProvider::ObjectMethodToken : public TokenWithDot
{
	ObjectMethodToken(ValueTree methodTree, DebugInformation::Ptr parent_) :
		TokenWithDot(getContent(methodTree, parent_->getTextForName()),
			parent_->getTextForName()),
		data(methodTree),
		parent(parent_)
	{
		priority = 100;
		c = Colour(0xFFEE88CC);
		markdownDescription = methodTree["description"].toString();

		String s;
		s << "scripting/scripting-api/";
		s << MarkdownLink::Helpers::getSanitizedFilename(methodTree.getParent().getType().toString());
		s << "#";
		s << MarkdownLink::Helpers::getSanitizedFilename(methodTree["name"].toString()) << "/";

		link = { File(), s };
		link.setType(MarkdownLink::Type::Folder);

		markdownDescription << "  \n[Doc Reference](https://docs.hise.audio/" + link.toString(MarkdownLink::FormattedLinkHtml) + ")";
	}

	

	Array<Range<int>> getSelectionRangeAfterInsert(const String& input) const override
	{
		auto code = getCodeToInsert(input);

		if (code.contains("\t \n"))
		{
			auto brIndex = code.indexOf("\t \n");

			Array<Range<int>> ranges;

			ranges.add({ brIndex + 1, brIndex + 2 });
			return ranges;
		}

		return TokenWithDot::getSelectionRangeAfterInsert(input);
	}

	


	MarkdownLink getLink() const override
	{
		return link;
	}
	

	MarkdownLink link;
	ValueTree data;
	DebugInformationBase::Ptr parent;
};

struct TemplateToken : public TokenWithDot
{
	TemplateToken(const String& expression, const ValueTree& mTree) :
		TokenWithDot(getContent(mTree, expression), expression)
	{
        // expression + "." + mTree["name"].toString() + mTree["arguments"].toString(), expression
		priority = expression == "g" ? 100 : 110;
		c = Colour(0xFFAAAA66);
		markdownDescription = mTree["description"].toString();
	}
};

struct HiseJavascriptEngine::TokenProvider::ApiToken : public TokenWithDot
{
	ApiToken(const Identifier& id, const ValueTree& mTree) :
		TokenWithDot(getContent(id, mTree), id.toString())
	{
		priority = 100;
		c = Colour(0xFF66AACC);
		markdownDescription = mTree["description"].toString();

		String s;
		s << "scripting/scripting-api/";
		s << MarkdownLink::Helpers::getSanitizedFilename(id.toString());
		s << "#";
		s << MarkdownLink::Helpers::getSanitizedFilename(mTree["name"].toString()) << "/";

		link = { File(), s };

		link.setType(MarkdownLink::Type::Folder);

		markdownDescription << "  \n[Doc Reference](https://docs.hise.audio/" + link.toString(MarkdownLink::FormattedLinkHtml) + ")";

	}

	MarkdownLink getLink() const override
	{
		return link;
	}

	static String getContent(const Identifier& id, const ValueTree& c) 
	{
		String s;
		s << id << "." << c["name"].toString() << c["arguments"].toString();
		return s;
	}

	String classId;
	MarkdownLink link;

};

struct HiseJavascriptEngine::TokenProvider::KeywordToken : public mcl::TokenCollection::Token
{
	KeywordToken(String n) :
		Token(n)
	{
		priority = 50;
		c = Colour(0x88EE55CC);
		markdownDescription = "HiseScript keyword";
	}

	bool matches(const String& input, const String& previousToken, int lineNumber) const override
	{
		if (previousToken.isNotEmpty())
		{
			return false;
		}

		return matchesInput(input, tokenContent);
	}
};


struct HiseJavascriptEngine::TokenProvider::DebugInformationToken : public TokenWithDot
{
	DebugInformationToken(DebugInformationBase::Ptr i, ValueTree apiTree_, Colour c_, DebugInformationBase::Ptr parent=nullptr) :
		TokenWithDot(i->getCodeToInsert(), parent != nullptr ? parent->getTextForName() : ""),
		info(i),
		apiTree(apiTree_)
	{
		if(parent != nullptr)
			tokenContent = DebugInformationBase::replaceParentWildcard(tokenContent, parent->getTextForName());

		bool isGlobalClass = false;

		auto s = i->getTextForDataType();

		if (s.isNotEmpty())
		{
			

			auto cId = Identifier(s);
			isGlobalClass = ApiHelpers::getGlobalApiClasses().contains(cId);

			String ml = "/scripting/scripting-api";
			ml << MarkdownLink::Helpers::getSanitizedURL(s);

			link = MarkdownLink(File(), ml);
		}
		
        priority = 110;
		c = c_;

		

		
		if (isGlobalClass)
		{
			if(link.isValid())
			{
				link.setType(MarkdownLink::Type::Folder);
				markdownDescription << " [Doc Reference](https://docs.hise.audio/"  + link.toString(MarkdownLink::FormattedLinkHtml) + ")";
			}
		}
		else
		{
			auto comment = i->getDescription().getText();

			markdownDescription << "**Type:** `" << i->getTextForType() << "`  \n";

			if (comment.isNotEmpty())
				markdownDescription << comment;

			//markdownDescription << "**Value:** " << i->getTextForValue();
		}
	};

	MarkdownLink getLink() const override { return link; }

	MarkdownLink link;

	DebugInformationBase::Ptr info;
	ValueTree apiTree;
};


HiseJavascriptEngine::TokenProvider::TokenProvider(JavascriptProcessor* jp_) :
	jp(jp_)
{
	if (auto pr = dynamic_cast<Processor*>(jp.get()))
		pr->getMainController()->addScriptListener(this);
}

HiseJavascriptEngine::TokenProvider::~TokenProvider()
{
	if (auto pr = dynamic_cast<Processor*>(jp.get()))
		pr->getMainController()->removeScriptListener(this);
}

struct LookAndFeelToken : public TokenWithDot
{
	LookAndFeelToken(const Identifier& oid, const Identifier& fid) :
		TokenWithDot(getContent(oid, fid), oid.toString())
	{
		c = Colours::seashell;
		markdownDescription << "Override the paint routine for `" << fid << "`.  \n> Press F1 for additional information.";

		String s;

		s << "/glossary/custom_lookandfeel#";
		s << MarkdownLink::Helpers::getSanitizedFilename(fid.toString());

		link = MarkdownLink(File(), s);
	};

	MarkdownLink link;

	MarkdownLink getLink() const override
	{
		return link;
	}

	Array<Range<int>> getSelectionRangeAfterInsert(const String& input) const override
	{
		auto code = getCodeToInsert(input);
		auto brIndex = code.indexOf("\t \n");
		Array<Range<int>> ranges;
		ranges.add({ brIndex + 1, brIndex + 2 });
		return ranges;
	}

	static String getContent(const Identifier& oid, const Identifier& fid)
	{
		String s;
		s << oid << ".registerFunction(\"" << fid << "\", function(g, obj)\n{\n\t \n});";
		return s;
	}
};

struct TokenHelpers
{
	static bool addObjectAPIMethods(JavascriptProcessor* jp, mcl::TokenCollection::List& tokens, DebugInformationBase::Ptr ptr, const ValueTree& apiTree, bool addStringMethods)
	{
		auto os = ptr->getTextForType();

		if (auto slaf = dynamic_cast<ScriptingObjects::ScriptedLookAndFeel*>(ptr->getObject()))
		{
			auto l = ScriptingObjects::ScriptedLookAndFeel::getAllFunctionNames();
			
			for (auto id : l)
				tokens.add(new LookAndFeelToken(ptr->getTextForName(), id));
			
		}


		if (os.isNotEmpty())
		{
			Identifier oid(os);

			auto classTree = apiTree.getChildWithName(oid);

			if (classTree.isValid() && (addStringMethods || os != String("String")))
			{
				for (auto method : classTree)
				{
					if (Thread::currentThreadShouldExit() || jp->shouldReleaseDebugLock())
						return false;

					tokens.add(new HiseJavascriptEngine::TokenProvider::ObjectMethodToken(method, ptr));
				}

				if (auto a = dynamic_cast<ApiClass*>(ptr->getObject()))
				{
					Array<Identifier> ids;
					a->getAllConstants(ids);



					int i = 0;
					for (const auto& id : ids)
					{
						auto constantValue = a->getConstantValue(i++);

						if (auto obj = constantValue.getDynamicObject())
						{
							auto thisIndex = i-1;

							auto f = [a, thisIndex]()
							{
								return a->getConstantValue(thisIndex);
							};

							DebugInformationBase::Ptr optr = new LambdaValueInformation(f, id, ptr->getCodeToInsert(), DebugInformation::Type::Constant, ptr->getLocation());

							tokens.add(new HiseJavascriptEngine::TokenProvider::DebugInformationToken(optr, apiTree, Colours::white, ptr));
							addRecursive(jp, tokens, optr, Colours::white, apiTree, false);
						}
						else
						{
							tokens.add(new ObjectConstantToken(ptr, id, constantValue));
						}



					}
				}

				return true;
			}
		}

		return false;
	}

	static void addRecursive(JavascriptProcessor* jp, mcl::TokenCollection::List& tokens, DebugInformationBase::Ptr ptr, Colour c2, ValueTree v, bool addStringMethods)
	{
		if (!ptr->isAutocompleteable())
			return;

		int numChildren = ptr->getNumChildElements();

		for (int j = 0; j < numChildren; j++)
		{
			if (Thread::currentThreadShouldExit() || jp->shouldReleaseDebugLock())
				return;

			auto c = ptr->getChildElement(j);

			if (c == nullptr)
				break;

			char s;

			jp->getProviderBase()->getColourAndLetterForType(c->getType(), c2, s);

			Colour childColour = c2;

			bool isColour = ptr->getTextForName() == "Colours";

			if (isColour)
			{
				auto vs = c->getTextForValue();
				childColour = ScriptingApi::Content::Helpers::getCleanedObjectColour(vs);
			}

			tokens.add(new HiseJavascriptEngine::TokenProvider::DebugInformationToken(c, v, childColour, ptr));

			if (isColour)
				tokens.getLast()->priority = 60;

			if (!addObjectAPIMethods(jp, tokens, c, v, addStringMethods))
				addRecursive(jp, tokens, c, childColour, v, addStringMethods);
		}
	}
};

bool HiseJavascriptEngine::TokenProvider::shouldAbortTokenRebuild(Thread* t) const
{
    return (t != nullptr && t->threadShouldExit()) ||
           (jp == nullptr) ||
           (jp != nullptr && jp->shouldReleaseDebugLock());
}


void HiseJavascriptEngine::TokenProvider::addTokens(mcl::TokenCollection::List& tokens)
{
	if (jp != nullptr)
	{
		LockHelpers::SafeLock ssl(dynamic_cast<Processor*>(jp.get())->getMainController(), LockHelpers::Type::ScriptLock);

		File scriptFolder = dynamic_cast<Processor*>(jp.get())->getMainController()->getCurrentFileHandler().getSubDirectory(FileHandlerBase::Scripts);
		auto scriptFiles = scriptFolder.findChildFiles(File::findFiles, true, "*.js");

		for (auto sf: scriptFiles)
		{
			if (sf.isHidden())
				continue;

			auto alreadyIncluded = false;

			for (int i = 0; i < jp->getNumWatchedFiles(); i++)
			{
				if (jp->getWatchedFile(i) == sf)
				{
					alreadyIncluded = true;
					break;
				}
			}

			if (alreadyIncluded)
				continue;

			auto pf = sf.getParentDirectory().getFullPathName();

			if (pf.contains("ScriptProcessors") || pf.contains("ConnectedScripts"))
				continue;

			tokens.add(new IncludeFileToken(scriptFolder, sf));
		}
		

		//ScopedReadLock sl(jp->getDebugLock());

		auto holder = dynamic_cast<ApiProviderBase::Holder*>(jp.get());

		auto v = holder->createApiTree();

		auto copy = jp->autoCompleteTemplates;

		copy.add({ "g", Identifier("Graphics") });

		for (const auto& t : copy)
		{
			auto cTree = v.getChildWithName(t.classId);

			for (auto m : cTree)
			{
				if (t.expression.isEmpty())
					continue;

				tokens.add(new TemplateToken(t.expression, m));
			}
		}

		if (WeakReference<HiseJavascriptEngine> e = jp->getScriptEngine())
		{
            e->preCompileListeners.addListener(*this, TokenProvider::precompileCallback, false);
            
			auto numObjects = e->getNumDebugObjects();

			if (true)
			{
				auto classTree = v.getChildWithName("Content");

				for (auto methodTree : classTree)
				{
					if (Thread::currentThreadShouldExit() || jp->shouldReleaseDebugLock())
						return;

					tokens.add(new ApiToken("Content", methodTree));
				}

			}

			for (int i = 0; i < numObjects; i++)
			{
				if (shouldAbortTokenRebuild(Thread::getCurrentThread()))
					return;

				if (e == nullptr)
				{
					tokens.clear();
					return;
				}

				auto ptr = e->getDebugInformation(i);

				if (ptr == nullptr)
					return;

				Colour c2;
				char s;

				holder->getProviderBase()->getColourAndLetterForType(ptr->getType(), c2, s);

				tokens.add(new DebugInformationToken(ptr, v, c2));

				auto t = ptr->getTextForDataType();

				if (t.isNotEmpty())
				{
					Identifier cid(ptr->getTextForDataType());

					if (ApiHelpers::getGlobalApiClasses().contains(cid))
					{
						auto classTree = v.getChildWithName(cid);

						for (auto methodTree : classTree)
						{
							if (shouldAbortTokenRebuild(Thread::getCurrentThread()))
								return;

							tokens.add(new ApiToken(cid, methodTree));
						}
					}
					else
					{
						TokenHelpers::addObjectAPIMethods(jp, tokens, ptr.get(), v, true);
					}

					TokenHelpers::addRecursive(jp, tokens, ptr, c2, v, true);
				}
			}
		}
		
#if USE_BACKEND
		for (const auto& def : jp->getScriptEngine()->preprocessor->definitions)
		{
			tokens.add(new snex::debug::PreprocessorMacroProvider::PreprocessorToken(def));
		}
#endif

		

#define X(unused, name) tokens.add(new KeywordToken(name));

		X(var, "var")      X(if_, "if")     X(else_, "else")   X(do_, "do")       X(null_, "null") 
		X(while_, "while")    X(for_, "for")    X(break_, "break")  X(continue_, "continue") X(undefined, "undefined") 
		X(function, "function") X(return_, "return") X(true_, "true")   X(false_, "false")    X(new_, "new") 
		X(typeof_, "typeof")	X(switch_, "switch") X(case_, "case")	 X(default_, "default")  X(register_var, "reg") 
		X(in, "in")		X(inline_, "inline") X(const_, "const")	 X(global_, "global")	  X(local_, "local") 
		X(include_, "include") X(rLock_, "readLock") X(wLock_, "writeLock") 	X(extern_, "extern") X(namespace_, "namespace") 
		X(isDefined_, "isDefined");

#undef X

		for (int i = 0; i < tokens.size(); i++)
		{
			if(tokens[i]->tokenContent.contains("[") ||
			   tokens[i]->tokenContent.contains("%PARENT%") ||
			   tokens[i]->tokenContent.endsWith(".args") ||
			   tokens[i]->tokenContent.endsWith(".locals"))
			{
				tokens.remove(i--);
			}
		}


	}
}

void HiseJavascriptEngine::TokenProvider::precompileCallback(TokenProvider& p, bool unused)
{
	p.signalClear(sendNotificationSync);
}

void HiseJavascriptEngine::TokenProvider::scriptWasCompiled(JavascriptProcessor* processor)
{
	if (jp == processor)
		signalRebuild();
}

hise::HiseJavascriptEngine::RootObject::OptimizationPass::OptimizationResult HiseJavascriptEngine::RootObject::OptimizationPass::executePass(Statement* rootStatementToOptimize)
{
	OptimizationResult r;
	r.passName = getPassName();

	callForEach(rootStatementToOptimize, [this, &r](Statement* st)
	{
		if (st == nullptr)
			return false;

		int index = 0;

		while (auto child = st->getChildStatement(index++))
		{
			auto optimizedStatement = getOptimizedStatement(st, child);

			if (optimizedStatement != child)
			{
				ScopedPointer<Statement> newExpr(optimizedStatement);
				auto ok = st->replaceChildStatement(newExpr, child);
                ignoreUnused(ok);
				jassert(ok);
				r.numOptimizedStatements++;
			}
		}

		return false;
	});

	return r;
}

HiseJavascriptEngine::RootObject::Error HiseJavascriptEngine::RootObject::Error::fromBreakpoint(const Breakpoint& bp)
{
	Error e;

	e.errorMessage = "Breakpoint " + String(bp.index + 1) + " was hit";
	e.externalLocation = bp.externalLocation;
	e.lineNumber = bp.lineNumber;
	e.charIndex = bp.charIndex;
	e.columnNumber = bp.colNumber;
				
	return e;
}

String HiseJavascriptEngine::RootObject::Error::getLocationString() const
{
	if (externalLocation.isEmpty() || externalLocation.contains("()"))
		return "Line " + String(lineNumber) + ", column " + String(columnNumber);
	else
	{
#if USE_BACKEND
		File f(externalLocation);
		const String fileName = f.getFileName();
#else
					const String fileName = externalLocation;
#endif

		return fileName + " (" + String(lineNumber) + ")"; 
	}
}

String HiseJavascriptEngine::RootObject::Error::getEncodedLocation(Processor* p) const
{
	ignoreUnused(p);

#if USE_BACKEND
	String l;

	l << p->getId() << "|";

	if (externalLocation.contains("()"))
		l << externalLocation;
	else if (!externalLocation.isEmpty())
		l << File(externalLocation).getRelativePathFrom(GET_PROJECT_HANDLER(p).getSubDirectory(ProjectHandler::SubDirectories::Scripts));

	l << "|" << String(charIndex);
	l << "|" << String(lineNumber) << "|" << String(columnNumber);

	return "{" + Base64::toBase64(l) + "}";
#else
				return {};
#endif
}

String HiseJavascriptEngine::RootObject::Error::toString(Processor* p) const
{
	String s;
	s << getLocationString();
	s << "\t" << getEncodedLocation(p);

	return s;
}

bool HiseJavascriptEngine::RootObject::OptimizationPass::callForEach(Statement* root, const std::function<bool(Statement* child)>& f)
{
	if (f(root))
		return true;

	int index = 0;

	while (auto child = root->getChildStatement(index++))
	{
		if (callForEach(child, f))
			return true;
	}

	return false;
}





} // namespace hise
