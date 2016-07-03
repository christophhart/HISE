
struct HiseJavascriptEngine::RootObject::ArraySubscript : public Expression
{
	ArraySubscript(const CodeLocation& l) noexcept : Expression(l) {}

	var getResult(const Scope& s) const override
	{
		var result = object->getResult(s);

		if (VariantBuffer *b = result.getBuffer())
		{
			const int i = index->getResult(s);
			return (*b)[i];
		}
		else if (const Array<var>* array = result.getArray())
			return (*array)[static_cast<int> (index->getResult(s))];

		return var::undefined();
	}

	void assign(const Scope& s, const var& newValue) const override
	{
		var result = object->getResult(s);

		if (VariantBuffer *b = result.getBuffer())
		{
			const int i = index->getResult(s);

			(*b)[i] = newValue;
			return;
		}
		else if (Array<var>* array = result.getArray())
		{
			const int i = index->getResult(s);
			while (array->size() < i)
				array->add(var::undefined());

			array->set(i, newValue);
			return;
		}


		Expression::assign(s, newValue);
	}

	ExpPtr object, index;
};



struct HiseJavascriptEngine::RootObject::DotOperator : public Expression
{
	DotOperator(const CodeLocation& l, ExpPtr& p, const Identifier& c) noexcept : Expression(l), parent(p), child(c) {}

	var getResult(const Scope& s) const override
	{
		var p(parent->getResult(s));
		static const Identifier lengthID("length");

		if (child == lengthID)
		{
			if (Array<var>* array = p.getArray())   return array->size();
			if (p.isBuffer()) return p.getBuffer()->size;

			if (p.isString())                       return p.toString().length();
		}

		if (DynamicObject* o = p.getDynamicObject())
			if (const var* v = getPropertyPointer(o, child))
				return *v;

		return var::undefined();
	}

	void assign(const Scope& s, const var& newValue) const override
	{
		if (DynamicObject* o = parent->getResult(s).getDynamicObject())
			o->setProperty(child, newValue);
		else
			Expression::assign(s, newValue);
	}

	ExpPtr parent;
	Identifier child;
};



struct HiseJavascriptEngine::RootObject::BinaryOperatorBase : public Expression
{
	BinaryOperatorBase(const CodeLocation& l, ExpPtr& a, ExpPtr& b, TokenType op) noexcept
		: Expression(l), lhs(a), rhs(b), operation(op) {}

	ExpPtr lhs, rhs;
	TokenType operation;
};

struct HiseJavascriptEngine::RootObject::BinaryOperator : public BinaryOperatorBase
{
	BinaryOperator(const CodeLocation& l, ExpPtr& a, ExpPtr& b, TokenType op) noexcept
		: BinaryOperatorBase(l, a, b, op) {}

	virtual var getWithUndefinedArg() const                           { return var::undefined(); }
	virtual var getWithDoubles(double, double) const                 { return throwError("Double"); }
	virtual var getWithInts(int64, int64) const                      { return throwError("Integer"); }
	virtual var getWithArrayOrObject(const var& a, const var&) const { return throwError(a.isArray() ? "Array" : "Object"); }
	virtual var getWithStrings(const String&, const String&) const   { return throwError("String"); }

	var getResult(const Scope& s) const override
	{
		var a(lhs->getResult(s)), b(rhs->getResult(s));

		if (isNumericOrUndefined(a) && isNumericOrUndefined(b))
			return (a.isDouble() || b.isDouble()) ? getWithDoubles(a, b) : getWithInts(a, b);

		if ((a.isUndefined() || a.isVoid()) && (b.isUndefined() || b.isVoid()))
			return getWithUndefinedArg();

		if (a.isArray() || a.isObject())
			return getWithArrayOrObject(a, b);

		return getWithStrings(a.toString(), b.toString());
	}

	var throwError(const char* typeName) const
	{
		location.throwError(getTokenName(operation) + " is not allowed on the " + typeName + " type"); return var();
	}
};

struct HiseJavascriptEngine::RootObject::EqualsOp : public BinaryOperator
{
	EqualsOp(const CodeLocation& l, ExpPtr& a, ExpPtr& b) noexcept : BinaryOperator(l, a, b, TokenTypes::equals) {}
	var getWithUndefinedArg() const override                               { return true; }
	var getWithDoubles(double a, double b) const override                 { return a == b; }
	var getWithInts(int64 a, int64 b) const override                      { return a == b; }
	var getWithStrings(const String& a, const String& b) const override   { return a == b; }
	var getWithArrayOrObject(const var& a, const var& b) const override   { return a == b; }
};

