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

#ifndef SCRIPTINGBASEOBJECTS_H_INCLUDED
#define SCRIPTINGBASEOBJECTS_H_INCLUDED

namespace hise { using namespace juce;

class ModulatorSynthGroup;
class ProcessorWithScriptingContent;
class JavascriptMidiProcessor;

class ScriptParameterHandler
{
public:

	virtual ~ScriptParameterHandler() {}

	virtual Identifier getParameterId(int index) const = 0;
	virtual int getNumParameters() const = 0;
	virtual void setParameter(int index, float newValue) = 0;
	virtual float getParameter(int index) const = 0;
};

/** The base class for all scripting API classes. 
*	@ingroup scripting
*
*	It contains some basic methods for error handling.
*/
class ScriptingObject
{
public:

	virtual ~ScriptingObject() {};

	/** \internal sanity check if the function call contains the correct amount of arguments. */
	bool checkArguments(const String &callName, int numArguments, int expectedArgumentAmount);
	
	/** \internal sanity check if all arguments are valid. 
	*
	*	It returns -1 if everything is ok or the index of the faulty argument. 
	*/
	int checkValidArguments(const var::NativeFunctionArgs &args);

	/** \internal Checks if the callback is made on the audio thread (check this with every API call that changes the MidiMessage. */
	bool checkIfSynchronous(const Identifier &callbackName) const;


	ProcessorWithScriptingContent *getScriptProcessor();
	const ProcessorWithScriptingContent *getScriptProcessor() const;

	Processor* getProcessor() { return thisAsProcessor; };
	const Processor* getProcessor() const { return thisAsProcessor; }

protected:

	ScriptingObject(ProcessorWithScriptingContent *p);


	/** \internal Prints a error in the script to the console. */
	void reportScriptError(const String &errorMessage) const;

	/** \internal Prints a specially formatted message if the function is not allowed to be called in this callback. */
	void reportIllegalCall(const String &callName, const String &allowedCallback) const;
	
    void logErrorAndContinue(const String& errorMessage) const;
    
private:

	WeakReference<ProcessorWithScriptingContent> processor;	
	Processor* thisAsProcessor;
};

class EffectProcessor;


class ConstScriptingObject : public ScriptingObject,
							 public ApiClass
{
public:

	ConstScriptingObject(ProcessorWithScriptingContent* p, int numConstants):
		ScriptingObject(p),
		ApiClass(numConstants)
	{

	}

	virtual Identifier getObjectName() const = 0;
	Identifier getInstanceName() const override { return name.isValid() ? name : getObjectName(); }

	/** \internal Overwrite this method and check if the object got deleted. Best thing is to use a WeakReference and check if it's nullptr. */
	virtual bool objectDeleted() const { return false; }

	/** \internal Overwrite this method and check if the object exists. Best thing is to initialize the pointer to nullptr and check that. */
	virtual bool objectExists() const { return false; }

	/** \internal This method combines the calls to objectDeleted() and objectExists() and creates a nice error message. */
	bool checkValidObject() const
	{
		if (!objectExists())
		{
			reportScriptError(getObjectName().toString() + " " + getInstanceName() + " does not exist.");
			RETURN_IF_NO_THROW(false)
		}

		if (objectDeleted())
		{
			reportScriptError(getObjectName().toString() + " " + getInstanceName() + " was deleted");
			RETURN_IF_NO_THROW(false)
		}

		return true;
	}

	void setName(const Identifier &name_) noexcept{ name = name_; };

private:

	Identifier name;

	JUCE_DECLARE_WEAK_REFERENCEABLE(ConstScriptingObject);
};

/** The base class for all objects that can be created by a script. 
*	@ingroup scripting
*
*	This class should be used whenever a complex data object should be created and used from within the script.
*	You need to overwrite the objectDeleted() and objectExists() methods.
*	From then on, you can use the checkValidObject() function within methods for sanity checking.
*/
class DynamicScriptingObject: public ScriptingObject,
							 public DynamicObject
{
public:

	DynamicScriptingObject(ProcessorWithScriptingContent *p):
		ScriptingObject(p)
	{
		setMethod("exists", Wrappers::checkExists);
		
	};

	virtual ~DynamicScriptingObject() {};

	/** \internal Overwrite this method and return the class name of the object which will be used in the script context. */
	virtual Identifier getObjectName() const = 0;

	String getInstanceName() const { return name; }

protected:

	/** \internal Overwrite this method and check if the object got deleted. Best thing is to use a WeakReference and check if it's nullptr. */
	virtual bool objectDeleted() const = 0;

	/** \internal Overwrite this method and check if the object exists. Best thing is to initialize the pointer to nullptr and check that. */
	virtual bool objectExists() const = 0;

	/** \internal This method combines the calls to objectDeleted() and objectExists() and creates a nice error message. */
	bool checkValidObject() const
	{
		if(!objectExists())
		{
			reportScriptError(getObjectName().toString() + " " + getInstanceName() + " does not exist.");
			RETURN_IF_NO_THROW(false)
		}

		if(objectDeleted())
		{
			reportScriptError(getObjectName().toString() + " " + getInstanceName() + " was deleted");	
			RETURN_IF_NO_THROW(false)
		}

		return true;
	}

	void setName(const String &name_) noexcept { name = name_; };

private:

	String name;

	struct Wrappers
	{
		static var checkExists(const var::NativeFunctionArgs& args);
	};

};

class HiseJavascriptEngine;


/** This object can hold a function that can be called asynchronously on the scripting thread. 

	If you pass in a callback to an object / function that should be executed on certain C++ events,
	use this wrapper around the function and it will:
	
	- automatically call it on the scripting thread
	- make sure that the function will not be executed when the script was recompiled
	- avoid cyclic references that might lead to memory leaks

	You can delete the object right after calling `call`...
	*/
struct WeakCallbackHolder : private ScriptingObject
{
	WeakCallbackHolder(ProcessorWithScriptingContent* p, const var& callback, int numExpectedArgs);

	/** @internal: used by the scripting thread. */
	WeakCallbackHolder(const WeakCallbackHolder& copy);

	WeakCallbackHolder(WeakCallbackHolder&& other);

	~WeakCallbackHolder();

	WeakCallbackHolder& operator=(WeakCallbackHolder&& other);

	/** Call the function with the given arguments. */
	void call(var* arguments, int numArgs);

	/** Call the functions synchronously. */
	Result callSync(var* arguments, int numArgs, var* returnValue=nullptr);

	/** Call the function with one argument that can be converted to a var. */
	template <typename T> void call1(const T& arg1)
	{
		var a(arg1);
		call(&a, 1);
	}

	/** Calls the function with any iteratable var container. */
	template <typename ContainerType> void call(const ContainerType& t)
	{
		// C++ Motherf%!(%...
		call(const_cast<var*>(&*t.begin()), (int)t.size());
	}

	void call(Array<var> v)
	{
		call(const_cast<var*>(v.getRawDataPointer()), v.size());
	}

	/** @internal: used by the scripting thread. */
	Result operator()(JavascriptProcessor* p);

	void setHighPriority()
	{
		highPriority = true;
	}

	/** Increases the reference count for the callback object. 
		Use this in order to assure the liveness of the callback, but beware of leaking. 
	*/
	void incRefCount()
	{
		anonymousFunctionRef = var(dynamic_cast<ReferenceCountedObject*>(weakCallback.get()));
	}

	

	DebugInformationBase* createDebugObject(const String& n) const;

	void decRefCount()
	{
		anonymousFunctionRef = var();
	}

	void clear();

	operator bool() const
	{
		return weakCallback.get() != nullptr && engineToUse.get() != nullptr;
	}

	void setThisObject(ReferenceCountedObject* thisObj)
	{
		thisObject = dynamic_cast<DebugableObjectBase*>(thisObj);
	}

