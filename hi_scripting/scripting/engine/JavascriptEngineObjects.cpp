
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
struct HiseJavascriptEngine::RootObject::MathClass : public ApiClass
{
	MathClass():
	ApiClass(2)
	{
		ADD_API_METHOD_1(abs);              
		ADD_API_METHOD_1(round);
		ADD_API_METHOD_0(random);           
		ADD_API_METHOD_2(randInt);
		ADD_API_METHOD_2(min);              
		ADD_API_METHOD_2(max);
		ADD_API_METHOD_3(range);            
		ADD_API_METHOD_1(sign);
		ADD_API_METHOD_1(toDegrees);        
		ADD_API_METHOD_1(toRadians);
		ADD_API_METHOD_1(sin);              
		ADD_API_METHOD_1(asin);
		ADD_API_METHOD_1(sinh);             
		ADD_API_METHOD_1(asinh);
		ADD_API_METHOD_1(cos);              
		ADD_API_METHOD_1(acos);
		ADD_API_METHOD_1(cosh);             
		ADD_API_METHOD_1(acosh);
		ADD_API_METHOD_1(tan);              
		ADD_API_METHOD_1(atan);
		ADD_API_METHOD_1(tanh);             
		ADD_API_METHOD_1(atanh);
		ADD_API_METHOD_1(log);              
		ADD_API_METHOD_1(log10);
		ADD_API_METHOD_1(exp);              
		ADD_API_METHOD_2(pow);
		ADD_API_METHOD_1(sqr);              
		ADD_API_METHOD_1(sqrt);
		ADD_API_METHOD_1(ceil);             
		ADD_API_METHOD_1(floor);

		addConstant("PI", double_Pi);
		addConstant("E", exp(1.0));
	}

	Identifier getName() const override { static const Identifier i("Math"); return i; }

	struct Wrapper
	{
		API_METHOD_WRAPPER_1(MathClass, abs);
		API_METHOD_WRAPPER_1(MathClass, round);
		API_METHOD_WRAPPER_0(MathClass, random);
		API_METHOD_WRAPPER_2(MathClass, randInt);
		API_METHOD_WRAPPER_2(MathClass, min);
		API_METHOD_WRAPPER_2(MathClass, max);
		API_METHOD_WRAPPER_3(MathClass, range);
		API_METHOD_WRAPPER_1(MathClass, sign);
		API_METHOD_WRAPPER_1(MathClass, toDegrees);
		API_METHOD_WRAPPER_1(MathClass, toRadians);
		API_METHOD_WRAPPER_1(MathClass, sin);
		API_METHOD_WRAPPER_1(MathClass, asin);
		API_METHOD_WRAPPER_1(MathClass, sinh);
		API_METHOD_WRAPPER_1(MathClass, asinh);
		API_METHOD_WRAPPER_1(MathClass, cos);
		API_METHOD_WRAPPER_1(MathClass, acos);
		API_METHOD_WRAPPER_1(MathClass, cosh);
		API_METHOD_WRAPPER_1(MathClass, acosh);
		API_METHOD_WRAPPER_1(MathClass, tan);
		API_METHOD_WRAPPER_1(MathClass, atan);
		API_METHOD_WRAPPER_1(MathClass, tanh);
		API_METHOD_WRAPPER_1(MathClass, atanh);
		API_METHOD_WRAPPER_1(MathClass, log);
		API_METHOD_WRAPPER_1(MathClass, log10);
		API_METHOD_WRAPPER_1(MathClass, exp);
		API_METHOD_WRAPPER_2(MathClass, pow);
		API_METHOD_WRAPPER_1(MathClass, sqr);
		API_METHOD_WRAPPER_1(MathClass, sqrt);
		API_METHOD_WRAPPER_1(MathClass, ceil);
		API_METHOD_WRAPPER_1(MathClass, floor);
	};

