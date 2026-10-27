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

#pragma once

namespace hise {
using namespace juce;
struct ApiValidationAnalyzer;

struct LightweightDiagnostics : public DiagnosticBase,
                                public ObjectWithJSONConverter
{
    using Ptr = ReferenceCountedObjectPtr<LightweightDiagnostics>;

    enum class CacheBehaviour
    {
        NeverCache,
        CacheIfSameId,
        ReplaceIfPolymorphic,
        AlwaysCache
    };

    enum class Category
    {
        RangeCheck,
        CallbackArgCheck,
        RuntimeIdCheck,
        Conditional,
        GraphicsCheck,
        StringValidator,
        ObjectType,
        MethodTouched,
        numCategories
    };

    static StringArray getCategories()
    {
        return {
            "RangeCheck",
            "CallbackArgCheck",
            "RuntimeIdCheck",
            "Conditional",
            "GraphicsCheck",
            "StringValidator",
            "ObjectType",
            "MethodTouched"
        };
    }

    struct CustomCheckData
    {
        Category category;
        Identifier methodName;
        Identifier parameterId;
        NamedValueSet resolvedArguments;
        String checkString;
    };

    virtual DiagnosticResult performCustomCheck(const CustomCheckData& cd)
    {
        // Convention: return unknown for callback checks, this will handle all default
        // checks. Override this when requiring special callback checks...
        if (cd.category == Category::CallbackArgCheck || cd.category == Category::RuntimeIdCheck)
            return DiagnosticResult::unknown();

        // must be implemented in derived class
        return DiagnosticResult::fail("You need to implement a check for " + cd.methodName.toString() + "." + cd.parameterId.toString());
    }

    using ScopeSafety = WeakCallbackHolder::CallableObject::CallScope;

    DiagnosticResult checkCallScope(ScopeSafety minimumScope, const Identifier& methodName) const;

    DiagnosticResult runTests(ShadowParserBase* sp, const Identifier& methodName, const Array<var>& args);

    virtual CacheBehaviour getCacheBehaviour() const
    {
        if (Helpers::isApiClass(getObjectName()))
            return CacheBehaviour::AlwaysCache;

        if (getArgId().isNotEmpty())
            return CacheBehaviour::CacheIfSameId;

        return CacheBehaviour::NeverCache;
    }

    LightweightDiagnostics(ProcessorWithScriptingContent* p_, const ValueTree& classTree, const String& argId_) :
        p(p_),
        argId(argId_)
    {
        if(classTree.isValid() && !Helpers::isApiRoot(classTree))
            initClassTree(classTree);
    }

    struct ApiMap
    {
        struct Item
        {
            Identifier className;
            Identifier methodName;
            Identifier returnType;

            juce::String toString() const;
        };

        ApiMap();

        std::vector<Item> items;
    };

    struct Factory
    {
        Factory();

        using CreatorFunction = std::function<LightweightDiagnostics::Ptr (ProcessorWithScriptingContent*, const ValueTree&, const String& argId)>;

        template <typename T> void registerType()
        {
            static_assert(std::is_base_of<LightweightDiagnostics, typename T::Diagnostics>(), "not a base of lightweight diagnostics");

            items[T::getClassName()] = [](ProcessorWithScriptingContent* p, const ValueTree& c, const String& argId)
            {
                return new typename T::Diagnostics(p, c, argId);
            };
        }

        /** Use this with a class that has getClassName() and a constructor that matches Lightweight diagnostics. 
            Note: the ValueTree supplied by the constructor is the full ApiTree. 
        */
        template <typename T> void registerMetaType()
        {
            static_assert(std::is_base_of<LightweightDiagnostics, T>(), "not a base of lightweight diagnostics");
            metaItems[T::getClassName()] = [](ProcessorWithScriptingContent* p, const ValueTree& c, const String& argId)
            {
                return new typename T(p, c, argId);
            };
        }

        LightweightDiagnostics::Ptr createLightweightPrototype(ProcessorWithScriptingContent* p, const Identifier& id, const String& argId) const;

        Identifier getReturnTypeFromApiMap(const Identifier& className, const Identifier& methodName) const;

        void clearCache()
        {
            cache.clear();
        }

    private:

        LightweightDiagnostics::Ptr createInternal(ProcessorWithScriptingContent* p, const Identifier& id, const String& argId) const;
        

        class Cache
        {
            struct Key
            {
                Identifier className;
                String argId;
                bool isApiClass;

                bool matches(const Identifier& className_, const String& argId_) const
                {
                    return className_ == className &&
                        (isApiClass || argId == argId_);
                }
            };

        public:
            

            LightweightDiagnostics::Ptr getCached(LightweightDiagnostics::Ptr ld)
            {
                if (ld->getCacheBehaviour() == CacheBehaviour::NeverCache)
                    return ld;

                if(auto existing = get(ld->getObjectName(), ld->getArgId()))
                    return existing;

                Key k;
                k.className = ld->getObjectName();
                k.argId = ld->getArgId();
                k.isApiClass = Helpers::isApiClass(ld->getObjectName());

                cacheMap.push_back({ k, ld });

                return ld;
            }

            void clear() { cacheMap.clear(); }

        private:

            LightweightDiagnostics::Ptr get(const Identifier& id, const String& argId) const
            {
                for (const auto& m : cacheMap)
                {
                    if (m.first.matches(id, argId))
                        return m.second.get();
                }

                return nullptr;
            }

            std::vector<std::pair<Key, ReferenceCountedObjectPtr<LightweightDiagnostics>>> cacheMap;
        };

        mutable Cache cache;

        SharedResourcePointer<ApiMap> apiMap;
        std::map<Identifier, CreatorFunction> items;
        std::map<Identifier, CreatorFunction> metaItems;
    };

    struct Validator
    {
        DiagnosticResult perform(DiagnosticBase* b, const Identifier& id, const Array<var>& args) const;

        operator bool() const { return !validators.empty(); }

        std::vector<std::pair<int, StringArray>> validators;
    };

    struct FunctionInfo
    {
        struct Args
        {
            Args(Identifier id_) : id(id_) {};

            Args withType(VarTypeChecker::VarTypes t) const;

            Args asOptional(bool isOptional) const;

            static Args parse(const String& argWithType);

            Identifier id;
            VarTypeChecker::VarTypes type = VarTypeChecker::VarTypes::NotUndefined;
            bool optional = false;
            StringArray validators;

            bool hasValidators() const { return !validators.isEmpty(); }
        };

        FunctionInfo(const Identifier& id_) :
            methodName(id_)
        {};

        FunctionInfo(const ValueTree& methodTree);

        FunctionInfo withNumArgs(int numArgs) const;
        FunctionInfo withArgs(Args a) const;

        Validator getValidators() const;

        Identifier className;
        Identifier methodName;
        Identifier returnType;

        WeakCallbackHolder::CallableObject::CallScope callscope = WeakCallbackHolder::CallableObject::CallScope::Unknown;
        String callScopeNote;

        std::vector<Args> arguments;

        StringArray thisValidators;
    };

    void addFunction(const FunctionInfo& fi) { functions.add(fi); }
    void addFunctionWithArgs(const Identifier& function, int numArgs) { functions.add(FunctionInfo(function).withNumArgs(numArgs)); }

    void addFunction(const Identifier& id, call0 unused) override { functions.add(FunctionInfo(id).withNumArgs(0)); }
    void addFunction1(const Identifier& id, call1 unused) override { functions.add(FunctionInfo(id).withNumArgs(1)); }
    void addFunction2(const Identifier& id, call2 unused) override { functions.add(FunctionInfo(id).withNumArgs(2)); }
    void addFunction3(const Identifier& id, call3 unused) override { functions.add(FunctionInfo(id).withNumArgs(3)); }
    void addFunction4(const Identifier& id, call4 unused) override { functions.add(FunctionInfo(id).withNumArgs(4)); }
    void addFunction5(const Identifier& id, call5 unused) override { functions.add(FunctionInfo(id).withNumArgs(5)); }

    DiagnosticResult checkArgCount(const Identifier& id, int numActual) const override;

    bool getIndexAndNumArgsForFunction(const Identifier& id, int& index, int& numArgs) const override;

    var getReturnType(const Identifier& functionName, const Array<var>& args) const override;

    virtual DiagnosticResult checkProperty(const Identifier& propertyId) const;

    DiagnosticResult checkFunction(const Identifier& methodName, int expectedNumArgs) const override;

    struct CallbackParseOptions
    {
        enum class FunctionType
        {
            Function,
            Realtime,
            InlineFunction
        };

        Result parseCheckString(const String& f, bool checkParameterCount);

        FunctionType ft;
        var callback;
        WeakCallbackHolder::CallableObject::StrictnessLevel strictness = WeakCallbackHolder::CallableObject::StrictnessLevel::Unsafe;
        
        std::vector<std::pair<Identifier, String>> argTypes;
        LightweightDiagnostics* thisObj;
    };

    DiagnosticResult shadowParseCallbackWithArgType(const CallbackParseOptions& callbackOptions);

    String getArgId() const { return argId; }

    Array<FunctionInfo> functions;
    ProcessorWithScriptingContent* p;
    
    struct Helpers
    {
        /** small helper function : returns false if an argument couldn't be resolved. Use this in your diagnostics. */
        static bool checkValidArgs(const Array<var>& args);

        static bool isApiRoot(const ValueTree& v) { return v.getType() == Identifier("Api"); }
        
        static bool isApiClass(const Identifier& id);

        template <typename T> static Identifier makeMetaId()
        {
            String s;
            s << "MetaObjects::" << T::getClassName().toString();
            return Identifier(s);
        }

        template <int FIndex, int NumExpectedArgs> static DiagnosticResult checkCallbackArgs(DiagnosticBase*, const Identifier&, const Array<var>& args)
        {
            if (auto f = dynamic_cast<WeakCallbackHolder::CallableObject*>(args[FIndex].getObject()))
            {
                auto numActual = f->getNumArguments();

                if (numActual != NumExpectedArgs)
                    return DiagnosticResult::fail("wrong argument count for callback").withExpectation(String(numActual) + " params", String(NumExpectedArgs) + " params");
            }

            return DiagnosticResult::unknown();
        };

        static StringArray getPrimitiveTypeList();
        static VarTypeChecker::VarTypes getVarTypeFromPrimitiveType(const String& s);

        static String getTypeForObjectTypeCheck(const var& value);
    };

    virtual var getConstantValue(const Identifier& id) const
    {
        return apiConstants[id];
    }

    Identifier getObjectName() const { return className; }

    JUCE_MAKE_STREAMABLE_OBJECT(7);

    void writeAsJSON(OutputStream& os, int indentLevel, bool allOnOneLine, int maximumDecimalPlaces) override
    {
        String rep;
        rep << getObjectName().toString();

        if (getArgId().isNotEmpty())
            rep << "(" << getArgId().quoted() << ")";

        os << rep;
    }

    void writeToStream(OutputStream& os) override
    {
        JUCE_WRITE_STREAMABLE_OBJECT_MARKERS(os);

        String rep;
        rep << getObjectName().toString();

        if (getArgId().isNotEmpty())
            rep << "(" << getArgId().quoted() << ")";

        os.writeCompressedInt(rep.length());
        os.writeString(rep);

        jassertfalse;
    }

    static ObjectWithJSONConverter* createFromStream(InputStream& input)
    {
        jassertfalse;
        return nullptr;
    }

protected:

    

    void initClassTree(const ValueTree& classTree)
    {
        jassert(classTree.isValid() && !Helpers::isApiRoot(classTree));

        className = classTree.getType();

        for (auto& m : classTree)
        {
            FunctionInfo fi(m);

            addFunction(fi);

            if (auto v = fi.getValidators())
            {
                addDiagnostic(fi.methodName, [v](DiagnosticBase* b, const Identifier& id, const Array<var>& args)
                {
                    return v.perform(b, id, args);
                });
            }
        }

        for (int i = 0; i < classTree.getNumProperties(); i++)
        {
            auto id = classTree.getPropertyName(i);
            apiConstants.set(id.toString().removeCharacters("`"), classTree[id]);
        }
    }

    struct ValidatorEvaluator;

private:

    NamedValueSet apiConstants;
    Identifier className;

    const String argId;
};

struct NativeTypeDiagnostic : public LightweightDiagnostics
{
    NativeTypeDiagnostic(ProcessorWithScriptingContent* p, VarTypeChecker::VarTypes type, const String& argId);

