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

		static bool gotoLocation(Component* ed, JavascriptProcessor* sp, const Location& location);

		static bool gotoLocation(Processor* processor, DebugInformationBase* info);
		
		static bool gotoLocation(ModulatorSynthChain* mainSynthChain, const String& encodedState);

		/** This will try to resolve the location from the provider if the obj has not a valid location. */
		static Location getLocationFromProvider(Processor* p, DebugableObjectBase* obj);

		static Component* showProcessorEditorPopup(const MouseEvent& e, Component* table, Processor* p);

		static Component* createJSONEditorForObject(const MouseEvent& e, Component* table, var object, const String& id);

		static void showJSONEditorForObject(const MouseEvent& e, Component* table, var object, const String& id);

		static var getCleanedVar(const var& value);

		static var getCleanedObjectForJSONDisplay(const var& object);

		static DebugInformationBase::Ptr getDebugInformation(DebugInformationBase::Ptr parent, DebugableObjectBase* object);

		static DebugInformationBase::Ptr getDebugInformation(ApiProviderBase* engine, DebugableObjectBase* object);

		static DebugInformationBase::Ptr getDebugInformation(ApiProviderBase* engine, const var& v);
        
        static DebugInformationBase::List getDebugInformationFromString(ApiProviderBase* engine, const String& token);
        
        static DebugInformationBase::List getDebugInformationFromString(DebugInformationBase::Ptr parent, const String& token);
        
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
	}

	virtual const var getVariantCopy() const { return var(); };

	virtual AttributedString getDescription() const override { return AttributedString(); };

	String getCodeToInsert() const override
	{
		return getTextForName();
	}

	Component* createPopupComponent(const MouseEvent& e, Component* componentToNotify) override;

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
		auto obj = v.getObject();
		return dynamic_cast<DebugableObjectBase*>(obj);
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

	bool isWatchable() const override
	{
		static const Array<Identifier> unwatchableIds = 
		{ 
			Identifier("Array"), 
			Identifier("String"), 
			Identifier("Buffer"),
			Identifier("Libraries")
		};

		return !unwatchableIds.contains(id);
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


class LambdaValueInformation : public DebugInformation
{
public:

	using ValueFunction = std::function<var()>;

	LambdaValueInformation(const ValueFunction& f, const Identifier &id_, const Identifier& namespaceId_, Type t, DebugableObjectBase::Location location_, const String& comment_=String()):
		DebugInformation(t),
		vf(f),
		namespaceId(namespaceId_),
		id(id_),
		location(location_)
	{
		cachedValue = f();
		DebugableObjectBase::updateLocation(location, cachedValue);

		if (comment_.isNotEmpty())
			comment.append(comment_, GLOBAL_FONT(), Colours::white);;
	}

	DebugableObjectBase::Location getLocation() const override
	{
		return location;
	}

	AttributedString getDescription() const override
	{
		return comment;
	}

	String getTextForDataType() const override { return getVarType(getCachedValueFunction(false)); }
	
	String getTextForName() const override 
	{ 
		return namespaceId.isNull() ? id.toString() :
									  namespaceId.toString() + "." + id.toString(); 
	}

	int getNumChildElements() const override
	{
		auto value = getCachedValueFunction(false);

		if (auto obj = getDebugableObject(value))
		{
			auto customSize = obj->getNumChildElements();

			if (customSize != -1)
				return customSize;
		}

		if (value.isBuffer())
		{
			auto s = value.getBuffer()->size;

			if (isPositiveAndBelow(s, 513))
				return s;

			return 0;
		}
		
		if (auto dyn = value.getDynamicObject())
			return dyn->getProperties().size();

		if (auto ar = value.getArray())
			return jmin<int>(128, ar->size());

		return 0;
	}

	 
	DebugInformation::Ptr getChildElement(int index) override
	{
		auto value = getCachedValueFunction(false);

		if (auto obj = getDebugableObject(value))
		{
			auto numCustom = obj->getNumChildElements();

			if (isPositiveAndBelow(index, numCustom))
				return obj->getChildElement(index);
		}

		WeakReference<LambdaValueInformation> safeThis(this);

		if (value.isBuffer())
		{
			auto actualValueFunction = [index, safeThis]()
			{
				if (safeThis == nullptr)
					return var();

				if (auto b = safeThis->getCachedValueFunction(false).getBuffer())
				{
					if (isPositiveAndBelow(index, b->size))
						return var(b->getSample(index));
				}

				return var(0.0f);
			};

			String cid = "%PARENT%[" + String(index) + "]";

			return new LambdaValueInformation(actualValueFunction, Identifier(cid), namespaceId, (Type)getType(), location);
		}
		else if (auto dyn = value.getDynamicObject())
		{
			String cid;

			const NamedValueSet& s = dyn->getProperties();

			if (isPositiveAndBelow(index, s.size()))
			{
				auto mid = s.getName(index);
				cid << id << "." << mid;

				auto cf = [safeThis, mid]()
				{
					if (safeThis == nullptr)
						return var();

					auto v = safeThis->getCachedValueFunction(false);
					return v.getProperty(mid, {});
				};

					return new LambdaValueInformation(cf, Identifier(cid), namespaceId, (Type)getType(), location);
			}
		}
		else if (auto ar = value.getArray())
		{
			String cid;
			cid << id << "[" << String(index) << "]";

			auto cf = [index, safeThis]()
			{
				if (safeThis == nullptr)
					return var();

				auto a = safeThis->getCachedValueFunction(false);

				if (auto ar = a.getArray())
					return (*ar)[index];

				return var();
			};

			return new LambdaValueInformation(cf, Identifier(cid), namespaceId, (Type)getType(), location);
		}

		return new DebugInformationBase();
	}

	var getCachedValueFunction(bool forceLookup) const
	{
		if (forceLookup || cachedValue.isUndefined())
			cachedValue = vf();

		return cachedValue;
	}

	bool isAutocompleteable() const override
	{
		if (customAutoComplete)
			return autocompleteable;

		auto v = getCachedValueFunction(false);

		if (v.isObject())
			return true;
        
        return false;
	}

	void setAutocompleteable(bool shouldBe)
	{
		customAutoComplete = true;
		autocompleteable = shouldBe;
	}

	const var getVariantCopy() const override { return var(getCachedValueFunction(false)); };

	String getTextForValue() const override {
		auto v = getCachedValueFunction(true);
		return getVarValue(v); 
	}
	DebugableObjectBase *getObject() override { return getDebugableObject(getCachedValueFunction(false)); }

	mutable var cachedValue;
	

	const Identifier id;
	const Identifier namespaceId;
	DebugableObjectBase::Location location;
	bool customAutoComplete = false;

private:

	AttributedString comment;
	bool autocompleteable = true;
	ValueFunction vf;

	JUCE_DECLARE_WEAK_REFERENCEABLE(LambdaValueInformation);
};


class DebugableObjectInformation : public DebugInformation
{
public:
	DebugableObjectInformation(DebugableObjectBase *object_, const Identifier &id_, Type t, const Identifier& namespaceId_=Identifier(), const String& comment_=String()) :
		DebugInformation(t),
		object(object_),
		id(id_),
		namespaceId(namespaceId_)
	{
		if (comment_.isNotEmpty())
		{
			comment.append(comment_, GLOBAL_FONT(), Colours::white);
		}
	};

	String getTextForDataType() const override { return object != nullptr ? object->getDebugDataType() : ""; }
	String getTextForName() const override 
	{ 
		if (object == nullptr)
			return "";

		return namespaceId.isNull() ? object->getDebugName() :
									  namespaceId.toString() + "." + object->getDebugName(); 
	}
	String getTextForValue() const override { return object != nullptr ? object->getDebugValue() : ""; }
	AttributedString getDescription() const override 
	{ return comment; }

	bool isWatchable() const override { return object != nullptr ? object->isWatchable() : false; }

	int getNumChildElements() const override
	{
		if (object != nullptr)
		{
			auto o = object->getNumChildElements();

			if (o != -1)
				return o;

			
		}

		return 0;
	}

	Ptr getChildElement(int index) override
	{
		if (object != nullptr)
			return object->getChildElement(index);
			
		return nullptr;
	}

	DebugableObjectBase *getObject() override { return object.get(); }
	const DebugableObjectBase *getObject() const override { return object.get(); }

	AttributedString comment;
	WeakReference<DebugableObjectBase> object;
	const Identifier id;
	const Identifier namespaceId;
};

} // namespace hise
#endif  // DEBUGHELPERS_H_INCLUDED
