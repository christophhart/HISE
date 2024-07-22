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
 *   GNU General Public License for more details.f
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

namespace hise {
namespace multipage {
using namespace juce;

bool ApiObject::Helpers::callRecursive(const var& obj, const std::function<bool(const var&)>& f)
{
	if(f(obj))
		return true;

	if(obj.isArray())
	{
		for(auto& c: *obj.getArray())
		{
			if(callRecursive(c, f))
				return true;
		}
	}
	if(auto d = obj.getDynamicObject())
	{
		if(d->getProperty(mpid::Children).isArray())
		{
			for(const auto& c: *obj[mpid::Children].getArray())
			{
				if(callRecursive(c, f))
					return true;
			}
		}
	}

	return false;
}

void ApiObject::updateWithLambda(const var& infoObject, const Identifier& id, const std::function<void(Component*)>& f)
{
	for(auto dialog: state.currentDialogs)
	{
		auto paf = [infoObject, id, dialog, f]()
		{
			Component::callRecursive<Dialog::PageBase>(dialog, [&](Dialog::PageBase* pb)
			{
				if( pb->getInfoObject() == infoObject ||
				   (id.isValid() && pb->getId() == id))
					f(pb);
						
				return false;
			});
		};

		if(State::getNotificationTypeForCurrentThread() == sendNotificationAsync)
			MessageManager::callAsync(paf);
		else
			paf();
	}
}

void ApiObject::setMethodWithHelp(const Identifier& id, var::NativeFunction f, const String& helpText)
{
	setMethod(id, f);
	help[id] = helpText;
}

void ApiObject::expectArguments(const var::NativeFunctionArgs& args, int numArgs, const String& customErrorMessage)
{
	if(args.numArguments != numArgs)
	{
		if(customErrorMessage.isNotEmpty())
			throw customErrorMessage;
		else
			throw "Argument amount mismatch: Expected " + String(numArgs);
	}
}

String ApiObject::getHelp(Identifier methodName) const
{
	if(help.find(methodName) != help.end())
	{
		return help.at(methodName);
	}

	return {};
}

bool ApiObject::callForEachInfoObject(const std::function<bool(const var& obj)>& f) const
{
	
	auto pageInfo = state.getFirstDialog()->getPageListVar();

	if(isPositiveAndBelow(state.currentPageIndex, pageInfo.size()))
	{
		auto currentPageInfo = state.currentDialogs.getFirst()->getPageListVar()[state.currentPageIndex];
		return Helpers::callRecursive(currentPageInfo, f);
	}
	else
	{
		return Component::callRecursive<Dialog::PageBase>(state.getFirstDialog(), [&](Dialog::PageBase* pb)
		{
			return f(pb->getInfoObject());
		});
	}
}

void HtmlParser::HeaderInformation::appendStyle(DataType t, const String& text)
{
	code[(int)t] << text;
}

Result HtmlParser::HeaderInformation::flush(DataProvider* d, State& state)
{
	simple_css::Parser p(code[(int)DataType::StyleSheet]);
	auto ok = p.parse();

	if(!ok.wasOk())
		return ok;

	css = p.getCSSValues();
	css.performAtRules(d);

	return state.createJavascriptEngine()->execute(code[(int)DataType::ScriptCode]);
}

HtmlParser::HtmlParser()
{
	using namespace factory;

	types.set("body",		List::getStaticId());
	types.set("button",		Button::getStaticId());
	types.set("img",		Image::getStaticId());
	types.set("div",		List::getStaticId());
	types.set("select",		Choice::getStaticId());
	types.set("input",		TextInput::getStaticId());
	types.set("textarea",   TextInput::getStaticId());
	types.set("p",			MarkdownText::getStaticId());
	types.set("span",		SimpleText::getStaticId() );
	types.set("li",			TagList::getStaticId());
	types.set("table",		Table::getStaticId());

	properties.set("id",			mpid::ID);
	properties.set("class",			mpid::Class);
	properties.set("style",			mpid::Style);
	properties.set("onclick",		mpid::Code);
	properties.set("onchange",		mpid::Code);
	properties.set("src",			mpid::Source);
	properties.set("required",      mpid::Required);
	properties.set("items",         mpid::Items);
	properties.set("disabled",      mpid::Enabled);
	properties.set("placeholder",	mpid::EmptyText);
	properties.set("autofocus",     mpid::Autofocus);
}

HtmlParser::HeaderInformation HtmlParser::parseHeader(DataProvider* d, XmlElement* header)
{
	HeaderInformation hi;

	for(int i = 0; i < header->getNumChildElements(); i++)
	{
		auto child = header->getChildElement(i);
		auto type = child->getTagName();

		if(type == "style")
		{
			hi.appendStyle(HeaderInformation::DataType::StyleSheet, child->getAllSubText());
		}
		else if(type == "script")
		{
			hi.appendStyle(HeaderInformation::DataType::ScriptCode, child->getAllSubText());
		}
		else if (type == "link")
		{
			auto dt = (HeaderInformation::DataType)(child->getStringAttribute("rel") != "stylesheet");
			auto url = child->getStringAttribute("href");
			hi.appendStyle(dt, d->importStyleSheet(url));
		}
	}

	return hi;
}

void HtmlParser::parseTable(XmlElement* xml, DynamicObject::Ptr nv)
{
	StringArray header, rows;

	for(int i = 0; i < xml->getNumChildElements(); i++)
	{
		auto child = xml->getChildElement(i);

		if(child->getTagName() == "tr")
		{
			if(i == 0)
			{
				// parse header
				for(int j = 0; j < child->getNumChildElements(); j++)
				{
					String c;
					c << "name:" << child->getChildElement(j)->getAllSubText();
					header.add(c);
				}
			}
			else
			{
				String row;

				for(int j = 0; j < child->getNumChildElements(); j++)
					row << child->getChildElement(j)->getAllSubText() << " | ";

				rows.add(row.upToLastOccurrenceOf(" | ", false, false));
			}
		}
	}

	nv->setProperty(mpid::Items, rows.joinIntoString("\n"));
	nv->setProperty(mpid::Columns, header.joinIntoString("\n"));
}

var HtmlParser::getElement(DataProvider* d, HeaderInformation& hi, XmlElement* xml)
{
	using namespace factory;

	if(xml->getTagName() == "html")
	{
		if(auto header = xml->getChildByName("head"))
		{
			hi = parseHeader(d, header);
		}
		if(auto body = xml->getChildByName("body"))
		{
			return getElement(d, hi, body);
		}
	}

	auto t = types.getTypeForId(xml->getTagName());
	
	if(t == IDConverter::Type::HtmlId)
	{
		auto typeId = types.convert(xml->getTagName());

		DynamicObject::Ptr nv = new DynamicObject();    

		nv->setProperty(mpid::Type, typeId.toString());
		nv->setProperty(mpid::NoLabel, true);

		if(typeId == factory::List::getStaticId())
			nv->setProperty(mpid::Children, var(Array<var>()));

		for(int i = 0; i < xml->getNumAttributes(); i++)
		{
			auto an = xml->getAttributeName(i);
			
			if(properties.getTypeForId(an) == IDConverter::Type::HtmlId)
			{
				auto v = var(xml->getAttributeName(i));

				auto id = properties.convert(an);;
				nv->setProperty(id, convertValue(id, xml->getAttributeValue(i), true));
			}
		}

		Array<var> children;
		String itemList;

		if(typeId == Table::getStaticId())
		{
			parseTable(xml, nv);
		}
		else
		{
			for(int i = 0; i < xml->getNumChildElements(); i++)
			{
				auto child = xml->getChildElement(i);

				if(child->isTextElement())
				{
					nv->setProperty(mpid::Text, child->getText());
				}
				else
				{
					if(child->getTagName() == "option")
					{
						itemList << child->getAllSubText() << "\n";
					}
					else
					{
						auto c = getElement(d, hi, child);
						if(c.isObject())
							children.add(c);
					}
				}
			}
		}

		if(itemList.isNotEmpty())
			nv->setProperty(mpid::Items, var(itemList.upToLastOccurrenceOf("\n", false, false)));

		if(!children.isEmpty())
			nv->setProperty(mpid::Children, var(children));

		if(xml->getTagName() == "textarea")
			nv->setProperty(mpid::Multiline, true);

		return var(nv.get());
	}

	return var();
}

HtmlParser::IDConverter::Type HtmlParser::IDConverter::getTypeForId(const Identifier& id) const
{
	for(const auto& i: items)
	{
		if(i.htmlId == id)
			return Type::HtmlId;
		if(i.multipageId == id)
			return Type::MultipageId;
	}

	return Type::Undefined;
}

Identifier HtmlParser::IDConverter::convert(const Identifier& id) const
{
	for(const auto& i: items)
	{
		if(i.htmlId == id)
			return i.multipageId;
		if(i.multipageId == id)
			return i.htmlId;
	}

	return {};
}

void HtmlParser::IDConverter::set(const Identifier& html, const Identifier& mp)
{
	items.add({ html, mp });
}

struct LogFunction: public ApiObject
{
	LogFunction(State& s):
	  ApiObject(s)
	{
		setMethodWithHelp("print", BIND_MEMBER_FUNCTION_1(LogFunction::print), "Prints a value to the console.");
		setMethodWithHelp("setError", BIND_MEMBER_FUNCTION_1(LogFunction::setError), "Throws an error and displays a popup with the message");
	}

