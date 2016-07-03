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

//==============================================================================
struct HiseJavascriptEngine::RootObject : public DynamicObject
{
	RootObject();

	Time timeout;

	typedef const var::NativeFunctionArgs& Args;
	typedef const char* TokenType;

	VarRegister varRegister;

	ReferenceCountedArray<ApiObject2> apiClasses;
	Array<Identifier> apiIds;

	void execute(const String& code);

	var evaluate(const String& code);

	//==============================================================================
	static bool areTypeEqual(const var& a, const var& b)
	{
		return a.hasSameTypeAs(b) && isFunction(a) == isFunction(b)
			&& (((a.isUndefined() || a.isVoid()) && (b.isUndefined() || b.isVoid())) || a == b);
	}

	static String getTokenName(TokenType t);
	static bool isFunction(const var& v) noexcept;
	static bool isNumeric(const var& v) noexcept;
	static bool isNumericOrUndefined(const var& v) noexcept;
	static int64 getOctalValue(const String& s);
	static Identifier getPrototypeIdentifier();
	static var* getPropertyPointer(DynamicObject* o, const Identifier& i) noexcept;

		//==============================================================================
	struct CodeLocation
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

	//==============================================================================
	struct Scope
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


	//==============================================================================
	struct Statement
	{
		Statement(const CodeLocation& l) noexcept : location(l) {}
		virtual ~Statement() {}

		enum ResultCode  { ok = 0, returnWasHit, breakWasHit, continueWasHit };
		virtual ResultCode perform(const Scope&, var*) const  { return ok; }