    CacheBehaviour getCacheBehaviour() const override { return custom ? CacheBehaviour::NeverCache : CacheBehaviour::AlwaysCache; };

    bool custom = false;
    VarTypeChecker::VarTypes t;
};

struct InbuiltFunction : public LightweightDiagnostics
{
    using CS = WeakCallbackHolder::CallableObject::CallScope;

    InbuiltFunction(ProcessorWithScriptingContent* p, const Identifier& funcName, VarTypeChecker::VarTypes returnType, VarTypeChecker::VarTypes argType, CS callScope) :
        LightweightDiagnostics(p, ValueTree(funcName), "")
    {
        addFunction(createFunctionInfo(returnType, argType, callScope));
    }

    FunctionInfo createFunctionInfo(VarTypeChecker::VarTypes returnType, VarTypeChecker::VarTypes argType, CS callScope) const
    {
        FunctionInfo self(getObjectName());
        
        self.callscope = callScope;
        self.methodName = getObjectName();

        if (self.methodName == Identifier("Rectangle"))
        {
            self.returnType = "Rectangle";
            
            FunctionInfo::Args a1("a1");
            a1.type = VarTypeChecker::IndexOrArray;

            self.arguments.push_back(a1);

            FunctionInfo::Args a2("a2");
            a2.type = VarTypeChecker::IndexOrArray;

            self.arguments.push_back(a2.asOptional(true));

            FunctionInfo::Args a3("a3");
            a3.type = VarTypeChecker::IndexOrArray;

            self.arguments.push_back(a3.asOptional(true));

            FunctionInfo::Args a4("a4");
            a4.type = VarTypeChecker::IndexOrArray;

            self.arguments.push_back(a4.asOptional(true));
        }
        else
        {
            self.returnType = VarTypeChecker::getTypeName(returnType);
            FunctionInfo::Args singleArg("argument");
            singleArg.type = argType;

            self.arguments.push_back(singleArg);
        }

        return self;
    }

};

}