	var setError(const var::NativeFunctionArgs& args)
	{
		expectArguments(args, 2);

		auto id = args.arguments[0].toString();

		for(auto d: state.currentDialogs)
		{
			if(auto p = d->findPageBaseForID(id))
			{
				p->setModalHelp(args.arguments[1].toString());
				d->setCurrentErrorPage(p);
			}
		}
		

		return var();
	}

	var print (const var::NativeFunctionArgs& args)
	{
		expectArguments(args, 1);
		state.eventLogger.sendMessage(State::getNotificationTypeForCurrentThread(), MessageType::Javascript, args.arguments[0].toString());
		
		return var();
	}
};

namespace ElementIds
{
#define DECLARE_ID(x) static const Identifier x(#x);

	DECLARE_ID(id);
	DECLARE_ID(style);
	DECLARE_ID(innerText);
	DECLARE_ID(innerHTML)
	DECLARE_ID(value);
	

#undef DECLARE_ID
}



struct Element: public ApiObject
{
	/** Allows setting the style properties using the `element.style.background = "red"` syntax. */
	struct StyleObject: public ApiObject
	{
		StyleObject(State& s, Element* parent_):
		  ApiObject(s),
		  parent(parent_)
		{
			auto style = parent->infoObject[mpid::Style].toString();
			auto existing = StringArray::fromTokens(style, ";", "\"");

			for(const auto& s: existing)
			{
				auto prop = s.upToFirstOccurrenceOf(":", false, false).trim();
				auto value = s.fromFirstOccurrenceOf(":", false, false).trim();

				if(prop.isNotEmpty() && value.isNotEmpty())
				{
					getProperties().set(Identifier(prop), var(value));
				}
			}
		}
        
