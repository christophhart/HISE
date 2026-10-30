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

namespace hise {
using namespace juce;

// ============================================================================================================



#if 0 
/** Use this for new classes:

1. Add a inner struct `Diagnostics` and make sure the class has a static Identifier getClassName() method
2. Register at LightweightDiagnostics::Factory() (at the bottom of this file)...
3. Use this class template and add a diagnostic for each method

4. Component special: add static void addComponentDiagnostics(LightweightDiagnostics* lb) {}
5. Component special: call ScriptComponentChecks::addBaseChecks(this) in this function
6. Call this method in the constructor. This will make the generic ScriptComponent diagnostic resolve
   the type from the API class tree and call the same functions.
7. add ADD_CUSTOM_DIAGNOSTICS(ClassName); in the ScriptComponent constructor

*/
struct Class::Diagnostics : public LightweightDiagnostics
{
    Diagnostics(ProcessorWithScriptingContent* pwsc, const ValueTree& v) :
        LightweightDiagnostics(pwsc, v)
    {
        addDiagnostic("methodName", [this](DiagnosticBase* b, const Identifier& id, const Array<var>& args)
        {
            return DiagnosticResult::ok();
        });
    }
};
#endif

// ============================================================================================================

namespace BaseObjects
{

struct ScriptComponentDiagnosticBase : public LightweightDiagnostics
{
    static Identifier getClassName() { RETURN_STATIC_IDENTIFIER("ScriptComponent"); }

    ScriptComponentDiagnosticBase(ProcessorWithScriptingContent* p, const ValueTree& v, const String& a) :
        LightweightDiagnostics(p, v, a)
    {
        if (Helpers::isApiRoot(v))
        {
            if (auto sc = getScriptComponent())
            {
                auto classTree = v.getChildWithName(sc->getObjectName());

                if (classTree.isValid())
                    initClassTree(classTree);
            }
        }
    };

    CacheBehaviour getCacheBehaviour() const override
    {
        return CacheBehaviour::ReplaceIfPolymorphic;
    }

    NamedValueSet writtenProperties;

    // Use this to fetch properties in validation checks. This checks the lightweight dictionary first
    // and only resorts to the lookup of the real object when it can't be found.
    var getPropertyFromObjectOrWrittenProperties(const Identifier& id) const
    {
        if (writtenProperties.contains(id))
            return writtenProperties[id];

        auto sc = getScriptComponent();
        return sc->getScriptObjectProperty(id);
    }

    using DR = DiagnosticBase::DiagnosticResult;

    ScriptComponent* getScriptComponent() const
    {
        return p->getScriptingContent()->getComponentWithName(getArgId());
    }

