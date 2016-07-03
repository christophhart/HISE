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
    X(rightShiftUnsigned, ">>>") X(rightShiftEquals, ">>=") X(rightShift,   ">>")   X(greaterThanOrEqual, ">=")  X(greaterThan,  ">")

#define JUCE_JS_KEYWORDS(X) \
    X(var,      "var")      X(if_,     "if")     X(else_,  "else")   X(do_,       "do")       X(null_,     "null") \
    X(while_,   "while")    X(for_,    "for")    X(break_, "break")  X(continue_, "continue") X(undefined, "undefined") \
    X(function, "function") X(return_, "return") X(true_,  "true")   X(false_,    "false")    X(new_,      "new") \
    X(typeof_,  "typeof")	X(switch_, "switch") X(case_, "case")	 X(default_,  "default")  X(register_var, "reg") \
	X(in, 		"in")		X(inline_, "inlinefunction")

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



HiseJavascriptEngine::~HiseJavascriptEngine() {}

void HiseJavascriptEngine::prepareTimeout() const noexcept{ root->timeout = Time::getCurrentTime() + maximumExecutionTime; }

void HiseJavascriptEngine::registerNativeObject(const Identifier& name, DynamicObject* object)
{
	root->setProperty(name, object);
}


struct HiseJavascriptEngine::RootObject::CodeLocation
{
	CodeLocation(const String& code) noexcept        : program(code), location(program.getCharPointer()) {}
	CodeLocation(const CodeLocation& other) noexcept : program(other.program), location(other.location) {}

	void throwError(const String& message) const
	{
		int col = 1, line = 1;

		for (String::CharPointerType i(program.getCharPointer()); i < location && !i.isEmpty(); ++i)
		{
			++col;
			if (*i == '\n')  { col = 1; ++line; }
		}

		throw "Line " + String(line) + ", column " + String(col) + " : " + message;
	}

	String program;
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
		root->execute(getString(a, 0));

	return var::undefined();
}

var HiseJavascriptEngine::RootObject::eval(Args a)
{
	if (RootObject* root = dynamic_cast<RootObject*> (a.thisObject.getObject()))
		return root->evaluate(getString(a, 0));

	return var::undefined();
}

Result HiseJavascriptEngine::execute(const String& code)
{
	try
	{
		prepareTimeout();
		root->execute(code);
	}
	catch (String& error)
	{
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

void HiseJavascriptEngine::registerApiClass(ApiObject2 *apiClass)
{
	root->apiClasses.add(apiClass);
	root->apiIds.add(apiClass->getName());
}

ApiObject2::Constant ApiObject2::Constant::null;

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