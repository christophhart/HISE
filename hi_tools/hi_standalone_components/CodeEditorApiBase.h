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
*   which must be separately licensed for closed source applications:
*
*   http://www.juce.com
*
*   ===========================================================================
*/

#pragma once

namespace hise { using namespace juce;


struct ValueTreeApiHelpers
{
	static AttributedString createAttributedStringFromApi(const ValueTree &method, const String &, bool multiLine, Colour textColour);

	static String createCodeToInsert(const ValueTree &method, const String &className);

	static void getColourAndCharForType(int type, char &c, Colour &colour);



};

class DebugInformationBase;

class DebugableObjectBase
{
public:

	struct Location
	{
		operator bool() const { return charNumber != 0 || fileName.isNotEmpty(); }
		bool operator==(const Location& other) { return other.charNumber == charNumber && fileName == other.fileName; }
		String fileName = String();
		int charNumber = 0;
	};

	virtual ~DebugableObjectBase() {};

	/** Override this and return the class id of this object. */
	virtual Identifier getObjectName() const = 0;

	/** This will be shown as value of the object. */
	virtual String getDebugValue() const { return ""; };

	/** This will be shown as name of the object. */
	virtual String getDebugName() const { return getInstanceName().toString(); };

	virtual String getDebugDataType() const { return getDebugName(); }

	virtual AttributedString getDescription() const
	{
		return AttributedString();
	}

	virtual String getCategory() const { return ""; }

	/** Override this if you don't want this object to show up in the list. */
	virtual bool isInternalObject() const { return false; }

	virtual int getTypeNumber() const { return 0; }

	virtual bool isWatchable() const { return true; }

	virtual bool isAutocompleteable() const { return true; }

	/** Override this and return something else than -1 for a custom child layout. */
	virtual int getNumChildElements() const { return -1; };

	virtual DebugInformationBase* getChildElement(int index) { return nullptr; }

	virtual Identifier getInstanceName() const { return getObjectName(); }

	/** This can be used to override the debug information created for one of its objects children (constant or function). If you do so, the object created here will be owned by the caller. */
	virtual DebugInformationBase* createDebugInformationForChild(const Identifier& id)
	{
		ignoreUnused(id);
		return nullptr;
	}

	virtual void getAllFunctionNames(Array<Identifier>& functions) const 
	{
		ignoreUnused(functions);
	};

	virtual void getAllConstants(Array<Identifier>& ids) const
	{
		ignoreUnused(ids);
	};

	virtual const var getConstantValue(int index) const 
	{
		ignoreUnused(index);
		return var(); 
	};

	/** This will be called if the user double clicks on the row. */
	virtual void doubleClickCallback(const MouseEvent &e, Component* componentToNotify)
	{
		ignoreUnused(e, componentToNotify);
	};

	void setCurrentExpression(const String& e) { currentExpression = e; }

	/** Override this and return a component that will be shown as popup in the value table. */
	virtual Component* createPopupComponent(const MouseEvent& e, Component* parent) { return nullptr; }

	virtual Location getLocation() const { return Location(); }

	static void updateLocation(Location& l, var possibleObject);

	String currentExpression;

	JUCE_DECLARE_WEAK_REFERENCEABLE(DebugableObjectBase);
};



class DynamicDebugableObjectWrapper : public DebugableObjectBase
{
public:

	DynamicDebugableObjectWrapper(DynamicObject::Ptr obj_, const Identifier& className_, const Identifier& instanceId_);

	virtual Identifier getObjectName() const override;
	virtual Identifier getInstanceName() const override;

	virtual String getDebugValue() const override;

	virtual void getAllFunctionNames(Array<Identifier>& functions) const;;

	virtual void getAllConstants(Array<Identifier>& ids) const;;

	

	virtual const var getConstantValue(int index) const;;

	Identifier className;
	Identifier instanceId;
	DynamicObject::Ptr obj;
};


/** A general representation of an entity that is supposed to be shown in the IDE. */
class DebugInformationBase: public ReferenceCountedObject
{
public:

	using Ptr = ReferenceCountedObjectPtr<DebugInformationBase>;
	using List = ReferenceCountedArray<DebugInformationBase>;

