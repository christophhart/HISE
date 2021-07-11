/*
  ==============================================================================

   This file is part of the JUCE library.
   Copyright (c) 2020 - Raw Material Software Limited

   JUCE is an open source library subject to commercial or open-source
   licensing.

   By using JUCE, you agree to the terms of both the JUCE 6 End-User License
   Agreement and JUCE Privacy Policy (both effective as of the 16th June 2020).

   End User License Agreement: www.juce.com/juce-6-licence
   Privacy Policy: www.juce.com/juce-privacy-policy

   Or: You may also use this code under the terms of the GPL v3 (see
   www.gnu.org/licenses).

   JUCE IS PROVIDED "AS IS" WITHOUT ANY WARRANTY, AND ALL WARRANTIES, WHETHER
   EXPRESSED OR IMPLIED, INCLUDING MERCHANTABILITY AND FITNESS FOR PURPOSE, ARE
   DISCLAIMED.

  ==============================================================================
*/

#include "../../Application/jucer_Headers.h"
#include "../../Application/jucer_Application.h"
#include "../jucer_ObjectTypes.h"
#include "../jucer_UtilityFunctions.h"
#include "../UI/jucer_JucerCommandIDs.h"
#include "../UI/jucer_ComponentOverlayComponent.h"
#include "jucer_ComponentNameProperty.h"
#include "../Properties/jucer_PositionPropertyBase.h"
#include "../Properties/jucer_ComponentColourProperty.h"
#include "../UI/jucer_TestComponent.h"

static String getTypeInfoName (const std::type_info& info)
{
   #if JUCE_MSVC
    return info.raw_name();
   #else
    return info.name();
   #endif
}

//==============================================================================
ComponentTypeHandler::ComponentTypeHandler (const String& typeName_,
                                            const String& className_,
                                            const std::type_info& componentClass_,
                                            const int defaultWidth_,
                                            const int defaultHeight_)
    : typeName (typeName_),
      className (className_),
      componentClassRawName (getTypeInfoName (componentClass_)),
      defaultWidth (defaultWidth_),
      defaultHeight (defaultHeight_)
{
}

Component* ComponentTypeHandler::createCopyOf (JucerDocument* document, Component& existing)
{
    jassert (getHandlerFor (existing) == this);

    Component* const newOne = createNewComponent (document);
    std::unique_ptr<XmlElement> xml (createXmlFor (&existing, document->getComponentLayout()));

    if (xml != nullptr)
        restoreFromXml (*xml, newOne, document->getComponentLayout());

    return newOne;
}

ComponentOverlayComponent* ComponentTypeHandler::createOverlayComponent (Component* child, ComponentLayout& layout)
{
    return new ComponentOverlayComponent (child, layout);
}

static void dummyMenuCallback (int, int) {}

void ComponentTypeHandler::showPopupMenu (Component*, ComponentLayout& layout)
{
    PopupMenu m;

    ApplicationCommandManager* commandManager = &ProjucerApplication::getCommandManager();

    m.addCommandItem (commandManager, JucerCommandIDs::toFront);
    m.addCommandItem (commandManager, JucerCommandIDs::toBack);
    m.addSeparator();

    if (layout.getSelectedSet().getNumSelected() > 1)
    {
        m.addCommandItem (commandManager, JucerCommandIDs::alignTop);
        m.addCommandItem (commandManager, JucerCommandIDs::alignRight);
        m.addCommandItem (commandManager, JucerCommandIDs::alignBottom);
        m.addCommandItem (commandManager, JucerCommandIDs::alignLeft);
        m.addSeparator();
    }

    m.addCommandItem (commandManager, StandardApplicationCommandIDs::cut);
    m.addCommandItem (commandManager, StandardApplicationCommandIDs::copy);
    m.addCommandItem (commandManager, StandardApplicationCommandIDs::paste);
    m.addCommandItem (commandManager, StandardApplicationCommandIDs::del);

    m.showMenuAsync (PopupMenu::Options(),
                     ModalCallbackFunction::create (dummyMenuCallback, 0));
}

