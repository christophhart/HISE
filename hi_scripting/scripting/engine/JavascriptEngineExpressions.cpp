namespace hise { using namespace juce;

struct HiseJavascriptEngine::RootObject::LiteralValue : public Expression
{
	LiteralValue(const CodeLocation& l, const var& v) noexcept : Expression(l), value(v) {}
	var getResult(const Scope&) const override   { return value; }
	var value;
};

struct HiseJavascriptEngine::RootObject::UnqualifiedName : public Expression
{
	UnqualifiedName(const CodeLocation& l, const Identifier& n, bool isFunction) noexcept : Expression(l), name(n), allowUnqualifiedDefinition(isFunction) {}

	var getResult(const Scope& s) const override  { return s.findSymbolInParentScopes(name); }

	void assign(const Scope& s, const var& newValue) const override
	{
		const Scope* currentScope = &s;
		var* v = getPropertyPointer(currentScope->scope.get(), name);

		while (v == nullptr && currentScope->parent != nullptr)
		{
			currentScope = currentScope->parent;
			v = getPropertyPointer(currentScope->scope.get(), name);
		}

		if (v == nullptr)
			v = getPropertyPointer(currentScope->root.get(), name);

		if (v != nullptr)
			*v = newValue;
		else
		{
			if (allowUnqualifiedDefinition)
			{
				currentScope->root->setProperty(name, newValue);
			}
			else
			{
				location.throwError("Unqualified assignments are not supported anymore. Use `var` or `const var` or `reg` for definitions");
			}

		}
			
	}

	bool allowUnqualifiedDefinition = false;

	JavascriptNamespace* ns = nullptr;
	Identifier name;
};



struct HiseJavascriptEngine::RootObject::ConstReference : public Expression
{
	ConstReference(const CodeLocation& l, JavascriptNamespace* ns_, int index_ ) noexcept : Expression(l), ns(ns_), index(index_) {}

	var getResult(const Scope& /*s*/) const override
	{
		return ns->constObjects.getValueAt(index);
	}

	void assign(const Scope& /*s*/, const var& /*newValue*/) const override
	{
		location.throwError("Can't assign to this expression!");
	}

	JavascriptNamespace* ns;
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

			if(cachedIndex != -1)
				return instance->getAssignedValue(cachedIndex);
			else
			{
				auto idx = index->getResult(s);
				return instance->getAssignedValue(idx);
			}
		}
		else if (const Array<var>* array = result.getArray())
			return (*array)[static_cast<int> (index->getResult(s))];

        else if (const DynamicObject* obj = result.getDynamicObject())
        {
            const String name = index->getResult(s).toString();
            
            if(name.isNotEmpty())
            {
                return obj->getProperty(Identifier(name));
            }
            
            
        }
        
		return var::undefined();
	}

	void assign(const Scope& s, const var& newValue) const override
	{
		var result = object->getResult(s);

		if (VariantBuffer *b = result.getBuffer())
		{
			const int i = index->getResult(s);

			float v = (float)newValue;

			(*b)[i] = FloatSanitizers::sanitizeFloatNumber(v);
			return;
		}
		else if (Array<var>* ar = result.getArray())
		{
			const int i = index->getResult(s);

			
			
			WARN_IF_AUDIO_THREAD(i >= ar->getNumAllocated(), ScriptAudioThreadGuard::ArrayResizing);

			while (ar->size() < i)
				ar->add(var::undefined());

			ar->set(i, newValue);
			return;
		}
		else if (AssignableObject * instance = dynamic_cast<AssignableObject*>(result.getObject()))
		{
			cacheIndex(instance, s);

			instance->assign(cachedIndex, newValue);
			
			return;
		}
        else if (DynamicObject* obj = result.getDynamicObject())
        {
			WARN_IF_AUDIO_THREAD(true, ScriptAudioThreadGuard::DynamicObjectAccess);

            const String name = index->getResult(s).toString();
            return obj->setProperty(Identifier(name), newValue);
        }

		Expression::assign(s, newValue);
	}

	void cacheIndex(AssignableObject *instance, const Scope &s) const;

	ExpPtr object, index;

	mutable int cachedIndex = -1;
};

