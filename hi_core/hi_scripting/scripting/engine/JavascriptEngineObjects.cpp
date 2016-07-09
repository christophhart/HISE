
//==============================================================================
struct HiseJavascriptEngine::RootObject::ObjectClass : public DynamicObject
{
	ObjectClass()
	{
		setMethod("dump", dump);
		setMethod("clone", cloneFn);
	}

	static Identifier getClassName()   { static const Identifier i("Object"); return i; }
	static var dump(Args a)          { DBG(JSON::toString(a.thisObject)); ignoreUnused(a); return var::undefined(); }
	static var cloneFn(Args a)        { return a.thisObject.clone(); }
};

//==============================================================================
struct HiseJavascriptEngine::RootObject::ArrayClass : public DynamicObject
{
	ArrayClass()
	{
		setMethod("contains", contains);
		setMethod("remove", remove);
		setMethod("join", join);
		setMethod("push", push);
        setMethod("sort", sort);
	}

	static Identifier getClassName()   { static const Identifier i("Array"); return i; }

	static var contains(Args a)
	{
		if (const Array<var>* array = a.thisObject.getArray())
			return array->contains(get(a, 0));

		return false;
	}

	static var remove(Args a)
	{
		if (Array<var>* array = a.thisObject.getArray())
			array->removeAllInstancesOf(get(a, 0));

		return var::undefined();
	}

	static var join(Args a)
	{
		StringArray strings;

		if (const Array<var>* array = a.thisObject.getArray())
			for (int i = 0; i < array->size(); ++i)
				strings.add(array->getReference(i).toString());

		return strings.joinIntoString(getString(a, 0));
	}

	static var push(Args a)
	{
		if (Array<var>* array = a.thisObject.getArray())
		{
			for (int i = 0; i < a.numArguments; ++i)
				array->add(a.arguments[i]);

			return array->size();
		}

		return var::undefined();
	}
    
    static var sort(Args a)
    {
        if (Array<var>* array = a.thisObject.getArray())
        {
            VariantComparator comparator;
            array->sort(comparator);
        }
        
        return var::undefined();
    }
};

//==============================================================================
struct HiseJavascriptEngine::RootObject::StringClass : public DynamicObject
{
	StringClass()
	{
		setMethod("substring", substring);
		setMethod("indexOf", indexOf);
		setMethod("charAt", charAt);
		setMethod("charCodeAt", charCodeAt);
		setMethod("fromCharCode", fromCharCode);
		setMethod("split", split);
	}

	static Identifier getClassName()  { static const Identifier i("String"); return i; }

	static var fromCharCode(Args a)  { return String::charToString(getInt(a, 0)); }
	static var substring(Args a)     { return a.thisObject.toString().substring(getInt(a, 0), getInt(a, 1)); }
	static var indexOf(Args a)       { return a.thisObject.toString().indexOf(getString(a, 0)); }
	static var charCodeAt(Args a)    { return (int)a.thisObject.toString()[getInt(a, 0)]; }
	static var charAt(Args a)        { int p = getInt(a, 0); return a.thisObject.toString().substring(p, p + 1); }

	static var split(Args a)
	{
		const String str(a.thisObject.toString());
		const String sep(getString(a, 0));
		StringArray strings;

		if (sep.isNotEmpty())
			strings.addTokens(str, sep.substring(0, 1), "");
		else // special-case for empty separator: split all chars separately
			for (String::CharPointerType pos = str.getCharPointer(); !pos.isEmpty(); ++pos)
				strings.add(String::charToString(*pos));

		var array;
		for (int i = 0; i < strings.size(); ++i)
			array.append(strings[i]);

		return array;
	}
};

//==============================================================================
struct HiseJavascriptEngine::RootObject::MathClass : public DynamicObject
{
	MathClass()
	{
		setMethod("abs", Math_abs);              setMethod("round", Math_round);
		setMethod("random", Math_random);           setMethod("randInt", Math_randInt);
		setMethod("min", Math_min);              setMethod("max", Math_max);
		setMethod("range", Math_range);            setMethod("sign", Math_sign);
		setMethod("toDegrees", Math_toDegrees);        setMethod("toRadians", Math_toRadians);
		setMethod("sin", Math_sin);              setMethod("asin", Math_asin);
		setMethod("sinh", Math_sinh);             setMethod("asinh", Math_asinh);
		setMethod("cos", Math_cos);              setMethod("acos", Math_acos);
		setMethod("cosh", Math_cosh);             setMethod("acosh", Math_acosh);
		setMethod("tan", Math_tan);              setMethod("atan", Math_atan);
		setMethod("tanh", Math_tanh);             setMethod("atanh", Math_atanh);
		setMethod("log", Math_log);              setMethod("log10", Math_log10);
		setMethod("exp", Math_exp);              setMethod("pow", Math_pow);
		setMethod("sqr", Math_sqr);              setMethod("sqrt", Math_sqrt);
		setMethod("ceil", Math_ceil);             setMethod("floor", Math_floor);

		setProperty("PI", double_Pi);
		setProperty("E", exp(1.0));
	}

