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

    struct PropertyMap
    {
        PropertyMap()
        {
             auto propertyTypes = JSON::parse(R"({
                "common": {
                    "text": "string",
                    "visible" : "bool",
                    "enabled" : "bool",
                    "locked" : "bool",
                    "x" : "int",
                    "y" : "int",
                    "width" : "int",
                    "height" : "int",
                    "min" : "double",
                    "max" : "double",
                    "defaultValue" : "double",
                    "tooltip" : "string",
                    "bgColour" : "string",
                    "itemColour" : "string",
                    "itemColour2" : "string",
                    "textColour" : "string",
                    "macroControl" : "int",
                    "saveInPreset" : "bool",
                    "isPluginParameter" : "bool",
                    "pluginParameterName" : "string",
                    "pluginParameterGroup" : "string",
                    "deferControlCallback" : "bool",
                    "isMetaParameter" : "bool",
                    "linkedTo" : "string",
                    "automationID" : "string",
                    "useUndoManager" : "bool",
                    "parentComponent" : "string",
                    "processorId" : "string",
                    "parameterId" : "string"
                },
                "ScriptButton": {
                    "filmstripImage": "string",
                    "numStrips" : "string",
                    "isVertical" : "bool",
                    "scaleFactor" : "double",
                    "radioGroup" : "int",
                    "isMomentary" : "int",
                    "enableMidiLearn" : "bool",
                    "setValueOnClick" : "bool",
                    "mouseCursor" : "string"
            },
                "ScriptSlider": {
                    "mode": "string",
                    "style" : "string",
                    "stepSize" : "double",
                    "middlePosition" : "double",
                    "suffix" : "string",
                    "filmstripImage" : "string",
                    "numStrips" : "int",
                    "isVertical" : "bool",
                    "scaleFactor" : "double",
                    "mouseSensitivity" : "double",
                    "dragDirection" : "string",
                    "showValuePopup" : "string",
                    "showTextBox" : "bool",
                    "scrollWheel" : "bool",
                    "enableMidiLearn" : "bool",
                    "sendValueOnDrag" : "bool",
                    "matrixTargetId" : "string"
            },
                "ScriptComboBox": {
                "items": "string",
                    "fontName" : "string",
                    "fontSize" : "double",
                    "fontStyle" : "string",
                    "enableMidiLearn" : "bool",
                    "popupAlignment" : "string",
                    "useCustomPopup" : "bool"
            },
                "ScriptLabel": {
                "fontName": "string",
                    "fontSize" : "double",
                    "fontStyle" : "string",
                    "alignment" : "string",
                    "editable" : "bool",
                    "multiline" : "bool",
                    "updateEachKey" : "bool"
            },
                "ScriptPanel": {
                "borderSize": "double",
                    "borderRadius" : "double",
                    "opaque" : "bool",
                    "allowDragging" : "int",
                    "allowCallbacks" : "string",
                    "popupMenuItems" : "string",
                    "popupOnRightClick" : "bool",
                    "popupMenuAlign" : "bool",
                    "selectedPopupIndex" : "int",
                    "stepSize" : "double",
                    "enableMidiLearn" : "bool",
                    "holdIsRightClick" : "bool",
                    "isPopupPanel" : "bool",
                    "bufferToImage" : "bool"
            },
                "ScriptTable": {
                "tableIndex": "int",
                    "customColours" : "string"
            },
                "ScriptSliderPack" : {
                "sliderAmount": "int",
                    "stepSize" : "double",
                    "flashActive" : "bool",
                    "showValueOverlay" : "bool",
                    "SliderPackIndex" : "int",
                    "mouseUpCallback" : "bool",
                    "stepSequencerMode" : "bool"
            },
                "ScriptAudioWaveform": {
                "itemColour3": "string",
                    "opaque" : "bool",
                    "showLines" : "bool",
                    "showFileName" : "bool",
                    "sampleIndex" : "int",
                    "enableRange" : "bool",
                    "loadWithLeftClick" : "bool"
            },
                "ScriptImage": {
                "alpha": "double",
                    "fileName" : "string",
                    "offset" : "int",
                    "scale" : "double",
                    "blendMode" : "string",
                    "allowCallbacks" : "string",
                    "popupMenuItems" : "string",
                    "popupOnRightClick" : "bool"
            },
                "ScriptedViewport": {
                "scrollBarThickness": "int",
                    "autoHide" : "bool",
                    "useList" : "bool",
                    "viewPositionX" : "int",
                    "viewPositionY" : "int",
                    "items" : "string",
                    "fontName" : "string",
                    "fontSize" : "double",
                    "fontStyle" : "string",
                    "alignment" : "string"
            },
                "ScriptFloatingTile": {
                "itemColour3": "string",
                    "updateAfterInit" : "bool",
                    "ContentType" : "string",
                    "Font" : "string",
                    "FontSize" : "double",
                    "Data" : "string"
            },
                "ScriptDynamicContainer": {}
            })");

            if (auto obj = propertyTypes.getDynamicObject())
            {
                std::vector<std::pair<Identifier, VarTypeChecker::WrappedType>> commonTypes;

                for (const auto& nv : obj->getProperty("common").getDynamicObject()->getProperties())
                    commonTypes.push_back({ nv.name, VarTypeChecker::getTypeFromString(nv.value.toString()) });

                for (auto& components : obj->getProperties())
                {
                    if (components.name == Identifier("common"))
                        continue;

                    std::map<Identifier, VarTypeChecker::WrappedType> typeMap;

                    for (auto& ct : commonTypes)
                        typeMap[ct.first] = ct.second;

                    for (const auto& property : components.value.getDynamicObject()->getProperties())
                    {
                        auto t = VarTypeChecker::getTypeFromString(property.value.toString());
                        typeMap[property.name] = t;
                    }

                    data[components.name] = typeMap;
                }
            }
        }

        VarTypeChecker::WrappedType getPropertyType(const Identifier& componentId, const Identifier& propertyId) const
        {
            if (data.find(componentId) != data.end())
            {
                const auto& m = data.at(componentId);

                if (m.find(propertyId) != m.end())
                    return m.at(propertyId);
            }

            return {};
        }

        DiagnosticResult checkProperty(const Identifier& componentId, const Identifier& propertyId) const
        {
            auto wt = getPropertyType(componentId, propertyId);

            if (!wt)
            {
                if (data.find(componentId) != data.end())
                {
                    const auto& m = data.at(componentId);

                    StringArray suggestions;

                    for (const auto& s : m)
                        suggestions.add(s.first.toString());

                    return DiagnosticResult::fail("Missing component property " + componentId + "." + propertyId)
                        .withSeverity(DiagnosticResult::Severity::Error)
                        .withErrorCode(DiagnosticResult::ErrorCode::MissingFunctionOrProperty)
                        .withWrongToken(propertyId.toString())
                        .withFuzzySuggestion(suggestions);
                }

                // must be catched otherwise
                jassertfalse;
            }

            return DiagnosticResult::ok();
        }

        bool hasMap(const Identifier& componentId) const { return data.find(componentId) != data.end(); }

        const std::map<Identifier, VarTypeChecker::WrappedType>& getPropertyMap(const Identifier& componentId) const
        {
            if (data.find(componentId) != data.end())
                return data.at(componentId);

            jassertfalse;
            static std::map<Identifier, VarTypeChecker::WrappedType> m;
            return m;
        }

    private:

        std::map<Identifier, std::map<Identifier, VarTypeChecker::WrappedType>> data;
    };

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

        if(sc != nullptr)
            return sc->getScriptObjectProperty(id);

        return var();
    }

    using DR = DiagnosticBase::DiagnosticResult;

    ScriptComponent* getScriptComponent() const
    {
        return p->getScriptingContent()->getComponentWithName(getArgId());
    }

    var getReturnType(const Identifier& methodName, const Array<var>& args) const override
    {
        if (methodName == Identifier("get"))
        {
            if (auto sc = getScriptComponent())
                return sc->get(args[0].toString());

            auto prop = args[0].toString();

            if (prop.isNotEmpty())
            {
                auto id = Identifier(prop);

                if (propertyMap->hasMap(getObjectName()))
                {
                    const auto& map = propertyMap->getPropertyMap(getObjectName());
                    
                    if (map.find(id) != map.end())
                    {
                        auto pt = map.at(id).asPrimitiveType();
                        return new NativeTypeDiagnostic(p, pt, "");
                    }
                }
            }
        }

        return LightweightDiagnostics::getReturnType(methodName, args);
    }

    virtual VarTypeChecker::WrappedType getValueTypeForControlCallback() const 
    {
        return VarTypeChecker::WrappedType(VarTypeChecker::VarTypes::Double);
    }

    static DiagnosticResult createMissingComponentError(const String& id)
    {
        return DiagnosticResult::fail("Can't find component with ID `" + id + "`")
            .withSeverity(DiagnosticResult::Severity::Warning)
            .withErrorCode(DiagnosticResult::ErrorCode::RuntimeIdFailure)
            .withSuggestion("If this script hasn't been compiled once, compile it to create the object");
    }

    DiagnosticResult performCustomCheck(const CustomCheckData& cd) final override
    {
        auto sc = getScriptComponent();
        
        if (sc == nullptr)
            return createMissingComponentError(getArgId());
        
        if (cd.methodName == Identifier("setControlCallback"))
        {
            auto f = cd.resolvedArguments[cd.parameterId];

            CallbackParseOptions cpo;

            cpo.callback = f;
            cpo.thisObj = this;

            auto ok = cpo.parseCheckString(cd.checkString, true);

            auto currentParentValidator = dynamic_cast<HiseJavascriptEngine::RootObject::ObjectTypeMap*>(currentShadowParser);
            
            
            String msg;

            auto realtimeProperties = sc->checkRealtimeRequirementProperties();

            

            if (!realtimeProperties.isEmpty())
            {
                cpo.strictness = WeakCallbackHolder::CallableObject::StrictnessLevel::Strict;

                msg << "Realtime safety required because the control callback could be executed in the audio thread. Deactivate the propert";
                msg << (realtimeProperties.size() > 1 ? "ies: " : "y: ");

                for (auto& ids : realtimeProperties)
                    msg << "`" << ids.toString() << "`, ";

                msg = msg.upToLastOccurrenceOf(", ", false, false);

                msg << " for the component that is assigned to this callback to loose this restriction";
            }
            else
            {
                cpo.strictness = WeakCallbackHolder::CallableObject::StrictnessLevel::Unsafe;
            }

            ScopedValueSetter<String> annotation(currentParentValidator->callscopeAnnotation, msg);

            cpo.argTypes[0].customData = getArgId();
            cpo.argTypes[1].typeInfo = getValueTypeForControlCallback();

            if (!ok.wasOk())
                return DiagnosticResult::fail(ok.getErrorMessage()).withWrongToken(cd.parameterId.toString());

            return shadowParseCallbackWithArgType(cpo);

        }

        if (cd.methodName == Identifier("set") || cd.methodName == Identifier("get"))
        {            
            auto propId = cd.resolvedArguments["propertyName"];
            auto props = sc->getAllProperties();

            

            if (cd.category == Category::RuntimeIdCheck)
            {
                // allow id and type queries
                if (cd.methodName == Identifier("get") && (propId == "id" || propId == "type"))
                    return DR::ok();

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

    juce::SharedResourcePointer<PropertyMap> propertyMap;

    
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

struct KeyEventTypeObject : public BaseObjects::FunctionJSONObject
{
    KeyEventTypeObject(ProcessorWithScriptingContent* p, const ValueTree&, const String&) :
        FunctionJSONObject(p, ValueTree("keyEvent"), "")
    {
        initClassTree(ValueTree(getClassName()));

        auto bool_ = var(new NativeTypeDiagnostic(p, VarTypeChecker::Integer, ""));
        auto int_ = bool_;
        auto String_ = var(new NativeTypeDiagnostic(p, VarTypeChecker::String, ""));
        
        apiConstants.set("isFocusChange", bool_); 
        apiConstants.set("hasFocus", bool_); 
        apiConstants.set("character", String_); 
        apiConstants.set("specialKey", bool_); 
        apiConstants.set("isWhitespace", bool_); 
        apiConstants.set("isLetter", bool_); 
        apiConstants.set("isDigit", bool_); 
        apiConstants.set("keyCode", int_); 
        apiConstants.set("description", String_); 
        apiConstants.set("shift", bool_); 
        apiConstants.set("cmd", bool_); 
        apiConstants.set("alt", bool_); 
    }

    static Identifier getClassName() { RETURN_STATIC_IDENTIFIER("keyEvent"); }
};


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
        if (cd.methodName == Identifier("setPropertiesFromJSON"))
        {
            auto name = cd.resolvedArguments.getValueAt(0).toString();

            if (auto c = p->getScriptingContent()->getComponentWithName(name))
            {
                auto cType = c->getObjectName();

                if (auto obj = cd.resolvedArguments.getValueAt(1).getDynamicObject())
                {
                    for (auto& nv : obj->getProperties())
                    {
                        auto ok = m->checkProperty(cType, nv.name);

                        if (ok.shouldReport())
                            return ok;
                    }
                }
            }
            else
            {
                return BaseObjects::ScriptComponentDiagnosticBase::createMissingComponentError(name);
            }

            return DiagnosticResult::ok();
        }

        return LightweightDiagnostics::performCustomCheck(cd);
    }

    SharedResourcePointer<BaseObjects::ScriptComponentDiagnosticBase::PropertyMap> m;

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
        if (methodName == Identifier("getAllComponents"))
        {
            auto regex = args[0].toString();

            Array<var> list;

            bool getAll = regex == ".*";

            auto c = p->getScriptingContent();

            for (int i = 0; i < c->getNumComponents(); i++)
            {
                auto sc = c->getComponent(i);
                
                if (getAll || RegexFunctions::matchesWildcard(regex, sc->getName().toString()))
                {
                    auto id = sc->getName().toString();

                    auto typed = dynamic_cast<HiseJavascriptEngine::RootObject::ObjectTypeMap*>(currentShadowParser);
                    list.add(var(typed->createAnonoymousDiagnosticObject(sc->getObjectName(), id).get()));
                }
            }

            return var(list);
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

    VarTypeChecker::WrappedType getValueTypeForControlCallback() const override
    {
        return VarTypeChecker::WrappedType(VarTypeChecker::VarTypes::Integer);
    }

};

struct ScriptingApi::Content::ScriptComboBox::Diagnostics : public BaseObjects::ScriptComponentDiagnosticBase
{
    Diagnostics(ProcessorWithScriptingContent* pwsc, const ValueTree& v, const String& argId) :
        ScriptComponentDiagnosticBase(pwsc, v, argId)
    {}

    VarTypeChecker::WrappedType getValueTypeForControlCallback() const override
    {
        return VarTypeChecker::WrappedType(VarTypeChecker::VarTypes::Integer);
    }
};

struct ScriptingApi::Content::ScriptLabel::Diagnostics : public BaseObjects::ScriptComponentDiagnosticBase
{
    Diagnostics(ProcessorWithScriptingContent* pwsc, const ValueTree& v, const String& argId) :
        ScriptComponentDiagnosticBase(pwsc, v, argId)
    {}

    VarTypeChecker::WrappedType getValueTypeForControlCallback() const override
    {
        return VarTypeChecker::WrappedType(VarTypeChecker::VarTypes::String);
    }
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
        apiConstants.set("data", var(new DynamicObject()));

        addDiagnostic("setMouseCallback", [](DiagnosticBase* b, const Identifier& id, const Array<var>& args)
        {
            auto typed = dynamic_cast<BaseObjects::ScriptComponentDiagnosticBase*>(b);
            auto cl = typed->getPropertyFromObjectOrWrittenProperties("allowCallbacks").toString();

            CallbackParseOptions cb;
            cb.callback = args[0];

            CallbackParseOptions::ArgumentData ed;
            ed.customData = Helpers::makeMetaId<EventTypeObject>().toString() + "." + cl;
            ed.id = "event";
            ed.typeInfo = VarTypeChecker::WrappedType(VarTypeChecker::JSON);

            cb.argTypes.push_back(ed);
            cb.thisObj = typed;

            return typed->shadowParseCallbackWithArgType(cb);
        });
    }

    VarTypeChecker::WrappedType getValueTypeForControlCallback() const override
    {
        return VarTypeChecker::WrappedType(VarTypeChecker::VarTypes::DataContent);
    }

    var dataObject;
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

struct ExtraDefinitionsObj : public BaseObjects::FunctionJSONObject
{
    static Identifier getClassName() { return "ExtraDefinitions"; }

    ExtraDefinitionsObj(ProcessorWithScriptingContent* p, const ValueTree& v, const String& argId) :
        FunctionJSONObject(p, v, argId)
    {
        ValueTree vt(getClassName());

        auto mc = p->getMainController_();
        auto obj = dynamic_cast<GlobalSettingManager*>(mc)->getSettingsObject().getExtraDefinitionsAsObject();

        for (const auto& nv : obj.getDynamicObject()->getProperties())
            vt.setProperty(nv.name, nv.value, nullptr);

        initClassTree(vt);
    }
};

struct SystemStatsObj : public BaseObjects::FunctionJSONObject
{
    static Identifier getClassName() { return "SystemStats"; }

    SystemStatsObj(ProcessorWithScriptingContent* p, const ValueTree& v, const String& argId) :
        FunctionJSONObject(p, v, argId)
    {
        ValueTree vt(getClassName());

        auto obj = ScriptingApi::Engine::getSystemStatsStatic();

        for (const auto& nv : obj.getDynamicObject()->getProperties())
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
                cb.argTypes[1].customData << "." << cd.resolvedArguments["functionName"].toString();

                return shadowParseCallbackWithArgType(cb);
            }
        }

        return DiagnosticResult::ok();
    }
};