    DiagnosticResult performCustomCheck(const CustomCheckData& cd) final override
    {
        auto sc = getScriptComponent();
        
        if(sc == nullptr)
            return DiagnosticResult::fail("Can't find component. If this script hasn't been compiled once, compile it to create the object")
            .withSeverity(DiagnosticResult::Severity::Warning)
            .withErrorCode(DiagnosticResult::ErrorCode::RuntimeIdFailure);
        
        if (cd.methodName == Identifier("set") || cd.methodName == Identifier("get"))
        {            
            auto propId = cd.resolvedArguments["propertyName"];
            auto props = sc->getAllProperties();

            if (cd.category == Category::RuntimeIdCheck)
            {
                if (props.indexOf(propId) == -1)
                {
                    StringArray sa;

                    for (const auto& a : *props.getArray())
                        sa.add(a.toString());

                    return DR::fail("Unknown property")
                        .withWrongToken(propId.toString())
                        .withFuzzySuggestion(sa)
                        .withErrorCode(DiagnosticResult::ErrorCode::InvalidOption);
                }

                return DR::ok();
            }

            if (cd.category == Category::Conditional)
            {
                auto v = cd.resolvedArguments["value"].toString();
                auto options = sc->getOptionsFor(propId.toString());

                writtenProperties.set(propId.toString(), v);

                if (!options.isEmpty() && !options.contains(v))
                    return DR::fail("Illegal option for property " + propId.toString())
                    .withWrongToken(v)
                    .withFuzzySuggestion(options)
                    .withErrorCode(DiagnosticResult::ErrorCode::InvalidOption);

                return DR::ok();
            }
        }

        return LightweightDiagnostics::performCustomCheck(cd);
    }
};

struct FunctionJSONObject : public LightweightDiagnostics
{
    FunctionJSONObject(ProcessorWithScriptingContent* p, const ValueTree&, const String& argId) :
        LightweightDiagnostics(p, {}, argId)
    {};
};

template <typename T> struct ModuleDiagnostic : public LightweightDiagnostics
{
    ModuleDiagnostic(ProcessorWithScriptingContent* pwsc, const ValueTree& v, const String& argId) :
        LightweightDiagnostics(pwsc, v, argId)
    {

    }

protected:

    DiagnosticResult checkProperty(const Identifier& id) const override
    {
        auto mt = getMetadata();

        for (const auto& p : mt.parameters)
        {
            if (p.id == id)
                return DiagnosticResult::ok();
        }

        return LightweightDiagnostics::checkProperty(id);
    }

    template <bool CheckValue> static DiagnosticResult checkAttribute(DiagnosticBase* b, const Identifier& id, const Array<var>& args)
    {
        if (!Helpers::checkValidArgs(args))
            return DiagnosticResult::unknown();

        auto idx = (int)args[0];
        auto value = (float)args[1];

        auto mt = dynamic_cast<ModuleDiagnostic<T>*>(b)->getMetadata();

        if (!isPositiveAndBelow(idx, mt.parameters.size()))
        {
            return DiagnosticResult::fail("Illegal parameter index " + String(idx)).withSuggestion("Valid parameter range: [0 - " + String(mt.parameters.size() - 1) + "]");
        }

        if constexpr (CheckValue)
        {
            auto p = mt.parameters[idx];

            if (!(p.range.getRange().contains(value) || p.range.rng.end == value))
            {
                return DiagnosticResult("value " + String(value, 2) + " outside default parameter range for " + p.id.toString())
                    .withSeverity(DiagnosticResult::Severity::Warning)
                    .withSuggestion(RangeHelpers::toDisplayString(p.range));
            }
        }

        return DiagnosticResult::ok();
    }

    T* getProcessor() const
    {
        if (resolved)
            return dynamic_cast<T*>(processor.get());

        auto chain = p->getMainController_()->getMainSynthChain();

        Processor::Iterator<T> it(chain);

        while (auto t = it.getNextProcessor())
        {
            if (dynamic_cast<Processor*>(t)->getId() == getArgId())
            {
                processor = dynamic_cast<Processor*>(t);
                metadata = processor->getMetadata();
                resolved = true;
                break;
            }
        }

        return dynamic_cast<T*>(processor.get());
    }

    ProcessorMetadata getMetadata() const
    {
        if (!resolved)
            getProcessor();

        return metadata;
    }

private:

    mutable bool resolved = false;
    mutable WeakReference<Processor> processor;
    mutable ProcessorMetadata metadata;
};

} // namespace BaseObjects





struct EventTypeObject: public BaseObjects::FunctionJSONObject
{
    EventTypeObject(ProcessorWithScriptingContent* p, const ValueTree&, const String& cl) :
        FunctionJSONObject(p, {}, cl)
    {
        auto level = MouseCallbackComponent::getCallbackLevel(cl);

        var obj(new DynamicObject());
        MouseCallbackComponent::createAutocompleteCallbackObject(obj, level);

        ValueTree v(getClassName());

        for (const auto& nv : obj.getDynamicObject()->getProperties())
            v.setProperty(nv.name, nv.value, nullptr);

        initClassTree(v);
    }