struct HiseJavascriptEngine::RootObject::NotEqualsOp : public BinaryOperator
{
	NotEqualsOp(const CodeLocation& l, ExpPtr& a, ExpPtr& b) noexcept : BinaryOperator(l, a, b, TokenTypes::notEquals) {}
	var getWithUndefinedArg() const override                               { return false; }
	var getWithDoubles(double a, double b) const override                 { return a != b; }
	var getWithInts(int64 a, int64 b) const override                      { return a != b; }
	var getWithStrings(const String& a, const String& b) const override   { return a != b; }
	var getWithArrayOrObject(const var& a, const var& b) const override   { return a != b; }
};

struct HiseJavascriptEngine::RootObject::LessThanOp : public BinaryOperator
{
	LessThanOp(const CodeLocation& l, ExpPtr& a, ExpPtr& b) noexcept : BinaryOperator(l, a, b, TokenTypes::lessThan) {}
	var getWithDoubles(double a, double b) const override                 { return a < b; }
	var getWithInts(int64 a, int64 b) const override                      { return a < b; }
	var getWithStrings(const String& a, const String& b) const override   { return a < b; }
};

struct HiseJavascriptEngine::RootObject::LessThanOrEqualOp : public BinaryOperator
{
	LessThanOrEqualOp(const CodeLocation& l, ExpPtr& a, ExpPtr& b) noexcept : BinaryOperator(l, a, b, TokenTypes::lessThanOrEqual) {}
	var getWithDoubles(double a, double b) const override                 { return a <= b; }
	var getWithInts(int64 a, int64 b) const override                      { return a <= b; }
	var getWithStrings(const String& a, const String& b) const override   { return a <= b; }
};

struct HiseJavascriptEngine::RootObject::GreaterThanOp : public BinaryOperator
{
	GreaterThanOp(const CodeLocation& l, ExpPtr& a, ExpPtr& b) noexcept : BinaryOperator(l, a, b, TokenTypes::greaterThan) {}
	var getWithDoubles(double a, double b) const override                 { return a > b; }
	var getWithInts(int64 a, int64 b) const override                      { return a > b; }
	var getWithStrings(const String& a, const String& b) const override   { return a > b; }
};

struct HiseJavascriptEngine::RootObject::GreaterThanOrEqualOp : public BinaryOperator
{
	GreaterThanOrEqualOp(const CodeLocation& l, ExpPtr& a, ExpPtr& b) noexcept : BinaryOperator(l, a, b, TokenTypes::greaterThanOrEqual) {}
	var getWithDoubles(double a, double b) const override                 { return a >= b; }
	var getWithInts(int64 a, int64 b) const override                      { return a >= b; }
	var getWithStrings(const String& a, const String& b) const override   { return a >= b; }
};

struct HiseJavascriptEngine::RootObject::AdditionOp : public BinaryOperator
{
	AdditionOp(const CodeLocation& l, ExpPtr& a, ExpPtr& b) noexcept : BinaryOperator(l, a, b, TokenTypes::plus) {}
	var getWithDoubles(double a, double b) const override                 { return a + b; }
	var getWithInts(int64 a, int64 b) const override                      { return a + b; }
	var getWithStrings(const String& a, const String& b) const override   { return a + b; }
};

struct HiseJavascriptEngine::RootObject::SubtractionOp : public BinaryOperator
{
	SubtractionOp(const CodeLocation& l, ExpPtr& a, ExpPtr& b) noexcept : BinaryOperator(l, a, b, TokenTypes::minus) {}
	var getWithDoubles(double a, double b) const override { return a - b; }
	var getWithInts(int64 a, int64 b) const override      { return a - b; }
};

struct HiseJavascriptEngine::RootObject::MultiplyOp : public BinaryOperator
{
	MultiplyOp(const CodeLocation& l, ExpPtr& a, ExpPtr& b) noexcept : BinaryOperator(l, a, b, TokenTypes::times) {}
	var getWithDoubles(double a, double b) const override { return a * b; }
	var getWithInts(int64 a, int64 b) const override      { return a * b; }

