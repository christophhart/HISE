/*
  ==============================================================================

    ScriptMacroDefinitions.h
    Created: 6 Jul 2016 11:24:36am
    Author:  Christoph

  ==============================================================================
*/

#ifndef SCRIPTMACRODEFINITIONS_H_INCLUDED
#define SCRIPTMACRODEFINITIONS_H_INCLUDED


// Macros for APIClass objects

#define ADD_API_METHOD_0(name) static const Identifier name ## _id (#name); addFunction(name ## _id, &Wrapper::name)
#define ADD_API_METHOD_1(name) static const Identifier name ## _id (#name); addFunction1(name ## _id, &Wrapper::name)
#define ADD_API_METHOD_2(name) static const Identifier name ## _id (#name); addFunction2(name ## _id, &Wrapper::name)
#define ADD_API_METHOD_3(name) static const Identifier name ## _id (#name); addFunction3(name ## _id, &Wrapper::name)
#define ADD_API_METHOD_4(name) static const Identifier name ## _id (#name); addFunction4(name ## _id, &Wrapper::name)
#define ADD_API_METHOD_5(name) static const Identifier name ## _id (#name); addFunction5(name ## _id, &Wrapper::name)

#if USE_BACKEND
#define ADD_TYPED_API_METHOD_1(name, t1) static const Identifier name ## _id (#name); addFunction1(name ## _id, &Wrapper::name); addForcedParameterTypes(name ## _id, VarTypeChecker::createParameterTypes(t1));
#define ADD_TYPED_API_METHOD_2(name, t1, t2) static const Identifier name ## _id (#name); addFunction2(name ## _id, &Wrapper::name); addForcedParameterTypes(name ## _id, VarTypeChecker::createParameterTypes(t1, t2));
#define ADD_TYPED_API_METHOD_3(name, t1, t2, t3) static const Identifier name ## _id (#name); addFunction3(name ## _id, &Wrapper::name); addForcedParameterTypes(name ## _id, VarTypeChecker::createParameterTypes(t1, t2, t3));
#define ADD_TYPED_API_METHOD_4(name, t1, t2, t3, t4) static const Identifier name ## _id (#name); addFunction4(name ## _id, &Wrapper::name); addForcedParameterTypes(name ## _id, VarTypeChecker::createParameterTypes(t1, t2, t3, t4));
#define ADD_TYPED_API_METHOD_5(name, t1, t2, t3, t4, t5) static const Identifier name ## _id (#name); addFunction5(name ## _id, &Wrapper::name); addForcedParameterTypes(name ## _id, VarTypeChecker::createParameterTypes(t1, t2, t3, t4, t5));
#else
#define ADD_TYPED_API_METHOD_1(name, ...) ADD_API_METHOD_1(name)
#define ADD_TYPED_API_METHOD_2(name, ...) ADD_API_METHOD_2(name)
#define ADD_TYPED_API_METHOD_3(name, ...) ADD_API_METHOD_3(name)
#define ADD_TYPED_API_METHOD_4(name, ...) ADD_API_METHOD_4(name)
#define ADD_TYPED_API_METHOD_5(name, ...) ADD_API_METHOD_5(name)
#endif

#define ADD_INLINEABLE_API_METHOD_0(name) ADD_API_METHOD(name) setFunctionIsInlineable(name ## _id);
#define ADD_INLINEABLE_API_METHOD_1(name) addFunction1(Identifier(#name), &Wrapper::name); setFunctionIsInlineable(Identifier(#name));
#define ADD_INLINEABLE_API_METHOD_2(name) addFunction2(Identifier(#name), &Wrapper::name); setFunctionIsInlineable(Identifier(#name));
#define ADD_INLINEABLE_API_METHOD_3(name) addFunction3(Identifier(#name), &Wrapper::name); setFunctionIsInlineable(Identifier(#name));
#define ADD_INLINEABLE_API_METHOD_4(name) addFunction4(Identifier(#name), &Wrapper::name); setFunctionIsInlineable(Identifier(#name));
#define ADD_INLINEABLE_API_METHOD_5(name) addFunction5(Identifier(#name), &Wrapper::name); setFunctionIsInlineable(Identifier(#name));