struct ScriptingObjects::ScriptBroadcaster::Diagnostics : public LightweightDiagnostics
{
    Diagnostics(ProcessorWithScriptingContent* pwsc, const ValueTree& v, const String& argId) :
        LightweightDiagnostics(pwsc, v, argId)
    {
        auto mt = JSON::parse(argId);

        if (auto obj = mt.getDynamicObject())
        {
            metadata = new ScriptBroadcaster::Metadata(mt, true);

            if (auto ar = obj->getProperty("args").getArray())
            {
                for (auto& v : *ar)
                {
                    auto p = v.toString();

                    if (p.containsChar(':'))
                    {
                        auto id = Identifier(p.upToFirstOccurrenceOf(":", false, false));
                        auto typeString = p.fromFirstOccurrenceOf(":", false, false);
                        auto at = VarTypeChecker::getTypeFromString(typeString);
                        args.push_back({ id, at });
                    }
                    else
                    {
                        args.push_back({ Identifier(p), {} });
                    }
                }
            }
        }
    };

    static DiagnosticResult forEachOrSelf(const var& obj, const std::function<DiagnosticResult(const var&)>& f)
    {
        if (auto ar = obj.getArray())
        {
            auto ok = DiagnosticResult::ok();

            for (auto& v : *ar)
                ok = DiagnosticResult::max(ok, f(v));

            return ok;
        }
        else
            return f(obj);
    }