	static var Math_random(Args)   { return Random::getSystemRandom().nextDouble(); }
	static var Math_randInt(Args a) { return Random::getSystemRandom().nextInt(Range<int>(getInt(a, 0), getInt(a, 1))); }
	static var Math_abs(Args a) { return isInt(a, 0) ? var(std::abs(getInt(a, 0))) : var(std::abs(getDouble(a, 0))); }
	static var Math_round(Args a) { return isInt(a, 0) ? var(roundToInt(getInt(a, 0))) : var(roundToInt(getDouble(a, 0))); }
	static var Math_sign(Args a) { return isInt(a, 0) ? var(sign(getInt(a, 0))) : var(sign(getDouble(a, 0))); }
	static var Math_range(Args a) { return isInt(a, 0) ? var(jlimit(getInt(a, 1), getInt(a, 2), getInt(a, 0))) : var(jlimit(getDouble(a, 1), getDouble(a, 2), getDouble(a, 0))); }
	static var Math_min(Args a) { return (isInt(a, 0) && isInt(a, 1)) ? var(jmin(getInt(a, 0), getInt(a, 1))) : var(jmin(getDouble(a, 0), getDouble(a, 1))); }
	static var Math_max(Args a) { return (isInt(a, 0) && isInt(a, 1)) ? var(jmax(getInt(a, 0), getInt(a, 1))) : var(jmax(getDouble(a, 0), getDouble(a, 1))); }
	static var Math_toDegrees(Args a) { return radiansToDegrees(getDouble(a, 0)); }
	static var Math_toRadians(Args a) { return degreesToRadians(getDouble(a, 0)); }
	static var Math_sin(Args a) { return sin(getDouble(a, 0)); }
	static var Math_asin(Args a) { return asin(getDouble(a, 0)); }
	static var Math_cos(Args a) { return cos(getDouble(a, 0)); }
	static var Math_acos(Args a) { return acos(getDouble(a, 0)); }
	static var Math_sinh(Args a) { return sinh(getDouble(a, 0)); }
	static var Math_asinh(Args a) { return asinh(getDouble(a, 0)); }
	static var Math_cosh(Args a) { return cosh(getDouble(a, 0)); }
	static var Math_acosh(Args a) { return acosh(getDouble(a, 0)); }
	static var Math_tan(Args a) { return tan(getDouble(a, 0)); }
	static var Math_tanh(Args a) { return tanh(getDouble(a, 0)); }
	static var Math_atan(Args a) { return atan(getDouble(a, 0)); }
	static var Math_atanh(Args a) { return atanh(getDouble(a, 0)); }
	static var Math_log(Args a) { return log(getDouble(a, 0)); }
	static var Math_log10(Args a) { return log10(getDouble(a, 0)); }
	static var Math_exp(Args a) { return exp(getDouble(a, 0)); }
	static var Math_pow(Args a) { return pow(getDouble(a, 0), getDouble(a, 1)); }
	static var Math_sqr(Args a) { double x = getDouble(a, 0); return x * x; }
	static var Math_sqrt(Args a) { return std::sqrt(getDouble(a, 0)); }
	static var Math_ceil(Args a) { return std::ceil(getDouble(a, 0)); }
	static var Math_floor(Args a) { return std::floor(getDouble(a, 0)); }

	static Identifier getClassName()   { static const Identifier i("Math"); return i; }
	template <typename Type> static Type sign(Type n) noexcept{ return n > 0 ? (Type)1 : (n < 0 ? (Type)-1 : 0); }
};

//==============================================================================
struct HiseJavascriptEngine::RootObject::JSONClass : public DynamicObject
{
	JSONClass()                        { setMethod("stringify", stringify); }
	static Identifier getClassName()   { static const Identifier i("JSON"); return i; }
	static var stringify(Args a)      { return JSON::toString(get(a, 0)); }
};