#define DECLARE_ID(x) const juce::Identifier x(#x);

namespace DotIds
{
DECLARE_ID(length);
}

#undef DECLARE_ID

struct HiseJavascriptEngine::RootObject::DotOperator : public Expression
{
	DotOperator(const CodeLocation& l, ExpPtr& p, const Identifier& c) noexcept : Expression(l), parent(p), child(c) {}

	var getResult(const Scope& s) const override
	{
		var p(parent->getResult(s));

		if (child == DotIds::length)
		{
			if (Array<var>* array = p.getArray())   return array->size();
			if (p.isBuffer()) return p.getBuffer()->size;

			if (p.isString())                       return p.toString().length();
		}

		if (DynamicObject* o = p.getDynamicObject())
		{
			if (const var* v = getPropertyPointer(o, child))
				return *v;

			return o->getProperty(child);
		}
			
		if (ConstScriptingObject* o = dynamic_cast<ConstScriptingObject*>(p.getObject()))
		{
			const int constantIndex = o->getConstantIndex(child);
			if (constantIndex != -1)
			{
				return o->getConstantValue(constantIndex);
			}
		}
        
        if(auto lb = dynamic_cast<fixobj::ObjectReference*>(p.getObject()))
        {
            if(auto member = (*lb)[child])
                return (var)*member;
            else
                location.throwError("can't find property " + child.toString());
        }

		return var::undefined();
	}

	void assign(const Scope& s, const var& newValue) const override
	{
        auto v = parent->getResult(s);
        
		if (DynamicObject* o = v.getDynamicObject())
		{
			WARN_IF_AUDIO_THREAD(!o->hasProperty(child), ScriptAudioThreadGuard::ObjectResizing);

			o->setProperty(child, newValue);
		}
		else if (auto mo = dynamic_cast<fixobj::ObjectReference::MemberReference*>(v.getObject()))
        {
            *mo = newValue;
        }
        else if(auto lb = dynamic_cast<fixobj::ObjectReference*>(v.getObject()))
        {
            if(auto member = (*lb)[child])
                *member = newValue;
            else
                location.throwError("Can't find property " + child.toString());
        }
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

	var getResult(const Scope& s) const override;

	var invokeFunction(const Scope& s, const var& function, const var& thisObject) const;

	ExpPtr object;
	OwnedArray<Expression> arguments;

	mutable bool initialised = false;
	mutable bool isConstObjectApiFunction = false;
	mutable bool parentIsConstReference = false;
	mutable ConstScriptingObject* constObject = nullptr;
	mutable int numArgs = -1;
	mutable int functionIndex = -1;
};

struct HiseJavascriptEngine::RootObject::NewOperator : public FunctionCall
{
	NewOperator(const CodeLocation& l) noexcept : FunctionCall(l) {}

	var getResult(const Scope& s) const override
	{
		location.throwError("the new operator is not supported anymore");

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
		WARN_IF_AUDIO_THREAD(true, ScriptAudioThreadGuard::ObjectCreation);

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
		WARN_IF_AUDIO_THREAD(!values.isEmpty(), ScriptAudioThreadGuard::ArrayCreation);

		Array<var> a;

		for (int i = 0; i < values.size(); ++i)
			a.add(values.getUnchecked(i)->getResult(s));

		return a;
	}