    DiagnosticResult checkProperty(const Identifier& id) const override
    {
        auto ok = FunctionJSONObject::checkProperty(id);

        if (ok.shouldReport())
        {
            auto cl = MouseCallbackComponent::getMinimumCallbackLevel(id);

            // let the default missing property handle that...
            if (cl == MouseCallbackComponent::CallbackLevel::Unknown)
                return ok;

            // override the generic missing property with a hint to the missing callback level
            return DiagnosticResult::fail("Trying to access mouse callback property without setting the proper `allowCallbacks` level")
                .withWrongToken(id.toString())
                .withSuggestion("ScriptPanel.set(\"allowCallbacks\", \"" + MouseCallbackComponent::getCallbackLevelAsIdentifier(cl).toString() + "\")");
        }

        return ok;
    }

    static Identifier getClassName() { RETURN_STATIC_IDENTIFIER("event"); }
};

struct ScriptingApi::Content::Diagnostics : public LightweightDiagnostics
{
    Diagnostics(ProcessorWithScriptingContent* pwsc, const ValueTree& v, const String& argId) :
        LightweightDiagnostics(pwsc, v, argId)
    {}

    DiagnosticResult performCustomCheck(const CustomCheckData& cd) override
    {
        return LightweightDiagnostics::performCustomCheck(cd);
    }