	bool matches(const var& f) const;

private:

	bool highPriority = false;
	int numExpectedArgs;
	Result r;
	Array<var> args;
	var anonymousFunctionRef;
	WeakReference<DebugableObjectBase> weakCallback;
	WeakReference<DebugableObjectBase> thisObject;
	ReferenceCountedObject* castedObj = nullptr;
	WeakReference<HiseJavascriptEngine> engineToUse;
};


/** @internal A interface class for objects that can be used with the [] operator in Javascript.
	
*
*	It uses a cached look up index on compilation to accelerate the look up.
*/
class AssignableObject
{
public:

	/** Use this to create a child data in the watch table. */
	struct IndexedValue
	{
		IndexedValue(AssignableObject* obj_, int idx): index(idx), obj(obj_) {}

		var operator()()
		{
			if (obj.get() != nullptr)
				return obj->getAssignedValue(index);

			return var();
		}

		Identifier getId() const
		{
			String s = "%PARENT%";
			s << "[" << String(index) << "]";
			return Identifier(s);
		}

	private:

		const int index;
		WeakReference<AssignableObject> obj;
	};

	virtual ~AssignableObject() {};

	/** Assign the value to the specified index. The parameter passed in must relate to the index created with getCachedIndex. */
	virtual void assign(const int index, var newValue) = 0;

	/** Return the value for the specified index. The parameter passed in must relate to the index created with getCachedIndex. */
	virtual var getAssignedValue(int index) const = 0;

	/** Overwrite this and return an index that can be used to look up the value when the script is executed. */
	virtual int getCachedIndex(const var &indexExpression) const = 0;

	JUCE_DECLARE_WEAK_REFERENCEABLE(AssignableObject);
};


struct JSONConversionHelpers
{
	static var valueTreeToJSON(const ValueTree& v);

	static ValueTree jsonToValueTree(var data, const Identifier& typeId, bool isParentData = true);

	static var convertBase64Data(const String& d, const ValueTree& cTree);

	static String convertDataToBase64(const var& d, const ValueTree& cTree);
};



struct ValueTreeConverters
{
	static String convertDynamicObjectToBase64(const var& object, const Identifier& id, bool compress);;

	static ValueTree convertDynamicObjectToValueTree(const var& object, const Identifier& id);

	static String convertValueTreeToBase64(const ValueTree& v, bool compress);

	static var convertBase64ToDynamicObject(const String& base64String, bool isCompressed);

	static ValueTree convertBase64ToValueTree(const String& base64String, bool isCompressed);

	static var convertValueTreeToDynamicObject(const ValueTree& v);

	static var convertFlatValueTreeToVarArray(const ValueTree& v);

	static ValueTree convertVarArrayToFlatValueTree(const var& ar, const Identifier& rootId, const Identifier& childId);

	static void copyDynamicObjectPropertiesToValueTree(ValueTree& v, const var& obj, bool skipArray=false);

	static void copyValueTreePropertiesToDynamicObject(const ValueTree& v, var& obj);

	static var convertContentPropertiesToDynamicObject(const ValueTree& v);

	static ValueTree convertDynamicObjectToContentProperties(const var& d);

	static var convertScriptNodeToDynamicObject(ValueTree v);

	static ValueTree convertDynamicObjectToScriptNodeTree(var obj);

private:

	static void v2d_internal(var& object, const ValueTree& v);

	static void d2v_internal(ValueTree& v, const Identifier& id, const var& object);;


};

namespace fixobj
{
struct LayoutBase: public ReferenceCountedObject
{
    
    
    enum class DataType
    {
        Integer,
        Boolean,
        Float,
        numTypes
    };
    
