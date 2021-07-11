/*
  ==============================================================================

   This file is part of the JUCE library.
   Copyright (c) 2020 - Raw Material Software Limited

   JUCE is an open source library subject to commercial or open-source
   licensing.

   The code included in this file is provided under the terms of the ISC license
   http://www.isc.org/downloads/software-support-policy/isc-license. Permission
   To use, copy, modify, and/or distribute this software for any purpose with or
   without fee is hereby granted provided that the above copyright notice and
   this permission notice appear in all copies.

   JUCE IS PROVIDED "AS IS" WITHOUT ANY WARRANTY, AND ALL WARRANTIES, WHETHER
   EXPRESSED OR IMPLIED, INCLUDING MERCHANTABILITY AND FITNESS FOR PURPOSE, ARE
   DISCLAIMED.

  ==============================================================================
*/

/* This file contains a few helper functions that are used internally but which
   need to be kept away from the public headers because they use obj-C symbols.
*/
namespace juce
{

//==============================================================================
inline String nsStringToJuce (NSString* s)
{
    return CharPointer_UTF8 ([s UTF8String]);
}

inline NSString* juceStringToNS (const String& s)
{
    return [NSString stringWithUTF8String: s.toUTF8()];
}

inline NSString* nsStringLiteral (const char* const s) noexcept
{
    return [NSString stringWithUTF8String: s];
}

inline NSString* nsEmptyString() noexcept
{
    return [NSString string];
}

inline NSURL* createNSURLFromFile (const String& f)
{
    return [NSURL fileURLWithPath: juceStringToNS (f)];
}

inline NSURL* createNSURLFromFile (const File& f)
{
    return createNSURLFromFile (f.getFullPathName());
}

inline NSArray* createNSArrayFromStringArray (const StringArray& strings)
{
    auto array = [[NSMutableArray alloc] init];

    for (auto string: strings)
        [array addObject:juceStringToNS (string)];

    return [array autorelease];
}

inline NSArray* varArrayToNSArray (const var& varToParse);

inline NSDictionary* varObjectToNSDictionary (const var& varToParse)
{
    auto dictionary = [NSMutableDictionary dictionary];

    if (varToParse.isObject())
    {
        auto* dynamicObject = varToParse.getDynamicObject();

        auto& properties = dynamicObject->getProperties();

        for (int i = 0; i < properties.size(); ++i)
        {
            auto* keyString = juceStringToNS (properties.getName (i).toString());

            const var& valueVar = properties.getValueAt (i);

            if (valueVar.isObject())
            {
                auto* valueDictionary = varObjectToNSDictionary (valueVar);

                [dictionary setObject: valueDictionary forKey: keyString];
            }
            else if (valueVar.isArray())
            {
                auto* valueArray = varArrayToNSArray (valueVar);

                [dictionary setObject: valueArray forKey: keyString];
            }
            else
            {
                auto* valueString = juceStringToNS (valueVar.toString());

                [dictionary setObject: valueString forKey: keyString];
            }
        }
    }

    return dictionary;
}

inline NSArray* varArrayToNSArray (const var& varToParse)
{
    jassert (varToParse.isArray());

    if (! varToParse.isArray())
        return nil;

    const auto* varArray = varToParse.getArray();

    auto array = [NSMutableArray arrayWithCapacity: (NSUInteger) varArray->size()];

    for (const auto& aVar : *varArray)
    {
        if (aVar.isObject())
        {
            auto* valueDictionary = varObjectToNSDictionary (aVar);

            [array addObject: valueDictionary];
        }
        else if (aVar.isArray())
        {
            auto* valueArray = varArrayToNSArray (aVar);

            [array addObject: valueArray];
        }
        else
        {
            auto* valueString = juceStringToNS (aVar.toString());

            [array addObject: valueString];
        }
    }

    return array;
}

var nsObjectToVar (NSObject* array);

inline var nsDictionaryToVar (NSDictionary* dictionary)
{
    DynamicObject::Ptr dynamicObject (new DynamicObject());

    for (NSString* key in dictionary)
        dynamicObject->setProperty (nsStringToJuce (key), nsObjectToVar (dictionary[key]));

    return var (dynamicObject.get());
}

inline var nsArrayToVar (NSArray* array)
{
    Array<var> resultArray;

    for (id value in array)
        resultArray.add (nsObjectToVar (value));

    return var (resultArray);
}

inline var nsObjectToVar (NSObject* obj)
{
    if ([obj isKindOfClass: [NSString class]])          return nsStringToJuce ((NSString*) obj);
    else if ([obj isKindOfClass: [NSNumber class]])     return nsStringToJuce ([(NSNumber*) obj stringValue]);
    else if ([obj isKindOfClass: [NSDictionary class]]) return nsDictionaryToVar ((NSDictionary*) obj);
    else if ([obj isKindOfClass: [NSArray class]])      return nsArrayToVar ((NSArray*) obj);
    else
    {
        // Unsupported yet, add here!
        jassertfalse;
    }

    return {};
}

#if JUCE_MAC
template <typename RectangleType>
NSRect makeNSRect (const RectangleType& r) noexcept
{
    return NSMakeRect (static_cast<CGFloat> (r.getX()),
                       static_cast<CGFloat> (r.getY()),
                       static_cast<CGFloat> (r.getWidth()),
                       static_cast<CGFloat> (r.getHeight()));
}
#endif

#if JUCE_INTEL
 template <typename T>
 struct NeedsStret
 {
    #if JUCE_32BIT
     static constexpr auto value = sizeof (T) > 8;
    #else
     static constexpr auto value = sizeof (T) > 16;
    #endif
 };