JucerDocument* ComponentTypeHandler::findParentDocument (Component* component)
{
    Component* p = component->getParentComponent();

    while (p != nullptr)
    {
        if (JucerDocumentEditor* const ed = dynamic_cast<JucerDocumentEditor*> (p))
            return ed->getDocument();

        if (TestComponent* const t = dynamic_cast<TestComponent*> (p))
            return t->getDocument();

        p = p->getParentComponent();
    }

    return nullptr;
}

//==============================================================================
bool ComponentTypeHandler::canHandle (Component& component) const
{
    return componentClassRawName == getTypeInfoName (typeid (component));
}

ComponentTypeHandler* ComponentTypeHandler::getHandlerFor (Component& component)
{
    for (int i = 0; i < ObjectTypes::numComponentTypes; ++i)
        if (ObjectTypes::componentTypeHandlers[i]->canHandle (component))
            return ObjectTypes::componentTypeHandlers[i];

    jassertfalse;
    return nullptr;
}

ComponentTypeHandler* ComponentTypeHandler::getHandlerForXmlTag (const String& tagName)
{
    for (int i = 0; i < ObjectTypes::numComponentTypes; ++i)
        if (ObjectTypes::componentTypeHandlers[i]->getXmlTagName().equalsIgnoreCase (tagName))
            return ObjectTypes::componentTypeHandlers[i];

    return nullptr;
}

XmlElement* ComponentTypeHandler::createXmlFor (Component* comp, const ComponentLayout* layout)
{
    XmlElement* e = new XmlElement (getXmlTagName());

    e->setAttribute ("name", comp->getName());
    e->setAttribute ("id", String::toHexString (getComponentId (comp)));
    e->setAttribute ("memberName", comp->getProperties() ["memberName"].toString());
    e->setAttribute ("virtualName", comp->getProperties() ["virtualName"].toString());
    e->setAttribute ("explicitFocusOrder", comp->getExplicitFocusOrder());

    RelativePositionedRectangle pos (getComponentPosition (comp));
    pos.updateFromComponent (*comp, layout);
    pos.applyToXml (*e);

    if (SettableTooltipClient* const ttc = dynamic_cast<SettableTooltipClient*> (comp))
        if (ttc->getTooltip().isNotEmpty())
            e->setAttribute ("tooltip", ttc->getTooltip());

    for (int i = 0; i < colours.size(); ++i)
    {
        if (comp->isColourSpecified (colours[i]->colourId))
        {
            e->setAttribute (colours[i]->xmlTagName,
                             comp->findColour (colours[i]->colourId).toString());
        }
    }

    return e;
}

bool ComponentTypeHandler::restoreFromXml (const XmlElement& xml,
                                           Component* comp,
                                           const ComponentLayout* layout)
{
    jassert (xml.hasTagName (getXmlTagName()));

    if (! xml.hasTagName (getXmlTagName()))
        return false;

    comp->setName (xml.getStringAttribute ("name", comp->getName()));
    setComponentId (comp, xml.getStringAttribute ("id").getHexValue64());
    comp->getProperties().set ("memberName", xml.getStringAttribute ("memberName"));
    comp->getProperties().set ("virtualName", xml.getStringAttribute ("virtualName"));
    comp->setExplicitFocusOrder (xml.getIntAttribute ("explicitFocusOrder"));

    RelativePositionedRectangle currentPos (getComponentPosition (comp));
    currentPos.updateFromComponent (*comp, layout);

    RelativePositionedRectangle rpr;
    rpr.restoreFromXml (xml, currentPos);

    jassert (layout != nullptr);
    setComponentPosition (comp, rpr, layout);

    if (SettableTooltipClient* const ttc = dynamic_cast<SettableTooltipClient*> (comp))
        ttc->setTooltip (xml.getStringAttribute ("tooltip"));

    for (int i = 0; i < colours.size(); ++i)
    {
        const String col (xml.getStringAttribute (colours[i]->xmlTagName, String()));

        if (col.isNotEmpty())
            comp->setColour (colours[i]->colourId, Colour::fromString (col));
    }

    return true;
}

//==============================================================================
int64 ComponentTypeHandler::getComponentId (Component* comp)
{
    if (comp == nullptr)
        return 0;

    int64 compId = comp->getProperties() ["jucerCompId"].toString().getHexValue64();

    if (compId == 0)
    {
        compId = Random::getSystemRandom().nextInt64();
        setComponentId (comp, compId);
    }

    return compId;
}