    var getReturnType(const Identifier& methodName, const Array<var>& args) const override
    {
        if (methodName == Identifier("getComponent"))
        {
            auto id = args[0].toString();
            
            if (auto sc = p->getScriptingContent()->getComponentWithName(id))
            {
                auto typed = dynamic_cast<HiseJavascriptEngine::RootObject::ObjectTypeMap*>(currentShadowParser);
                return var(typed->createAnonoymousDiagnosticObject(sc->getObjectName(), id).get());
            }
        }

        return LightweightDiagnostics::getReturnType(methodName, args);
    }
};

struct ScriptingApi::Content::ScriptSlider::Diagnostics : public BaseObjects::ScriptComponentDiagnosticBase
{
    Diagnostics(ProcessorWithScriptingContent* pwsc, const ValueTree& v, const String& argId) :
        ScriptComponentDiagnosticBase(pwsc, v, argId)
    {}
};

struct ScriptingApi::Content::ScriptButton::Diagnostics : public BaseObjects::ScriptComponentDiagnosticBase
{
    Diagnostics(ProcessorWithScriptingContent* pwsc, const ValueTree& v, const String& argId) :
        ScriptComponentDiagnosticBase(pwsc, v, argId)
    {}
};

struct ScriptingApi::Content::ScriptComboBox::Diagnostics : public BaseObjects::ScriptComponentDiagnosticBase
{
    Diagnostics(ProcessorWithScriptingContent* pwsc, const ValueTree& v, const String& argId) :
        ScriptComponentDiagnosticBase(pwsc, v, argId)
    {}
};

struct ScriptingApi::Content::ScriptLabel::Diagnostics : public BaseObjects::ScriptComponentDiagnosticBase
{
    Diagnostics(ProcessorWithScriptingContent* pwsc, const ValueTree& v, const String& argId) :
        ScriptComponentDiagnosticBase(pwsc, v, argId)
    {}
};

struct ScriptingApi::Content::ScriptImage::Diagnostics : public BaseObjects::ScriptComponentDiagnosticBase
{
    Diagnostics(ProcessorWithScriptingContent* pwsc, const ValueTree& v, const String& argId) :
        ScriptComponentDiagnosticBase(pwsc, v, argId)
    {}
};

struct ScriptingApi::Content::ScriptPanel::Diagnostics : public BaseObjects::ScriptComponentDiagnosticBase
{
    Diagnostics(ProcessorWithScriptingContent* pwsc, const ValueTree& v, const String& argId) :
        ScriptComponentDiagnosticBase(pwsc, v, argId)
    {
        addDiagnostic("setMouseCallback", [](DiagnosticBase* b, const Identifier& id, const Array<var>& args)
        {
            auto typed = dynamic_cast<BaseObjects::ScriptComponentDiagnosticBase*>(b);
            auto cl = typed->getPropertyFromObjectOrWrittenProperties("allowCallbacks").toString();

            CallbackParseOptions cb;
            cb.callback = args[0];
            cb.argTypes.push_back({ Helpers::makeMetaId<EventTypeObject>(), cl });
            cb.thisObj = typed;

            return typed->shadowParseCallbackWithArgType(cb);
        });
    }
};

struct ScriptingApi::Content::ScriptedViewport::Diagnostics : public BaseObjects::ScriptComponentDiagnosticBase
{
    Diagnostics(ProcessorWithScriptingContent* pwsc, const ValueTree& v, const String& argId) :
        ScriptComponentDiagnosticBase(pwsc, v, argId)
    {}
};

struct ScriptingApi::Content::ScriptTable::Diagnostics : public BaseObjects::ScriptComponentDiagnosticBase
{
    Diagnostics(ProcessorWithScriptingContent* pwsc, const ValueTree& v, const String& argId) :
        ScriptComponentDiagnosticBase(pwsc, v, argId)
    {}
};

struct ScriptingApi::Content::ScriptAudioWaveform::Diagnostics : public BaseObjects::ScriptComponentDiagnosticBase
{
    Diagnostics(ProcessorWithScriptingContent* pwsc, const ValueTree& v, const String& argId) :
        ScriptComponentDiagnosticBase(pwsc, v, argId)
    {}
};

struct ScriptingApi::Content::ScriptSliderPack::Diagnostics : public BaseObjects::ScriptComponentDiagnosticBase
{
    Diagnostics(ProcessorWithScriptingContent* pwsc, const ValueTree& v, const String& argId) :
        ScriptComponentDiagnosticBase(pwsc, v, argId)
    {}
};

struct ScriptingApi::Content::ScriptDynamicContainer::Diagnostics : public BaseObjects::ScriptComponentDiagnosticBase
{
    Diagnostics(ProcessorWithScriptingContent* pwsc, const ValueTree& v, const String& argId) :
        ScriptComponentDiagnosticBase(pwsc, v, argId)
    {}
};

struct ScriptingApi::Content::ScriptFloatingTile::Diagnostics : public BaseObjects::ScriptComponentDiagnosticBase
{
    Diagnostics(ProcessorWithScriptingContent* pwsc, const ValueTree& v, const String& argId) :
        ScriptComponentDiagnosticBase(pwsc, v, argId)
    {}
};

struct ScriptingApi::Content::ScriptWebView::Diagnostics : public BaseObjects::ScriptComponentDiagnosticBase
{
    Diagnostics(ProcessorWithScriptingContent* pwsc, const ValueTree& v, const String& argId) :
        ScriptComponentDiagnosticBase(pwsc, v, argId)
    {}
};

struct ScriptingApi::Synth::Diagnostics : public LightweightDiagnostics
{
    template <typename T> static DiagnosticResult check(DiagnosticBase* c, const Identifier&, const Array<var>& args)
    {
        if (args[0].isString())
        {
            auto id = args[0].toString();

            auto thisObj = dynamic_cast<Diagnostics*>(c);
            jassert(thisObj != nullptr);

            auto mc = thisObj->p->getMainController_();
            Processor::Iterator<T> iter(mc->getMainSynthChain());

            StringArray sa;

            while (auto p = iter.getNextProcessor())
            {
                auto pid = dynamic_cast<Processor*>(p)->getId();

                if (pid == id)
                    return ApiClass::DiagnosticResult::ok();

                sa.add(pid);
            }

            return ApiClass::DiagnosticResult::fail("module not found").withWrongToken(id).withFuzzySuggestion(sa);
        }

        return ApiClass::DiagnosticResult::unknown();
    }