	/** This will be called if the user double clicks on the row. */
	virtual void doubleClickCallback(const MouseEvent &e, Component* componentToNotify);;
	virtual Component* createPopupComponent(const MouseEvent& e, Component* componentToNotify);;
	virtual int getType() const;;
	virtual int getNumChildElements() const;
	virtual Ptr getChildElement(int index);
	virtual String getTextForName() const;
	virtual String getCategory() const;
	virtual DebugableObjectBase::Location getLocation() const;
	virtual String getTextForType() const;
	virtual String getTextForDataType() const;
	virtual String getTextForValue() const;
	virtual bool isWatchable() const;
	virtual bool isAutocompleteable() const;
	virtual String getCodeToInsert() const;;
	virtual AttributedString getDescription() const;
	virtual DebugableObjectBase* getObject();
	virtual const DebugableObjectBase* getObject() const;

	virtual ~DebugInformationBase();;

	static String replaceParentWildcard(const String& id, const String& parentId);

	static String getVarType(const var &v);

	StringArray createTextArray() const;
};


class SettableDebugInfo : public DebugInformationBase
{
public:

	SettableDebugInfo()
	{};

	int typeValue = 0;
	String name;
	String dataType;
	String type;
	String value;
	String codeToInsert;
	String category;
	AttributedString description;

	bool watchable = true;
	bool autocompleteable = true;

	int getType() const { return typeValue; };
	String getTextForName() const { return name; }
	String getTextForType() const { return type; }
	String getTextForDataType() const { return dataType; }
	String getTextForValue() const { return value; }
	String getCodeToInsert() const { return codeToInsert; }
	String getCategory() const { return category; }

	bool isWatchable() const override { return watchable; }
	bool isAutocompleteable() const override { return autocompleteable; }

	AttributedString getDescription() const { return description; }
};



class ObjectDebugInformation : public DebugInformationBase
{
public:

	ObjectDebugInformation(DebugableObjectBase* b, int type_=-1) :
		obj(b),
		type(type_)
	{

	}

	int getType() const override { return type >= 0 ? type : obj->getTypeNumber(); }

	int type;

	virtual String getCodeToInsert() const override 
	{ 
		if(obj != nullptr)
			return obj->getInstanceName().toString(); 

		return {};

	};
	DebugableObjectBase* getObject() override { return obj.get(); }
	const DebugableObjectBase* getObject() const override { return obj.get(); }

	WeakReference<DebugableObjectBase> obj;
};

class ObjectDebugInformationWithCustomName: public ObjectDebugInformation
{
public:

	ObjectDebugInformationWithCustomName(DebugableObjectBase* b, int t, String n) :
		ObjectDebugInformation(b, t),
		name(n)
	{};

	virtual String getTextForName() const
	{
		return name;
	}

	String name;
};


struct ManualDebugObject : public DebugableObjectBase
{
	juce::String getDebugValue() const override { return ""; }
	Identifier getObjectName() const override { return objectName; }
	juce::String getCategory() const override { return category; }

	template <class T> static DebugInformationBase* create()
	{
		return new Holder(new T());
	}

	void getAllConstants(Array<Identifier>& ids) const override
	{
		ids.addArray(childNames);
	}

	DebugInformationBase* createDebugInformationForChild(const Identifier& id) override
	{
		if (childNames.contains(id))
		{
			auto i = new SettableDebugInfo();
			i->category = category;
			fillInfo(i, id.toString());

			return i;
		}

		return nullptr;
	}

protected:

	Identifier objectName;
	Array<Identifier> childNames;
	juce::String category;

	virtual void fillInfo(SettableDebugInfo* childInfo, const juce::String& id) = 0;

private:

	struct Holder : public ObjectDebugInformation
	{
		Holder(DebugableObjectBase* ownedObject) :
			ObjectDebugInformation(nullptr)
		{
			dummy = ownedObject;
			obj = dummy;
		}

		DebugableObjectBase* getObject() override
		{
			return dummy.get();
		}

		const DebugableObjectBase* getObject() const override
		{
			return dummy.get();
		}

		ScopedPointer<DebugableObjectBase> dummy;
	};
};

class JavascriptCodeEditor;

class ApiProviderBase
{
public:

	class ApiComponentBase;

	/** This object is supposed to manage an ApiProviderBase object.

		The lifetime of an ApiProviderBase object might be very short, so this makes sure that
		the getProviderBase() method always returns the most up to date object. */
	struct Holder
	{
		virtual ~Holder();;

		/** Override this method and return a provider if it exists. */
		virtual ApiProviderBase* getProviderBase() = 0;

		void addEditor(Component* editor);

		void removeEditor(Component* editor);

		virtual int getCodeFontSize() const;