        ~StyleObject()
        {
            if(somethingChanged)
                update();
        }
        
        bool somethingChanged = false;

		void update()
		{
			String newStyle;

			for(const auto& nv: getProperties())
				newStyle << nv.name << ":" << nv.value.toString() << ";";

			if(parent != nullptr)
			{
				parent->infoObject.getDynamicObject()->setProperty(mpid::Style, newStyle);
				updateWithLambda(parent->infoObject, {}, [newStyle](Component* c)
				{
					auto pb = dynamic_cast<Dialog::PageBase*>(c);
					pb->updateStyleSheetInfo(true);
				});
			}
		}

		void setProperty(const Identifier& propertyName, const var& newValue) override
		{
			getProperties().set(propertyName, newValue);
            somethingChanged = true;
		}
		
		WeakReference<Element> parent;
	};

	Element(State& s, const var& infoObject_):
	  ApiObject(s),
	  infoObject(infoObject_)
    {
        auto id = infoObject[mpid::ID];
        getProperties().set(ElementIds::innerText, infoObject[mpid::Text]);
        getProperties().set(ElementIds::id, id);
        getProperties().set(ElementIds::value, s.globalState[Identifier(id.toString())]);
        
        setProperty(ElementIds::style, new StyleObject(s, this));
        
        setMethodWithHelp("addEventListener", BIND_MEMBER_FUNCTION_1(Element::addEventListener), "Adds an event listener to the element");
        setMethodWithHelp("removeEventListener", BIND_MEMBER_FUNCTION_1(Element::removeEventListener), "Removes an event listener to the element");
        setMethodWithHelp("appendChild", BIND_MEMBER_FUNCTION_1(Element::appendChild), "Appends a child to the element");
        setMethodWithHelp("replaceChildren", BIND_MEMBER_FUNCTION_1(Element::replaceChildren), "Replaces all children with an array of new elements");
        setMethodWithHelp("updateElement", BIND_MEMBER_FUNCTION_1(Element::updateElement), "Refreshes the element (call this after you change any property).");
        setMethodWithHelp("setAttribute", BIND_MEMBER_FUNCTION_1(Element::setAttribute), "Sets an attribute (using HTML ids)");
        setMethodWithHelp("getAttribute", BIND_MEMBER_FUNCTION_1(Element::getAttribute), "Returns an attribute (using HTML ids)");
    }