	OwnedArray<Expression> values;
};

//==============================================================================
struct HiseJavascriptEngine::RootObject::FunctionObject : public DynamicObject,
														  public DebugableObject,
														  public CyclicReferenceCheckBase
{
	FunctionObject() noexcept{}

	FunctionObject(const FunctionObject& other);

	DynamicObject::Ptr clone() override    { return new FunctionObject(*this); }

	void writeAsJSON(OutputStream& out, int /*indentLevel*/, bool /*allOnOneLine*/, int /*maximumDecimalPlaces*/) override
	{
		out << "function " << functionCode;
	}

	Identifier getObjectName() const override { RETURN_STATIC_IDENTIFIER("Function"); }

	bool updateCyclicReferenceList(ThreadData& data, const Identifier& id) override;

	void prepareCycleReferenceCheck() override;

	var invoke(const Scope& s, const var::NativeFunctionArgs& args) const
	{
		WARN_IF_AUDIO_THREAD(true, ScriptAudioThreadGuard::FunctionCall);

		DynamicObject::Ptr functionRoot(new DynamicObject());

		static const Identifier thisIdent("this");
		functionRoot->setProperty(thisIdent, args.thisObject);

		for (int i = 0; i < parameters.size(); ++i)
			functionRoot->setProperty(parameters.getReference(i),
			i < args.numArguments ? args.arguments[i] : var::undefined());

		var result;
		body->perform(Scope(&s, s.root.get(), functionRoot.get()), &result);

#if ENABLE_SCRIPTING_SAFE_CHECKS
		if(enableCycleCheck)
			lastScopeForCycleCheck = var(functionRoot.get());
#endif

#if ENABLE_SCRIPTING_BREAKPOINTS
		lastScope = functionRoot;
#endif

		functionRoot->removeProperty("this");

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

		body->perform(Scope(&s, s.root.get(), scope), &result);

		return result;
	}

	void createFunctionDefinition(const Identifier &functionName)
	{
		functionDef = functionName.toString();
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

	int getNumChildElements() const override
	{
		if (lastScope != nullptr)
			return lastScope->getProperties().size() - 1;

		return 0;
	}

	DebugInformationBase* getChildElement(int index) override
	{
		if (lastScope != nullptr)
		{
			WeakReference<FunctionObject> safeThis(this);

			DynamicObject::Ptr l = lastScope;

			auto vf = [safeThis, index]()
			{
				if (safeThis == nullptr)
					return var();

				if (auto l = safeThis->lastScope)
				{
					if (auto v = l->getProperties().getVarPointerAt(index + 1))
						return *v;
				}

				return var();
			};

			auto& prop = l->getProperties();

			

			if (isPositiveAndBelow(index + 1, prop.size()))
			{
				String mid;
				mid << "%PARENT%" << "." << l->getProperties().getName(index + 1);
				return new LambdaValueInformation(vf, mid, {}, DebugInformation::Type::ExternalFunction, getLocation());
			}
		}

		return nullptr;
	}

	AttributedString getDescription() const override
	{
		return DebugableObject::Helpers::getFunctionDoc(commentDoc, parameters);
	}

	Location getLocation() const override { return location; }

	bool enableCycleCheck = false;

	Identifier name;

	Location location;

	String functionCode;
	Array<Identifier> parameters;
	ScopedPointer<Statement> body;

	String commentDoc;
	String functionDef;

	mutable var lastScopeForCycleCheck;

	DynamicObject::Ptr unneededScope;

	mutable DynamicObject::Ptr lastScope;

	JUCE_DECLARE_WEAK_REFERENCEABLE(FunctionObject);
};


var HiseJavascriptEngine::RootObject::FunctionCall::invokeFunction(const Scope& s, const var& function, const var& thisObject) const
{
	s.checkTimeOut(location);

	var argVars[16];

	const int thisNumArgs = jmin<int>(16, arguments.size());

	for (int i = 0; i < thisNumArgs; i++)
	{
		argVars[i] = arguments.getUnchecked(i)->getResult(s);
	}

	const var::NativeFunctionArgs args(thisObject, argVars, thisNumArgs);


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
	return isNumeric(v) || v.isUndefined() || v.isVoid();
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

	if (target == nullptr || target == scope.get())
	{
		if (const var* m = getPropertyPointer(scope.get(), function))
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
			if (Scope(this, root.get(), o).findAndInvokeMethod(function, args, result))
				return true;

	return false;
}

bool HiseJavascriptEngine::RootObject::Scope::invokeMidiCallback(const Identifier &callbackName, const var::NativeFunctionArgs &args, var &result, DynamicObject*functionScope) const
{
	if (const var* m = getPropertyPointer(scope.get(), callbackName))
	{
		if (FunctionObject* fo = dynamic_cast<FunctionObject*> (m->getObject()))
		{
			result = fo->invokeWithoutAllocation(*this, args, functionScope);
			return true;
		}
	}

	return false;
}

} // namespace hise