    Diagnostics(ProcessorWithScriptingContent* pwsc, const ValueTree& v, const String&) :
        LightweightDiagnostics(pwsc, v, {})
    {
        addDiagnostic("getMidiPlayer", check<hise::MidiPlayer>);
        addDiagnostic("getModulator", check<Modulator>);
        addDiagnostic("getAudioSampleProcessor", check<ProcessorWithExternalData>);
        addDiagnostic("getDisplayBufferSource", check<ProcessorWithExternalData>);
        addDiagnostic("getTableProcessor", check<ProcessorWithExternalData>);
        addDiagnostic("getSliderPackProcessor", check<ProcessorWithExternalData>);
        addDiagnostic("getWavetableController", check<hise::WavetableSynth>);
        addDiagnostic("getSampler", check<ModulatorSampler>);
        addDiagnostic("getSlotFX", check<HotswappableProcessor>);
        addDiagnostic("getEffect", check<EffectProcessor>);
        addDiagnostic("getRoutingMatrix", check<hise::RoutableProcessor>);
        addDiagnostic("getMidiProcessor", check<hise::MidiProcessor>);
        addDiagnostic("getChildSynth", check<hise::ModulatorSynth>);
    }
};



struct ScriptingObjects::ScriptingEffect::Diagnostics : public BaseObjects::ModuleDiagnostic<EffectProcessor>
{
    Diagnostics(ProcessorWithScriptingContent* pwsc, const ValueTree& v, const String& argId) :
        ModuleDiagnostic<EffectProcessor>(pwsc, v, argId)
    {
        addDiagnostic("setAttribute", checkAttribute<true>);
        addDiagnostic("getAttribute", checkAttribute<false>);
    }
};

struct ProjectInfoObj: public BaseObjects::FunctionJSONObject
{
    static Identifier getClassName() { return "ProjectInfo"; }
    
    ProjectInfoObj(ProcessorWithScriptingContent* p, const ValueTree& v, const String& argId):
      FunctionJSONObject(p, v, argId)
    {
        ValueTree vt(getClassName());
        
        auto obj = HiseSettings::Data::getProjectInfoAsJSON(p->getMainController_());
        
        for(const auto& nv: obj.getDynamicObject()->getProperties())
            vt.setProperty(nv.name, nv.value, nullptr);
        
        initClassTree(vt);
    }
};

struct LafObj : public BaseObjects::FunctionJSONObject
{
    static Identifier getClassName() { return "obj"; }

    LafObj(ProcessorWithScriptingContent* p, const ValueTree& v, const String& argId) :
        FunctionJSONObject(p, {}, argId)
    {
        auto obj = ApiHelpers::getLafObjectTree().getChildWithName(argId);

        ValueTree reduced(getClassName());

        for (auto c : obj)
        {
            reduced.setProperty(c.getType(), true, nullptr);
            properties.add("obj." + c.getType().toString());
        }

        initClassTree(reduced);

        auto useRectangleClass = HISE_GET_PREPROCESSOR(p->getMainController_(), HISE_USE_SCRIPT_RECTANGLE_OBJECT);

        LightweightDiagnostics::Factory f;

        areaObj = f.createLightweightPrototype(p, useRectangleClass ? "Rectangle" : "Array", "").get();
        stringObj = f.createLightweightPrototype(p, "String", "").get();
    }

    var getConstantValue(const Identifier& childId) const override
    {
        if (childId == Identifier("area"))
            return var(areaObj.get());

        if(childId == Identifier("id") || 
           childId == Identifier("text") ||
            childId == Identifier("parentType"))
        {
            return var(stringObj.get());
        }

        return LightweightDiagnostics::getConstantValue(childId);
    }

    DiagnosticResult checkProperty(const Identifier& id) const override
    {
        auto ok = FunctionJSONObject::checkProperty(id);

        if(ok.shouldReport())
            return DiagnosticResult::fail("illegal property for LAF obj in function " + getArgId())
            .withWrongToken("obj." + id.toString())
            .withFuzzySuggestion(properties)
            .withErrorCode(DiagnosticResult::ErrorCode::MissingFunctionOrProperty);

        return ok;
    }

    StringArray properties;