		virtual void setActiveEditor(JavascriptCodeEditor* e, CodeDocument::Position pos);

		virtual JavascriptCodeEditor* getActiveEditor();

		/** Override this method and return a ValueTree containing the API documentation. */
		virtual ValueTree createApiTree();;

		virtual void jumpToDefinition(const String& token, const String& namespaceId);

		virtual void rebuild();;

        void sendClearMessage();
        
		virtual bool handleKeyPress(const KeyPress& k, Component* c);

		virtual void addPopupMenuItems(PopupMenu &m, Component* c, const MouseEvent& e);;

		virtual bool performPopupMenuAction(int menuId, Component* c);;

		virtual void handleBreakpoints(const Identifier& codeFile, Graphics& g, Component* c);;

		virtual void handleBreakpointClick(const Identifier& codeFile, CodeEditorComponent& ed, const MouseEvent& e);

		juce::ReadWriteLock& getDebugLock();

		bool shouldReleaseDebugLock() const;

	protected:

		struct CompileDebugLock
		{
			CompileDebugLock(Holder& h);

			~CompileDebugLock();

			Holder& p;
			bool prevValue = false;

			ScopedWriteLock sl;
		};

		struct RepaintUpdater : public AsyncUpdater
		{
			void update(int index);

			void handleAsyncUpdate() override;

			int lastIndex = -1;
			Array<Component::SafePointer<Component>> editors;
		};

		ReadWriteLock debugLock;
		bool wantsToCompile = false;

		friend class ApiComponentBase;

		Array<WeakReference<ApiComponentBase>> registeredComponents;

		RepaintUpdater repaintUpdater;

		JUCE_DECLARE_WEAK_REFERENCEABLE(Holder);
	};

	/** Subclass any component from this interface class and use getProviderBase(). Check for nullptr each time!. */
	class ApiComponentBase
	{
	public:
		ApiProviderBase* getProviderBase();

		virtual void providerWasRebuilt();;

        virtual void providerCleared();;
        
	protected:

		void registerAtHolder();

		void deregisterAtHolder();

		ApiComponentBase(Holder* h);;

		virtual ~ApiComponentBase();

		WeakReference<Holder> holder;

		JUCE_DECLARE_WEAK_REFERENCEABLE(ApiComponentBase);
	};

	virtual ~ApiProviderBase();;

	/** Override this method and return the number of all objects that you want to use. */
	virtual int getNumDebugObjects() const = 0;

	virtual DebugInformationBase::Ptr getDebugInformation(int index) = 0;

	/** Override this method and return a Colour and a (uppercase) letter to be displayed in the autocomplete window for each type. */
	virtual void getColourAndLetterForType(int type, Colour& colour, char& letter);

	/** Override this and return a string that will be displayed when you hover over a token. */
	virtual String getHoverString(const String& token);

	/** Override this method and return the object for the given token.

		The default implementation checks the name and instance id of each registered debug object, but you can overload it with more complex functions.
	*/
	virtual DebugableObjectBase* getDebugObject(const String& token);
};

/** This interface class can be used for components that display a debug information. 
	
	It automatically tries to find a suitable replacement if this component exceeds the
	lifetime of the original debug information object.

	In order to use it, just call getObject(), which creates a intermediate helper class
	that automatically locks the debug lock during its lifetime
*/
struct ComponentForDebugInformation
{
	ComponentForDebugInformation(DebugableObjectBase* obj_, ApiProviderBase::Holder* h);
	virtual ~ComponentForDebugInformation() {};

protected:

	virtual void refresh() {};

	String getTitle() const;

	template <typename T> struct SafeObject
	{
		SafeObject(ReadWriteLock& l, DebugableObjectBase* obj_) :
			lock(l),
			obj(dynamic_cast<T*>(obj_))
		{};

		T* obj;

		operator bool() const
		{
			return obj != nullptr;
		}

		T* operator->() { return obj; }

	private:
		ScopedReadLock lock;
	};

	template <typename T> SafeObject<T> getObject()
	{
		static_assert(std::is_base_of<DebugableObjectBase, T>(), "not a base class");
		search();

		return { holder != nullptr ? holder->getDebugLock() : dummyLock, obj.get() };
	}

private:

	ReadWriteLock dummyLock;

	void search();
	bool searchRecursive(DebugInformationBase* b);

	String expression;
	WeakReference<ApiProviderBase::Holder> holder;
	WeakReference<DebugableObjectBase> obj;
};

} // namespace hise