 template <>
 struct NeedsStret<void> { static constexpr auto value = false; };

 template <typename T, bool b = NeedsStret<T>::value>
 struct MetaSuperFn { static constexpr auto value = objc_msgSendSuper_stret; };

 template <typename T>
 struct MetaSuperFn<T, false> { static constexpr auto value = objc_msgSendSuper; };
#else
 template <typename>
 struct MetaSuperFn { static constexpr auto value = objc_msgSendSuper; };
#endif

template <typename SuperType, typename ReturnType, typename... Params>
static ReturnType ObjCMsgSendSuper (id self, SEL sel, Params... params)
{
    using SuperFn = ReturnType (*) (struct objc_super*, SEL, Params...);
    const auto fn = reinterpret_cast<SuperFn> (MetaSuperFn<ReturnType>::value);

    objc_super s = { self, [SuperType class] };
    return fn (&s, sel, params...);
}

//==============================================================================
struct NSObjectDeleter
{
    void operator()(NSObject* object) const
    {
        [object release];
    }
};

//==============================================================================
template <typename SuperclassType>
struct ObjCClass
{
    ObjCClass (const char* nameRoot)
        : cls (objc_allocateClassPair ([SuperclassType class], getRandomisedName (nameRoot).toUTF8(), 0))
    {
    }

    ~ObjCClass()
    {
        auto kvoSubclassName = String ("NSKVONotifying_") + class_getName (cls);

        if (objc_getClass (kvoSubclassName.toUTF8()) == nullptr)
            objc_disposeClassPair (cls);
    }

    void registerClass()
    {
        objc_registerClassPair (cls);
    }

    SuperclassType* createInstance() const
    {
        return class_createInstance (cls, 0);
    }

    template <typename Type>
    void addIvar (const char* name)
    {
        BOOL b = class_addIvar (cls, name, sizeof (Type), (uint8_t) rint (log2 (sizeof (Type))), @encode (Type));
        jassert (b); ignoreUnused (b);
    }

    template <typename FunctionType>
    void addMethod (SEL selector, FunctionType callbackFn, const char* signature)
    {
        BOOL b = class_addMethod (cls, selector, (IMP) callbackFn, signature);
        jassert (b); ignoreUnused (b);
    }

    template <typename FunctionType>
    void addMethod (SEL selector, FunctionType callbackFn, const char* sig1, const char* sig2)
    {
        addMethod (selector, callbackFn, (String (sig1) + sig2).toUTF8());
    }

    template <typename FunctionType>
    void addMethod (SEL selector, FunctionType callbackFn, const char* sig1, const char* sig2, const char* sig3)
    {
        addMethod (selector, callbackFn, (String (sig1) + sig2 + sig3).toUTF8());
    }

    template <typename FunctionType>
    void addMethod (SEL selector, FunctionType callbackFn, const char* sig1, const char* sig2, const char* sig3, const char* sig4)
    {
        addMethod (selector, callbackFn, (String (sig1) + sig2 + sig3 + sig4).toUTF8());
    }

    void addProtocol (Protocol* protocol)
    {
        BOOL b = class_addProtocol (cls, protocol);
        jassert (b); ignoreUnused (b);
    }