	/** Returns a random number between 0.0 and 1.0. */
	var random()
	{ 
		return Random::getSystemRandom().nextDouble(); 
	}

	/** Returns a random integer between the low and the high values. */
	var randInt(var low, var high) 
	{ 
		return Random::getSystemRandom().nextInt(Range<int>((int)low, (int)high)); 
	}

	/** Returns the absolute (unsigned) value. */
	var abs(var value) 
	{
		return value.isInt() ? var(std::abs((int)value)) : 
							   var(std::abs((double)value)); 
	}

	/** Rounds the value to the next integer. */
	var round(var value) 
	{ 
		return value.isInt() ? var(roundToInt((int)value)) :
							   var(roundToInt((double)value));
	}

	/** Returns the sign of the value. */
	var sign(var value) 
	{ 
		return value.isInt() ? var(sign_((int)value)) : 
							   var(sign_((double)value));
	}

	/** Limits the value to the given range. */
	var range(var value, var lowerLimit, var upperLimit)
	{	
		return value.isInt() ? var(jlimit<int>(lowerLimit, upperLimit, value)) :
							   var(jlimit<double>(lowerLimit, upperLimit, value));
	}

	/** Returns the smaller number. */
	var min(var first, var second)
	{ 
		return (first.isInt() && second.isInt()) ? var(jmin((int)first, (int)second)) : 
												   var(jmin((double)first, (double)second)); 
	}

	/** Returns the bigger number. */
	var max(var first, var second) 
	{ 
		return (first.isInt() && second.isInt()) ? var(jmax((int)first, (int)second)) :
												   var(jmax((double)first, (double)second));
	}

	/** Converts radian (0...2*PI) to degree (0...360°). */
	var toDegrees(var value) { return radiansToDegrees((double)value); }

	/** Converts degree  (0...360°) to radian (0...2*PI). */
	var toRadians(var value) { return degreesToRadians((double)value); }

	/** Calculates the sine value (radian based). */
	var sin(var value) { return std::sin((double)value); }

	/** Calculates the asine value (radian based). */
	var asin(var value) { return std::asin((double)value); }

	/** Calculates the cosine value (radian based). */
	var cos(var value) { return std::cos((double)value); }

	/** Calculates the acosine value (radian based). */
	var acos(var value) { return std::acos((double)value); }

	/** Calculates the sinh value (radian based). */
	var sinh(var value) { return std::sinh((double)value); }

	/** Calculates the asinh value (radian based). */
	var asinh(var value) { return std::asinh((double)value); }

	/** Calculates the cosh value (radian based). */
	var cosh(var value) { return std::cosh((double)value); }

	/** Calculates the acosh value (radian based). */
	var acosh(var value) { return std::acosh((double)value); }

	/** Calculates the tan value (radian based). */
	var tan(var value) { return std::tan((double)value); }

	/** Calculates the tanh value (radian based). */
	var tanh(var value) { return std::tanh((double)value); }

	/** Calculates the atan value (radian based). */
	var atan(var value) { return std::atan((double)value); }

	/** Calculates the atanh value (radian based). */
	var atanh(var value) { return std::atanh((double)value); }

	/** Calculates the log value (with base E). */
	var log(var value) { return std::log((double)value); }

	/** Calculates the log value (with base 10). */
	var log10(var value) { return std::log10((double)value); }

	/** Calculates the exp value. */
	var exp(var value) { return std::exp((double)value); }

	/** Calculates the power of base and exponent. */
	var pow(var base, var exp) { return std::pow((double)base, (double)exp); }

	/** Calculates the square (x*x) of the value. */
	var sqr(var value) { double x = (double)value; return x * x; }

	/** Calculates the square root of the value. */
	var sqrt(var value) { return std::sqrt((double)value); }

	/** Rounds up the value. */
	var ceil(var value) { return std::ceil((double)value); }

	/** Rounds down the value. */
	var floor(var value) { return std::floor((double)value); }
	