#define API_METHOD_WRAPPER_0(className, name)	inline static var name(ApiClass *m) { return var(static_cast<className*>(m)->name()); };
#define API_METHOD_WRAPPER_1(className, name)	inline static var name(ApiClass *m, var value1) { return var(static_cast<className*>(m)->name(value1)); };
#define API_METHOD_WRAPPER_2(className, name)	inline static var name(ApiClass *m, var value1, var value2) { return var(static_cast<className*>(m)->name(value1, value2)); };
#define API_METHOD_WRAPPER_3(className, name)	inline static var name(ApiClass *m, var value1, var value2, var value3) { return var(static_cast<className*>(m)->name(value1, value2, value3)); };
#define API_METHOD_WRAPPER_4(className, name)	inline static var name(ApiClass *m, var value1, var value2, var value3, var value4) { return var(static_cast<className*>(m)->name(value1, value2, value3, value4)); };
#define API_METHOD_WRAPPER_5(className, name)   inline static var name(ApiClass *m, var value1, var value2, var value3, var value4, var value5) { return static_cast<className*>(m)->name(value1, value2, value3, value4, value5); };

#define API_VOID_METHOD_WRAPPER_0(className, name)	inline static var name(ApiClass *m) { static_cast<className*>(m)->name(); return var::undefined(); };
#define API_VOID_METHOD_WRAPPER_1(className, name)	inline static var name(ApiClass *m, var value1) { static_cast<className*>(m)->name(value1); return var::undefined(); };
#define API_VOID_METHOD_WRAPPER_2(className, name)	inline static var name(ApiClass *m, var value1, var value2) { static_cast<className*>(m)->name(value1, value2); return var::undefined(); };
#define API_VOID_METHOD_WRAPPER_3(className, name)	inline static var name(ApiClass *m, var value1, var value2, var value3) { static_cast<className*>(m)->name(value1, value2, value3); return var::undefined(); };
#define API_VOID_METHOD_WRAPPER_4(className, name)  inline static var name(ApiClass *m, var value1, var value2, var value3, var value4) { static_cast<className*>(m)->name(value1, value2, value3, value4); return var::undefined(); };
#define API_VOID_METHOD_WRAPPER_5(className, name)  inline static var name(ApiClass *m, var value1, var value2, var value3, var value4, var value5) { static_cast<className*>(m)->name(value1, value2, value3, value4, value5); return var::undefined(); };

// Macros for DynamicObject objects


#define SET_MODULE_NAME(x) static Identifier getName() {static const Identifier id(x); return id; };

/** Quick way of accessing the argument of a function call. */
#define ARG(index) args.arguments[index]

/** Quick way of getting a casted object of an argument. */
#define GET_ARGUMENT_OBJECT(classType, argumentIndex) dynamic_cast<classType*>(args.arguments[argumentIndex].getObject())

/** Adds a wrapper function with a variable amount of arguments. */
#define DYNAMIC_METHOD_WRAPPER(className, functionName, ...) static var functionName(const var::NativeFunctionArgs& args) \
{ \
	if (className* thisObject = dynamic_cast<className*>(args.thisObject.getObject())) \
		thisObject->functionName(__VA_ARGS__); \
	return var::undefined(); \
};

/** Adds a wrapper function with a variable amount of arguments that return the result of the function. */
#define DYNAMIC_METHOD_WRAPPER_WITH_RETURN(className, functionName, ...) static var functionName(const var::NativeFunctionArgs& args) \
{ \
	if (className* thisObject = dynamic_cast<className*>(args.thisObject.getObject())) \
		return thisObject->functionName(__VA_ARGS__); \
	return var::undefined(); \
};

/** Quick way of adding a method to a Dynamic Object. Use this in combination with */
#define ADD_DYNAMIC_METHOD(name) setMethod(#name, Wrapper::name);

#define SANITIZED(x) FloatSanitizers::sanitizeFloatNumber(x)

#endif  // SCRIPTMACRODEFINITIONS_H_INCLUDED