	var getWithArrayOrObject(const var& a, const var&b) const override
	{
		if (a.isBuffer())
		{
			VariantBuffer *vba = a.getBuffer();
			jassert(vba != nullptr);

			if (b.isBuffer())
			{
				VariantBuffer *vbb = b.getBuffer();
				jassert(vbb != nullptr);

				if (vbb->buffer.getNumSamples() != vba->buffer.getNumSamples())
				{
					throw String("Buffer size mismatch: " + String(b.getBuffer()->buffer.getNumSamples()) + " vs. " + String(a.getBuffer()->buffer.getNumSamples()));
				}

				*vba *= *vbb;
			}
			else
			{
				*vba *= (float)b;
			}
		}
		else
		{
			return BinaryOperator::getWithArrayOrObject(a, b);
		}

		return a;
	}

};

struct HiseJavascriptEngine::RootObject::DivideOp : public BinaryOperator
{
	DivideOp(const CodeLocation& l, ExpPtr& a, ExpPtr& b) noexcept : BinaryOperator(l, a, b, TokenTypes::divide) {}
	var getWithDoubles(double a, double b) const override  { return b != 0 ? a / b : std::numeric_limits<double>::infinity(); }
	var getWithInts(int64 a, int64 b) const override       { return b != 0 ? var(a / (double)b) : var(std::numeric_limits<double>::infinity()); }
};

struct HiseJavascriptEngine::RootObject::ModuloOp : public BinaryOperator
{
	ModuloOp(const CodeLocation& l, ExpPtr& a, ExpPtr& b) noexcept : BinaryOperator(l, a, b, TokenTypes::modulo) {}
	var getWithInts(int64 a, int64 b) const override   { return b != 0 ? var(a % b) : var(std::numeric_limits<double>::infinity()); }
};

struct HiseJavascriptEngine::RootObject::BitwiseOrOp : public BinaryOperator
{
	BitwiseOrOp(const CodeLocation& l, ExpPtr& a, ExpPtr& b) noexcept : BinaryOperator(l, a, b, TokenTypes::bitwiseOr) {}
	var getWithInts(int64 a, int64 b) const override   { return a | b; }
};

struct HiseJavascriptEngine::RootObject::BitwiseAndOp : public BinaryOperator
{
	BitwiseAndOp(const CodeLocation& l, ExpPtr& a, ExpPtr& b) noexcept : BinaryOperator(l, a, b, TokenTypes::bitwiseAnd) {}
	var getWithInts(int64 a, int64 b) const override   { return a & b; }
};

struct HiseJavascriptEngine::RootObject::BitwiseXorOp : public BinaryOperator
{
	BitwiseXorOp(const CodeLocation& l, ExpPtr& a, ExpPtr& b) noexcept : BinaryOperator(l, a, b, TokenTypes::bitwiseXor) {}
	var getWithInts(int64 a, int64 b) const override   { return a ^ b; }
};

struct HiseJavascriptEngine::RootObject::LeftShiftOp : public BinaryOperator
{
	LeftShiftOp(const CodeLocation& l, ExpPtr& a, ExpPtr& b) noexcept : BinaryOperator(l, a, b, TokenTypes::leftShift) {}
	var getWithInts(int64 a, int64 b) const override   { return ((int)a) << (int)b; }

	var getWithArrayOrObject(const var& a, const var&b) const override
	{
		if (VariantBuffer *buffer = dynamic_cast<VariantBuffer*>(a.getDynamicObject()))
		{
			if (VariantBuffer *otherBuffer = dynamic_cast<VariantBuffer*>(b.getDynamicObject()))
			{
				if (otherBuffer->buffer.getNumSamples() != buffer->buffer.getNumSamples())
				{
					throw String("Buffer size mismatch: " + String(buffer->buffer.getNumSamples()) + " vs. " + String(otherBuffer->buffer.getNumSamples()));
				}

				*buffer << *otherBuffer;
			}
			else
			{
				*buffer << (float)b;
			}
		}

		return a;
	}
};

struct HiseJavascriptEngine::RootObject::RightShiftOp : public BinaryOperator
{
	RightShiftOp(const CodeLocation& l, ExpPtr& a, ExpPtr& b) noexcept : BinaryOperator(l, a, b, TokenTypes::rightShift) {}
	var getWithInts(int64 a, int64 b) const override   { return ((int)a) >> (int)b; }
};