	template <typename Type> static Type sign_(Type n) noexcept{ return n > 0 ? (Type)1 : (n < 0 ? (Type)-1 : 0); }
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
HiseJavascriptEngine::HiseJavascriptEngine(JavascriptProcessor *p) : maximumExecutionTime(15.0), root(new RootObject()), unneededScope(new DynamicObject())
{
	root->hiseSpecialData.setProcessor(p);

	registerNativeObject(RootObject::ObjectClass::getClassName(), new RootObject::ObjectClass());
	registerNativeObject(RootObject::ArrayClass::getClassName(), new RootObject::ArrayClass());
	registerNativeObject(RootObject::StringClass::getClassName(), new RootObject::StringClass());
	registerApiClass(new RootObject::MathClass());
	registerNativeObject(RootObject::JSONClass::getClassName(), new RootObject::JSONClass());
	registerNativeObject(RootObject::IntegerClass::getClassName(), new RootObject::IntegerClass());
}

HiseJavascriptEngine::RootObject::RootObject():
hiseSpecialData(this)
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


var HiseJavascriptEngine::callExternalFunction(var callback, const var::NativeFunctionArgs& args, Result* result)
{
    var returnVal(var::undefined());
    
    try
    {
        prepareTimeout();
        if (result != nullptr) *result = Result::ok();
        
        RootObject::FunctionObject *fo = dynamic_cast<RootObject::FunctionObject*>(callback.getObject());
        
        if (fo != nullptr)
        {
            RootObject::Scope s(nullptr, root, root);
            returnVal = fo->invoke(s, args);
        }
    }
    catch (String& error)
    {
        if (result != nullptr) *result = Result::fail(error);
    }
    
    return returnVal;
}

Array<Identifier> HiseJavascriptEngine::RootObject::HiseSpecialData::hiddenProperties;

bool HiseJavascriptEngine::RootObject::HiseSpecialData::initHiddenProperties = true;

HiseJavascriptEngine::RootObject::HiseSpecialData::HiseSpecialData(RootObject* root_):
root(root_)
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

void HiseJavascriptEngine::RootObject::HiseSpecialData::clear()
{
    clearDebugInformation();
    apiClasses.clear();
    inlineFunctions.clear();
    constObjects.clear();
    callbackNEW.clear();
    globals = nullptr;
}

HiseJavascriptEngine::RootObject::Callback *HiseJavascriptEngine::RootObject::HiseSpecialData::getCallback(const Identifier &id)
{
    for (int i = 0; i < callbackNEW.size(); i++)
    {
        if (callbackNEW[i]->getName() == id)
        {
            return callbackNEW[i];
        }
    }
    
    return nullptr;
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


var HiseJavascriptEngine::executeCallback(int callbackIndex, Result *result)
{
	RootObject::Callback *c = root->hiseSpecialData.callbackNEW[callbackIndex];

	// You need to register the callback correctly...
	jassert(c != nullptr);

	if (c != nullptr && c->isDefined())
	{
		try
		{
			prepareTimeout();
			return c->perform(root);
		}
		catch (String &error)
		{
			if (result != nullptr) *result = Result::fail(error);
		}
	}

	return var::undefined();
}

void HiseJavascriptEngine::RootObject::Callback::setStatements(BlockStatement *s) noexcept
{
	statements = s;
	isCallbackDefined = s->statements.size() != 0;
}


var HiseJavascriptEngine::RootObject::Callback::perform(RootObject *root)
{
	RootObject::Scope s(nullptr, root, root);

	var returnValue = var::undefined();

#if USE_BACKEND
	const double pre = Time::getMillisecondCounterHiRes();

	statements->perform(s, &returnValue);

	const double post = Time::getMillisecondCounterHiRes();
	lastExecutionTime = post - pre;
#else
	statements->perform(s, &returnValue);
#endif

	return returnValue;
}



AttributedString DynamicObjectDebugInformation::getDescription() const
{
	return AttributedString();
}