    ~Element() override
    {
        setProperty(ElementIds::style, var());
        
        if(somethingChanged)
        {
            var::NativeFunctionArgs a(var(), nullptr, 0);
            updateElement(a);
        }
    }
    
    bool somethingChanged = false;
    
    
	 void writeAsJSON (OutputStream& os, int indentLevel, bool allOnOneLine, int maximumDecimalPlaces) override
	{
		infoObject.getDynamicObject()->writeAsJSON(os, indentLevel, allOnOneLine, maximumDecimalPlaces);
	}

	var updateElement(const var::NativeFunctionArgs& args)
	{
		expectArguments(args, 0);

		updateWithLambda(infoObject, {}, [](Component* c)
		{
			auto pb = dynamic_cast<Dialog::PageBase*>(c);
			pb->postInit();
			auto d = pb->findParentComponentOfClass<Dialog>();
			d->body.rebuildLayout();
			d->refreshBroadcaster.sendMessage(sendNotificationAsync, d->getState().currentPageIndex);
		});

		return var();
	}

	var addEventListener(const var::NativeFunctionArgs& args)
	{
		expectArguments(args, 2, "addEventListener needs 2 arguments (event type and function)");

		auto eventType = args.arguments[0].toString();
		auto fo = args.arguments[1];

		updateWithLambda(infoObject, {}, [eventType, fo](Component* c)
		{
			dynamic_cast<Dialog::PageBase*>(c)->addEventListener(eventType, fo);
		});

		return var();
	}

	

	var removeEventListener(const var::NativeFunctionArgs& args)
	{
		expectArguments(args, 2, "addEventListener needs 2 arguments (event type and function)");

		auto eventType = args.arguments[0].toString();
		auto fo = args.arguments[1];

		updateWithLambda(infoObject, {}, [eventType, fo](Component* c)
		{
			dynamic_cast<Dialog::PageBase*>(c)->removeEventListener(eventType, fo);
		});

		return var();
	}

	

	void replaceChildrenInternal()
	{
		updateWithLambda(infoObject, {}, [](Component* c)
		{
			auto pb = dynamic_cast<factory::Container*>(c);

			pb->replaceChildrenDynamic();
			auto d = pb->findParentComponentOfClass<Dialog>();
			d->refreshBroadcaster.sendMessage(sendNotificationAsync, d->getState().currentPageIndex);
		});
	}

	

	Identifier convertPropertyId(const var& v)
	{
		auto id = v.toString();

		if(id.isNotEmpty())
		{
			Identifier id_(id);
			HtmlParser p;
			auto t = p.properties.getTypeForId(id_);

			if(t == HtmlParser::IDConverter::Type::HtmlId)
			{
				return p.properties.convert(id_);
			}
			else
			{
				throw String("Unknown attribute " + id);
			}
		}

		return {};
	}

	var setAttribute(const var::NativeFunctionArgs& args)
	{
		expectArguments(args, 2);

		auto id = convertPropertyId(args.arguments[0]);
		infoObject.getDynamicObject()->setProperty(id, HtmlParser::convertValue(id, args.arguments[1], true));
        
        somethingChanged = true;
        
		return var(0);
	}

	var getAttribute(const var::NativeFunctionArgs& args)
	{
		expectArguments(args, 1);
		auto id = convertPropertyId(args.arguments[0]);
		return HtmlParser::convertValue(id, infoObject[id], false);
	}

