namespace hise { using namespace juce;

struct HiseJavascriptEngine::RootObject::BinaryOperatorBase : public Expression
{
	BinaryOperatorBase(const CodeLocation& l, ExpPtr& a, ExpPtr& b, TokenType op) noexcept
	: Expression(l), lhs(a), rhs(b), operation(op) {}

	bool isConstant() const override
	{
		return lhs->isConstant() && rhs->isConstant();
	}

	Statement* getChildStatement(int index) override 
	{
		if (index == 0) return lhs.get();
		if (index == 1) return rhs.get();
		return nullptr;
	};
	
	bool replaceChildStatement(Ptr& newS, Statement* sToReplace) override
	{
		return swapIf(newS, sToReplace, lhs) ||
			   swapIf(newS, sToReplace, rhs);
	}

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

		if (isNumericOrUndefined(a) && b.isBuffer())
			return getWithArrayOrObject(a, b);

		return getWithStrings(a.toString(), b.toString());
	}

	

	var throwError(const char* typeName) const
	{
		location.throwError(getTokenName(operation) + " is not allowed on the " + typeName + " type"); 
		
		return var();
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

#if JUCE_MSVC
#pragma warning (push)
#pragma warning (disable : 4702)
#endif

struct HiseJavascriptEngine::RootObject::AdditionOp : public BinaryOperator
{
	AdditionOp(const CodeLocation& l, ExpPtr& a, ExpPtr& b) noexcept : BinaryOperator(l, a, b, TokenTypes::plus) {}
	var getWithDoubles(double a, double b) const override                 { return a + b; }
	var getWithInts(int64 a, int64 b) const override                      { return a + b; }
	var getWithStrings(const String& a, const String& b) const override   
	{ 
		WARN_IF_AUDIO_THREAD(true, IllegalAudioThreadOps::StringCreation);
		return a + b; 
	}

	var getWithArrayOrObject(const var &a, const var& b) const override
	{
		if (a.isBuffer())
		{
			VariantBuffer* vba = a.getBuffer();

			if (b.isBuffer())
			{
				VariantBuffer* vbb = b.getBuffer();

				if (vbb->buffer.getNumSamples() != vba->buffer.getNumSamples())
				{
					location.throwError("Buffer size mismatch: " + String(b.getBuffer()->buffer.getNumSamples()) + " vs. " + String(a.getBuffer()->buffer.getNumSamples()));
				}

				*vba += *vbb;
			}
			else
			{
				*vba += (float)b;
			}

			return a;
		}
		else
		{
			return BinaryOperator::getWithArrayOrObject(a, b);
		}
	}

};

struct HiseJavascriptEngine::RootObject::SubtractionOp : public BinaryOperator
{
	SubtractionOp(const CodeLocation& l, ExpPtr& a, ExpPtr& b) noexcept : BinaryOperator(l, a, b, TokenTypes::minus) {}
	var getWithDoubles(double a, double b) const override { return a - b; }
	var getWithInts(int64 a, int64 b) const override      { return a - b; }

	var getWithArrayOrObject(const var &a, const var& b) const override
	{
		if (a.isBuffer())
		{
			VariantBuffer* vba = a.getBuffer();

			if (b.isBuffer())
			{
				VariantBuffer* vbb = b.getBuffer();

				if (vbb->buffer.getNumSamples() != vba->buffer.getNumSamples())
				{
					location.throwError("Buffer size mismatch: " + String(b.getBuffer()->buffer.getNumSamples()) + " vs. " + String(a.getBuffer()->buffer.getNumSamples()));
				}

				*vba -= *vbb;
			}
			else
			{
				*vba -= (float)b;
			}

			return a;
		}
		else
		{
			return BinaryOperator::getWithArrayOrObject(a, b);
		}
	}
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
					location.throwError("Buffer size mismatch: " + String(b.getBuffer()->buffer.getNumSamples()) + " vs. " + String(a.getBuffer()->buffer.getNumSamples()));
				}

				*vba *= *vbb;
			}
			else
			{
				*vba *= (float)b;
			}

			return a;
		}
		else
		{
			return BinaryOperator::getWithArrayOrObject(a, b);
		}
	}

};

#if JUCE_MSVC
#pragma warning (pop)
#endif


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
    var getWithDoubles(double a, double b) const override
    {
        return b != 0.0 ? var(roundToInt(a) % roundToInt(b)) : var(std::numeric_limits<double>::infinity());
    }
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
		if (a.isBuffer())
		{
			if (isNumericOrUndefined(b))
			{
				*a.getBuffer() << (float)b;
			}
			else if (b.isBuffer())
			{
				*a.getBuffer() << *b.getBuffer();
			}

			return a;
		}
		else if (DspInstance* instance = dynamic_cast<DspInstance*>(a.getObject()))
		{
			if (b.isBuffer() || b.isArray())
			{
				*instance >> b;
			}

			return a;
		}
		
		return a;
	}
};

struct HiseJavascriptEngine::RootObject::RightShiftOp : public BinaryOperator
{
	RightShiftOp(const CodeLocation& l, ExpPtr& a, ExpPtr& b) noexcept : BinaryOperator(l, a, b, TokenTypes::rightShift) {}
	var getWithInts(int64 a, int64 b) const override   { return ((int)a) >> (int)b; }
	
	var getWithArrayOrObject(const var& a, const var&b) const override
	{
		if (isNumericOrUndefined(a))
		{
			if (b.isBuffer())
			{
				(float)a >> *b.getBuffer();
			}
		}
		else if (a.isBuffer())
		{
			if (b.isBuffer())
			{
				*a.getBuffer() >> *b.getBuffer();
			}
		}
		else if (a.isObject())
		{
			if (DspInstance* instance = dynamic_cast<DspInstance*>(a.getObject()))
			{
				if (b.isBuffer() || b.isArray())
				{
					*instance >> b;
				}
			}
		}

		return a;
	}

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

	bool isConstant() const override
	{
		return condition->isConstant() && trueBranch->isConstant() && falseBranch->isConstant();
	}

	Statement* getChildStatement(int index) override 
	{
		if (index == 0) return condition;
		if (index == 1) return trueBranch;
		if (index == 2) return falseBranch;
		return nullptr;
	};
	
	bool replaceChildStatement(Ptr& n, Statement* sToReplace) override
	{
		return swapIf(n, sToReplace, condition) ||
			   swapIf(n, sToReplace, trueBranch) ||
			   swapIf(n, sToReplace, falseBranch);
	}



	ExpPtr condition, trueBranch, falseBranch;
};

} // namespace hise