    struct Helpers
    {
        static void writeElement(DataType type, uint8* dataWithOffset, const var& newValue)
        {
            switch (type)
            {
            case DataType::Integer: *reinterpret_cast<int*>(dataWithOffset) = (int)newValue; break;
            case DataType::Float:    *reinterpret_cast<float*>(dataWithOffset) = (float)newValue; break;
            case DataType::Boolean: *reinterpret_cast<int*>(dataWithOffset) = (int)(bool)newValue; break;
            }
        }

        static var getElement(DataType type, const uint8* dataWithOffset)
        {
            switch (type)
            {
            case DataType::Integer: return var(*reinterpret_cast<const int*>(dataWithOffset));
            case DataType::Float:    return var(*reinterpret_cast<const float*>(dataWithOffset));
            case DataType::Boolean: return var(*reinterpret_cast<const int*>(dataWithOffset) != 0);
            default:                jassertfalse; return var();
            }
        }

        static DataType getTypeFromVar(const var& value, Result* r)
        {
            if (value.isArray())
                return getTypeFromVar(value[0], r);

            if (value.isInt() || value.isInt64())
                return DataType::Integer;

            if (value.isDouble())
                return DataType::Float;

            if (value.isBool())
                return DataType::Boolean;

            if (r != nullptr)
                *r = Result::fail("illegal data type: \"" + value.toString() + "\"");

            return DataType::numTypes;
        }

        static int getElementSizeFromVar(const var& value, Result* r)
        {
            if (value.isArray())
                return value.size();

            if (value.isObject() || value.isString())
            {
                if (r != nullptr)
                    *r = Result::fail("illegal type");
            }

            return 1;
        }

        static uint32 getTypeSize(DataType type)
        {
            switch (type)
            {
            case DataType::Integer: return sizeof(int);
            case DataType::Boolean: return sizeof(int);
            case DataType::Float:   return sizeof(float);
            }
        }
    };

    virtual ~LayoutBase() {};
    
    
    
protected:

    LayoutBase() :
        initResult(Result::ok())
    {
    };

    struct MemoryLayoutItem: public ReferenceCountedObject
    {
        using Ptr = ReferenceCountedObjectPtr<MemoryLayoutItem>;
        using List = ReferenceCountedArray<MemoryLayoutItem>;

        MemoryLayoutItem(uint32 offset_, const Identifier& id_, var defaultValue_, Result* r) :
            id(id_),
            type(Helpers::getTypeFromVar(defaultValue_, r)),
            elementSize(Helpers::getElementSizeFromVar(defaultValue_, r)),
            offset(offset_),
            defaultValue(defaultValue_)
        {

        }

        void resetToDefaultValue(uint8* dataStart)
        {
            write(dataStart, defaultValue, nullptr);
        }

        int getByteSize() const { return Helpers::getTypeSize(type) * elementSize; }

        void writeArrayElement(uint8* dataStart, int index, const var& newValue, Result* r)
        {
            if (isPositiveAndBelow(index, elementSize - 1))
            {
                Helpers::writeElement(type, dataStart + Helpers::getTypeSize(type) * index, newValue);
            }
            else
            {
                if (r != nullptr)
                    *r = Result::fail("out of bounds");
            }
        }

        var getData(uint8* dataStart, Result* r) const
        {
            if (elementSize == 1)
            {
                return Helpers::getElement(type, dataStart + offset);
            }

            if (r != nullptr)
                *r = Result::fail("Can't get reference to fix array");

            return var();
        }

        void write(uint8* dataStart, const var& newValue, Result* r)
        {
            if (elementSize == 1)
            {
                if (newValue.isArray())
                {
                    if (r != nullptr)
                        *r = Result::fail("Can't write array to single element");

                    return;
                }

                Helpers::writeElement(type, dataStart + offset, newValue);
            }
            else
            {
                if (auto ar = newValue.getArray())
                {
                    auto numElementsToRead = ar->size();

                    if (elementSize != numElementsToRead)
                    {
                        if (r != nullptr)
                            *r = Result::fail("array size mismatch. Expected " + String(elementSize));

                        return;
                    }

                    auto ts = Helpers::getTypeSize(type);

                    for (int i = 0; i < numElementsToRead; i++)
                    {
                        auto d = dataStart + i * ts;
                        Helpers::writeElement(type, d, ar->getUnchecked(i));
                    }
                }
                else
                {
                    if (r != nullptr)
                        *r = Result::fail("This data type requires an array.");
                }
            }
        }

