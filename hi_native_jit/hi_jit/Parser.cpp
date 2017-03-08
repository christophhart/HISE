/*
  ==============================================================================

    Parser.cpp
    Created: 23 Feb 2017 1:20:45pm
    Author:  Christoph

  ==============================================================================
*/


template <typename T> String NativeJITTypeHelpers::getTypeName() { return getTypeName(typeid(T)); }
String NativeJITTypeHelpers::getTypeName(const String &t) { return String(getTypeForLiteral(t).name()); }


String NativeJITTypeHelpers::getTypeName(const TypeInfo& info)
{
	return info.name();
}

template <typename ActualType, typename ExpectedType>
String NativeJITTypeHelpers::getTypeMismatchErrorMessage()
{
	return getTypeMismatchErrorMessage<ExpectedType>(typeid(ActualType));
}


template <typename ExpectedType>
String NativeJITTypeHelpers::getTypeMismatchErrorMessage(const TypeInfo& actualType)
{
	String message = "Type mismatch: ";
	message << getTypeName(actualType);
	message << ", Expected: ";
	message << getTypeName<ExpectedType>();

	return message;
}


TypeInfo NativeJITTypeHelpers::getTypeForLiteral(const String &t)
{
	if (t.endsWithChar('f'))			  return typeid(float);
	else if (t.containsChar('.'))		  return typeid(double);
	else if (t == "true" || t == "false") return typeid(bool);
	else return							  typeid(int);
}

template <typename ExpectedType> bool NativeJITTypeHelpers::matchesType(const TypeInfo& actualType) { return actualType == typeid(ExpectedType); }
template <typename ExpectedType> bool NativeJITTypeHelpers::matchesType(const String& t) { return getTypeForLiteral(t) == typeid(ExpectedType); }

TypeInfo NativeJITTypeHelpers::getTypeForToken(const char* token)
{
	if (String(token) == String(NativeJitTokens::double_)) return typeid(double);
	else if (String(token) == String(NativeJitTokens::int_))  return typeid(int);
	else if (String(token) == String(NativeJitTokens::float_))  return typeid(float);
	else if (String(token) == String(NativeJitTokens::buffer_)) return typeid(Buffer);
	else return typeid(void);
}

template <typename R1, typename R2> bool NativeJITTypeHelpers::is() { return typeid(R1) == typeid(R2); }