	var replaceChildren(const var::NativeFunctionArgs& args)
	{
		expectArguments(args, 1);

		if(auto ar = infoObject[mpid::Children].getArray())
		{
			ar->clearQuick();

			if(auto cl = args.arguments[0].getArray())
			{
				for(auto& v: *cl)
				{
					if(auto e = dynamic_cast<Element*>(v.getDynamicObject()))
						ar->add(e->infoObject);
				}
			}
			else if(auto e = dynamic_cast<Element*>(args.arguments[0].getDynamicObject()))
			{
				auto ar = infoObject[mpid::Children].getArray();
				ar->add(e->infoObject);
			}

			replaceChildrenInternal();
		}
		else
		{
			throw String("Can't replace children of non-container type");
		}

		return true;
	}

	var appendChild(const var::NativeFunctionArgs& args)
	{
		expectArguments(args, 1);

		if(infoObject[mpid::Children].isArray())
		{
			if(auto e = dynamic_cast<Element*>(args.arguments[0].getDynamicObject()))
			{
				infoObject[mpid::Children].getArray()->add(e->infoObject);

				auto obj = e->infoObject;

				updateWithLambda(infoObject, {}, [obj](Component* c)
				{
					auto pb = dynamic_cast<factory::Container*>(c);
					pb->addChildDynamic(obj);
					auto d = pb->findParentComponentOfClass<Dialog>();
					d->refreshBroadcaster.sendMessage(sendNotificationAsync, d->getState().currentPageIndex);
				});
			}
		}
		else
		{
			throw String("Can't append to non-container type");
		}

		return true;
	}

	void setProperty(const Identifier& id, const var& newValue)
	{
		if(id == ElementIds::innerText)
		{
			infoObject.getDynamicObject()->setProperty(mpid::Text, newValue);
            somethingChanged = true;
            
#if 0
			updateWithLambda(infoObject, {}, [](Component* c)
			{
				dynamic_cast<Dialog::PageBase*>(c)->postInit();
				auto d = c->findParentComponentOfClass<Dialog>();
				d->body.rebuildLayout();
			});
#endif
		}
		else if (id == ElementIds::value)
		{
			auto objectId = infoObject[mpid::ID].toString();

			if(objectId.isNotEmpty())
			{
				state.globalState.getDynamicObject()->setProperty(Identifier(objectId), newValue);

				updateWithLambda(infoObject, id, [&](Component* c)
				{
					dynamic_cast<Dialog::PageBase*>(c)->check(state.globalState);
				});
			}
		}
		else if (id == ElementIds::id)
		{
			infoObject.getDynamicObject()->setProperty(mpid::ID, newValue);
            somethingChanged = true;
		}
		else if (id == ElementIds::innerHTML)
		{
			if(auto ar = infoObject[mpid::Children].getArray())
			{
				String code;
				code << "<div>" << newValue.toString() << "</div>";
				XmlDocument doc(code);

				if(auto xml = doc.getDocumentElement())
				{
					HtmlParser p;
					HtmlParser::HeaderInformation hi;
					auto newObject = p.getElement(nullptr, hi, xml.get());

					ar->swapWith(*newObject[mpid::Children].getArray());
					replaceChildrenInternal();
				}
				else
				{
					throw doc.getLastParseError();
				}
			}
			else
			{
				setProperty(ElementIds::innerText, newValue);
			}
		}
		else if (id == ElementIds::style)
		{
			
		}

		ApiObject::setProperty(id, newValue);
	}

	var infoObject;

