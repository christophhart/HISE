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

struct ScriptComponentChecks
{
    using DR = DiagnosticBase::DiagnosticResult;

    static DR checkProperty(DiagnosticBase* b, const Identifier& id, const Array<var>& args)
    {
        auto typed = dynamic_cast<LightweightDiagnostics*>(b);

        auto sc = typed->p->getScriptingContent()->getComponentWithName(typed->getArgId());

        auto propId = args[0];

        auto props = sc->getAllProperties();

        if (props.indexOf(propId) == -1)
        {
            StringArray sa;

            for (const auto& a : *props.getArray())
                sa.add(a.toString());

            return DR::fail("Unknown property").withWrongToken(propId.toString()).withFuzzySuggestion(sa);
        }

        auto isSet = id == Identifier("set");

        if (isSet && typed->checkValidArgs(args))
        {
            auto v = args[1].toString();
            auto options = sc->getOptionsFor(propId.toString());

            if (!options.isEmpty())
                return DR::fail("Illegal option for property " + propId.toString()).withWrongToken(v).withFuzzySuggestion(options);
        }

        return DR::ok();
    }

    static DiagnosticBase::DiagnosticResult setControlCallback(DiagnosticBase* b, const Identifier& id, const Array<var>& args)
    {
        auto typed = dynamic_cast<LightweightDiagnostics*>(b);

        auto callback = dynamic_cast<WeakCallbackHolder::CallableObject*>(args[0].getObject());

        if (callback == nullptr || !callback->isRealtimeSafe())
        {
            return DR::fail("A control callback must be an inline function");
        }

        LightweightDiagnostics::CallbackParseOptions cb;
        cb.callback = args[0];
        cb.argTypes.push_back({ typed->getObjectName(), typed->getArgId() });
        cb.argTypes.push_back({});
        cb.thisObj = typed;

        return typed->shadowParseCallbackWithArgType(cb);
    }

    static void addBaseChecks(LightweightDiagnostics* lb)
    {
        lb->addDiagnostic("set", checkProperty);
        lb->addDiagnostic("get", checkProperty);
        lb->addDiagnostic("setControlCallback", setControlCallback);
    }
};

struct ScriptingApi::Content::ScriptSlider::Diagnostics : public LightweightDiagnostics
{
    static void addComponentDiagnostics(LightweightDiagnostics* lb)
    {
        ScriptComponentChecks::addBaseChecks(lb);
    }

    Diagnostics(ProcessorWithScriptingContent* pwsc, const ValueTree& v, const String& argId) :
        LightweightDiagnostics(pwsc, v, argId)
    {
        addComponentDiagnostics(this);
    }
};


struct ScriptingApi::Content::ScriptPanel::Diagnostics : public LightweightDiagnostics
{
    static void addComponentDiagnostics(LightweightDiagnostics* lb)
    {
        ScriptComponentChecks::addBaseChecks(lb);

        lb->addDiagnostic("setPaintRoutine", [lb](DiagnosticBase* b, const Identifier& id, const Array<var>& args)
        {
            CallbackParseOptions cb;
            cb.callback = args[0];
            cb.argTypes.push_back({ ScriptingObjects::GraphicsObject::getClassName(), "" });
            cb.thisObj = lb;

            return lb->shadowParseCallbackWithArgType(cb);
        });
    }

	Diagnostics(ProcessorWithScriptingContent* pwsc, const ValueTree& v, const String& argId) :
		LightweightDiagnostics(pwsc, v, argId)
	{
        addComponentDiagnostics(this);
	}
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

template <typename T> struct ModuleDiagnostic: public LightweightDiagnostics
{
    ModuleDiagnostic(ProcessorWithScriptingContent* pwsc, const ValueTree& v, const String& argId) :
        LightweightDiagnostics(pwsc, v, argId)
    {

    }

protected:

    bool checkProperty(const Identifier& id) const override
    {
        auto mt = getMetadata();

        for (const auto& p : mt.parameters)
        {
            if (p.id == id)
                return true;
        }

        return LightweightDiagnostics::checkProperty(id);
    }

    template <bool CheckValue> static DiagnosticResult checkAttribute(DiagnosticBase* b, const Identifier& id, const Array<var>& args)
    {
        if (!checkValidArgs(args))
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
        if(resolved)
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

struct ScriptingObjects::ScriptingEffect::Diagnostics : public ModuleDiagnostic<EffectProcessor>
{
    

    Diagnostics(ProcessorWithScriptingContent* pwsc, const ValueTree& v, const String& argId) :
        ModuleDiagnostic<EffectProcessor>(pwsc, v, argId)
    {
        addDiagnostic("setAttribute", checkAttribute<true>);
        addDiagnostic("getAttribute", checkAttribute<false>);
    }
};

struct ScriptingObjects::PathObject::Diagnostics : public LightweightDiagnostics
{
    Diagnostics(ProcessorWithScriptingContent* p, const ValueTree& classTree, const String&) :
        LightweightDiagnostics(p, classTree, {})
    {
        addDiagnostic("clear", [this](DiagnosticBase* b, const Identifier& id, const Array<var>& args)
        {
            clearTouchedMethods();
            return DiagnosticResult::ok();
        });

        addDiagnostic("addArc", [this](DiagnosticBase* b, const Identifier& id, const Array<var>& args)
        {
            if (!wasMethodTouched("startNewSubPath"))
                return DiagnosticResult(DiagnosticResult::Severity::Warning, 
                       "addArc needs a bounding box to be rendered consistently")
                       .withSuggestion("Call startNewSubPath twice with the top-left and bottom right corner of the area that should be the consistent bounding box before calling this method.");

            return DiagnosticResult::ok();
        });
    }
};

#define ADD_CUSTOM_DIAGNOSTICS(className) if (sc != nullptr && sc->getObjectName() == ScriptingApi::Content::className::getClassName()) \
                ScriptingApi::Content::className::Diagnostics::addComponentDiagnostics(this);

struct ScriptComponentDiagnostics
{
    static Identifier getClassName() { RETURN_STATIC_IDENTIFIER("ScriptComponent"); }

    struct Diagnostics : public LightweightDiagnostics
    {
        Diagnostics(ProcessorWithScriptingContent* p, const ValueTree& fullApiTree, const String& arg):
            LightweightDiagnostics(p, {}, arg)
        {
            auto sc = p->getScriptingContent()->getComponentWithName(arg);

            ADD_CUSTOM_DIAGNOSTICS(ScriptPanel);
            ADD_CUSTOM_DIAGNOSTICS(ScriptSlider);

            if (sc != nullptr)
                initClassTree(fullApiTree.getChildWithName(sc->getObjectName()));
        }
    };
};

#undef ADD_CUSTOM_DIAGNOSTICS

LightweightDiagnostics::Factory::Factory()
{
    registerType<ScriptingObjects::PathObject>();
    registerType<ScriptingObjects::ScriptingEffect>();
    registerType<ScriptingApi::Content::ScriptPanel>();
    registerType<ScriptingApi::Content::ScriptSlider>();
    registerType<ScriptingApi::Synth>();
    registerType<ScriptComponentDiagnostics>();

    // final validation gate: all API class must be creatable
    //jassert(items.size() == getNumApiClasses());
}

}