//==============================================================================
struct HiseJavascriptEngine::RootObject::IntegerClass : public DynamicObject
{
	IntegerClass()                     { setMethod("parseInt", parseInt); }
	static Identifier getClassName()   { static const Identifier i("Integer"); return i; }

	static var parseInt(Args a)
	{
		const String s(getString(a, 0).trim());

		return s[0] == '0' ? (s[1] == 'x' ? s.substring(2).getHexValue64() : getOctalValue(s))
			: s.getLargeIntValue();
	}
};



//==============================================================================
HiseJavascriptEngine::HiseJavascriptEngine() : maximumExecutionTime(15.0), root(new RootObject()), unneededScope(new DynamicObject())
{
	registerNativeObject(RootObject::ObjectClass::getClassName(), new RootObject::ObjectClass());
	registerNativeObject(RootObject::ArrayClass::getClassName(), new RootObject::ArrayClass());
	registerNativeObject(RootObject::StringClass::getClassName(), new RootObject::StringClass());
	registerNativeObject(RootObject::MathClass::getClassName(), new RootObject::MathClass());
	registerNativeObject(RootObject::JSONClass::getClassName(), new RootObject::JSONClass());
	registerNativeObject(RootObject::IntegerClass::getClassName(), new RootObject::IntegerClass());
}

HiseJavascriptEngine::RootObject::RootObject()
{
	setMethod("exec", exec);
	setMethod("eval", eval);
	setMethod("trace", trace);
	setMethod("charToInt", charToInt);
	setMethod("parseInt", IntegerClass::parseInt);
	setMethod("typeof", typeof_internal);
}

var HiseJavascriptEngine::RootObject::Scope::findFunctionCall(const CodeLocation& location, const var& targetObject, const Identifier& functionName) const
{
	if (DynamicObject* o = targetObject.getDynamicObject())
	{
		if (const var* prop = getPropertyPointer(o, functionName))
			return *prop;

		for (DynamicObject* p = o->getProperty(getPrototypeIdentifier()).getDynamicObject(); p != nullptr;
			p = p->getProperty(getPrototypeIdentifier()).getDynamicObject())
		{
			if (const var* prop = getPropertyPointer(p, functionName))
				return *prop;
		}

		// if there's a class with an overridden DynamicObject::hasMethod, this avoids an error
		if (o->hasMethod(functionName))
			return var();
	}

	if (targetObject.isString())
		if (var* m = findRootClassProperty(StringClass::getClassName(), functionName))
			return *m;

	if (targetObject.isArray())
		if (var* m = findRootClassProperty(ArrayClass::getClassName(), functionName))
			return *m;

	if (var* m = findRootClassProperty(ObjectClass::getClassName(), functionName))
		return *m;

	location.throwError("Unknown function '" + functionName.toString() + "'");
	return var();
}

Array<Identifier> HiseJavascriptEngine::RootObject::HiseSpecialData::hiddenProperties;

bool HiseJavascriptEngine::RootObject::HiseSpecialData::initHiddenProperties = true;

HiseJavascriptEngine::RootObject::HiseSpecialData::HiseSpecialData()
{
	if (initHiddenProperties)
	{
		hiddenProperties.addIfNotAlreadyThere(Identifier("exec"));
		hiddenProperties.addIfNotAlreadyThere(Identifier("eval"));
		hiddenProperties.addIfNotAlreadyThere(Identifier("trace"));
		hiddenProperties.addIfNotAlreadyThere(Identifier("charToInt"));
		hiddenProperties.addIfNotAlreadyThere(Identifier("parseInt"));
		hiddenProperties.addIfNotAlreadyThere(Identifier("typeof"));
		hiddenProperties.addIfNotAlreadyThere(Identifier("Object"));
		hiddenProperties.addIfNotAlreadyThere(Identifier("Array"));
		hiddenProperties.addIfNotAlreadyThere(Identifier("String"));
		hiddenProperties.addIfNotAlreadyThere(Identifier("Math"));
		hiddenProperties.addIfNotAlreadyThere(Identifier("JSON"));
		hiddenProperties.addIfNotAlreadyThere(Identifier("Integer"));
		hiddenProperties.addIfNotAlreadyThere(Identifier("Content"));
		hiddenProperties.addIfNotAlreadyThere(Identifier("SynthParameters"));
		hiddenProperties.addIfNotAlreadyThere(Identifier("Engine"));
		hiddenProperties.addIfNotAlreadyThere(Identifier("Synth"));
		hiddenProperties.addIfNotAlreadyThere(Identifier("Sampler"));
		hiddenProperties.addIfNotAlreadyThere(Identifier("Globals"));
		hiddenProperties.addIfNotAlreadyThere(Identifier("include"));

		initHiddenProperties = false;
	}

	for (int i = 0; i < 32; i++)
	{
		callbackTimes[i] = 0.0;
	}
}

