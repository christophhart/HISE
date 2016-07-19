

struct HiseJavascriptEngine::RootObject::LiteralValue : public Expression
{
	LiteralValue(const CodeLocation& l, const var& v) noexcept : Expression(l), value(v) {}
	var getResult(const Scope&) const override   { return value; }
	var value;
};

struct HiseJavascriptEngine::RootObject::UnqualifiedName : public Expression
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



struct HiseJavascriptEngine::RootObject::ConstReference : public Expression
{
	ConstReference(const CodeLocation& l, int i) noexcept : Expression(l), index(i) {}

	var getResult(const Scope& s) const override
	{
		return s.root->hiseSpecialData.constObjects.getValueAt(index);
	}

	void assign(const Scope& /*s*/, const var& /*newValue*/) const override
	{
		location.throwError("Can't assign to this expression!");
	}

	int index;
};



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
		else if (AssignableObject * instance = dynamic_cast<AssignableObject*>(result.getObject()))
		{
			cacheIndex(instance, s);

			return instance->getAssignedValue(cachedIndex);
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
		else if (AssignableObject * instance = dynamic_cast<AssignableObject*>(result.getObject()))
		{
			cacheIndex(instance, s);

			instance->assign(cachedIndex, (float)newValue);
			
			return;
		}


		Expression::assign(s, newValue);
	}

	void cacheIndex(AssignableObject *instance, const Scope &s) const
	{
		if (cachedIndex == -1)			
		{
			if (dynamic_cast<LiteralValue*>(index.get()) != nullptr ||
				dynamic_cast<ConstReference*>(index.get()) != nullptr)
			{
				const var i = index->getResult(s);
				cachedIndex = instance->getCachedIndex(i);

				if (cachedIndex == -1) location.throwError("Property " + i.toString() + " not found");
			}
			else location.throwError("[]-access must be used with a literal or constant");
		}
	}

	ExpPtr object, index;

	mutable int cachedIndex = -1;
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


struct HiseJavascriptEngine::RootObject::FunctionCall : public Expression
{
	FunctionCall(const CodeLocation& l) noexcept : Expression(l) {}

	var getResult(const Scope& s) const override
	{
		if (!initialised)
		{
			initialised = true;

			if (DotOperator* dot = dynamic_cast<DotOperator*> (object.get()))
			{
				parentIsConstReference = dynamic_cast<ConstReference*>(dot->parent.get());

				if (parentIsConstReference)
				{
					constObject = dynamic_cast<ConstObjectWithApiCalls*>(dot->parent->getResult(s).getObject());

					if (constObject != nullptr)
					{
						constObject->getIndexAndNumArgsForFunction(dot->child, functionIndex, numArgs);
						isConstObjectApiFunction = true;

						CHECK_CONDITION(functionIndex != -1, "function not found");
						CHECK_CONDITION(numArgs == arguments.size(), "argument amount mismatch: " + String(arguments.size()) + ", Expected: " + String(numArgs));
					}
				}
			}
		}

		if (isConstObjectApiFunction)
		{
			var parameters[4];

			for (int i = 0; i < arguments.size(); i++)
				parameters[i] = arguments[i]->getResult(s);

			return constObject->callFunction(functionIndex, parameters, numArgs);
		}

		if (DotOperator* dot = dynamic_cast<DotOperator*> (object.get()))
		{

			var thisObject(dot->parent->getResult(s));

			if (ConstObjectWithApiCalls* c = dynamic_cast<ConstObjectWithApiCalls*>(thisObject.getObject()))
			{
				c->getIndexAndNumArgsForFunction(dot->child, functionIndex, numArgs);

				CHECK_CONDITION(functionIndex != -1, "function not found");
				CHECK_CONDITION(numArgs == arguments.size(), "argument amount mismatch: " + String(arguments.size()) + ", Expected: " + String(numArgs));

				var parameters[4];

				for (int i = 0; i < arguments.size(); i++)
					parameters[i] = arguments[i]->getResult(s);

				return c->callFunction(functionIndex, parameters, numArgs);
			}

			return invokeFunction(s, s.findFunctionCall(location, thisObject, dot->child), thisObject);
		}

		var function(object->getResult(s));
		return invokeFunction(s, function, var(s.scope));
	}

	var invokeFunction(const Scope& s, const var& function, const var& thisObject) const;

	ExpPtr object;
	OwnedArray<Expression> arguments;

	mutable bool initialised = false;
	mutable bool isConstObjectApiFunction = false;
	mutable bool parentIsConstReference = false;
	mutable ConstObjectWithApiCalls* constObject = nullptr;
	mutable int numArgs = -1;
	mutable int functionIndex = -1;
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
struct HiseJavascriptEngine::RootObject::FunctionObject : public DynamicObject,
														  public DebugableObject
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

	void createFunctionDefinition(const Identifier &name)
	{
		functionDef = name.toString();
		functionDef << "(";

		for (int i = 0; i < parameters.size(); i++)
		{
			functionDef << parameters[i].toString();
			if (i != parameters.size() - 1) functionDef << ", ";
		}

		functionDef << ")";
	}

	
	String getDebugValue() const override { return ""; }

	String getDebugDataType() const override { return "function"; }
	
	String getDebugName() const override
	{
		return functionDef;
	}

	AttributedString getDescription() const override
	{
		return DebugableObject::Helpers::getFunctionDoc(commentDoc, parameters);
	}

	Identifier name;

	String functionCode;
	Array<Identifier> parameters;
	ScopedPointer<Statement> body;

	String commentDoc;
	String functionDef;

	DynamicObject::Ptr unneededScope;
};


var HiseJavascriptEngine::RootObject::FunctionCall::invokeFunction(const Scope& s, const var& function, const var& thisObject) const
{
	s.checkTimeOut(location);

	var argVars[16];

	const int numArgs = jmin<int>(16, arguments.size());

	for (int i = 0; i < numArgs; i++)
	{
		argVars[i] = arguments.getUnchecked(i)->getResult(s);
	}

	const var::NativeFunctionArgs args(thisObject, argVars, numArgs);


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