struct HiseJavascriptEngine::RootObject::RightShiftUnsignedOp : public BinaryOperator
{
	RightShiftUnsignedOp(const CodeLocation& l, ExpPtr& a, ExpPtr& b) noexcept : BinaryOperator(l, a, b, TokenTypes::rightShiftUnsigned) {}
	var getWithInts(int64 a, int64 b) const override   { return (int)(((uint32)a) >> (int)b); }
};

struct HiseJavascriptEngine::RootObject::LogicalAndOp : public BinaryOperatorBase
{
	LogicalAndOp(const CodeLocation& l, ExpPtr& a, ExpPtr& b) noexcept : BinaryOperatorBase(l, a, b, TokenTypes::logicalAnd) {}
	var getResult(const Scope& s) const override       { return lhs->getResult(s) && rhs->getResult(s); }
};

struct HiseJavascriptEngine::RootObject::LogicalOrOp : public BinaryOperatorBase
{
	LogicalOrOp(const CodeLocation& l, ExpPtr& a, ExpPtr& b) noexcept : BinaryOperatorBase(l, a, b, TokenTypes::logicalOr) {}
	var getResult(const Scope& s) const override       { return lhs->getResult(s) || rhs->getResult(s); }
};

struct HiseJavascriptEngine::RootObject::TypeEqualsOp : public BinaryOperatorBase
{
	TypeEqualsOp(const CodeLocation& l, ExpPtr& a, ExpPtr& b) noexcept : BinaryOperatorBase(l, a, b, TokenTypes::typeEquals) {}
	var getResult(const Scope& s) const override       { return areTypeEqual(lhs->getResult(s), rhs->getResult(s)); }
};

struct HiseJavascriptEngine::RootObject::TypeNotEqualsOp : public BinaryOperatorBase
{
	TypeNotEqualsOp(const CodeLocation& l, ExpPtr& a, ExpPtr& b) noexcept : BinaryOperatorBase(l, a, b, TokenTypes::typeNotEquals) {}
	var getResult(const Scope& s) const override       { return !areTypeEqual(lhs->getResult(s), rhs->getResult(s)); }
};

struct HiseJavascriptEngine::RootObject::ConditionalOp : public Expression
{
	ConditionalOp(const CodeLocation& l) noexcept : Expression(l) {}

	var getResult(const Scope& s) const override              { return (condition->getResult(s) ? trueBranch : falseBranch)->getResult(s); }
	void assign(const Scope& s, const var& v) const override  { (condition->getResult(s) ? trueBranch : falseBranch)->assign(s, v); }

	ExpPtr condition, trueBranch, falseBranch;
};



struct HiseJavascriptEngine::RootObject::Assignment : public Expression
{
	Assignment(const CodeLocation& l, ExpPtr& dest, ExpPtr& source) noexcept : Expression(l), target(dest), newValue(source) {}

	var getResult(const Scope& s) const override
	{
		var value(newValue->getResult(s));
		target->assign(s, value);
		return value;
	}

	ExpPtr target, newValue;
};

struct HiseJavascriptEngine::RootObject::RegisterAssignment : public Expression
{
	RegisterAssignment(const CodeLocation &l, int registerId, ExpPtr source_) noexcept: Expression(l), registerIndex(registerId), source(source_) {}

	var getResult(const Scope &s) const override
	{
		var value(source->getResult(s));

		s.root->varRegister.setRegister(registerIndex, value);
		return value;

	}

	int registerIndex;

	ExpPtr source;
};

struct HiseJavascriptEngine::RootObject::SelfAssignment : public Expression
{
	SelfAssignment(const CodeLocation& l, Expression* dest, Expression* source) noexcept
		: Expression(l), target(dest), newValue(source) {}

	var getResult(const Scope& s) const override
	{
		var value(newValue->getResult(s));
		target->assign(s, value);
		return value;
	}

	Expression* target; // Careful! this pointer aliases a sub-term of newValue!
	ExpPtr newValue;
	TokenType op;
};

struct HiseJavascriptEngine::RootObject::PostAssignment : public SelfAssignment
{
	PostAssignment(const CodeLocation& l, Expression* dest, Expression* source) noexcept : SelfAssignment(l, dest, source) {}

	var getResult(const Scope& s) const override
	{
		var oldValue(target->getResult(s));
		target->assign(s, newValue->getResult(s));
		return oldValue;
	}
};

struct HiseJavascriptEngine::RootObject::ApiConstant : public Expression
{
	ApiConstant(const CodeLocation& l) noexcept : Expression(l) {}
	var getResult(const Scope&) const override   { return value; }

	var value;
};

