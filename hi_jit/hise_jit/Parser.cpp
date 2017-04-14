/*
  ==============================================================================

    Parser.cpp
    Created: 23 Feb 2017 1:20:45pm
    Author:  Christoph

  ==============================================================================
*/


template <typename T> String HiseJITTypeHelpers::getTypeName() { return getTypeName(typeid(T)); }
String HiseJITTypeHelpers::getTypeName(const String &t) { return String(getTypeForLiteral(t).name()); }


String HiseJITTypeHelpers::getTypeName(const TypeInfo& info)
{
	return info.name();
}

template <typename ActualType, typename ExpectedType>
String HiseJITTypeHelpers::getTypeMismatchErrorMessage()
{
	return getTypeMismatchErrorMessage(typeid(ActualType), typeid(ExpectedType));
}


template <typename ExpectedType>
String HiseJITTypeHelpers::getTypeMismatchErrorMessage(const TypeInfo& actualType)
{
	return getTypeMismatchErrorMessage(actualType, typeid(ExpectedType));
}


String HiseJITTypeHelpers::getTypeMismatchErrorMessage(const TypeInfo& actualType, const TypeInfo& expectedType)
{
	String message = "Type mismatch: ";
	message << getTypeName(actualType);
	message << ", Expected: ";
	message << getTypeName(expectedType);

	return message;
}



TypeInfo HiseJITTypeHelpers::getTypeForLiteral(const String &t)
{
	if (t.endsWithChar('f'))			  return typeid(float);
	else if (t.containsChar('.'))		  return typeid(double);
	else if (t == "true" || t == "false") return typeid(BooleanType);
	else return							  typeid(int);
}

template <typename ExpectedType> bool HiseJITTypeHelpers::matchesType(const TypeInfo& actualType) { return actualType == typeid(ExpectedType); }
template <typename ExpectedType> bool HiseJITTypeHelpers::matchesType(const String& t) { return getTypeForLiteral(t) == typeid(ExpectedType); }

TypeInfo HiseJITTypeHelpers::getTypeForToken(const char* token)
{
	if (String(token) == String(HiseJitTokens::double_)) return typeid(double);
	else if (String(token) == String(HiseJitTokens::int_))  return typeid(int);
	else if (String(token) == String(HiseJitTokens::float_))  return typeid(float);
#if INCLUDE_BUFFERS
	else if (String(token) == String(HiseJitTokens::buffer_)) return typeid(Buffer);
#endif
	else if (String(token) == String(HiseJitTokens::bool_)) return typeid(BooleanType);
	else return typeid(void);
}

template <typename R1, typename R2> bool HiseJITTypeHelpers::is() { return typeid(R1) == typeid(R2); }
