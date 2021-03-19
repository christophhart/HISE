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
*   Commercial licenses for using HISE in an closed source project are
*   available on request. Please visit the project's website to get more
*   information about commercial licensing:
*
*   http://www.hise.audio/
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

namespace hise { using namespace juce;

class DebugInformation;
class HiseJavascriptEngine;

/** Overwrite this method if you want to add debugging functionality to a object. */
class DebugableObject: public DebugableObjectBase
{
public:

    virtual ~DebugableObject() {};
    
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

		static void gotoLocation(Processor* processor, DebugInformationBase* info);
		
		static void gotoLocation(ModulatorSynthChain* mainSynthChain, const String& encodedState);

		static void showProcessorEditorPopup(const MouseEvent& e, Component* table, Processor* p);

		static void showJSONEditorForObject(const MouseEvent& e, Component* table, var object, const String& id);

		static var getCleanedVar(const var& value);

		static var getCleanedObjectForJSONDisplay(const var& object);

		static DebugInformationBase* getDebugInformation(ApiProviderBase* engine, DebugableObjectBase* object);

		static DebugInformationBase* getDebugInformation(ApiProviderBase* engine, const var& v);
	};

};




class DebugInformation: public DebugInformationBase
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
		ApiClass,
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

	String getTextForDataType() const override
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
        case Type::numTypes:
        default:                     return {};
		}
		return "";
	}

	virtual const var getVariantCopy() const { return var(); };

	virtual AttributedString getDescription() const override { return AttributedString(); };

	String getCodeToInsert() const override
	{
		return getTextForName();
	}

	void rightClickCallback(const MouseEvent& e, Component* componentToNotify) override;

	virtual void doubleClickCallback(const MouseEvent &e, Component* componentToNotify);

	String getTextForType() const override
	{
		return getVarType(getVariantCopy());
	}

	int getType() const { return (int)type; }

	String getTextForRow(Row r);
	String toString();

	

	//DebugableObject::Location location;

protected:

	String getVarValue(const var &v) const
	{
		if (DebugableObjectBase *d = getDebugableObject(v))
		{
			return d->getDebugValue();
		}
		else if (v.isArray())
		{
			return varArrayToString(*v.getArray());
		}
		else if (v.isBuffer())
		{
			return v.getBuffer()->toDebugString();
		}
		else return v.toString();
	}

	static DebugableObjectBase *getDebugableObject(const var &v)
	{
		return dynamic_cast<DebugableObjectBase*>(v.getObject());
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

	String getTextForDataType() const override { return obj != nullptr ? getVarType(obj->getProperty(id)) : "dangling"; }

	String getTextForValue() const override { return obj != nullptr ? getVarValue(obj->getProperty(id)) : ""; }

	const var getVariantCopy() const override { return obj != nullptr ? obj->getProperty(id) : var(); };

	AttributedString getDescription() const override;;

	DebugableObjectBase *getObject() override 
	{
		auto v = getVariantCopy();

		if (auto dyn = v.getDynamicObject())
		{
			wrapper = new DynamicDebugableObjectWrapper(dyn, id, id);
			return wrapper.get();
		}

		return nullptr;
	}

	DynamicObject::Ptr obj;
	const Identifier id;

	ScopedPointer<DynamicDebugableObjectWrapper> wrapper;
};


class FixedVarPointerInformation : public DebugInformation
{
public:
	FixedVarPointerInformation(const var* v, const Identifier &id_, const Identifier& namespaceId_, Type t, DebugableObjectBase::Location location_):
		DebugInformation(t),
		value(v),
		namespaceId(namespaceId_),
		id(id_),
		location(location_)
	{}

	DebugableObjectBase::Location getLocation() const override
	{
		return location;
	}

	String getTextForDataType() const override { return getVarType(*value); }
	
	String getTextForName() const override 
	{ 
		return namespaceId.isNull() ? id.toString() :
									  namespaceId.toString() + "." + id.toString(); 
	}

	const var getVariantCopy() const override { return var(*value); };

	String getTextForValue() const override { return getVarValue(*value); }
	DebugableObjectBase *getObject() override { return getDebugableObject(*value); }

	const var *value;
	const Identifier id;
	const Identifier namespaceId;
	DebugableObjectBase::Location location;
};


class DebugableObjectInformation : public DebugInformation
{
public:
	DebugableObjectInformation(DebugableObjectBase *object_, const Identifier &id_, Type t, const Identifier& namespaceId_=Identifier()) :
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
	AttributedString getDescription() const override { return AttributedString(); }

	DebugableObjectBase *getObject() override { return object; }

	DebugableObjectBase* object;
	const Identifier id;
	const Identifier namespaceId;
};

} // namespace hise
#endif  // DEBUGHELPERS_H_INCLUDED