        Identifier id;
        DataType type;
        uint32 offset;
        int elementSize;
        var defaultValue;
    };

    MemoryLayoutItem::List layout;

    static MemoryLayoutItem::List createLayout(var layoutDescription, Result* r = nullptr)
    {
        MemoryLayoutItem::List items;

        if (auto obj = layoutDescription.getDynamicObject())
        {
            uint32 offset = 0;

            for (const auto& prop : obj->getProperties())
            {
                auto id = prop.name;
                auto v = prop.value;

                auto newItem = new MemoryLayoutItem(offset, prop.name, prop.value, r);
                items.add(newItem);
                offset += newItem->getByteSize();
            }
        }

        if (items.isEmpty())
            *r = Result::fail("No data");

        return items;
    }

    friend class ObjectReference;

    Result initResult;
    
    JUCE_DECLARE_WEAK_REFERENCEABLE(LayoutBase);
};

struct ObjectReference : public LayoutBase
{
    using Ptr = ReferenceCountedObjectPtr<ObjectReference>;
    
    ObjectReference()
    {
        reset();
    }

    void reset()
    {
        data = nullptr;
        layoutReference = nullptr;
        elementSize = 0;
        initResult = Result::fail("uninitialised");
    }
    
    ObjectReference& operator=(const ObjectReference& other)
    {
        if (!isValid())
        {
            data = other.data;
            elementSize = other.elementSize;
            layoutReference = other.layoutReference;
            initResult = other.initResult;
        }
        else
        {
            if(!other.isValid())
            {
                reset();
            }
            else
            {
                jassert(other.elementSize == elementSize);
                jassert(layout.size() == other.layout.size());
                memcpy(data, other.data, elementSize);
            }
        }

        return *this;
    }

    bool operator==(const ObjectReference& other) const
    {
        if (data == other.data)
            return true;

        if (layoutReference == other.layoutReference)
        {
            bool same = true;

            for (int i = 0; i < elementSize; i++)
                same &= (other.data[i] == data[i]);

            return true;
        }

        return false;
    }

    ObjectReference(const ObjectReference& other)
    {
        *this = other;
    }

    bool isValid() const
    {
        return layoutReference != nullptr && data != nullptr;
    }
    
    struct MemberReference: public ReferenceCountedObject,
                            public AssignableObject
    {
        using Ptr = ReferenceCountedObjectPtr<MemberReference>;
        
        MemoryLayoutItem::Ptr memberProperties;
        ReferenceCountedArray<MemberReference> arrayMembers;
        uint8* data;
        int indexInArray = -1;
        
        MemberReference(MemoryLayoutItem::Ptr p, uint8* data_, int indexInArray_):
          memberProperties(p),
          data(data_),
          indexInArray(indexInArray_)
        {
            if(p->elementSize > 1 && indexInArray == -1)
            {
                auto ptr = data;
                
                for(int i = 0; i < p->elementSize; i++)
                {
                    arrayMembers.add(new MemberReference(p, ptr, i));
                    ptr += Helpers::getTypeSize(memberProperties->type);
                }
            }
        }
        
        MemberReference& operator=(var newValue)
        {
            if (memberProperties->elementSize == 1 || indexInArray != -1)
                Helpers::writeElement(memberProperties->type, data, newValue);

            return *this;
        }

        MemberReference::Ptr operator[](int index) const
        {
            return arrayMembers[index];
        }

        bool isValid() const
        {
            return data != nullptr;
        }

        void assign(const int index, var newValue) override
        {
            auto c = arrayMembers[index];
            
            *c = newValue;
        }

        /** Return the value for the specified index. The parameter passed in must relate to the index created with getCachedIndex. */
        var getAssignedValue(int index) const override
        {
            if(isPositiveAndBelow(index, arrayMembers.size()))
            {
                return var((var)*arrayMembers[index]);
            }
            
            return var();
        }

        /** Overwrite this and return an index that can be used to look up the value when the script is executed. */
        int getCachedIndex(const var &indexExpression) const override
        {
            return (int)indexExpression;
        }
        
        explicit operator var()
        {
            if(!arrayMembers.isEmpty())
                return var(this);
            
            return isValid() ? Helpers::getElement(memberProperties->type, data) : var();
        }
        
        explicit operator int() const
        {
            return isValid() ? (int)Helpers::getElement(memberProperties->type, data) : 0;
        }

        explicit operator float() const
        {
            return isValid() ? (float)Helpers::getElement(memberProperties->type, data) : 0.0f;
        }

        explicit operator bool() const
        {
            return isValid() ? (bool)Helpers::getElement(memberProperties->type, data) : false;
        }
    };
    