		CodeLocation location;
		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Statement)
	};

	struct Expression : public Statement
	{
		Expression(const CodeLocation& l) noexcept : Statement(l) {}

		virtual var getResult(const Scope&) const            { return var::undefined(); }
		virtual void assign(const Scope&, const var&) const  { location.throwError("Cannot assign to this expression!"); }

		ResultCode perform(const Scope& s, var*) const override  { getResult(s); return ok; }
	};

	typedef ScopedPointer<Expression> ExpPtr;

	struct BlockStatement : public Statement
	{
		BlockStatement(const CodeLocation& l) noexcept : Statement(l) {}

		ResultCode perform(const Scope& s, var* returnedValue) const override
		{
			for (int i = 0; i < statements.size(); ++i)
				if (ResultCode r = statements.getUnchecked(i)->perform(s, returnedValue))
					return r;

			return ok;
		}

		OwnedArray<Statement> statements;
	};

	struct IfStatement : public Statement
	{
		IfStatement(const CodeLocation& l) noexcept : Statement(l) {}

		ResultCode perform(const Scope& s, var* returnedValue) const override
		{
			return (condition->getResult(s) ? trueBranch : falseBranch)->perform(s, returnedValue);
		}

		ExpPtr condition;
		ScopedPointer<Statement> trueBranch, falseBranch;
	};

	struct CaseStatement: public Statement
	{
		CaseStatement(const CodeLocation &l, bool isNotDefault_) noexcept: Statement(l), isNotDefault(isNotDefault_), initialized(false) {};

		ResultCode perform(const Scope &s, var *returnValue) const override
		{
			return body->perform(s, returnValue);
		}

		void initValues(const Scope &s)
		{
			if (!initialized)
			{
				initialized = true;

				values.ensureStorageAllocated(conditions.size());

				for (int i = 0; i < conditions.size(); i++)
				{
					values.add(conditions[i]->getResult(s));
				}
			}
		}

		Array<ExpPtr> conditions;
		Array<var> values;
		const bool isNotDefault;
		bool initialized;
		ScopedPointer<BlockStatement> body;
	};

	struct SwitchStatement : public Statement
	{
		SwitchStatement(const CodeLocation &l) noexcept: Statement(l) {};

		ResultCode perform(const Scope &s, var *returnValue) const override
		{
			var selectedCase = condition->getResult(s);

			bool caseFound = false;

			for (int i = 0; i < cases.size(); i++)
			{
				cases[i]->initValues(s);

				if (cases[i]->values.contains(selectedCase))
				{
					ResultCode caseResult = cases[i]->body->perform(s, returnValue);

					if (caseResult == Statement::breakWasHit)
					{
						caseFound = true;
						break;
					}
				}
			}

			if (!caseFound)
			{
				if (defaultCase != nullptr)
				{
					defaultCase->perform(s, returnValue);
				}
			}

			return Statement::ok;
		}

		OwnedArray<CaseStatement> cases;

		ScopedPointer<CaseStatement> defaultCase;
		ExpPtr condition;
	};

	

	struct VarStatement : public Statement
	{
		VarStatement(const CodeLocation& l) noexcept : Statement(l) {}

		ResultCode perform(const Scope& s, var*) const override
		{
			s.scope->setProperty(name, initialiser->getResult(s));
			return ok;
		}

		Identifier name;
		ExpPtr initialiser;
	};

	struct RegisterVarStatement : public Statement
	{
		RegisterVarStatement(const CodeLocation& l) noexcept : Statement(l) {}

		ResultCode perform(const Scope& s, var*) const override
		{
			s.root->varRegister.addRegister(name, initialiser->getResult(s));
			return ok;
		}

		Identifier name;
		ExpPtr initialiser;
	};

	struct NextIteratorStatement : public Statement
	{
		

		// This is used just for parsing the statement
		struct IteratorOp : public Expression
		{
			IteratorOp(const CodeLocation& l, Expression *i, Expression *o, Identifier &iteratorId_) noexcept : Expression(l), iterator(i), object(o), iteratorId(iteratorId_) {}

			var getResult(const Scope& s) const override { jassertfalse; return var::undefined(); }

			void assign(const Scope& s, const var& v) const override { jassertfalse; }

			Expression *iterator;
			Expression *object;

			const Identifier iteratorId;
		};

		NextIteratorStatement(const CodeLocation& l, IteratorOp &op) noexcept : 
		  Statement(l), 
	      iterator(op.iterator),
		  object(op.object),
		  iteratorId(op.iteratorId)
		{}

		void initObject(const Scope &s)
		{
			s.currentIteratorObject = object->getResult(s);

			a = s.currentIteratorObject.getArray();

			s.currentIteratorIndex = 0;
			
			isArrayIterator = a != nullptr;
			index = 0;
		}

		ResultCode perform(const Scope& s, var* result) const override
		{
			if (isArrayIterator)
			{
				if (s.currentIteratorIndex <s.currentIteratorObject.getArray()->size())
				{
					return Statement::continueWasHit;
				}
				else return Statement::returnWasHit;
			}
			else return Statement::returnWasHit;
		}

		ExpPtr iterator;
		ExpPtr object;

		Identifier iteratorId;

		mutable int index;

		bool isArrayIterator;
		Array<var> *a;
	};

	

	struct LoopStatement : public Statement
	{
		struct IteratorName : public Expression
		{
			IteratorName(const CodeLocation& l, Identifier id_) noexcept : Expression(l), id(id_) {}

			var getResult(const Scope& s) const override
			{
				LoopStatement *loop = (LoopStatement*)(s.currentLoopStatement);

				var *data = &loop->currentObject;

				CHECK_CONDITION(data != nullptr, "data does not exist");

				if (data->isArray())
				{
					return data->getArray()->getUnchecked(loop->index);
				}
				else if (data->isBuffer())
				{
					return data->getBuffer()->getSample(loop->index);
				}
			}

			void assign(const Scope& s, const var& newValue) const override
			{
				LoopStatement *loop = (LoopStatement*)(s.currentLoopStatement);

				var *data = &loop->currentObject;

				CHECK_CONDITION(data != nullptr, "data does not exist");

				if (data->isArray())
				{
					data->getArray()->set(loop->index, newValue);
				}
				else if (data->isBuffer())
				{
					return data->getBuffer()->setSample(loop->index, newValue);
				}
			}

			bool isArray;
			bool isBuffer;

			const Identifier id;
		};

		LoopStatement(const CodeLocation& l, bool isDo, bool isIterator_=false) noexcept : Statement(l), isDoLoop(isDo), isIterator(isIterator_) {}

		ResultCode perform(const Scope& s, var* returnedValue) const override
		{
			if (isIterator)
			{
				CHECK_CONDITION((currentIterator != nullptr), "Iterator does not exist");

				currentObject = currentIterator->getResult(s);

				ScopedValueSetter<void*> loopScoper(s.currentLoopStatement, (void*)this);
	
				index = 0;
				
				const int size = currentObject.isArray() ? currentObject.getArray()->size() :
								 currentObject.isBuffer() ? currentObject.getBuffer()->size : 0;

				while (index < size)
				{
					ResultCode r = body->perform(s, returnedValue);
							
					index++;

					if (r == returnWasHit)   return r;
					if (r == breakWasHit)    break;
				}

				return ok;
			}
			else
			{
				initialiser->perform(s, nullptr);

				while (isDoLoop || condition->getResult(s))
				{
					s.checkTimeOut(location);
					ResultCode r = body->perform(s, returnedValue);

					if (r == returnWasHit)   return r;
					if (r == breakWasHit)    break;

					iterator->perform(s, nullptr);

					if (isDoLoop && r != continueWasHit && !condition->getResult(s))
						break;
				}

				return ok;
			}
		}

		ScopedPointer<Statement> initialiser, iterator, body;

		ExpPtr condition;

		ExpPtr currentIterator;

		bool isDoLoop;

		bool isIterator;

		mutable int index;
		
		mutable var currentObject;
	};

	
	

	struct ReturnStatement : public Statement
	{
		ReturnStatement(const CodeLocation& l, Expression* v) noexcept : Statement(l), returnValue(v) {}

		ResultCode perform(const Scope& s, var* ret) const override
		{
			if (ret != nullptr)  *ret = returnValue->getResult(s);
			return returnWasHit;
		}

		ExpPtr returnValue;
	};

	struct BreakStatement : public Statement
	{
		BreakStatement(const CodeLocation& l) noexcept : Statement(l) {}
		ResultCode perform(const Scope&, var*) const override  { return breakWasHit; }
	};

	struct ContinueStatement : public Statement
	{
		ContinueStatement(const CodeLocation& l) noexcept : Statement(l) {}
		ResultCode perform(const Scope&, var*) const override  { return continueWasHit; }
	};

	struct LiteralValue : public Expression
	{
		LiteralValue(const CodeLocation& l, const var& v) noexcept : Expression(l), value(v) {}
		var getResult(const Scope&) const override   { return value; }
		var value;
	};

	struct UnqualifiedName : public Expression
	{
		UnqualifiedName(const CodeLocation& l, const Identifier& n) noexcept : Expression(l), name(n) {}

		var getResult(const Scope& s) const override  { return s.findSymbolInParentScopes(name); }

		void assign(const Scope& s, const var& newValue) const override
		{
			if (var* v = getPropertyPointer(s.scope, name))
				*v = newValue;
			else
				s.root->setProperty(name, newValue);
		}

		Identifier name;
	};

	struct RegisterName;
	struct ArraySubscript;
	struct DotOperator;
	struct BinaryOperatorBase;
	struct BinaryOperator;
	struct EqualsOp;
	struct NotEqualsOp;
	struct LessThanOp;
	struct LessThanOrEqualOp;
	struct GreaterThanOp;
	struct GreaterThanOrEqualOp;
	struct AdditionOp;
	struct SubtractionOp;
	struct MultiplyOp;
	struct DivideOp;
	struct ModuloOp;
	struct BitwiseAndOp;
	struct BitwiseOrOp;
	struct BitwiseXorOp;
	struct LeftShiftOp;
	struct RightShiftOp;
	struct RightShiftUnsignedOp;

	struct LogicalAndOp;
	struct LogicalOrOp;
	struct TypeEqualsOp;
	struct TypeNotEqualsOp;
	struct ConditionalOp;
	struct Assignment;

	struct RegisterAssignment;
	struct SelfAssignment;
	struct PostAssignment;
	struct ApiConstant;
	struct ApiCall;

	struct FunctionCall;
	struct NewOperator;

	struct ObjectDeclaration;
	struct ArrayDeclaration;
	struct FunctionObject;


	// Parser classes

	struct TokenIterator;
	struct ExpressionTreeBuilder;

	//==============================================================================
	static var get(Args a, int index) noexcept{ return index < a.numArguments ? a.arguments[index] : var(); }
	static bool isInt(Args a, int index) noexcept{ return get(a, index).isInt() || get(a, index).isInt64(); }
	static int getInt(Args a, int index) noexcept{ return get(a, index); }
	static double getDouble(Args a, int index) noexcept{ return get(a, index); }
	static String getString(Args a, int index) noexcept{ return get(a, index).toString(); }

	// Object classes

	struct MathClass;
	struct IntegerClass;
	struct ObjectClass;
	struct ArrayClass;
	struct StringClass;
	struct JSONClass;

	//==============================================================================
	static var trace(Args a)      { Logger::outputDebugString(JSON::toString(a.thisObject)); return var::undefined(); }
	static var charToInt(Args a)  { return (int)(getString(a, 0)[0]); }

	static var typeof_internal(Args a)
	{
		var v(get(a, 0));

		if (v.isVoid())                      return "void";
		if (v.isString())                    return "string";
		if (isNumeric(v))                   return "number";
		if (isFunction(v) || v.isMethod())  return "function";
		if (v.isObject())                    return "object";

		return "undefined";
	}

	static var exec(Args a)
	{
		if (RootObject* root = dynamic_cast<RootObject*> (a.thisObject.getObject()))
			root->execute(getString(a, 0));

		return var::undefined();
	}

	static var eval(Args a)
	{
		if (RootObject* root = dynamic_cast<RootObject*> (a.thisObject.getObject()))
			return root->evaluate(getString(a, 0));

		return var::undefined();
	}
};


HiseJavascriptEngine::~HiseJavascriptEngine() {}

void HiseJavascriptEngine::prepareTimeout() const noexcept{ root->timeout = Time::getCurrentTime() + maximumExecutionTime; }

void HiseJavascriptEngine::registerNativeObject(const Identifier& name, DynamicObject* object)
{
	root->setProperty(name, object);
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
