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
	X(include_,  "include") X(rLock_,   "readLock") X(wLock_,"writeLock") 	X(extern_, "extern")
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
	root->hiseSpecialData.clear();
	root = nullptr;
}

void HiseJavascriptEngine::prepareTimeout() const noexcept{ root->timeout = Time::getCurrentTime() + maximumExecutionTime; }




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

	void throwError(const String& message) const
	{
		int col = 1, line = 1;

		for (String::CharPointerType i(program.getCharPointer()); i < location && !i.isEmpty(); ++i)
		{
			++col;
			if (*i == '\n')  { col = 1; ++line; }
		}

		if (!externalFile.isEmpty())
		{
#if USE_BACKEND
			File f(externalFile);
			const String fileName = f.getFileName();
#else
			const String fileName = externalFile;
#endif


#if USE_BACKEND
			throw fileName + " - Line " + String(line) + ", column " + String(col) + ": " + message;
#else
			if(message != "Execution timed-out") DBG(fileName + " - Line " + String(line) + ", column " + String(col) + ": " + message);
#endif
		}
		else
		{
#if USE_BACKEND
			throw "Line " + String(line) + ", column " + String(col) + ": " + message;
#else
			if (message != "Execution timed-out") DBG("Line " + String(line) + ", column " + String(col) + ": " + message);
#endif
		}
		

		
	}

	String program;
	String externalFile;
	String::CharPointerType location;
	
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
		if (Time::getCurrentTime() > root->timeout)
			location.throwError("Execution timed-out");
	}
};



struct HiseJavascriptEngine::RootObject::Statement
{
	Statement(const CodeLocation& l) noexcept : location(l) {}
	virtual ~Statement() {}

	enum ResultCode  { ok = 0, returnWasHit, breakWasHit, continueWasHit };
	virtual ResultCode perform(const Scope&, var*) const  { return ok; }

	CodeLocation location;
	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Statement)
};

struct HiseJavascriptEngine::RootObject::Expression : public Statement
{
	Expression(const CodeLocation& l) noexcept : Statement(l) {}

	virtual var getResult(const Scope&) const            { return var::undefined(); }
	virtual void assign(const Scope&, const var&) const  { location.throwError("Cannot assign to this expression!"); }

	ResultCode perform(const Scope& s, var*) const override  { getResult(s); return ok; }
};


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
	try
	{
		prepareTimeout();
		root->execute(javascriptCode, allowConstDeclarations);
	}
	catch (String& error)
	{
#if USE_FRONTEND
		DBG(error);
#endif
		return Result::fail(error);
	}

	return Result::ok();
}

var HiseJavascriptEngine::evaluate(const String& code, Result* result)
{
	try
	{
		prepareTimeout();
		if (result != nullptr) *result = Result::ok();
		return root->evaluate(code);
	}
	catch (String& error)
	{
#if USE_FRONTEND
		DBG(error);
#endif
		if (result != nullptr) *result = Result::fail(error);
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

void HiseJavascriptEngine::registerApiClass(ApiClass *apiClass)
{
	root->hiseSpecialData.apiClasses.add(apiClass);
	root->hiseSpecialData.apiIds.add(apiClass->getName());
}

bool HiseJavascriptEngine::isApiClassRegistered(const String& className)
{
	Identifier id(className);

	return root->hiseSpecialData.apiIds.contains(id);
}


const ApiClass* HiseJavascriptEngine::getApiClass(const Identifier &className) const
{
	const int index = root->hiseSpecialData.apiIds.indexOf(className);

	if (index != -1)
	{
		return root->hiseSpecialData.apiClasses[index];
	}

	return nullptr;
}

ApiClass::Constant ApiClass::Constant::null;

#if JUCE_MSVC
#pragma warning (pop)
#endif



var HiseJavascriptEngine::callFunction(const Identifier& function, const var::NativeFunctionArgs& args, Result* result)
{
	var returnVal(var::undefined());

	try
	{
		prepareTimeout();
		if (result != nullptr) *result = Result::ok();
		RootObject::Scope(nullptr, root, root).findAndInvokeMethod(function, args, returnVal);
	}
	catch (String& error)
	{
		if (result != nullptr) *result = Result::fail(error);
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


void HiseJavascriptEngine::setCallbackParameter(int callbackIndex, int parameterIndex, var newValue)
{
	root->hiseSpecialData.callbackNEW[callbackIndex]->setParameterValue(parameterIndex, newValue);
}

DebugInformation* HiseJavascriptEngine::getDebugInformation(int index)
{
	return root->hiseSpecialData.getDebugInformation(index);
}


const ReferenceCountedObject* HiseJavascriptEngine::getScriptObject(const Identifier &id) const
{
	String idAsString = id.toString();

	if (idAsString.containsChar('.'))
	{
		StringArray sa = StringArray::fromTokens(idAsString, ".", "");

		const ReferenceCountedObject* o = getScriptObjectFromRootNamespace(Identifier(sa[0]));

		if (auto dyn = dynamic_cast<const DynamicObject*>(o))
		{
			const ReferenceCountedObject* o2 = dyn->getProperty(Identifier(sa[1])).getObject();

			return o2;

		}
		else if (auto api = dynamic_cast<const ApiClass*>(o))
		{
			const int index = api->getConstantIndex(Identifier(sa[1]));
			const ReferenceCountedObject* o2 = api->getConstantValue(index).getObject();

			return o2;
		}
		else return nullptr;
	}
	else
	{
		return getScriptObjectFromRootNamespace(id);
	}
}


const ReferenceCountedObject* HiseJavascriptEngine::getScriptObjectFromRootNamespace(const Identifier & id) const
{
	var v = root->getProperty(id);
	if (v.isObject())
		return v.getObject();

	v = root->hiseSpecialData.constObjects[id];
	if (v.isObject())
		return v.getObject();

	int registerIndex = root->hiseSpecialData.varRegister.getRegisterIndex(id);
	if (registerIndex != -1)
	{
		v = root->hiseSpecialData.varRegister.getFromRegister(registerIndex);

		if (v.isObject())
			return v.getObject();
	}

	DynamicObject* globals = root->hiseSpecialData.globals;
	if (globals != nullptr)
	{
		v = globals->getProperty(id);

		if (v.isObject())
			return v.getObject();
	}

	return nullptr;
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