    MemberReference::Ptr operator[](const Identifier& id) const
    {
        return dynamic_cast<MemberReference*>(memberReferences[id].getObject());
    }

    void init(LayoutBase* referencedLayout, uint8* preallocatedData, bool resetToDefault)
    {
        data = preallocatedData;
        layoutReference = referencedLayout;
        
        initResult = Result::ok();

        elementSize = 0;

        if(!isValid())
            return;
        
        for (auto l : layoutReference->layout)
        {
            if(data != nullptr && resetToDefault)
                l->resetToDefaultValue(data + elementSize);

            auto m = new MemberReference(l, data + elementSize, -1);
            memberReferences.set(l->id, var(m));
            
            elementSize += l->getByteSize();
        }
    }

    size_t elementSize = 0;
    uint8* data = nullptr;
    
    WeakReference<LayoutBase> layoutReference;
    NamedValueSet memberReferences;
};

struct SingleObject: public LayoutBase
{
    using Ptr = ReferenceCountedObjectPtr<SingleObject>;

    SingleObject(var dataDescription):
        LayoutBase()
    {
        auto l = createLayout(dataDescription, &initResult);

        init(l);
    }
    
    SingleObject(MemoryLayoutItem::List predefinedLayout)
    {
        init(predefinedLayout);
    }
    
    void init(MemoryLayoutItem::List layoutToUse)
    {
        layout = layoutToUse;
        
        if(initResult.failed())
            layout.clear();
        
        for (auto l : layout)
            numAllocated += l->getByteSize();

        if (numAllocated != 0)
        {
            data.allocate(numAllocated, true);
            reset();
        }
        
        selfReference = new ObjectReference();
        selfReference->init(this, data.get(), false);
    }

    void reset()
    {
        if (numAllocated > 0)
        {
            auto ptr = data.get();

            for (auto l : layout)
            {
                l->resetToDefaultValue(ptr);
                ptr += l->getByteSize();
            }
        }
    }

    operator ObjectReference::Ptr() const
    {
        return selfReference;
    }

    bool isValid() const
    {
        return initResult.wasOk();
    }

private:

    ObjectReference::Ptr selfReference;
    