struct HiseJavascriptEngine::RootObject::ApiCall : public Expression
{
	ApiCall(const CodeLocation &l, ApiObject2 *apiClass_, int expectedArguments_, int functionIndex) noexcept:
	Expression(l),
		expectedNumArguments(expectedArguments_),
		functionIndex(functionIndex),
		apiClass(apiClass_)
	{
		for (int i = 0; i < 4; i++)
		{
			results[i] = var::undefined();
			argumentList[i] = nullptr;
		}
	};

	var getResult(const Scope& s) const override
	{
		for (int i = 0; i < expectedNumArguments; i++)
		{
			results[i] = argumentList[i]->getResult(s);
		}

		CHECK_CONDITION(apiClass != nullptr, "API class does not exist");

		return apiClass->callFunction(functionIndex, results, expectedNumArguments);
	}

	const int expectedNumArguments;

	ExpPtr argumentList[4];
	const int functionIndex;
	mutable var results[4];

	const ReferenceCountedObjectPtr<ApiObject2> apiClass;
};

struct HiseJavascriptEngine::RootObject::FunctionCall : public Expression
{
	FunctionCall(const CodeLocation& l) noexcept : Expression(l) {}

	var getResult(const Scope& s) const override
	{
		if (DotOperator* dot = dynamic_cast<DotOperator*> (object.get()))
		{
			var thisObject(dot->parent->getResult(s));
			return invokeFunction(s, s.findFunctionCall(location, thisObject, dot->child), thisObject);
		}

		var function(object->getResult(s));
		return invokeFunction(s, function, var(s.scope));
	}

	var invokeFunction(const Scope& s, const var& function, const var& thisObject) const;

	ExpPtr object;
	OwnedArray<Expression> arguments;
};

struct HiseJavascriptEngine::RootObject::NewOperator : public FunctionCall
{
	NewOperator(const CodeLocation& l) noexcept : FunctionCall(l) {}

	var getResult(const Scope& s) const override
	{
		var classOrFunc = object->getResult(s);

		const bool isFunc = isFunction(classOrFunc);
		if (!(isFunc || classOrFunc.getDynamicObject() != nullptr))
			return var::undefined();

		DynamicObject::Ptr newObject(new DynamicObject());

		if (isFunc)
			invokeFunction(s, classOrFunc, newObject.get());
		else
			newObject->setProperty(getPrototypeIdentifier(), classOrFunc);

		return newObject.get();
	}
};

struct HiseJavascriptEngine::RootObject::ObjectDeclaration : public Expression
{
	ObjectDeclaration(const CodeLocation& l) noexcept : Expression(l) {}

	var getResult(const Scope& s) const override
	{
		DynamicObject::Ptr newObject(new DynamicObject());

		for (int i = 0; i < names.size(); ++i)
			newObject->setProperty(names.getUnchecked(i), initialisers.getUnchecked(i)->getResult(s));

		return newObject.get();
	}

	Array<Identifier> names;
	OwnedArray<Expression> initialisers;
};

struct HiseJavascriptEngine::RootObject::ArrayDeclaration : public Expression
{
	ArrayDeclaration(const CodeLocation& l) noexcept : Expression(l) {}

	var getResult(const Scope& s) const override
	{
		Array<var> a;

		for (int i = 0; i < values.size(); ++i)
			a.add(values.getUnchecked(i)->getResult(s));

		return a;
	}

	OwnedArray<Expression> values;
};

//==============================================================================
struct HiseJavascriptEngine::RootObject::FunctionObject : public DynamicObject
{
	FunctionObject() noexcept{}

	FunctionObject(const FunctionObject& other);

	DynamicObject::Ptr clone() override    { return new FunctionObject(*this); }

	void writeAsJSON(OutputStream& out, int /*indentLevel*/, bool /*allOnOneLine*/) override
	{
		out << "function " << functionCode;
	}

	var invoke(const Scope& s, const var::NativeFunctionArgs& args) const
	{
		DynamicObject::Ptr functionRoot(new DynamicObject());

		static const Identifier thisIdent("this");
		functionRoot->setProperty(thisIdent, args.thisObject);

		for (int i = 0; i < parameters.size(); ++i)
			functionRoot->setProperty(parameters.getReference(i),
			i < args.numArguments ? args.arguments[i] : var::undefined());

		var result;
		body->perform(Scope(&s, s.root, functionRoot), &result);
		return result;
	}

