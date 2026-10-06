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


/** TODO:

- migrate all DiagnosticBase lambdas to ApiDiagnostics.cpp

- make ApiClass not a DiagnosticBase method





*/


namespace hise {
using namespace juce;
struct ApiValidationAnalyzer;

struct LightweightDiagnostics : public DiagnosticBase
{
    LightweightDiagnostics(ProcessorWithScriptingContent* p_, const ValueTree& classTree, const String& argId_) :
        p(p_),
        argId(argId_)
    {
        for (auto m : classTree)
        {
            addFunction(FunctionInfo(m));
        }

        for (int i = 0; i < classTree.getNumProperties(); i++)
        {
            auto id = classTree.getPropertyName(i);
            apiConstants.set(id.toString().removeCharacters("`"), classTree[id]);
        }
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

        using CreatorFunction = std::function<DiagnosticBase* (ProcessorWithScriptingContent*, const ValueTree&, const String& argId)>;

        template <typename T> void registerType()
        {
            items[T::getClassName()] = [](ProcessorWithScriptingContent* p, const ValueTree& c, const String& argId)
            {
                return new typename T::Diagnostics(p, c, argId);
            };
        }

        DiagnosticBase* createLightweightPrototype(ProcessorWithScriptingContent* p, const Identifier& id, const String& argId) const;

        Identifier getReturnTypeFromApiMap(const Identifier& className, const Identifier& methodName) const;

    private:

        SharedResourcePointer<ApiMap> apiMap;
        std::map<Identifier, CreatorFunction> items;
    };

    struct FunctionInfo
    {
        /** TODO: - Complete parsing of types OK
        *         - add optional args to Api pipeline as "requiredNumArgs" field OK
        *         - change factory to attach live checks to valuetree base class OK
        *         - big refactor: remove all the stuff that isn't needed with this new approach
        */
         
        struct Args
        {
            Args(Identifier id_) : id(id_) {};

            Args withType(VarTypeChecker::VarTypes t) const;

            Args asOptional(bool isOptional) const;

            static Args parse(const String& argWithType);

            Identifier id;
            VarTypeChecker::VarTypes type = VarTypeChecker::VarTypes::NotUndefined;
            bool optional = false;
        };

        FunctionInfo(const Identifier& id_) :
            methodName(id_)
        {};

        FunctionInfo(const ValueTree& methodTree);

        FunctionInfo withNumArgs(int numArgs) const;
        FunctionInfo withArgs(Args a) const;

        Identifier methodName;
        Identifier returnType;
        std::vector<Args> arguments;
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

    Identifier getReturnType(const Identifier& functionName) const override
    {
        for (const auto& f : functions)
        {
            if (f.methodName == functionName)
                return f.returnType;
        }
    }

    virtual bool checkProperty(const Identifier& propertyId) const;


    DiagnosticResult shadowParseCallbackWithArgType(const var& callback, const std::vector<Identifier>& argTypes);

    String getArgId() const { return argId; }

    Array<FunctionInfo> functions;
    ProcessorWithScriptingContent* p;
    ApiValidationAnalyzer* currentParentValidator = nullptr;

protected:

    /** small helper function : returns false if an argument couldn't be resolved. Use this in your diagnostics. */
    static bool checkValidArgs(const Array<var>& args)
    {
        for (auto& v : args)
        {
            if (v.isUndefined() || v.isVoid())
                return false;
        }

        return true;
    }


private:

    NamedValueSet apiConstants;

    const String argId;
};

}