void ComponentTypeHandler::setComponentId (Component* comp, const int64 newID)
{
    jassert (comp != nullptr);
    if (newID != 0)
        comp->getProperties().set ("jucerCompId", String::toHexString (newID));
}

RelativePositionedRectangle ComponentTypeHandler::getComponentPosition (Component* comp)
{
    RelativePositionedRectangle rp;
    rp.rect = PositionedRectangle (comp->getProperties() ["pos"]);
    rp.relativeToX = comp->getProperties() ["relativeToX"].toString().getHexValue64();
    rp.relativeToY = comp->getProperties() ["relativeToY"].toString().getHexValue64();
    rp.relativeToW = comp->getProperties() ["relativeToW"].toString().getHexValue64();
    rp.relativeToH = comp->getProperties() ["relativeToH"].toString().getHexValue64();

    return rp;
}

void ComponentTypeHandler::setComponentPosition (Component* comp,
                                                 const RelativePositionedRectangle& newPos,
                                                 const ComponentLayout* layout)
{
    comp->getProperties().set ("pos", newPos.rect.toString());
    comp->getProperties().set ("relativeToX", String::toHexString (newPos.relativeToX));
    comp->getProperties().set ("relativeToY", String::toHexString (newPos.relativeToY));
    comp->getProperties().set ("relativeToW", String::toHexString (newPos.relativeToW));
    comp->getProperties().set ("relativeToH", String::toHexString (newPos.relativeToH));

    comp->setBounds (newPos.getRectangle (Rectangle<int> (0, 0, comp->getParentWidth(), comp->getParentHeight()),
                                          layout));
}

//==============================================================================
class TooltipProperty   : public ComponentTextProperty <Component>
{
public:
    TooltipProperty (Component* comp, JucerDocument& doc)
        : ComponentTextProperty<Component> ("tooltip", 1024, true, comp, doc)
    {
    }

    String getText() const override
    {
        SettableTooltipClient* ttc = dynamic_cast<SettableTooltipClient*> (component);
        return ttc->getTooltip();
    }

    void setText (const String& newText) override
    {
        document.perform (new SetTooltipAction (component, *document.getComponentLayout(), newText),
                          "Change tooltip");
    }

private:
    class SetTooltipAction  : public ComponentUndoableAction <Component>
    {
    public:
        SetTooltipAction (Component* const comp, ComponentLayout& l, const String& newValue_)
            : ComponentUndoableAction<Component> (comp, l),
              newValue (newValue_)
        {
            SettableTooltipClient* ttc = dynamic_cast<SettableTooltipClient*> (comp);
            jassert (ttc != nullptr);
            oldValue = ttc->getTooltip();
        }

        bool perform()
        {
            showCorrectTab();

            if (SettableTooltipClient* ttc = dynamic_cast<SettableTooltipClient*> (getComponent()))
            {
                ttc->setTooltip (newValue);
                changed();
                return true;
            }

            return false;
        }

        bool undo()
        {
            showCorrectTab();

            if (SettableTooltipClient* ttc = dynamic_cast<SettableTooltipClient*> (getComponent()))
            {
                ttc->setTooltip (oldValue);
                changed();
                return true;
            }

            return false;
        }

        String newValue, oldValue;
    };
};

//==============================================================================
class ComponentPositionProperty   : public PositionPropertyBase
{
public:
    ComponentPositionProperty (Component* comp,
                               JucerDocument& doc,
                               const String& name,
                               ComponentPositionDimension dimension_)
        : PositionPropertyBase (comp, name, dimension_,
                                true, true,
                                doc.getComponentLayout()),
          document (doc)
    {
        document.addChangeListener (this);
    }

    ~ComponentPositionProperty()
    {
        document.removeChangeListener (this);
    }

    void setPosition (const RelativePositionedRectangle& newPos)
    {
        auto* l = document.getComponentLayout();

        if (l->getSelectedSet().getNumSelected() > 1)
            positionOtherSelectedComponents (ComponentTypeHandler::getComponentPosition (component), newPos);

        l->setComponentPosition (component, newPos, true);
    }

    RelativePositionedRectangle getPosition() const
    {
        return ComponentTypeHandler::getComponentPosition (component);
    }

private:
    JucerDocument& document;

