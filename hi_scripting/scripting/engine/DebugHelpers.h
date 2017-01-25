/*  ===========================================================================
*
*   This file is part of HISE.
*   Copyright 2016 Christoph Hart
*
*   HISE is free software: you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation, either version 3 of the License, or
*   (at your option) any later version.
*
*   HISE is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public License
*   along with HISE.  If not, see <http://www.gnu.org/licenses/>.
*
*   Commercial licences for using HISE in an closed source project are
*   available on request. Please visit the project's website to get more
*   information about commercial licencing:
*
*   http://www.hartinstruments.net/hise/
*
*   HISE is based on the JUCE library,
*   which also must be licenced for commercial applications:
*
*   http://www.juce.com
*
*   ===========================================================================
*/

#ifndef DEBUGHELPERS_H_INCLUDED
#define DEBUGHELPERS_H_INCLUDED





/** Overwrite this method if you want to add debugging functionality to a object. */
class DebugableObject
{
public:

	struct Location
	{
		String fileName = String();
		int charNumber = 0;
	};

    virtual ~DebugableObject() {};
    
	/** This will be shown as value of the object. */
	virtual String getDebugValue() const = 0;

	/** This will be shown as name of the object. */
	virtual String getDebugName() const = 0;

	virtual String getDebugDataType() const { return getDebugName(); }

	virtual AttributedString getDescription() const { return AttributedString(); }

	/** This will be called if the user double clicks on the row. */
	virtual void doubleClickCallback(const MouseEvent &/*e*/, Component* /*componentToNotify*/) {};

	struct Helpers
	{
		static AttributedString getFunctionDoc(const String &docBody, const Array<Identifier> &parameters)
		{
			AttributedString info;
			info.setJustification(Justification::centredLeft);

			info.append("Description: ", GLOBAL_BOLD_FONT(), Colours::black);
			info.append(docBody, GLOBAL_FONT(), Colours::black.withBrightness(0.2f));
			info.append("\nParameters: ", GLOBAL_BOLD_FONT(), Colours::black);
			for (int i = 0; i < parameters.size(); i++)
			{
				info.append(parameters[i].toString(), GLOBAL_MONOSPACE_FONT(), Colours::darkblue);
				if (i != parameters.size() - 1) info.append(", ", GLOBAL_BOLD_FONT(), Colours::black);
			}

			return info;
		}

		static void gotoLocation(Component* ed, JavascriptProcessor* sp, const Location& location);

		
	};

};




class DebugInformation
{
public:

	enum class Type
	{
		RegisterVariable = 0,
		Variables,
		Constant,
		InlineFunction,
		Globals,
		Callback,
		ExternalFunction,
		Namespace,
		numTypes
	};

	enum class Row
	{
		Type = 0,
		DataType,
		Name,
		Value,
		numRows
	};

	static DebugInformation *createDebugInformationFor(var *value, const Identifier &id, Type t);

	DebugInformation(Type t): type(t) {};

	virtual ~DebugInformation() {};

	static String varArrayToString(const Array<var> &arrayToStringify);;

	StringArray createTextArray();
	
	String getTextForType() const
	{
		switch (type)
		{
		case Type::RegisterVariable: return "Register";
		case Type::Variables:		 return "Variables";
		case Type::Constant:		 return "Constant";
		case Type::InlineFunction:	 return "InlineFunction";
		case Type::Globals:			 return "Globals";
		case Type::Callback:		 return "Callback";
		case Type::ExternalFunction: return "ExternalFunction";
		case Type::Namespace:		 return "Namespace";
        case Type::numTypes:         return "";
		}
		return "";
	}

	virtual String getTextForDataType() const = 0;
	virtual String getTextForName() const = 0;
	virtual String getTextForValue() const = 0;

	virtual AttributedString getDescription() const { return AttributedString(); };

	virtual DebugableObject *getObject() { return nullptr; }

	virtual DebugableObject::Location getLocation() { return DebugableObject::Location(); }

	int getType() const { return (int)type; }

	String getTextForRow(Row r);
	String toString();

	static String getVarType(const var &v)
	{
		if (v.isUndefined())	return "undefined";
		else if (v.isArray())	return "Array";
		else if (v.isBool())	return "bool";
		else if (v.isInt() ||
			v.isInt64())	return "int";
		else if (v.isObject())
		{
			if (DebugableObject *d = dynamic_cast<DebugableObject*>(v.getObject()))
			{
				return d->getDebugDataType();
			}
			else return "Object";
		}
		else if (v.isObject()) return "Object";
		else if (v.isDouble()) return "double";
		else if (v.isString()) return "String";
		else if (v.isMethod()) return "function";

		return "undefined";
	}

	DebugableObject::Location location;

protected:
	
	String getVarValue(const var &v) const
	{
		if (DebugableObject *d = getDebugableObject(v))
		{
			return d->getDebugValue();
		}
		else if (v.isArray())
		{
			return varArrayToString(*v.getArray());
		}
		else return v.toString();
	}

	static DebugableObject *getDebugableObject(const var &v)
	{
		return dynamic_cast<DebugableObject*>(v.getObject());
	}

	private:

	Type type;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DebugInformation)
};

class DynamicObjectDebugInformation : public DebugInformation
{
public:

	DynamicObjectDebugInformation(DynamicObject *obj_, const Identifier &id_, Type t) :
		DebugInformation(t),
		obj(obj_),
		id(id_)
	{}

	~DynamicObjectDebugInformation()
	{
		obj = nullptr;
	}

	String getTextForName() const override { return id.toString(); }

	String getTextForDataType() const override { return getVarType(obj->getProperty(id)); }

	String getTextForValue() const override { return getVarValue(obj->getProperty(id)); }

	AttributedString getDescription() const override;;

	DebugableObject *getObject() override { return getDebugableObject(obj->getProperty(id)); }

	DynamicObject::Ptr obj;
	const Identifier id;

	
};


class FixedVarPointerInformation : public DebugInformation
{
public:
	FixedVarPointerInformation(const var* v, const Identifier &id_, const Identifier& namespaceId_, Type t):
		DebugInformation(t),
		value(v),
		namespaceId(namespaceId_),
		id(id_)
	{}

	String getTextForDataType() const override { return getVarType(*value); }
	
	String getTextForName() const override 
	{ 
		return namespaceId.isNull() ? id.toString() :
									  namespaceId.toString() + "." + id.toString(); 
	}

	String getTextForValue() const override { return getVarValue(*value); }
	DebugableObject *getObject() override { return getDebugableObject(*value); }

	const var *value;
	const Identifier id;
	const Identifier namespaceId;
	
};


class DebugableObjectInformation : public DebugInformation
{
public:
	DebugableObjectInformation(DebugableObject *object_, const Identifier &id_, Type t, const Identifier& namespaceId_=Identifier()) :
		DebugInformation(t),
		object(object_),
		id(id_),
		namespaceId(namespaceId_)
	{};

	String getTextForDataType() const override { return object->getDebugDataType(); }
	String getTextForName() const override 
	{ 
		return namespaceId.isNull() ? object->getDebugName() :
									  namespaceId.toString() + "." + object->getDebugName(); 
	}
	String getTextForValue() const override { return object->getDebugValue(); }
	AttributedString getDescription() const override { return object->getDescription(); }

	DebugableObject *getObject() { return object; }

	DebugableObject *object;
	const Identifier id;
	const Identifier namespaceId;
};

#endif  // DEBUGHELPERS_H_INCLUDED
