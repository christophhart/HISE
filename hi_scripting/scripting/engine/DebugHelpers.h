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
		static AttributedString getFunctionDoc(const String &docBody, const Array<Identifier> &parameters);

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

	DebugInformation(Type t);;

	virtual ~DebugInformation();;

	static String varArrayToString(const Array<var> &arrayToStringify);;

	String getTextForDataType() const override;

	virtual const var getVariantCopy() const;;

	virtual AttributedString getDescription() const;;

	String getCodeToInsert() const override;

	Component* createPopupComponent(const MouseEvent& e, Component* componentToNotify) override;

	virtual void doubleClickCallback(const MouseEvent &e, Component* componentToNotify);

	String getTextForType() const override;

	int getType() const;

	String getTextForRow(Row r);
	String toString();

	

	//DebugableObject::Location location;

protected:

	String getVarValue(const var &v) const;

	static DebugableObjectBase *getDebugableObject(const var &v);

private:

	Type type;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DebugInformation)
};

class DynamicObjectDebugInformation : public DebugInformation
{
public:

	DynamicObjectDebugInformation(DynamicObject *obj_, const Identifier &id_, Type t);

	~DynamicObjectDebugInformation();

	bool isWatchable() const override;

	String getTextForName() const override;

	String getTextForDataType() const override;

	String getTextForValue() const override;

	const var getVariantCopy() const override;;

	AttributedString getDescription() const override;;

	DebugableObjectBase *getObject() override;

	DynamicObject::Ptr obj;
	const Identifier id;

	ScopedPointer<DynamicDebugableObjectWrapper> wrapper;
};


class LambdaValueInformation : public DebugInformation
{
public:

	using ValueFunction = std::function<var()>;

	LambdaValueInformation(const ValueFunction& f, const Identifier &id_, const Identifier& namespaceId_, Type t, DebugableObjectBase::Location location_, const String& comment_=String());

	DebugableObjectBase::Location getLocation() const;

	AttributedString getDescription() const override;

	String getTextForDataType() const;

	String getTextForName() const;

	int getNumChildElements() const;


	DebugInformation::Ptr getChildElement(int index);

	var getCachedValueFunction(bool forceLookup) const;

	bool isAutocompleteable() const;

	void setAutocompleteable(bool shouldBe);

	const var getVariantCopy() const override;;

	String getTextForValue() const;
	DebugableObjectBase *getObject();

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
	DebugableObjectInformation(DebugableObjectBase *object_, const Identifier &id_, Type t, const Identifier& namespaceId_=Identifier(), const String& comment_=String());
	;

	String getTextForDataType() const;

	String getTextForName() const;
	String getTextForValue() const;

	AttributedString getDescription() const override;

	bool isWatchable() const;

	int getNumChildElements() const;

	Ptr getChildElement(int index);

	DebugableObjectBase *getObject();
	const DebugableObjectBase *getObject() const;

	AttributedString comment;
	WeakReference<DebugableObjectBase> object;
	const Identifier id;
	const Identifier namespaceId;
};

} // namespace hise
#endif  // DEBUGHELPERS_H_INCLUDED