    template <typename ReturnType, typename... Params>
    static ReturnType sendSuperclassMessage (id self, SEL sel, Params... params)
    {
        return ObjCMsgSendSuper<SuperclassType, ReturnType, Params...> (self, sel, params...);
    }

    template <typename Type>
    static Type getIvar (id self, const char* name)
    {
        void* v = nullptr;
        object_getInstanceVariable (self, name, &v);
        return static_cast<Type> (v);
    }

    Class cls;

private:
    static String getRandomisedName (const char* root)
    {
        return root + String::toHexString (juce::Random::getSystemRandom().nextInt64());
    }

    JUCE_DECLARE_NON_COPYABLE (ObjCClass)
};

//==============================================================================
#ifndef DOXYGEN
template <class JuceClass>
struct ObjCLifetimeManagedClass : public ObjCClass<NSObject>
{
    ObjCLifetimeManagedClass()
        : ObjCClass<NSObject> ("ObjCLifetimeManagedClass_")
    {
        addIvar<JuceClass*> ("cppObject");

        JUCE_BEGIN_IGNORE_WARNINGS_GCC_LIKE ("-Wundeclared-selector")
        addMethod (@selector (initWithJuceObject:), initWithJuceObject, "@@:@");
        JUCE_END_IGNORE_WARNINGS_GCC_LIKE

        addMethod (@selector (dealloc),             dealloc,            "v@:");

        registerClass();
    }

    static id initWithJuceObject (id _self, SEL, JuceClass* obj)
    {
        NSObject* self = sendSuperclassMessage<NSObject*> (_self, @selector (init));
        object_setInstanceVariable (self, "cppObject", obj);

        return self;
    }

    static void dealloc (id _self, SEL)
    {
        if (auto* obj = getIvar<JuceClass*> (_self, "cppObject"))
        {
            delete obj;
            object_setInstanceVariable (_self, "cppObject", nullptr);
        }

        sendSuperclassMessage<void> (_self, @selector (dealloc));
    }

    static ObjCLifetimeManagedClass objCLifetimeManagedClass;
};

template <typename Class>
ObjCLifetimeManagedClass<Class> ObjCLifetimeManagedClass<Class>::objCLifetimeManagedClass;
#endif

// this will return an NSObject which takes ownership of the JUCE instance passed-in
// This is useful to tie the life-time of a juce instance to the life-time of an NSObject
template <typename Class>
NSObject* createNSObjectFromJuceClass (Class* obj)
{
    JUCE_BEGIN_IGNORE_WARNINGS_GCC_LIKE ("-Wobjc-method-access")
    return [ObjCLifetimeManagedClass<Class>::objCLifetimeManagedClass.createInstance() initWithJuceObject:obj];
    JUCE_END_IGNORE_WARNINGS_GCC_LIKE
}

// Get the JUCE class instance that was tied to the life-time of an NSObject with the
// function above
template <typename Class>
Class* getJuceClassFromNSObject (NSObject* obj)
{
    return obj != nullptr ? ObjCLifetimeManagedClass<Class>:: template getIvar<Class*> (obj, "cppObject") : nullptr;
}

template <typename ReturnT, class Class, typename... Params>
ReturnT (^CreateObjCBlock(Class* object, ReturnT (Class::*fn)(Params...))) (Params...)
{
    __block Class* _this = object;
    __block ReturnT (Class::*_fn)(Params...) = fn;

    return [[^ReturnT (Params... params) { return (_this->*_fn) (params...); } copy] autorelease];
}

template <typename BlockType>
class ObjCBlock
{
public:
    ObjCBlock()  { block = nullptr; }
    template <typename R, class C, typename... P>
    ObjCBlock (C* _this, R (C::*fn)(P...))  : block (CreateObjCBlock (_this, fn)) {}
    ObjCBlock (BlockType b) : block ([b copy]) {}
    ObjCBlock& operator= (const BlockType& other) { if (block != nullptr) { [block release]; } block = [other copy]; return *this; }
    bool operator== (const void* ptr) const  { return ((const void*) block == ptr); }
    bool operator!= (const void* ptr) const  { return ((const void*) block != ptr); }
    ~ObjCBlock() { if (block != nullptr) [block release]; }

    operator BlockType() { return block; }

private:
    BlockType block;
};

struct ScopedCFString
{
    ScopedCFString() = default;
    ScopedCFString (String s) : cfString (s.toCFString())  {}

    ~ScopedCFString() noexcept
    {
        if (cfString != nullptr)
            CFRelease (cfString);
    }

    CFStringRef cfString = {};
};


} // namespace juce