	JUCE_DECLARE_WEAK_REFERENCEABLE(Element);
};

struct Dom: public ApiObject
{
	Dom(State& s):
	  ApiObject(s)
	{
		setMethodWithHelp("getElementById", BIND_MEMBER_FUNCTION_1(Dom::getElementById), "Returns the first element that matches the given ID");
		setMethodWithHelp("getElementByTagName", BIND_MEMBER_FUNCTION_1(Dom::getElementByTagName), "Returns an array with all elements that match the given Type.");
		setMethodWithHelp("getStyleData", BIND_MEMBER_FUNCTION_1(Dom::getStyleData), "Returns the global markdown style data.");
		setMethodWithHelp("setStyleData", BIND_MEMBER_FUNCTION_1(Dom::setStyleData), "Sets the global markdown style data");
		setMethodWithHelp("getClipboardContent", BIND_MEMBER_FUNCTION_1(Dom::getClipboardContent), "Returns the current clipboard content");
        setMethodWithHelp("copyToClipboard", BIND_MEMBER_FUNCTION_1(Dom::copyToClipboard), "Copies the string to the system clipboard");
		setMethodWithHelp("writeFile", BIND_MEMBER_FUNCTION_1(Dom::writeFile), "Writes the string content to the file");
		setMethodWithHelp("readFile", BIND_MEMBER_FUNCTION_1(Dom::readFile), "Loads string content of the file");
		setMethodWithHelp("navigate", BIND_MEMBER_FUNCTION_1(Dom::navigate), "Navigates to the page with the given index");
		setMethodWithHelp("createElement", BIND_MEMBER_FUNCTION_1(Dom::createElement), "Creates an element");
		setMethodWithHelp("callAction", BIND_MEMBER_FUNCTION_1(Dom::callAction), "Calls the action for the given ID");
		setMethodWithHelp("bindCallback", BIND_MEMBER_FUNCTION_1(Dom::bindCallback), "Registers an external function");
        
        setMethodWithHelp("addEventListener", BIND_MEMBER_FUNCTION_1(Dom::addEventListener), "Adds a event listener to a global event");
        setMethodWithHelp("removeEventListener", BIND_MEMBER_FUNCTION_1(Dom::removeEventListener), "Removes the event listener.");
        
        setMethodWithHelp("clearEventListeners", BIND_MEMBER_FUNCTION_1(Dom::clearEventListeners), "Clears all listeners with the given group ID");
        
	}

	Array<var> createdElements;

	var getElementByTagName(const var::NativeFunctionArgs& args)
	{
		expectArguments(args, 1);
		Array<var> matches;

		if(state.getFirstDialog() != nullptr)
		{
			auto id = args.arguments[0].toString();

			Component::callRecursive<Dialog::PageBase>(state.getFirstDialog(), [&](Dialog::PageBase* pb)
			{
				if(pb->getPropertyFromInfoObject(mpid::Type) == id)
					matches.add(pb->getInfoObject());

				return false;
			});
		}

		return var(matches);
	}

	var createElement(const var::NativeFunctionArgs& args)
	{
		expectArguments(args, 1);
		
		HtmlParser p;
		HtmlParser::HeaderInformation hi;
		auto xml = std::make_unique<XmlElement>(args.arguments[0].toString());
		auto newObject = p.getElement(nullptr, hi, xml.get());
		return new Element(state, newObject);
	}

	var bindCallback(const var::NativeFunctionArgs& args)
	{
		expectArguments(args, 2);

		auto name = args.arguments[0].toString();
		auto fallback = args.arguments[1];
		auto s = &state;

        var::NativeFunction f = [name, s, fallback](const var::NativeFunctionArgs& args)
        {
            var rv;
            auto found = s->callNativeFunction(name, args, &rv);

            if(!found)
                rv = s->createJavascriptEngine()->callFunctionObject(args.thisObject.getDynamicObject(), fallback, args);

            return rv;
        };
        
		return var(f);
	}

	var writeFile(const var::NativeFunctionArgs& args) const
	{
		if(args.numArguments == 2)
		{
			auto f = args.arguments[0].toString();
			f = factory::MarkdownText::getString(f, state);

			if(File::isAbsolutePath(f))
			{
				state.logMessage(MessageType::FileOperation, "write " + f + " from JS");
				File(f).getParentDirectory().createDirectory();
				return File(f).replaceWithText(args.arguments[1].toString());
			}
		}

		return var(false);
	}

	var navigate(const var::NativeFunctionArgs& args) const
	{
		
		if(args.numArguments >= 1)
		{
			auto newIndex = args.arguments[0];

			auto shouldCheck = args.numArguments > 1 ? (bool)args.arguments[1] : true;

			int pageIndex = -1;

			if(newIndex.isInt() || newIndex.isInt64())
				pageIndex = (int)newIndex;
			else
			{
				// implement this at some point
				jassertfalse;
				pageIndex = 0;//state.currentDialog->getPageIndex(newIndex.toString());
			}
			
			if(isPositiveAndBelow(newIndex, state.getFirstDialog()->getNumPages()))
			{
				WeakReference<State> s = &state;

				auto f = [s, pageIndex, shouldCheck]()
				{
					s->currentPageIndex = pageIndex;

					for(auto dialog: s.get()->currentDialogs)
					{
						if(s != nullptr && s.get()->getFirstDialog() != nullptr)
						{
							if(shouldCheck)
                            {
                                s->currentPageIndex--;
                                dialog->navigate(true);
                            }
							else
								dialog->refreshCurrentPage();
						}
					}
				};

				MessageManager::callAsync(f);
				
				return var(true);
			}
		}

		return var(false);
	}