HiseJavascriptEngine::RootObject::HiseSpecialData::~HiseSpecialData()
{
	debugInformation.clear();
}


void HiseJavascriptEngine::RootObject::HiseSpecialData::createDebugInformation(DynamicObject *root)
{
	ScopedLock sl(debugLock);

	debugInformation.clear();

	for (int i = 0; i < constObjects.size(); i++)
	{
		debugInformation.add(new FixedVarPointerInformation(constObjects.getVarPointerAt(i), constObjects.getName(i), DebugInformation::Type::Constant));
	}

	const int numRegisters = varRegister.getNumUsedRegisters();

	for (int i = 0; i < numRegisters; i++)
		debugInformation.add(new FixedVarPointerInformation(varRegister.getVarPointer(i), varRegister.getRegisterId(i), DebugInformation::Type::RegisterVariable));

	DynamicObject *globals = root->getProperty("Globals").getDynamicObject();

	for (int i = 0; i < globals->getProperties().size(); i++)
		debugInformation.add(new DynamicObjectDebugInformation(globals, globals->getProperties().getName(i), DebugInformation::Type::Globals));

	for (int i = 0; i < root->getProperties().size(); i++)
	{
		const Identifier id = root->getProperties().getName(i);
		if (hiddenProperties.contains(id)) continue;

		debugInformation.add(new DynamicObjectDebugInformation(root, id, DebugInformation::Type::Variables));
	}
	
	for (int i = 0; i < inlineFunctions.size(); i++)
	{
		InlineFunction::Object *o = dynamic_cast<InlineFunction::Object*>(inlineFunctions.getUnchecked(i).get());

		debugInformation.add(new DebugableObjectInformation(o, o->name, DebugInformation::Type::InlineFunction));
	}

	for (int i = 0; i < callbackNEW.size(); i++)
	{
		if (!callbackNEW[i]->isDefined()) continue;

		debugInformation.add(new DebugableObjectInformation(callbackNEW[i], callbackNEW[i]->getName(), DebugInformation::Type::Callback));
	}
}


void HiseJavascriptEngine::executeCallback(int callbackIndex, Result *result)
{
	RootObject::Callback *c = root->hiseSpecialData.callbackNEW[callbackIndex];

	// You need to register the callback correctly...
	jassert(c != nullptr);

	if (c != nullptr && c->isDefined())
	{
		try
		{
			prepareTimeout();
			c->perform(root);
		}
		catch (String &error)
		{
			if (result != nullptr) *result = Result::fail(error);
		}
	}
}

void HiseJavascriptEngine::RootObject::Callback::setStatements(BlockStatement *s) noexcept
{
	statements = s;
	isCallbackDefined = s->statements.size() != 0;
}


void HiseJavascriptEngine::RootObject::Callback::perform(RootObject *root)
{
	RootObject::Scope s(nullptr, root, root);

#if USE_BACKEND
	const double pre = Time::getMillisecondCounterHiRes();

	statements->perform(s, nullptr);

	const double post = Time::getMillisecondCounterHiRes();
	lastExecutionTime = post - pre;
#else
	statements->perform(s, nullptr);
#endif
}



AttributedString DynamicObjectDebugInformation::getDescription() const
{
	var v = obj->getProperty(id);

	if (HiseJavascriptEngine::RootObject::FunctionObject *o = dynamic_cast<HiseJavascriptEngine::RootObject::FunctionObject*>(v.getObject()))
	{
		AttributedString info;
		info.setJustification(Justification::centredLeft);

		info.append("Description: ", GLOBAL_BOLD_FONT(), Colours::black);
		info.append(o->commentDoc, GLOBAL_FONT(), Colours::black.withBrightness(0.2f));
		info.append("\nParameters: ", GLOBAL_BOLD_FONT(), Colours::black);
		for (int i = 0; i < o->parameters.size(); i++)
		{
			info.append(o->parameters[i].toString(), GLOBAL_MONOSPACE_FONT(), Colours::darkblue);
			if (i != o->parameters.size() - 1) info.append(", ", GLOBAL_BOLD_FONT(), Colours::black);
		}

		return info;
	}

	return AttributedString();
}