    void positionOtherSelectedComponents (const RelativePositionedRectangle& oldPos, const RelativePositionedRectangle& newPos)
    {
        for (auto* s : document.getComponentLayout()->getSelectedSet())
        {
            if (s != component)
            {
                auto currentPos = ComponentTypeHandler::getComponentPosition (s);
                auto diff = 0.0;

                if (dimension == ComponentPositionDimension::componentX)
                {
                    diff = newPos.rect.getX() - oldPos.rect.getX();
                    currentPos.rect.setX (currentPos.rect.getX() + diff);
                }
                else if (dimension == ComponentPositionDimension::componentY)
                {
                    diff = newPos.rect.getY() - oldPos.rect.getY();
                    currentPos.rect.setY (currentPos.rect.getY() + diff);
                }
                else if (dimension == ComponentPositionDimension::componentWidth)
                {
                    diff = newPos.rect.getWidth() - oldPos.rect.getWidth();
                    currentPos.rect.setWidth (currentPos.rect.getWidth() + diff);
                }
                else if (dimension == ComponentPositionDimension::componentHeight)
                {
                    diff = newPos.rect.getHeight() - oldPos.rect.getHeight();
                    currentPos.rect.setHeight (currentPos.rect.getHeight() + diff);
                }

                document.getComponentLayout()->setComponentPosition (s, currentPos, true);
            }
        }
    }
};


//==============================================================================
class FocusOrderProperty   : public ComponentTextProperty <Component>
{
public:
    FocusOrderProperty (Component* comp, JucerDocument& doc)
        : ComponentTextProperty <Component> ("focus order", 8, false, comp, doc)
    {
    }

    String getText() const override
    {
        return String (component->getExplicitFocusOrder());
    }

    void setText (const String& newText) override
    {
        document.perform (new SetFocusOrderAction (component, *document.getComponentLayout(), jmax (0, newText.getIntValue())),
                          "Change focus order");
    }

private:
    class SetFocusOrderAction  : public ComponentUndoableAction <Component>
    {
    public:
        SetFocusOrderAction (Component* const comp, ComponentLayout& l, const int newOrder_)
            : ComponentUndoableAction <Component> (comp, l),
              newValue (newOrder_)
        {
            oldValue = comp->getExplicitFocusOrder();
        }

        bool perform()
        {
            showCorrectTab();
            getComponent()->setExplicitFocusOrder (newValue);
            changed();
            return true;
        }

        bool undo()
        {
            showCorrectTab();
            getComponent()->setExplicitFocusOrder (oldValue);
            changed();
            return true;
        }

        int newValue, oldValue;
    };
};

//==============================================================================
void ComponentTypeHandler::getEditableProperties (Component* component,
                                                  JucerDocument& document,
                                                  Array<PropertyComponent*>& props,
                                                  bool multipleSelected)
{
    if (! multipleSelected)
    {
        props.add (new ComponentMemberNameProperty (component, document));
        props.add (new ComponentNameProperty (component, document));
        props.add (new ComponentVirtualClassProperty (component, document));

        if (dynamic_cast<SettableTooltipClient*> (component) != nullptr)
            props.add (new TooltipProperty (component, document));

        props.add (new FocusOrderProperty (component, document));
    }

    props.add (new ComponentPositionProperty (component, document, "x", ComponentPositionProperty::componentX));
    props.add (new ComponentPositionProperty (component, document, "y", ComponentPositionProperty::componentY));
    props.add (new ComponentPositionProperty (component, document, "width", ComponentPositionProperty::componentWidth));
    props.add (new ComponentPositionProperty (component, document, "height", ComponentPositionProperty::componentHeight));
}

void ComponentTypeHandler::addPropertiesToPropertyPanel (Component* comp, JucerDocument& document,
                                                         PropertyPanel& panel, bool multipleSelected)
{
    Array <PropertyComponent*> props;
    getEditableProperties (comp, document, props, multipleSelected);

    panel.addSection (getClassName (comp), props);
}

void ComponentTypeHandler::registerEditableColour (int colourId,
                                                   const String& colourIdCode,
                                                   const String& colourName, const String& xmlTagName)
{
    ComponentColourInfo* const c = new ComponentColourInfo();

    c->colourId = colourId;
    c->colourIdCode = colourIdCode;
    c->colourName = colourName;
    c->xmlTagName = xmlTagName;

    colours.add (c);
}