    LightweightDiagnostics::Ptr areaObj, stringObj;
};

struct ScriptingObjects::ScriptedLookAndFeel::Diagnostics : public LightweightDiagnostics
{
    Diagnostics(ProcessorWithScriptingContent* pwsc, const ValueTree& v, const String& argId) :
        LightweightDiagnostics(pwsc, v, argId)
    {}

    DiagnosticResult performCustomCheck(const CustomCheckData& cd) override
    {
        if (cd.methodName == Identifier("setInlineStyleSheet"))
        {
            simple_css::Parser p(cd.checkString);
            auto ok = p.parse();

            if (!ok.wasOk())
                return DiagnosticResult::fail("CSS parser error: " + ok.getErrorMessage());

            auto warnings = p.getWarnings();

#if 0
                for (auto w : warnings)
                {
                    auto currentParentValidator = dynamic_cast<ShadowParserType*>(currentShadowParser);
                    currentParentValidator->addCustomDiagnostics(DiagnosticResult::fail("CSS Warning " + w)
                        .withSeverity(DiagnosticResult::Severity::Warning));
                }
#endif

            return DiagnosticResult::ok();
        }
        if (cd.methodName == Identifier("registerFunction"))
        {
            

            if (cd.category == Category::Conditional)
            {
                auto fn = cd.resolvedArguments[cd.parameterId].toString();
                auto lafTree = ApiHelpers::getLafObjectTree();
                auto mTree = lafTree.getChildWithName(fn);

                if (!mTree.isValid())
                {
                    StringArray candidates;

                    for (auto c : lafTree)
                        candidates.add(c.getType().toString());

                    return DiagnosticResult::fail("unknown function for LAF")
                        .withWrongToken(fn)
                        .withFuzzySuggestion(candidates)
                        .withErrorCode(DiagnosticResult::ErrorCode::InvalidOption);
                }
            }

            if(cd.category == Category::CallbackArgCheck)
            {
                CallbackParseOptions cb;
                cb.callback = cd.resolvedArguments["paintFunction"];

                auto ok = cb.parseCheckString(cd.checkString, true);

                if (ok.failed())
                    return DiagnosticResult::fail(ok.getErrorMessage()).withErrorCode(DiagnosticResult::ErrorCode::CallbackArgMismatch);

                // now pass the function name to the laf obj so it can resolve it later
                cb.argTypes[1].second = cd.resolvedArguments["functionName"].toString();

                return shadowParseCallbackWithArgType(cb);
            }
        }

        return DiagnosticResult::ok();
    }
};




LightweightDiagnostics::Factory::Factory()
{
    //registerType<ScriptingObjects::PathObject>();
    registerType<ScriptingObjects::ScriptingEffect>();
    
    registerType<ScriptingApi::Content::ScriptSlider>();
    registerType<ScriptingApi::Content::ScriptButton>();
    registerType<ScriptingApi::Content::ScriptComboBox>();
    registerType<ScriptingApi::Content::ScriptLabel>();
    registerType<ScriptingApi::Content::ScriptedViewport>();
    registerType<ScriptingApi::Content::ScriptTable>();
    registerType<ScriptingApi::Content::ScriptImage>();
    registerType<ScriptingApi::Content::ScriptWebView>();
    registerType<ScriptingApi::Content::ScriptSliderPack>();
    registerType<ScriptingApi::Content::ScriptAudioWaveform>();
    registerType<ScriptingApi::Content::ScriptFloatingTile>();
    registerType<ScriptingApi::Content::ScriptDynamicContainer>();
    registerType<ScriptingApi::Content::ScriptPanel>();

    registerType<ScriptingApi::Synth>();
    //registerType<ScriptingApi::Server>();
    registerType<ScriptingApi::Content>();
    registerType<ScriptingObjects::ScriptedLookAndFeel>();
    
    registerMetaType<BaseObjects::ScriptComponentDiagnosticBase>();
    registerMetaType<EventTypeObject>();
    registerMetaType<LafObj>();
    registerMetaType<ProjectInfoObj>();
}



}
