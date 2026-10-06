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


struct ScriptingApi::Content::ScriptPanel::Diagnostics : public LightweightDiagnostics
{
	Diagnostics(ProcessorWithScriptingContent* pwsc, const ValueTree& v, const String& argId) :
		LightweightDiagnostics(pwsc, v, argId)
	{
		addDiagnostic("setPaintRoutine", [this](DiagnosticBase* b, const Identifier& id, const Array<var>& args)
		{
			return shadowParseCallbackWithArgType(args[0], { ScriptingObjects::GraphicsObject::getClassName() });
		});
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

LightweightDiagnostics::Factory::Factory()
{
    registerType<ScriptingObjects::PathObject>();
    registerType<ScriptingObjects::ScriptingEffect>();
    registerType<ScriptingApi::Content::ScriptPanel>();
    registerType<ScriptingApi::Synth>();

    // final validation gate: all API class must be creatable
    //jassert(items.size() == getNumApiClasses());
}

}