    HeapBlock<uint8> data;
    size_t numAllocated = 0;
};



struct Stack : public LayoutBase,
               public hise::UnorderedStack<ObjectReference, 128>
{
    Stack(const var& description)
    {
        layout = createLayout(description, &initResult);

        if (initResult.wasOk())
        {
            auto ptr = begin();
            
            for (auto l : layout)
                elementSize += l->getByteSize();

            numAllocated = elementSize * 128;

            if (numAllocated > 0)
            {
                allocatedData.allocate(numAllocated, true);
                auto dataPtr = allocatedData.get();

                for (int i = 0; i < 128; i++)
                {
                    ptr[i].init(this, dataPtr, true);
                    dataPtr += elementSize;
                }
            }
        }
    }

private:

    int numAllocated = 0;
    int elementSize = 0;
    HeapBlock<uint8> allocatedData;
};

struct Array : public LayoutBase,
               public AssignableObject
{
    std::function<int(ObjectReference::Ptr, ObjectReference::Ptr)> compareFunction;
    
    Array(const var& description, int numElements):
        LayoutBase()
    {
        auto l = createLayout(description, &initResult);
        init(l, numElements);
    }
    
    Array(MemoryLayoutItem::List externalLayout, int numElements)
    {
        init(externalLayout, numElements);
    };
    
    void init(MemoryLayoutItem::List newL, int numElements)
    {
        layout = newL;
        
        if (!initResult.wasOk())
            layout.clear();

        for (auto l : layout)
            elementSize += l->getByteSize();

        numAllocated = elementSize * numElements;

        if (numAllocated > 0)
        {
            data.allocate(numAllocated, true);

            for (int i = 0; i < numElements; i++)
            {
                auto ptr = data.get() + i * elementSize;

                uint32 offset = 0;

                auto obj = new ObjectReference();
                obj->init(this, ptr, true);
                items.add(obj);
            }
        }

    }
    
    void assign(const int index, var newValue) override
    {
        if(auto fo = dynamic_cast<ObjectReference*>(newValue.getObject()))
        {
            if(auto i = items[index])
            {
                *i = *fo;
            }
        }
    }

    /** Return the value for the specified index. The parameter passed in must relate to the index created with getCachedIndex. */
    var getAssignedValue(int index) const override
    {
        if(isPositiveAndBelow(index, items.size()))
        {
            return var(items[index]);
        }
        
        return var();
    }

    /** Overwrite this and return an index that can be used to look up the value when the script is executed. */
    int getCachedIndex(const var &indexExpression) const override
    {
        return (int)indexExpression;
    }

    void fill(ObjectReference::Ptr obj)
    {
        if(obj != nullptr)
        {
            for(auto i: items)
                *i = *obj;
        }
        else
        {
            for(auto i: items)
                i->reset();
        }
    }
    
    void clear()
    {
        fill(nullptr);
        
    }
    
    int indexOf(ObjectReference::Ptr obj)
    {
        int index = 0;
        
        for(auto i: items)
        {
            if(compareFunction && (compareFunction(i, obj) == 0))
                return index;
                
            if(*i == *obj)
                return index;
            
            index++;
        }
        
        return -1;
    }
    
    ObjectReference::Ptr operator[](int index) const
    {
        if (isPositiveAndBelow(index, items.size()))
            return items[index];

        return {};
    }

private:
    
    size_t elementSize = 0;
    size_t numElements = 0;
    size_t numAllocated = 0;
    ReferenceCountedArray<ObjectReference> items;
    HeapBlock<uint8> data;
};

struct Factory: public LayoutBase
{
    Factory(const var& d)
    {
        layout = createLayout(d, &initResult);
    }

    var createSingleObject()
    {
        if(initResult.wasOk())
        {
            auto newElement = new SingleObject(layout);
            singleObjects.add(newElement);
            
            auto ref = (ObjectReference::Ptr)(*newElement);
            
            return var(ref);
        }

        return {};
    }
    
    var createArray(int numElements)
    {
        if(initResult.wasOk())
        {
            auto newElement = new Array(layout, numElements);
            newElement->compareFunction = compareFunction;
            arrays.add(newElement);
            return var(newElement);
        }

        return {};
    }
    
    std::function<int(ObjectReference::Ptr, ObjectReference::Ptr)> compareFunction;
    
    ReferenceCountedArray<SingleObject> singleObjects;
    ReferenceCountedArray<Array> arrays;
    ReferenceCountedArray<Stack> stacks;
};
}


} // namespace hise
#endif  // SCRIPTINGBASEOBJECTS_H_INCLUDED