    DiagnosticResult performCustomCheck(const CustomCheckData& cd) override
    {
        auto cat = parseCategory(cd.checkString);

        if (cat == Category::ObjectType)
        {
            auto list = JSON::parse(cd.checkString.fromFirstOccurrenceOf("[", true, false));

            if (auto ar = list.getArray())
            {
                auto numExpected = ar->size();
                auto numActual = args.size();

                if (numExpected != numActual)
                    return DiagnosticResult::fail("broadcaster argument amount mismatch for method " + cd.methodName.toString())
                    .withExpectation(String(numExpected), String(numActual))
                    .withErrorCode(DiagnosticResult::ErrorCode::ArgMismatch)
                    .withSuggestion("metadata with a argument list that matches the expected argument count");

                Array<VarTypeChecker::WrappedType> thisTypes;

                for (auto& v : *ar)
                {
                    auto type = VarTypeChecker::getTypeFromString(v.toString());
                    jassert(type);
                    thisTypes.add(type);
                }

                for (int i = 0; i < args.size(); i++)
                {
                    auto e = args[i].second;
                    auto a = thisTypes[i];

                    if (e && !e.matches(a, false))
                        return DiagnosticResult::fail("broadcaster argument type mismatch at argument " + String(i + 1))
                        .withExpectation(VarTypeChecker::getTypeName(e).toString(), VarTypeChecker::getTypeName(a).toString())
                        .withErrorCode(DiagnosticResult::ErrorCode::CallbackArgMismatch);

                    args[i].second = a;
                }

                return DiagnosticResult::ok();
            }
            
            return DiagnosticResult::fail("must be an JSON formatted array");
        }

        if (cat == Category::RuntimeIdCheck)
        {
            auto targetType = cd.checkString.fromFirstOccurrenceOf(".", false, false);

            auto v = cd.resolvedArguments[cd.parameterId];
            auto sc = p->getScriptingContent();

            if (targetType == "ScriptComponent")
            {
                auto typeList = &targetTypeList;

                auto ok = forEachOrSelf(v, [sc, typeList, this](const var& value)
                {
                    if (value.isString())
                    {
                        if (auto c = sc->getComponentWithName(value.toString()))
                        {
                            typeList->addIfNotAlreadyThere(c->getObjectName());
                            return DiagnosticResult::ok();
                        }
                        else
                            return DiagnosticResult::fail("Can't find component with ID " + value.toString())
                            .withErrorCode(DiagnosticResult::ErrorCode::RuntimeIdFailure);
                    }
                    else if (auto lb = dynamic_cast<BaseObjects::ScriptComponentDiagnosticBase*>(value.getObject()))
                    {
                        typeList->addIfNotAlreadyThere(lb->getObjectName());
                        return DiagnosticResult::ok();
                    }

                    return DiagnosticResult::fail("unknown type for component list " + value.toString())
                        .withErrorCode(DiagnosticResult::ErrorCode::TypeMismatch);
                });

                if (targetTypeList.size() == 1)
                {
                    auto specialType = VarTypeChecker::getTypeFromString(targetTypeList[0].toString());

                    for (auto& t : args)
                    {
                        if (t.second.isPrimitive())
                            continue;

                        if (t.second.asComplexType() == VarTypeChecker::TypedObject::ScriptComponent)
                            t.second = specialType;
                    }
                }

                return ok;
            }
            if (targetType == "ScriptComponentProperty")
            {
                SharedResourcePointer<BaseObjects::ScriptComponentDiagnosticBase::PropertyMap> pm;

                auto ok = forEachOrSelf(v, [&](const var& obj)
                {
                    Identifier id(obj.toString());

                    for (auto& t : targetTypeList)
                    {
                        auto typeValue = pm->getPropertyType(t, id);

                        if (!typeValue)
                            return DiagnosticResult::fail("Unknown property " + t.toString() + "." + id.toString());
                    }
                    
                    return DiagnosticResult::ok();
                });

                return ok;
            }
        }

        if (cat == Category::CallbackArgCheck)
        {
            auto f = cd.resolvedArguments[cd.parameterId];

            if (dynamic_cast<WeakCallbackHolder::CallableObject*>(f.getObject()) == nullptr)
                return DiagnosticResult::ok();

            auto undefinedArgs = false;

            for (auto& a : args)
                undefinedArgs |= !(bool)a.second;

            if (undefinedArgs)
            {
                auto dr = DiagnosticResult("Can't parse broadcaster callback because of missing type info")
                    .withSuggestion("Use a `attachTo` method to defer the types or annotate the types in the `args` metadata property")
                    .withSeverity(DiagnosticResult::Severity::Warning)
                    .withErrorCode(DiagnosticResult::ErrorCode::CallbackArgMismatch);

                return dr;
            }

            using RO = HiseJavascriptEngine::RootObject;

            auto thisObj = cd.resolvedArguments.getValueAt(0);

            

            auto currentParentValidator = dynamic_cast<RO::ObjectTypeMap*>(currentShadowParser);
            jassert(currentParentValidator != nullptr);

            auto& s = currentParentValidator->getCurrentScope();
            Array<var> thisArgs;

            static const Array<Identifier> indexMethods({
                Identifier("addComponentPropertyListener"),
                Identifier("addComponentValueListener")
            });

            if (indexMethods.contains(cd.methodName))
                thisArgs.add(var(currentParentValidator->createAnonoymousDiagnosticObject("int", "").get()));

            for (const auto& a : args)
            {
                auto className = VarTypeChecker::getTypeName(a.second);
                auto argId = "";

                if (auto ptr = currentParentValidator->createAnonoymousDiagnosticObject(className, argId))
                {
                    thisArgs.add(var(ptr.get()));
                }
            }

            auto strictness = WeakCallbackHolder::CallableObject::StrictnessLevel::Unsafe;

            if (auto ifo = dynamic_cast<RO::InlineFunction::Object*>(f.getObject()))
                return ifo->invokeShadowParse(s, var::NativeFunctionArgs(thisObj, thisArgs.getRawDataPointer(), thisArgs.size()), strictness).second;

            if (auto fo = dynamic_cast<RO::FunctionObject*>(f.getObject()))
                return fo->invokeShadowParse(s, var::NativeFunctionArgs(thisObj, thisArgs.getRawDataPointer(), thisArgs.size()), strictness).second;

            return DiagnosticResult::ok();

        }

        return DiagnosticResult::fail("unimplemented")
            .withErrorCode(DiagnosticResult::ErrorCode::InternalError);
    }
    

    ScopedPointer<ScriptBroadcaster::Metadata> metadata;

    std::vector<std::pair<Identifier, VarTypeChecker::WrappedType>> args;
    Array<Identifier> targetTypeList;
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
    registerType<ScriptingObjects::ScriptBroadcaster>();
    registerType<ArrayWithKnownElementType>();

    registerType<ScriptingApi::Synth>();

    //registerType<ScriptingApi::Server>();
    registerType<ScriptingApi::Content>();
    registerType<ScriptingObjects::ScriptedLookAndFeel>();
    
    registerMetaType<BaseObjects::ScriptComponentDiagnosticBase>();
    registerMetaType<EventTypeObject>();
    registerMetaType<KeyEventTypeObject>();
    registerMetaType<LafObj>();
    registerMetaType<ProjectInfoObj>();
    registerMetaType<ExtraDefinitionsObj>();
    registerMetaType<SystemStatsObj>();
}



}