	var invokeWithoutAllocation(const Scope &s, const var::NativeFunctionArgs &args, DynamicObject *scope) const
	{
		var result;

		for (int i = 0; i < parameters.size(); i++)
		{
			scope->setProperty(parameters.getReference(i),
				i < args.numArguments ? args.arguments[i] : var::undefined());
		}

		body->perform(Scope(&s, s.root, scope), &result);

		return result;
	}

	String functionCode;
	Array<Identifier> parameters;
	ScopedPointer<Statement> body;

	DynamicObject::Ptr unneededScope;
};


var HiseJavascriptEngine::RootObject::FunctionCall::invokeFunction(const Scope& s, const var& function, const var& thisObject) const
{
	s.checkTimeOut(location);

#if 0

	Array<var> argVars;
	for (int i = 0; i < arguments.size(); ++i)
		argVars.add(arguments.getUnchecked(i)->getResult(s));

	const var::NativeFunctionArgs args(thisObject, argVars.begin(), argVars.size());

#else

	var argVars[16];

	const int numArgs = jmin<int>(16, arguments.size());

	for (int i = 0; i < numArgs; i++)
	{
		argVars[i] = arguments.getUnchecked(i)->getResult(s);
	}

	const var::NativeFunctionArgs args(thisObject, argVars, numArgs);

#endif



	if (var::NativeFunction nativeFunction = function.getNativeFunction())
		return nativeFunction(args);

	if (FunctionObject* fo = dynamic_cast<FunctionObject*> (function.getObject()))
		return fo->invoke(s, args);

	if (DotOperator* dot = dynamic_cast<DotOperator*> (object.get()))
		if (DynamicObject* o = thisObject.getDynamicObject())
			if (o->hasMethod(dot->child)) // allow an overridden DynamicObject::invokeMethod to accept a method call.
				return o->invokeMethod(dot->child, args);

	location.throwError("This expression is not a function!"); return var();
}


String HiseJavascriptEngine::RootObject::getTokenName(TokenType t)
{
	return t[0] == '$' ? String(t + 1) : ("'" + String(t) + "'");
}

bool HiseJavascriptEngine::RootObject::isFunction(const var& v) noexcept
{
	return dynamic_cast<FunctionObject*> (v.getObject()) != nullptr;
}

bool HiseJavascriptEngine::RootObject::isNumeric(const var& v) noexcept
{
	return v.isInt() || v.isDouble() || v.isInt64() || v.isBool();
}

bool HiseJavascriptEngine::RootObject::isNumericOrUndefined(const var& v) noexcept
{
	return isNumeric(v) || v.isUndefined();
}

int64 HiseJavascriptEngine::RootObject::getOctalValue(const String& s)
{
	BigInteger b; b.parseString(s, 8); return b.toInt64();
}

Identifier HiseJavascriptEngine::RootObject::getPrototypeIdentifier()
{
	static const Identifier i("prototype"); return i;
}

var* HiseJavascriptEngine::RootObject::getPropertyPointer(DynamicObject* o, const Identifier& i) noexcept
{
	return o->getProperties().getVarPointer(i);
}

bool HiseJavascriptEngine::RootObject::Scope::findAndInvokeMethod(const Identifier& function, const var::NativeFunctionArgs& args, var& result) const
{
	DynamicObject* target = args.thisObject.getDynamicObject();

	if (target == nullptr || target == scope)
	{
		if (const var* m = getPropertyPointer(scope, function))
		{
			if (FunctionObject* fo = dynamic_cast<FunctionObject*> (m->getObject()))
			{
				result = fo->invoke(*this, args);
				return true;
			}
		}
	}

	const NamedValueSet& props = scope->getProperties();

	for (int i = 0; i < props.size(); ++i)
		if (DynamicObject* o = props.getValueAt(i).getDynamicObject())
			if (Scope(this, root, o).findAndInvokeMethod(function, args, result))
				return true;

	return false;
}

bool HiseJavascriptEngine::RootObject::Scope::invokeMidiCallback(const Identifier &callbackName, const var::NativeFunctionArgs &args, var &result, DynamicObject*functionScope) const
{
	if (const var* m = getPropertyPointer(scope, callbackName))
	{
		if (FunctionObject* fo = dynamic_cast<FunctionObject*> (m->getObject()))
		{
			result = fo->invokeWithoutAllocation(*this, args, functionScope);
			return true;
		}
	}

	return false;
}