	var readFile(const var::NativeFunctionArgs& args) const
	{
		if(args.numArguments == 1)
		{
			auto f = args.arguments[0].toString();

			f = factory::MarkdownText::getString(f, state);

			if(File::isAbsolutePath(f))
			{
				state.getFirstDialog()->logMessage(MessageType::FileOperation, "load " + f + " into JS");
				return File(f).loadFileAsString();
			}
		}

		return var("");
	}
	
	var getClipboardContent(const var::NativeFunctionArgs& args) const
	{
		return var(SystemClipboard::getTextFromClipboard());
	}
    
    var copyToClipboard(const var::NativeFunctionArgs& args) const
    {
        if(args.numArguments == 1)
        {
            auto f = args.arguments[0].toString();
            SystemClipboard::copyTextToClipboard(f);
        }
        
        return var();
    }


	var callAction(const var::NativeFunctionArgs& args)
	{
		Identifier id(args.arguments[0].toString());

		updateWithLambda(var(), id, [](Component* c)
		{
			auto a = dynamic_cast<factory::Action*>(c);

			if(a->triggerType != factory::Action::TriggerType::OnCall)
				throw String("Only manual actions can be called");

			a->perform();
		});

		return var();
	}
    
    var addEventListener(const var::NativeFunctionArgs& args)
    {
        expectArguments(args, 2);
        
        auto eventType = args.arguments[0].toString();
        auto f = args.arguments[1];
        
        state.addEventListener(eventType, f);
        return var();
    }
    
    var removeEventListener(const var::NativeFunctionArgs& args)
    {
        expectArguments(args, 2);
        
        auto eventType = args.arguments[0].toString();
        auto f = args.arguments[1];
        
        state.removeEventListener(eventType, f);
        return var();
    }
    
    var clearEventListeners(const var::NativeFunctionArgs& args)
    {
        expectArguments(args, 1);
        
        auto id = args.arguments[0].toString();

        state.clearAndSetGroup(id);
        return var();
    }
    
	var getElementById(const var::NativeFunctionArgs& args)
	{
		expectArguments(args, 1);
		
		Array<var> matches;
		auto id = args.arguments[0].toString();

		for(const auto& e: createdElements)
		{
			if(auto el = dynamic_cast<Element*>(e.getDynamicObject()))
			{
				if(el->infoObject[mpid::ID].toString() == id)
					return e;
			}
		}

		callForEachInfoObject([&](const var& obj)
		{
			if(obj[mpid::ID].toString() == id)
			{
				matches.add(new Element(state, obj));
			}
			
			return false;
		});

		if(matches.isEmpty())
		{
			// scan the component tree (elements inside a HTML element don't have a info object
			updateWithLambda(var(), Identifier(id), [&](Component* c)
			{
				auto pb = dynamic_cast<Dialog::PageBase*>(c);
				auto infoObject = pb->getInfoObject();
				matches.add(new Element(state, infoObject));
			});
		}

		createdElements.add(matches.getFirst());
		return matches.getFirst();
	}

	var getStyleData(const var::NativeFunctionArgs& args)
	{
		expectArguments(args, 0);

		if(auto fd = state.getFirstDialog())
		{
			return fd->getStyleData().toDynamicObject();
		}

		return var();
	}

	var setStyleData(const var::NativeFunctionArgs& args)
	{
		expectArguments(args, 1);

		MarkdownLayout::StyleData sd;
		sd.fromDynamicObject(args.arguments[0], std::bind(&State::loadFont, &state, std::placeholders::_1));

		for(auto d: state.currentDialogs)
			d->setStyleData(sd);
		
		return var();
	}
};


	


} // multipage	
} // hise
