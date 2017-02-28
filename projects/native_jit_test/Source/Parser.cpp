/*
  ==============================================================================

    Parser.cpp
    Created: 23 Feb 2017 1:20:45pm
    Author:  Christoph

  ==============================================================================
*/


template <typename T> String NativeJITTypeHelpers::getTypeName() { return String(typeid(T).name()); }
String NativeJITTypeHelpers::getTypeName(const String &t) { return String(getTypeForLiteral(t).name()); }

template <typename ActualType, typename ExpectedType>
String NativeJITTypeHelpers::getTypeMismatchErrorMessage()
{
	String message = "Type mismatch: ";
	message << getTypeName<ActualType>();
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
template <typename R1, typename R2> bool NativeJITTypeHelpers::is() { return typeid(R1) == typeid(R2); }