void ComponentTypeHandler::addColourProperties (Component* component,
                                                JucerDocument& document,
                                                Array<PropertyComponent*>& props)
{
    for (int i = 0; i < colours.size(); ++i)
        props.add (new ComponentColourIdProperty (component, document,
                                                  colours[i]->colourId,
                                                  colours[i]->colourName,
                                                  true));
}

String ComponentTypeHandler::getColourIntialisationCode (Component* component,
                                                         const String& objectName)
{
    String s;

    for (int i = 0; i < colours.size(); ++i)
    {
        if (component->isColourSpecified (colours[i]->colourId))
        {
            s << objectName << "->setColour ("
              << colours[i]->colourIdCode
              << ", "
              << CodeHelpers::colourToCode (component->findColour (colours[i]->colourId))
              << ");\n";
        }
    }

    return s;
}

//==============================================================================
void ComponentTypeHandler::fillInGeneratedCode (Component* component, GeneratedCode& code)
{
    const String memberVariableName (code.document->getComponentLayout()->getComponentMemberVariableName (component));

    fillInMemberVariableDeclarations (code, component, memberVariableName);
    fillInCreationCode (code, component, memberVariableName);
    fillInDeletionCode (code, component, memberVariableName);
    fillInResizeCode (code, component, memberVariableName);
}

void ComponentTypeHandler::fillInMemberVariableDeclarations (GeneratedCode& code, Component* component, const String& memberVariableName)
{
    String clsName (component->getProperties() ["virtualName"].toString());

    if (clsName.isNotEmpty())
        clsName = build_tools::makeValidIdentifier (clsName, false, false, true);
    else
        clsName = getClassName (component);

    code.privateMemberDeclarations
        << "std::unique_ptr<" << clsName << "> " << memberVariableName << ";\n";
}

void ComponentTypeHandler::fillInResizeCode (GeneratedCode& code, Component* component, const String& memberVariableName)
{
    const RelativePositionedRectangle pos (getComponentPosition (component));

    String x, y, w, h, r;
    positionToCode (pos, code.document->getComponentLayout(), x, y, w, h);

    r << memberVariableName << "->setBounds ("
      << x << ", " << y << ", " << w << ", " << h << ");\n";

    if (pos.rect.isPositionAbsolute() && ! code.document->getComponentLayout()->isComponentPositionRelative (component))
        code.constructorCode += r + "\n";
    else
        code.getCallbackCode (String(), "void", "resized()", false) += r;
}

String ComponentTypeHandler::getCreationParameters (GeneratedCode&, Component*)
{
    return {};
}

void ComponentTypeHandler::fillInCreationCode (GeneratedCode& code, Component* component, const String& memberVariableName)
{
    String params (getCreationParameters (code, component));
    const String virtualName (component->getProperties() ["virtualName"].toString());

    String s;
    s << memberVariableName << ".reset (new ";

    if (virtualName.isNotEmpty())
        s << build_tools::makeValidIdentifier (virtualName, false, false, true);
    else
        s << getClassName (component);

    if (params.isEmpty())
    {
        s << "());\n";
    }
    else
    {
        StringArray lines;
        lines.addLines (params);

        params = lines.joinIntoString ("\n" + String::repeatedString (" ", s.length() + 2));

        s << " (" << params << "));\n";
    }

    s << "addAndMakeVisible (" << memberVariableName << ".get());\n";


    if (SettableTooltipClient* ttc = dynamic_cast<SettableTooltipClient*> (component))
    {
        if (ttc->getTooltip().isNotEmpty())
        {
            s << memberVariableName << "->setTooltip ("
              << quotedString (ttc->getTooltip(), code.shouldUseTransMacro())
              << ");\n";
        }
    }

    if (component->getExplicitFocusOrder() > 0)
        s << memberVariableName << "->setExplicitFocusOrder ("
          << component->getExplicitFocusOrder()
          << ");\n";

    code.constructorCode += s;
}

void ComponentTypeHandler::fillInDeletionCode (GeneratedCode& code, Component*,
                                               const String& memberVariableName)
{
    code.destructorCode
        << memberVariableName << " = nullptr;\n";
}
