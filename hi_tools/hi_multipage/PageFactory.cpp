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
 *   which must be separately licensed for closed source applications:
 *
 *   http://www.juce.com
 *
 *   ===========================================================================
 */

namespace hise {
namespace multipage {
namespace factory {
using namespace juce;


Type::Type(Dialog& r, int width, const var& d):
	PageBase(r, width, d)
{
	

	setSize(width, 42);
	typeId = d[mpid::Type].toString();
	Helpers::writeInlineStyle(*this, "background-color: red; height: 38px;width: 100%;");

	Factory fac;
	
	icon = fac.createPath(typeId);
	fac.scalePath(icon, Rectangle<float>(70.0f, 0.0f, 32.0f, 32.0f).reduced(6.0f));
}

void Type::resized()
{
	if(helpButton != nullptr)
	{
		auto b = getLocalBounds();
		b.removeFromBottom(10);
		helpButton->setBounds(b.removeFromRight(b.getHeight()).reduced(3));
	}
}

Result Type::checkGlobalState(var globalState)
{
	if(typeId.isNotEmpty())
	{
		writeState(typeId);
		return Result::ok();
	}
	else
	{
		return Result::fail("Must define Type property");
	}
}

void Type::paint(Graphics& g)
{
	auto b = getLocalBounds().toFloat();
	g.setColour(Colours::white);

	g.setFont(GLOBAL_FONT());
	
	g.drawText("Type", b.reduced(1.0f), Justification::left);


	b.removeFromLeft(b.getHeight() + 70);

	auto f = Dialog::getDefaultFont(*this);

	g.setColour(f.second);
	g.setFont(f.first);

	String m;
	m << typeId;

	g.setFont(GLOBAL_FONT());

	g.setFont(GLOBAL_BOLD_FONT());
	g.drawText(m, b.reduced(1.0f), Justification::left);
	
	g.fillPath(icon);

}

#if HISE_MULTIPAGE_INCLUDE_EDIT
void Spacer::createEditor(Dialog::PageInfo& rootList)
{
    rootList.addChild<Type>({
        { mpid::ID, "Type"},
        { mpid::Type, "Spacer"},
		{ mpid::Help, "A simple spacer for convenient layouting." }
    });
}
#endif

void Spacer::paint(Graphics& g)
{
	if(isEditModeAndNotInPopup())
	{
		g.setColour(Dialog::getDefaultFont(*this).second.withAlpha(0.3f));
		g.drawRoundedRectangle(getLocalBounds().toFloat(), 5.0f, 1.0f);
	}
}

HtmlElement::HtmlElement(Dialog& r, int width, const var& d):
	PageBase(r, width, d)
{
	Helpers::setFallbackStyleSheet(*this, "width: 100%; height: auto;display:flex;");
}

void HtmlElement::postInit()
{
	init();

    auto content = infoObject[mpid::Code].toString();

    if(content.startsWithChar('$'))
	    content = rootDialog.getState().loadText(content, true);

    if(auto xml = XmlDocument::parse(content))
    {
        HtmlParser::HeaderInformation hi;

        ScopedPointer<simple_css::StyleSheet::Collection::DataProvider> dp = rootDialog.createDataProvider();

		HtmlParser p;

        auto obj = p.getElement(dp, hi, xml.get());

        auto ok = hi.flush(dp, rootDialog.getState());

		if(!ok.wasOk())
		{
			rootDialog.setCurrentErrorPage(this);
			setModalHelp(ok.getErrorMessage());
		}

        if(auto r = simple_css::CSSRootComponent::find(*this))
        {
	        r->css.addCollectionForComponent(this, hi.css);
        }
        
        Factory factory;

	    if(auto pi = factory.create(obj))
        {
            pi->useGlobalStateObject = false;
            auto nc = pi->pageCreator(rootDialog, getWidth(), obj);
            childElements.add(nc);
            addFlexItem(*nc);
            nc->postInit();
        }
    }
}

#if HISE_MULTIPAGE_INCLUDE_EDIT
void HtmlElement::createEditor(Dialog::PageInfo& rootList)
{
	rootList.addChild<Type>({
		{ mpid::ID, "HtmlElement"},
		{ mpid::Type, "HtmlElement"},
		{ mpid::Help, "A UI element defined by HTML markup language" }
	});
            
	rootList.addChild<TextInput>({
		{ mpid::ID, "ID" },
		{ mpid::Text, "ID" },
		{ mpid::Value, infoObject[mpid::ID].toString() },
		{ mpid::Help, "The ID for the element (used as key in the state `var`." }
	});

	rootList.addChild<CodeEditor>({
		{ mpid::ID, mpid::Code.toString() },
		{ mpid::Text, mpid::Code.toString() },
		{ mpid::Help, "The HTML code" },
		{ mpid::Syntax, "HTML" },
		{ mpid::Value, infoObject[mpid::Code] }
	});

	rootList.addChild<TextInput>({
		{ mpid::ID, mpid::Class.toString() },
		{ mpid::Text, mpid::Class.toString() },
		{ mpid::Help, "The CSS class that is applied to the UI element." },
		{ mpid::Value, infoObject[mpid::Class] }
	});

	rootList.addChild<TextInput>({
		{ mpid::ID, mpid::Style.toString() },
		{ mpid::Text, mpid::Style.toString() },
		{ mpid::Value, infoObject[mpid::Style] },
		{ mpid::Help, "Additional inline properties that will be used by the UI element" }
	});
}
#endif


Image::Image(Dialog& r, int width, const var& d):
	PageBase(r, width, d)
{
	setSize(width, 0);
	addFlexItem(img);
	Helpers::setFallbackStyleSheet(*this, "display:flex;gap:0px;width:100%;height:100px;");
	Helpers::setFallbackStyleSheet(img, "width:100%;height:100%;");
}

#if HISE_MULTIPAGE_INCLUDE_EDIT
void Image::createEditor(Dialog::PageInfo& rootList)
{
	rootList.addChild<Type>({
		{ mpid::ID, "Image"},
		{ mpid::Type, "Image"},
		{ mpid::Help, "A component with fix columns and a dynamic amount of rows" }
	});
            
	rootList.addChild<TextInput>({
		{ mpid::ID, "ID" },
		{ mpid::Text, "ID" },
		{ mpid::Value, infoObject[mpid::ID].toString() },
		{ mpid::Help, "The ID for the element (used as key in the state `var`." }
	});

	rootList.addChild<TextInput>({
		{ mpid::ID, mpid::Source.toString() },
		{ mpid::Text, mpid::Source.toString() },
		{ mpid::Help, "The image source - either a URL or a asset reference" },
		{ mpid::Items, rootDialog.getState().getAssetReferenceList(Asset::Type::Image) },
		{ mpid::Value, infoObject[mpid::Source] }
	});

	rootList.addChild<TextInput>({
		{ mpid::ID, mpid::Class.toString() },
		{ mpid::Text, mpid::Class.toString() },
		{ mpid::Help, "The CSS class that is applied to the UI element." },
		{ mpid::Value, infoObject[mpid::Class] }
	});

	rootList.addChild<TextInput>({
		{ mpid::ID, mpid::Style.toString() },
		{ mpid::Text, mpid::Style.toString() },
		{ mpid::Value, infoObject[mpid::Style] },
		{ mpid::Help, "Additional inline properties that will be used by the UI element" }
	});
}
#endif

void Image::postInit()
{
	auto imgPath = infoObject[mpid::Source].toString();

	if(URL::isProbablyAWebsiteURL(imgPath))
		img.setImage(URL(imgPath));
	else
		img.setImage(rootDialog.getState().loadImage(imgPath));
}

EventLogger::EventLogger(Dialog& r, int w, const var& obj):
	PageBase(r, w, obj),
	console(r.getState())
{
	addAndMakeVisible(console);
	setSize(w, 200);
}

SimpleText::SimpleText(Dialog& r, int width, const var& obj):
	PageBase(r, width, obj)
{
	auto t = addTextElement({}, obj[mpid::Text].toString());
	setIsInvisibleWrapper(true);

	this->updateStyleSheetInfo(true);

	//setDefaultStyleSheet("width: 100%;height: auto;display: flex;");
	
	//Helpers::writeInlineStyle(*t, "flex-grow:1;");
	setSize(width, t->getHeight());
}

void SimpleText::postInit()
{
	init();

	if(auto st = dynamic_cast<SimpleTextDisplay*>(getChildComponent(0)))
	{
		auto text = infoObject[mpid::Text].toString();
		st->setText(text);
	}
}

#if HISE_MULTIPAGE_INCLUDE_EDIT
void SimpleText::createEditor(Dialog::PageInfo& rootList)
{
	rootList.addChild<Type>({
		{ mpid::ID, "Type"},
		{ mpid::Type, "SimpleText"},
		{ mpid::Help, "A field to display text messages with markdown support" }
	});
    
    rootList.addChild<TextInput>({
        { mpid::ID, "ID" },
        { mpid::Text, "ID" },
        { mpid::Value, id.toString() },
        { mpid::Help, "The ID for the element (used as key in the state `var`.\n>" }
    });

	rootList.addChild<TextInput>({
		{ mpid::ID, "Text" },
		{ mpid::Text, "Text" },
		{ mpid::Required, true },
		{ mpid::Multiline, true },
		{ mpid::Height, 80 },
		{ mpid::Value, infoObject[mpid::Text].toString() },
		{ mpid::Help, "The plain text content." }
	});

	rootList.addChild<TextInput>({
		{ mpid::ID, mpid::Class.toString() },
		{ mpid::Text, mpid::Class.toString() },
		{ mpid::Help, "The CSS class that is applied to the action UI element (progress bar & label)." },
		{ mpid::Value, infoObject[mpid::Class] }
	});

	rootList.addChild<TextInput>({
		{ mpid::ID, mpid::Style.toString() },
		{ mpid::Text, mpid::Style.toString() },
		{ mpid::Value, infoObject[mpid::Style] },
		{ mpid::Help, "Additional inline properties that will be used by the UI element" }
	});
}
#endif

String MarkdownText::getString(const String& markdownText, const State& state)
{
	if(markdownText.contains("$"))
	{
		String other;

		auto it = markdownText.begin();
		auto end = markdownText.end();

		while(it < (end - 1))
		{
			auto c = *it;

			if(c == '$')
			{
				++it;
				String variableId;

				while(it < (end))
				{
					c = *it;
					if(!CharacterFunctions::isLetterOrDigit(c) &&c != '_')
					{
						if(variableId.isNotEmpty())
						{
							auto v = state.globalState[Identifier(variableId)].toString();

							if(v.isNotEmpty())
								other << getString(v, state);

							variableId = {};
						}

						break;
					}
					else
					{
						variableId << c;
					}

					++it;
				}

				if(variableId.isNotEmpty())
				{
					auto v = state.globalState[Identifier(variableId)].toString();

                    v = getString(v, state);
                    
					if(v.isNotEmpty())
						other << v;
				}
			}
			else
			{
				other << *it;
				++it;
			}
		}

		other << *it;

		return other;
	}

	return markdownText;
}

struct AssetImageProvider: public MarkdownParser::ImageProvider
{
	AssetImageProvider(MarkdownParser* parent, State& s):
	  ImageProvider(parent),
	  state(s)
	{};

	State& state;

	MarkdownParser::ResolveType getPriority() const override { return MarkdownParser::ResolveType::EmbeddedPath; }

	juce::Image getImage(const MarkdownLink& imageURL, float width) override
	{
		auto id = imageURL.toString(MarkdownLink::Format::UrlWithoutAnchor).substring(1, 10000);

		updateWidthFromURL(imageURL, width);
		
		for(auto a: state.assets)
		{
			if(a->type == Asset::Type::Image && a->id == id)
			{
				auto img = a->toImage();
				auto ratio = width / (float)img.getWidth();
				auto newHeight = roundToInt(ratio * img.getHeight());
				return img.rescaled(roundToInt(width), newHeight);
			}
		}

		return {};
	}
	ImageProvider* clone(MarkdownParser* newParent) const { return new AssetImageProvider(newParent, state); }

	Identifier getId() const { RETURN_STATIC_IDENTIFIER("AssetImageProvider"); };
};

MarkdownText::MarkdownText(Dialog& d, int width_, const var& obj_):
	PageBase(d, width_, obj_),
	display(),
	width((float)width_),
	obj(obj_)
{
	Helpers::writeClassSelectors(*this, { ".markdown" }, true);
	
	display.r.setImageProvider(new AssetImageProvider(&display.r, d.getState()));
	display.setResizeToFit(true);

	//Helpers::writeSelectorsToProperties(display, {".markdown"});

	setDefaultStyleSheet("width: 100%; height: auto;");
	Helpers::setFallbackStyleSheet(display, "width: 100%;");

	addFlexItem(display);

	forwardInlineStyleToChildren();

	setSize(width, 0);
	//setInterceptsMouseClicks(false, false);
}

void MarkdownText::postInit()
{
	init();

	if(auto root = simple_css::CSSRootComponent::find(*this))
	{
		display.r.setStyleData(root->css.getMarkdownStyleData(&display));
	}

	auto markdownText = getString(infoObject[mpid::Text].toString(), rootDialog.getState());

	if(markdownText.startsWith("${"))
		markdownText = rootDialog.getState().loadText(markdownText, true);

	display.setSize(width, 0);
	display.setText(markdownText);

	auto sd = findParentComponentOfClass<Dialog>()->getStyleData();

	sd.fromDynamicObject(obj, [](const String& name){ return Font(name, 13.0f, Font::plain); });

	if(findParentComponentOfClass<Dialog::ModalPopup>() != nullptr)
		sd.backgroundColour = Colours::transparentBlack;
}

void MarkdownText::resized()
{
	FlexboxComponent::resized();
	display.resized();
}

#if HISE_MULTIPAGE_INCLUDE_EDIT
void MarkdownText::createEditor(Dialog::PageInfo& rootList)
{
    
    rootList.addChild<Type>({
        { mpid::ID, "Type"},
        { mpid::Type, "MarkdownText"},
		{ mpid::Help, "A field to display text messages with markdown support" }
    });

    rootList.addChild<TextInput>({
        { mpid::ID, "ID" },
        { mpid::Text, "ID" },
        { mpid::Value, id.toString() },
        { mpid::Help, "The ID for the element.\n>" }
    });
    
    rootList.addChild<TextInput>({
        { mpid::ID, "Text" },
        { mpid::Text, "Text" },
        { mpid::Required, true },
        { mpid::Multiline, true },
		{ mpid::Height, 250 },
        { mpid::Value, "Some markdown **text**." },
		{ mpid::Help, "The text content in markdown syntax." }
    });

	rootList.addChild<TextInput>({
		{ mpid::ID, mpid::Code.toString() },
		{ mpid::Text, mpid::Code.toString() },
        { mpid::Value, infoObject[mpid::Code] },
        { mpid::Items, "{BIND::functionName}" },
		{ mpid::Help, "The callback that is executed using the syntax `{BIND::myMethod}`" }
	});

	rootList.addChild<TextInput>({
        { mpid::ID, mpid::Class.toString() },
        { mpid::Text, mpid::Class.toString() },
        { mpid::Help, "The CSS class that is applied to the action UI element (progress bar & label)." },
		{ mpid::Value, infoObject[mpid::Class] }
    });

	rootList.addChild<TextInput>({
		{ mpid::ID, mpid::Style.toString() },
		{ mpid::Text, mpid::Style.toString() },
        { mpid::Value, infoObject[mpid::Style] },
		{ mpid::Help, "Additional inline properties that will be used by the UI element" }
	});
}
#endif


Result MarkdownText::checkGlobalState(var)
{
	return Result::ok();
}

#if HISE_MULTIPAGE_INCLUDE_EDIT
void DummyContent::createEditor(const var& infoObject, Dialog::PageInfo& rootList)
{
	rootList.addChild<Type>({
		{ mpid::ID, "Type"},
		{ mpid::Type, "Placeholder"},
		{ mpid::Help, "A basic component placeholder for any juce::Component" }
	});
	    
	rootList.addChild<TextInput>({
		{ mpid::ID, "ID" },
		{ mpid::Text, "ID" },
		{ mpid::Value, infoObject[mpid::ID].toString() },
		{ mpid::Help, "The ID for the element (used as key in the state `var`." }
	});

	rootList.addChild<TextInput>({
		{ mpid::ID, mpid::ContentType.toString() },
		{ mpid::Text, mpid::ContentType.toString() },
		{ mpid::Required, true },
		{ mpid::Multiline, false },
		{ mpid::Value, infoObject[mpid::ContentType] },
		{ mpid::Help, "The C++ class name that will be used for the content.  \n> The class must be derived from `juce::Component` and `hise::multipage::PlaceholderContentBase`" }
	});

	rootList.addChild<TextInput>({
		{ mpid::ID, mpid::Class.toString() },
		{ mpid::Text, mpid::Class.toString() },
		{ mpid::Help, "The CSS class that is applied to the UI element." },
		{ mpid::Value, infoObject[mpid::Class] }
	});

	rootList.addChild<TextInput>({
		{ mpid::ID, mpid::Style.toString() },
		{ mpid::Text, mpid::Style.toString() },
		{ mpid::Value, infoObject[mpid::Style] },
		{ mpid::Help, "Additional inline properties that will be used by the UI element" }
	});
}
#endif

TagList::TagList(Dialog& parent, int w, const var& obj):
	PageBase(parent, w, obj)
{
	Helpers::setFallbackStyleSheet(*this, "display:flex;width:100%;height:auto;flex-wrap:wrap;");
	Helpers::writeClassSelectors(*this, simple_css::Selector(".tag-list"), true);
}

void TagList::buttonClicked(juce::Button*)
{
	Array<var> values;
        
	for(auto b: buttons)
	{
		if(b->getToggleState())
			values.add(b->getButtonText());
	}
	
	writeState(var(values));
	callOnValueChange("click");
}



void TagList::postInit()
{
	PageBase::init();

	buttons.clear();
        
	auto items = getItemsAsStringArray();

	auto valueList = getValueFromGlobalState(var(Array<var>()));

	for(auto i: items)
	{
		auto b = new TextButton(i);
		b->setClickingTogglesState(true);

		auto toggleState = valueList.indexOf(var(i)) != -1;

		b->setToggleState(toggleState, dontSendNotification);

		Helpers::writeClassSelectors(*b, simple_css::Selector(".tag-button"), true);
		buttons.add(b);
		b->addListener(this);
		addFlexItem(*b);
	}
        
	resized();
}

Table::CellComponent::CellComponent(Table& parent_):
	parent(parent_)
{
	setWantsKeyboardFocus(true);

	using namespace simple_css;
            
	setInterceptsMouseClicks(true, false);
	setRepaintsOnMouseActivity(true);
	Helpers::setCustomType(*this, Selector(ElementType::TableCell));
}

void Table::CellComponent::update(Point<int> newPos, const String& newContent)
{
	cellPosition = newPos;
	content = newContent;
            
	auto isFirst = newPos.x == 0;
	auto isLast = newPos.x == (parent.table.getHeader().getNumColumns(true)-1);
            
	getProperties().set("first-child", isFirst);
	getProperties().set("last-child", isLast);
            
	repaint();
}

void Table::CellComponent::paint(Graphics& g)
{
	using namespace simple_css;
            
	Renderer r(nullptr, parent.rootDialog.stateWatcher);

	if(auto ss = parent.rootDialog.css.getForComponent(this))
	{
                
		auto state = r.getPseudoClassFromComponent(this);
                
		if(parent.table.isRowSelected(cellPosition.y))
			state |= (int)PseudoClassType::Focus;

		auto v = parent.getValueFromGlobalState();

		if(v.isInt() && (int)v == cellPosition.y)
			state |= (int)PseudoClassType::Checked;

		r.setPseudoClassState(state);
                
		r.drawBackground(g, getLocalBounds().toFloat(), ss);
		r.renderText(g, getLocalBounds().toFloat(), content, ss);
	}
}

Table::Table(Dialog& parent, int w, const var& obj):
	PageBase(parent, w, obj),
	table(obj[mpid::ID].toString(), this),
	repainter(table)
{
	if(!obj.hasProperty(mpid::ValueMode))
		obj.getDynamicObject()->setProperty(mpid::ValueMode, "Row");
        
	addFlexItem(table);
	
        
	setSize(w, 250);
	
	Helpers::setFallbackStyleSheet(table, "height: 100%; width: 100%;");

	setIsInvisibleWrapper(true);

	table.setColour(TableListBox::ColourIds::backgroundColourId, Colours::transparentBlack);
	table.setHeaderHeight(32);
	table.autoSizeAllColumns();
	table.setRepaintsOnMouseActivity(true);
	parent.stateWatcher.registerComponentToUpdate(&table.getHeader());


	//sf.addScrollBarToAnimate(table.getViewport()->getVerticalScrollBar());
	table.getViewport()->setScrollBarThickness(13);

}



void Table::rebuildColumns()
{
	auto columns = StringArray::fromLines(infoObject[mpid::Columns].toString());

	auto& th = table.getHeader();

	th.removeAllColumns();
        
	int columnId = 1;

	for(auto c: columns)
	{
		auto instructions = StringArray::fromTokens(c, ";", "\"'");

		String name;
		int width = 100;
		int minWidth = 30;
		int maxWidth = -1;

		enum class Properties
		{
			Name,
			MinWidth,
			MaxWidth,
			Width,
			Undefined,
			numProperties
		};

		static const StringArray properties =
		{
			"name",
			"min-width",
			"max-width",
			"width"
		};

		auto getProperty = [&](const String& v)
		{
			auto prop = v.upToFirstOccurrenceOf(":", false, false).trim();
			auto idx = properties.indexOf(prop);
			if(idx != -1)
				return (Properties)idx;
			else
				return Properties::Undefined;
		};

		auto getPixelValue = [&](const String& v)
		{
			if(v.trim().endsWithChar('%'))
			{
				auto pv = v.getIntValue();
				return roundToInt((float)pv * 0.01 * (float)table.getWidth());
			}
			else
			{
				return v.getIntValue();
			}
		};

		for(auto i: instructions)
		{
			auto p = getProperty(i);
			auto value = i.fromFirstOccurrenceOf(":", false, false).trim().unquoted();

			switch(p)
			{
			case Properties::Name:
				name = value;
				break;
			case Properties::MinWidth:
				minWidth = jlimit(0, 1000, getPixelValue(value));
				break;
			case Properties::MaxWidth:
				maxWidth = jlimit(-1, 1000, getPixelValue(value));
				break;
			case Properties::Width:
				width = jlimit(10, 1000, getPixelValue(value));
				break;
			case Properties::Undefined:
				break;
			case Properties::numProperties:
				break;
			default: ;
			}
		}

		th.addColumn(name, columnId++, width, minWidth, maxWidth, TableHeaderComponent::ColumnPropertyFlags::visible);
	}

	th.setStretchToFitActive(true);
	th.resizeAllColumnsToFit(table.getWidth() - table.getViewport()->getScrollBarThickness());
	table.setMultipleSelectionEnabled(infoObject[mpid::Multiline]);

	using namespace simple_css;

	rootDialog.stateWatcher.resetComponent(&th);

	if(auto ss = rootDialog.css.getWithAllStates(this, Selector(ElementType::TableCell)))
	{
		
		table.setRowHeight(ss->getLocalBoundsFromText("M").getHeight());
	}

	th.repaint();
}

String Table::getCellContent(int columnId, int rowNumber) const
{
	if(getFilterFunctionId().isValid())
	{
		if(isPositiveAndBelow(rowNumber, visibleItems.size()))
		{
			if(auto cells = visibleItems[rowNumber].second.getArray())
			{
				if(isPositiveAndBelow(columnId - 1, cells->size()))
					return (*cells)[columnId-1].toString();
			}
		}
	}
	else
	{
		if(isPositiveAndBelow(rowNumber, items.size()))
		{
			if(auto cells = items[rowNumber].getArray())
			{
				if(isPositiveAndBelow(columnId - 1, cells->size()))
					return (*cells)[columnId-1].toString();
			}
		}
	}
        
	return {};
}

Component* Table::refreshComponentForCell(int rowNumber, int columnId, bool isRowSelected,
	Component* existingComponentToUpdate)
{
	if(existingComponentToUpdate == nullptr)
	{
		auto newComponent = new CellComponent(*this);
		newComponent->update({columnId-1, rowNumber}, getCellContent(columnId, rowNumber));
		return newComponent;
	}
	else if(auto c = dynamic_cast<CellComponent*>(existingComponentToUpdate))
	{
		c->update({ columnId-1, rowNumber }, getCellContent(columnId, rowNumber));
		return existingComponentToUpdate;
	}
	return nullptr;
}

String Table::itemsToString(const var& data)
{
	if(data.isString())
		return data;
        
	if(auto items = data.getArray())
	{
		String s;
		for(auto& rows: *items)
		{
			if(auto cells = rows.getArray())
			{
				for(auto& cell: *cells)
					s << cell.toString() << " | ";
                    
				s << "\n";
			}
		}
            
		return s;
	}
        
	return {};
}

Array<var> Table::stringToItems(const var& data)
{
	if(data.isArray())
		return *data.getArray();
        
	Array<var> rows;
        
	for(auto& l: StringArray::fromLines(data.toString()))
	{
		Array<var> row;
            
		for(auto& c: StringArray::fromTokens(l, "|", "\"'"))
		{
			row.add(var(c.trim()));
		}
            
		rows.add(var(row));
	}
        
	return rows;
}


int Table::getNumRows()
{
	if(getFilterFunctionId().isValid())
		return visibleItems.size();
	else
		return items.size();
}

void Table::rebuildRows()
{
	visibleItems.clear();

	table.getViewport()->setScrollBarsShown(true, false);

	auto fid = getFilterFunctionId().toString();

	if(fid.isNotEmpty())
	{
		if(rootDialog.getState().hasNativeFunction(fid))
		{
			int index = 0;
			auto ok = Result::ok();
			var args[2];
			var thisObj;

			for(auto v: items)
			{
				args[0] = index;
				args[1] = v;

				var rv;

				auto wasCalled = rootDialog.getState().callNativeFunction(fid, var::NativeFunctionArgs(thisObj, args, 2), &rv);

				if(!wasCalled || (bool)rv)
				{
					visibleItems.add({index, v});
				}
				
				index++;
			}
		}
		else
		{
			int index = 0;

			for(auto v: items)
			{
				visibleItems.add({index++, v});
			}
		}
	}

	if(originalSelectionIndex != -1)
	{
		if(getFilterFunctionId().isValid())
		{
			int index = 0;

			for(const auto& v: visibleItems)
			{
				if(v.first == originalSelectionIndex)
				{
					table.selectRow(index);
					break;
				}

				index++;
			}
		}
		else
		{
			table.selectRow(originalSelectionIndex);
		}
	}
}

void Table::postInit()
{
	if(auto ss = rootDialog.css.getForComponent(&table.getHeader()))
	{
		rootDialog.stateWatcher.checkChanges(&table.getHeader(), ss, 0);
	}
        
	init();
        
	rebuildColumns();
	items = stringToItems(infoObject[mpid::Items]);
	rebuildRows();
	table.updateContent();
	table.setWantsKeyboardFocus(true);

}

void Table::paintRowBackground(Graphics& g, int rowNumber, int width, int height, bool rowIsSelected)
{
	using namespace simple_css;
	simple_css::Renderer r(nullptr, rootDialog.stateWatcher);

	auto point = table.getMouseXYRelative();

	auto hoverRow = table.getRowContainingPosition(point.getX(), point.getY());

	int flags = 0;

	if(rowNumber == hoverRow)
	{
		flags |= (int)PseudoClassType::Hover;

		if(isMouseButtonDownAnywhere())
		{
			flags |= (int)PseudoClassType::Active;
		}
	}

	if(rowIsSelected)
		flags |= (int)PseudoClassType::Focus;

	auto v = getValueFromGlobalState();

	if(v.isInt() && (int)v == rowNumber)
		flags |= (int)PseudoClassType::Checked;

	r.setPseudoClassState(flags);

	if(auto ss = rootDialog.css.getWithAllStates(this, (Selector(ElementType::TableRow))))
	{
		r.drawBackground(g, {0.0f, 0.0f, (float)width, (float)height}, ss);
	}
}

Result Table::checkGlobalState(var state)
{
	return Result::ok();
}

void Table::paint(Graphics& g)
{
	FlexboxComponent::paint(g);
        
	if(auto ss = rootDialog.css.getForComponent(&table))
	{
		simple_css::Renderer r(&table, rootDialog.stateWatcher);

		auto currentState = r.getPseudoClassState();

		rootDialog.stateWatcher.checkChanges(&table, ss, currentState);

		r.drawBackground(g, getLocalBounds().toFloat(), ss);

		if(getNumRows() == 0)
		{
			auto et = infoObject[mpid::EmptyText].toString();

			if(et.isNotEmpty())
				r.renderText(g, getLocalBounds().toFloat(), et, ss);
		}
	}
}

void Table::resized()
{
	FlexboxComponent::resized();

	table.getViewport()->getVerticalScrollBar().setAutoHide(false);

	auto area = getLocalBounds().toFloat();

	if(getParentComponent() != nullptr && !area.isEmpty())
	{ 
		using namespace simple_css;
		
		if(auto ss = rootDialog.css.getForComponent(&table))
		{
			area = ss->getArea(area, { "margin", 0});
			area = ss->getArea(area, { "padding", 0});
		}
		
		table.setBounds(area.toNearestInt());
	}
}

void Table::updateValue(EventType t, int row, int column)
{
	if(row == -1)
	{
		if(getFilterFunctionId().isValid())
			originalSelectionIndex = visibleItems[row].first;
		else
			originalSelectionIndex = row;
	}
	else
		originalSelectionIndex = -1;

	enum class ValueMode
	{
		Row,
		Grid,
		FirstColumnText
	};

	static const StringArray ValueModes = 
	{
		"Row",
		"Grid",
		"FirstColumnText"
	};

	auto idx = infoObject[mpid::ValueMode].toString();
	auto vm = ValueModes.indexOf(idx);

	if(vm == -1)
		return;

	static const StringArray EventTypes =
	{
		"click",
		"click",
		"dblclick",
		"select",
		"keydown"
	};
        
	auto etype = EventTypes[(int)t];

	auto toValue = [&]()
	{
		DynamicObject::Ptr v = new DynamicObject();

		v->setProperty("eventType", etype);
		v->setProperty("row", row);
		v->setProperty("originalRow", getFilterFunctionId().isValid() ? visibleItems[row].first : row);
		v->setProperty("column", column);
		return v;
	};

	if(t == EventType::DoubleClick || t == EventType::ReturnKey || infoObject[mpid::SelectOnClick])
	{
		writeState(row);
	}

	auto codeToExecute = infoObject[mpid::Code].toString();

	if(codeToExecute.startsWith("{BIND::"))
	{
		auto callbackName = codeToExecute.fromFirstOccurrenceOf("{BIND::", false, false).upToLastOccurrenceOf("}", false, false);

		var a[2];
		a[0] = var(id.toString());
		a[1] = var(toValue().get());

		auto state = &rootDialog.getState();

		var::NativeFunctionArgs args(state->globalState, a, 2);

		state->callNativeFunction(callbackName, args, nullptr);
	}
	
}

void Table::TableRepainter::mouseMove(const MouseEvent& event)
{
	if(event.eventComponent != lastComponent)
	{
		if(lastComponent != nullptr)
			lastComponent->repaint();

		lastComponent = event.eventComponent;
		table.repaint();

		if(lastComponent != nullptr)
			lastComponent->repaint();
	}
}
} // factory


template <typename T>
void Factory::registerPage()
{
	Item item;
	item.id = T::getStaticId();
	item.category = T::getCategoryId();
	item.f = [](Dialog& r, int width, const var& data) { return new T(r, width, data); };

	item.isContainer = std::is_base_of<factory::Container, T>();
        
	items.add(std::move(item));
}

Factory::Factory()
{
	registerPage<factory::Branch>();
	registerPage<factory::FileSelector>();
	registerPage<factory::Choice>();
	registerPage<factory::ColourChooser>();
	registerPage<factory::Column>();
	registerPage<factory::Spacer>();
	registerPage<factory::Image>();
	registerPage<factory::HtmlElement>();
	registerPage<factory::List>();
	registerPage<factory::SimpleText>();
    registerPage<factory::MarkdownText>();
	registerPage<factory::Placeholder<factory::DummyContent>>();
	registerPage<factory::Button>();
	registerPage<factory::TextInput>();
	registerPage<factory::Skip>();
	registerPage<factory::Launch>();
	registerPage<factory::DummyWait>();
	registerPage<factory::LambdaTask>();
	registerPage<factory::DownloadTask>();
	registerPage<factory::UnzipTask>();
	registerPage<factory::CommandLineTask>();
	registerPage<factory::HlacDecoder>();
	registerPage<factory::AppDataFileWriter>();
	registerPage<factory::RelativeFileLoader>();
	registerPage<factory::HttpRequest>();
	registerPage<factory::CodeEditor>();
	registerPage<factory::CopyProtection>();
	registerPage<factory::FileAction>();
	registerPage<factory::ProjectInfo>();
	registerPage<factory::PluginDirectories>();
	registerPage<factory::PersistentSettings>();
	registerPage<factory::CopyAsset>();
    registerPage<factory::CopySiblingFile>();
	registerPage<factory::OperatingSystem>();
	registerPage<factory::HiseActivator>();
	registerPage<factory::EventLogger>();
	registerPage<factory::FileLogger>();
	registerPage<factory::JavascriptFunction>();
    registerPage<factory::Table>();
    registerPage<factory::TagList>();
	registerPage<factory::DirectoryScanner>();
	registerPage<factory::ClipboardLoader>();
    registerPage<factory::CoallascatedTask>();
}

Dialog::PageInfo::Ptr Factory::create(const var& obj)
{
	Dialog::PageInfo::Ptr info = new Dialog::PageInfo(obj);
	
	auto typeProp = obj[mpid::Type].toString();

	if(typeProp.isEmpty())
		return nullptr;

	if(typeProp.isNotEmpty())
	{
		Identifier id(typeProp);

		for(const auto& i: items)
		{
			if(i.id == id)
			{
				info->setCreateFunction(i.f);
				break;
			}
		}
	}

	jassert(info->pageCreator);

	return info;
}

StringArray Factory::getIdList() const
{
	StringArray sa;
            
	for(const auto& i: items)
	{
		sa.add(i.id.toString());
	}
		
	return sa;
}

String Factory::getCategoryName(const String& id) const
{
	for(const auto& i: items)
		if(i.id == Identifier(id))
			return i.category.toString();

	return id;
}

StringArray Factory::getPopupMenuList() const
{
	StringArray sa;

	std::map<Identifier, StringArray> map; 

	map["UI Elements"] = {};
	map["Layout"] = {};
	map["Actions"] = {};

	for(const auto& i: items)
	{
		map[i.category].add(i.id.toString());
	}

	auto addCategory = [&](const String& c)
	{
		auto& m = map[c];
		m.sort(true);

		if(c != "Constants")
			m.getReference(m.size()-1) << '|';
		sa.add("**" + c + "**");
		sa.addArray(m);
	};

	addCategory("UI Elements");
	addCategory("Layout");
	addCategory("Actions");
	addCategory("Constants");
	
	return sa;
}


Colour Factory::getColourForCategory(const String& id) const
{
    std::map<Identifier, Colour> map;
    
    map["UI Elements"] = Colour(0xFFBE6093);
    map["Actions"] = Colour(0xFF9CC05B);
    map["Layout"] = Colour(0xFF7EB7C5);
    
    Identifier thisId(id);

    for(auto& i: items)
    {
        if(i.id == thisId)
            return map[i.category];
    }
    
    jassertfalse;
    return Colours::transparentBlack;
}

bool Factory::needsIdAtCreation(const String& id) const
{
	Array<Identifier> categoriesWithId = { Identifier("UI Elements"), Identifier("Actions") };

	Identifier thisId(id);

	for(auto& i: items)
	{
		if(i.id == thisId)
			return categoriesWithId.contains(i.category);
	}

	return false;
}

#if HISE_MULTIPAGE_INCLUDE_EDIT
namespace Icons
{

static const unsigned char EventLogger[] = { 110,109,152,168,9,68,192,166,171,67,108,152,168,9,68,48,98,144,67,98,104,237,8,68,128,92,144,67,148,96,8,68,144,6,144,67,116,7,8,68,128,97,143,67,98,84,174,7,68,24,188,142,67,0,128,7,68,224,230,141,67,0,128,7,68,144,225,140,67,98,0,128,7,68,136,214,139,
67,84,174,7,68,128,254,138,67,60,9,8,68,16,89,138,67,98,240,101,8,68,160,179,137,67,116,253,8,68,240,96,137,67,40,213,9,68,240,96,137,67,108,128,31,17,68,216,105,137,67,98,16,66,18,68,216,105,137,67,124,93,19,68,40,213,137,67,204,113,20,68,192,171,138,
67,98,220,135,21,68,104,130,139,67,176,102,22,68,32,147,140,67,64,14,23,68,248,221,141,67,98,152,142,23,68,136,215,142,67,128,18,24,68,200,45,144,67,140,157,24,68,24,225,145,67,98,208,38,25,68,104,148,147,67,48,142,25,68,120,68,149,67,184,211,25,68,24,
242,150,67,98,60,25,26,68,80,159,152,67,224,60,26,68,120,172,154,67,224,60,26,68,136,25,157,67,108,224,60,26,68,80,110,160,67,98,224,60,26,68,176,96,163,67,180,252,25,68,128,253,165,67,96,124,25,68,200,68,168,67,98,208,253,24,68,168,139,170,67,60,86,
24,68,160,103,172,67,172,133,23,68,64,216,173,67,98,232,182,22,68,136,72,175,67,152,244,21,68,216,87,176,67,0,61,21,68,208,5,177,67,98,112,26,20,68,144,22,178,67,84,162,18,68,24,159,178,67,104,214,16,68,24,159,178,67,108,40,213,9,68,24,159,178,67,98,
116,253,8,68,24,159,178,67,240,101,8,68,96,76,178,67,60,9,8,68,248,166,177,67,98,84,174,7,68,136,1,177,67,0,128,7,68,128,41,176,67,0,128,7,68,120,30,175,67,98,0,128,7,68,128,25,174,67,84,174,7,68,136,66,173,67,60,9,8,68,64,154,172,67,98,240,101,8,68,
248,241,171,67,48,239,8,68,8,161,171,67,152,168,9,68,192,166,171,67,99,109,168,37,13,68,192,166,171,67,108,248,217,16,68,192,166,171,67,98,180,60,18,68,192,166,171,67,80,70,19,68,32,65,171,67,88,250,19,68,240,117,170,67,98,168,229,20,68,64,107,169,67,
36,150,21,68,216,23,168,67,140,13,22,68,184,123,166,67,98,252,132,22,68,152,223,164,67,208,191,22,68,232,201,162,67,208,191,22,68,64,58,160,67,108,208,191,22,68,8,238,156,67,98,208,191,22,68,24,187,154,67,88,138,22,68,176,205,152,67,100,31,22,68,32,38,
151,67,98,212,119,21,68,208,144,148,67,24,185,20,68,160,202,146,67,192,230,19,68,232,211,145,67,98,100,20,19,68,144,221,144,67,92,14,18,68,48,98,144,67,16,209,16,68,48,98,144,67,108,168,37,13,68,48,98,144,67,108,168,37,13,68,192,166,171,67,99,109,252,
58,31,68,192,166,171,67,108,252,58,31,68,48,98,144,67,108,240,175,30,68,48,98,144,67,98,12,218,29,68,48,98,144,67,192,64,29,68,128,15,144,67,212,229,28,68,16,106,143,67,98,236,138,28,68,160,196,142,67,148,92,28,68,152,236,141,67,148,92,28,68,144,225,
140,67,98,148,92,28,68,136,214,139,67,236,138,28,68,128,254,138,67,212,229,28,68,16,89,138,67,98,192,64,29,68,160,179,137,67,12,218,29,68,240,96,137,67,240,175,30,68,240,96,137,67,108,20,109,39,68,240,96,137,67,98,216,131,41,68,240,96,137,67,112,49,43,
68,160,138,138,67,224,117,44,68,72,221,140,67,98,76,186,45,68,248,47,143,67,132,92,46,68,144,242,145,67,132,92,46,68,192,36,149,67,98,132,92,46,68,96,169,150,67,24,55,46,68,40,23,152,67,56,236,45,68,104,109,153,67,98,84,161,45,68,168,195,154,67,124,45,
45,68,224,255,155,67,48,148,44,68,24,34,157,67,98,212,173,45,68,160,114,158,67,40,128,46,68,224,251,159,67,252,12,47,68,112,189,161,67,98,208,153,47,68,88,127,163,67,32,225,47,68,112,124,165,67,32,225,47,68,24,181,167,67,98,32,225,47,68,128,121,169,67,
108,173,47,68,48,30,171,67,204,71,47,68,40,163,172,67,98,244,252,46,68,16,203,173,67,64,160,46,68,0,182,174,67,188,49,46,68,248,99,175,67,98,200,157,45,68,216,87,176,67,248,231,44,68,104,30,177,67,12,18,44,68,112,184,177,67,98,92,58,43,68,24,82,178,67,
248,46,42,68,24,159,178,67,88,236,40,68,24,159,178,67,108,240,175,30,68,24,159,178,67,98,12,218,29,68,24,159,178,67,192,64,29,68,96,76,178,67,212,229,28,68,248,166,177,67,98,236,138,28,68,136,1,177,67,148,92,28,68,128,41,176,67,148,92,28,68,120,30,175,
67,98,148,92,28,68,128,25,174,67,236,138,28,68,80,68,173,67,160,231,28,68,224,158,172,67,98,76,68,29,68,112,249,171,67,156,221,29,68,192,166,171,67,240,175,30,68,192,166,171,67,108,252,58,31,68,192,166,171,67,99,109,16,184,34,68,192,166,171,67,108,52,
188,40,68,192,166,171,67,98,212,39,42,68,192,166,171,67,192,38,43,68,112,59,171,67,180,186,43,68,208,100,170,67,98,0,43,44,68,64,194,169,67,12,100,44,68,40,218,168,67,12,100,44,68,48,172,167,67,98,12,100,44,68,120,68,166,67,192,243,43,68,112,226,164,
67,240,20,43,68,128,134,163,67,98,28,54,42,68,48,42,162,67,60,245,40,68,48,124,161,67,32,84,39,68,48,124,161,67,108,16,184,34,68,48,124,161,67,108,16,184,34,68,192,166,171,67,99,109,16,184,34,68,216,131,154,67,108,176,138,38,68,216,131,154,67,98,216,
233,39,68,216,131,154,67,52,14,41,68,0,211,153,67,240,245,41,68,248,112,152,67,98,208,146,42,68,48,131,151,67,60,225,42,68,72,91,150,67,60,225,42,68,64,249,148,67,98,60,225,42,68,232,191,147,67,92,150,42,68,184,173,146,67,104,2,42,68,200,194,145,67,98,
120,110,41,68,128,215,144,67,240,132,40,68,48,98,144,67,76,66,39,68,48,98,144,67,108,16,184,34,68,48,98,144,67,108,16,184,34,68,216,131,154,67,99,109,24,6,68,68,216,222,165,67,108,24,6,68,68,128,56,175,67,98,112,113,66,68,200,235,176,67,52,27,65,68,32,
21,178,67,196,255,63,68,216,180,178,67,98,32,230,62,68,48,84,179,67,136,179,61,68,16,164,179,67,136,107,60,68,16,164,179,67,98,248,164,58,68,16,164,179,67,128,254,56,68,224,27,179,67,228,121,55,68,40,11,178,67,98,20,73,54,68,56,58,177,67,112,88,53,68,
8,50,176,67,192,169,52,68,248,242,174,67,98,72,249,51,68,224,179,173,67,216,88,51,68,56,220,171,67,224,196,50,68,168,108,169,67,98,236,48,50,68,200,252,166,67,12,230,49,68,24,67,164,67,12,230,49,68,64,63,161,67,108,12,230,49,68,32,55,156,67,98,12,230,
49,68,120,145,151,67,224,155,50,68,192,107,147,67,192,5,52,68,88,197,143,67,98,44,238,53,68,104,212,138,67,72,133,56,68,248,91,136,67,24,203,59,68,248,91,136,67,98,80,191,60,68,248,91,136,67,8,167,61,68,40,141,136,67,80,130,62,68,232,239,136,67,98,200,
91,63,68,176,82,137,67,84,44,64,68,160,230,137,67,52,242,64,68,192,171,138,67,98,160,105,65,68,40,213,137,67,128,221,65,68,216,105,137,67,204,77,66,68,216,105,137,67,98,236,207,66,68,216,105,137,67,224,58,67,68,176,195,137,67,28,139,67,68,184,119,138,
67,98,24,221,67,68,104,43,139,67,24,6,68,68,56,92,140,67,24,6,68,68,120,9,142,67,108,24,6,68,68,16,143,146,67,98,24,6,68,68,168,60,148,67,24,221,67,68,144,110,149,67,80,137,67,68,120,37,150,67,98,80,55,67,68,88,220,150,67,92,204,66,68,160,55,151,67,60,
74,66,68,160,55,151,67,98,16,225,65,68,160,55,151,67,240,135,65,68,200,247,150,67,72,59,65,68,40,120,150,67,98,64,2,65,68,40,33,150,67,28,210,64,68,72,96,149,67,28,169,64,68,48,53,148,67,98,172,131,64,68,104,10,147,67,28,87,64,68,0,65,146,67,52,37,64,
68,136,216,145,67,98,196,214,63,68,72,48,145,67,184,75,63,68,136,154,144,67,220,133,62,68,8,24,144,67,98,52,190,61,68,144,149,143,67,100,223,60,68,80,84,143,67,100,233,59,68,80,84,143,67,98,112,136,58,68,80,84,143,67,16,84,57,68,104,229,143,67,208,79,
56,68,160,7,145,67,98,164,148,55,68,64,222,145,67,132,233,54,68,144,84,147,67,112,78,54,68,72,106,149,67,98,144,177,53,68,0,128,151,67,32,99,53,68,96,196,153,67,32,99,53,68,32,55,156,67,108,32,99,53,68,64,63,161,67,98,32,99,53,68,208,252,164,67,100,236,
53,68,104,211,167,67,120,2,55,68,168,195,169,67,98,196,22,56,68,144,179,171,67,20,207,57,68,176,171,172,67,88,43,60,68,176,171,172,67,98,144,195,61,68,176,171,172,67,28,56,63,68,16,18,172,67,4,137,64,68,104,222,170,67,108,4,137,64,68,216,222,165,67,108,
232,251,60,68,216,222,165,67,98,60,36,60,68,216,222,165,67,184,140,59,68,128,140,165,67,4,48,59,68,24,231,164,67,98,28,213,58,68,168,65,164,67,140,168,58,68,160,105,163,67,140,168,58,68,144,94,162,67,98,140,168,58,68,136,83,161,67,28,213,58,68,128,123,
160,67,4,48,59,68,24,214,159,67,98,184,140,59,68,168,48,159,67,60,36,60,68,240,221,158,67,232,251,60,68,240,221,158,67,108,160,44,67,68,216,230,158,67,98,132,2,68,68,216,230,158,67,212,155,68,68,200,55,159,67,188,246,68,68,88,218,159,67,98,112,83,69,
68,232,124,160,67,0,128,69,68,136,83,161,67,0,128,69,68,144,94,162,67,98,0,128,69,68,128,47,163,67,180,97,69,68,8,230,163,67,76,35,69,68,232,130,164,67,98,232,228,68,68,104,31,165,67,164,132,68,68,160,147,165,67,24,6,68,68,216,222,165,67,99,101,0,0 };

static const unsigned char List[] = { 110,109,1,128,69,68,84,19,85,67,108,1,128,69,68,106,235,121,67,108,1,128,7,68,106,235,121,67,108,1,128,7,68,84,19,85,67,108,1,128,69,68,84,19,85,67,99,109,1,128,69,68,167,201,148,67,108,1,128,69,68,6,54,167,67,108,1,128,7,68,6,54,167,67,108,1,128,7,68,
167,201,148,67,108,1,128,69,68,167,201,148,67,99,109,1,128,69,68,247,9,191,67,108,1,128,69,68,87,118,209,67,108,1,128,7,68,87,118,209,67,108,1,128,7,68,247,9,191,67,108,1,128,69,68,247,9,191,67,99,101,0,0 };

static const unsigned char Column[] = { 110,109,0,128,69,68,198,144,208,67,108,252,102,58,68,198,144,208,67,108,252,102,58,68,116,222,86,67,108,0,128,69,68,116,222,86,67,108,0,128,69,68,198,144,208,67,99,109,130,12,44,68,198,144,208,67,108,126,243,32,68,198,144,208,67,108,126,243,32,68,116,
222,86,67,108,130,12,44,68,116,222,86,67,108,130,12,44,68,198,144,208,67,99,109,5,153,18,68,198,144,208,67,108,0,128,7,68,198,144,208,67,108,0,128,7,68,116,222,86,67,108,5,153,18,68,116,222,86,67,108,5,153,18,68,198,144,208,67,99,101,0,0 };

static const unsigned char Branch[] = { 110,109,246,192,33,68,92,80,167,67,108,0,128,7,68,92,80,167,67,108,0,128,7,68,126,184,148,67,108,246,192,33,68,126,184,148,67,108,246,192,33,68,172,21,84,67,108,232,149,39,68,172,21,84,67,108,232,149,39,68,92,39,84,67,108,0,128,69,68,92,39,84,67,108,
0,128,69,68,188,87,121,67,108,232,149,39,68,188,87,121,67,108,232,149,39,68,126,184,148,67,108,0,128,69,68,126,184,148,67,108,0,128,69,68,92,80,167,67,108,232,149,39,68,92,80,167,67,108,232,149,39,68,250,92,191,67,108,0,128,69,68,250,92,191,67,108,0,
128,69,68,42,245,209,67,108,119,255,35,68,42,245,209,67,108,119,255,35,68,230,241,209,67,108,246,192,33,68,230,241,209,67,108,246,192,33,68,92,80,167,67,99,101,0,0 };

static const unsigned char MarkdownText[] = { 110,109,94,87,36,68,204,30,87,67,108,94,87,36,68,28,171,113,67,108,206,89,21,68,28,171,113,67,108,206,89,21,68,62,123,140,67,108,50,94,22,68,62,123,140,67,98,56,199,22,68,150,32,135,67,116,194,23,68,128,1,131,67,10,80,25,68,84,29,128,67,98,68,107,26,
68,16,23,124,67,204,49,28,68,240,4,122,67,156,163,30,68,240,4,122,67,108,26,67,33,68,240,4,122,67,108,26,67,33,68,132,1,181,67,98,26,67,33,68,102,165,184,67,4,42,33,68,50,229,186,67,172,247,32,68,66,192,187,67,98,40,179,32,68,254,246,188,67,140,85,32,
68,96,210,189,67,216,222,31,68,18,82,190,67,98,80,58,31,68,30,18,191,67,88,97,30,68,250,113,191,67,200,83,29,68,250,113,191,67,108,118,31,28,68,250,113,191,67,108,118,31,28,68,238,108,193,67,108,94,87,36,68,238,108,193,67,108,94,87,36,68,154,112,208,
67,108,88,159,16,68,154,112,208,67,98,102,150,11,68,154,112,208,67,0,128,7,68,124,67,200,67,0,128,7,68,150,49,190,67,108,0,128,7,68,44,156,123,67,98,0,128,7,68,108,120,103,67,102,150,11,68,204,30,87,67,88,159,16,68,204,30,87,67,108,94,87,36,68,204,30,
87,67,99,109,36,144,36,68,154,112,208,67,108,36,144,36,68,238,108,193,67,108,156,32,47,68,238,108,193,67,108,156,32,47,68,250,113,191,67,108,72,236,45,68,250,113,191,67,98,98,227,44,68,250,113,191,67,76,18,44,68,222,24,191,67,88,121,43,68,172,102,190,
67,98,56,224,42,68,116,180,189,67,116,121,42,68,168,228,188,67,4,69,42,68,66,247,187,67,98,108,16,42,68,140,9,187,67,52,246,41,68,188,183,184,67,52,246,41,68,132,1,181,67,108,52,246,41,68,240,4,122,67,108,74,170,44,68,240,4,122,67,98,58,92,46,68,240,
4,122,67,12,140,47,68,0,169,122,67,150,57,48,68,100,242,123,67,98,42,112,49,68,52,77,126,67,162,100,50,68,242,186,128,67,216,22,51,68,232,181,130,67,98,228,200,51,68,48,177,132,67,44,107,52,68,164,242,135,67,96,253,52,68,62,123,140,67,108,238,243,53,
68,62,123,140,67,108,238,243,53,68,28,171,113,67,108,36,144,36,68,28,171,113,67,108,36,144,36,68,204,30,87,67,108,166,96,60,68,204,30,87,67,98,152,105,65,68,204,30,87,67,0,128,69,68,108,120,103,67,0,128,69,68,44,156,123,67,108,0,128,69,68,150,49,190,
67,98,0,128,69,68,124,67,200,67,152,105,65,68,154,112,208,67,166,96,60,68,154,112,208,67,108,36,144,36,68,154,112,208,67,99,101,0,0 };

static const unsigned char ProjectInfo[] = { 110,109,220,24,38,68,196,254,219,67,98,96,44,21,68,70,144,219,67,0,128,7,68,100,243,191,67,0,128,7,68,0,0,158,67,98,0,128,7,68,60,25,120,67,96,44,21,68,120,223,64,67,220,24,38,68,120,2,64,67,108,220,24,38,68,170,209,94,67,98,120,37,37,68,64,156,95,67,
156,78,36,68,20,123,97,67,152,148,35,68,178,107,100,67,98,136,149,34,68,6,115,104,67,40,22,34,68,76,76,109,67,40,22,34,68,74,246,114,67,98,40,22,34,68,132,161,120,67,136,149,34,68,168,116,125,67,152,148,35,68,120,184,128,67,98,156,78,36,68,123,44,130,
67,120,37,37,68,212,24,131,67,220,24,38,68,130,125,131,67,108,220,24,38,68,138,69,141,67,108,28,213,30,68,138,69,141,67,108,28,213,30,68,198,186,143,67,98,244,74,32,68,52,232,143,67,68,71,33,68,58,154,144,67,184,201,33,68,20,210,145,67,98,220,75,34,68,
240,9,147,67,64,141,34,68,244,214,149,67,64,141,34,68,132,56,154,67,108,64,141,34,68,18,123,190,67,98,64,141,34,68,66,221,194,67,116,84,34,68,124,144,197,67,48,227,33,68,100,149,198,67,98,36,57,33,68,74,22,200,67,140,52,32,68,184,226,200,67,28,213,30,
68,110,249,200,67,108,28,213,30,68,122,93,203,67,108,220,24,38,68,122,93,203,67,108,220,24,38,68,196,254,219,67,99,109,24,119,38,68,4,0,64,67,98,44,122,38,68,4,0,64,67,240,124,38,68,4,0,64,67,0,128,38,68,4,0,64,67,98,16,156,55,68,4,0,64,67,0,128,69,68,
186,143,119,67,0,128,69,68,0,0,158,67,98,0,128,69,68,36,56,192,67,16,156,55,68,254,255,219,67,0,128,38,68,254,255,219,67,98,240,124,38,68,254,255,219,67,44,122,38,68,254,255,219,67,24,119,38,68,254,255,219,67,108,24,119,38,68,122,93,203,67,108,180,153,
47,68,122,93,203,67,108,180,153,47,68,110,249,200,67,98,4,30,46,68,2,204,200,67,240,30,45,68,94,25,200,67,204,156,44,68,132,225,198,67,98,88,26,44,68,70,170,197,67,68,217,43,68,66,221,194,67,68,217,43,68,18,123,190,67,108,68,217,43,68,138,69,141,67,108,
24,119,38,68,138,69,141,67,108,24,119,38,68,108,157,131,67,98,48,180,38,68,98,173,131,67,204,242,38,68,94,181,131,67,68,51,39,68,94,181,131,67,98,192,157,40,68,94,181,131,67,16,212,41,68,154,182,130,67,232,213,42,68,120,184,128,67,98,192,215,43,68,168,
116,125,67,168,88,44,68,132,161,120,67,168,88,44,68,74,246,114,67,98,168,88,44,68,76,76,109,67,68,217,43,68,6,115,104,67,52,218,42,68,178,107,100,67,98,32,219,41,68,94,100,96,67,148,163,40,68,180,96,94,67,68,51,39,68,180,96,94,67,98,204,242,38,68,180,
96,94,67,48,180,38,68,170,112,94,67,24,119,38,68,210,145,94,67,108,24,119,38,68,4,0,64,67,99,101,0,0 };

static const unsigned char CopyProtection[] = { 110,109,123,223,13,68,224,67,46,67,98,123,223,13,68,224,67,46,67,89,157,24,68,203,234,72,67,55,250,36,68,147,75,46,67,98,58,46,37,68,134,219,45,67,63,236,38,68,226,17,42,67,24,59,41,68,18,64,47,67,98,62,31,43,68,27,127,51,67,222,112,52,68,220,253,72,
67,19,46,63,68,213,54,46,67,98,182,74,64,68,21,113,43,67,69,105,65,68,211,37,43,67,37,131,66,68,16,144,44,67,98,142,44,67,68,34,106,45,67,3,94,68,68,209,86,47,67,170,3,69,68,161,1,54,67,98,159,16,69,68,107,134,54,67,191,77,69,68,83,219,57,67,29,87,69,
68,4,159,63,67,98,1,128,69,68,116,245,88,67,15,96,69,68,208,249,172,67,15,96,69,68,208,249,172,67,108,185,95,69,68,80,188,173,67,108,254,76,69,68,33,123,174,67,98,254,76,69,68,33,123,174,67,32,149,66,68,220,166,206,67,179,174,40,68,161,130,229,67,98,
174,144,40,68,56,157,229,67,30,223,38,68,16,247,230,67,100,130,36,68,187,171,228,67,98,96,38,33,68,141,104,225,67,214,223,20,68,212,248,211,67,240,186,13,68,192,102,196,67,98,194,21,10,68,252,116,188,67,131,203,7,68,27,206,179,67,72,185,7,68,125,164,
171,67,98,1,128,7,68,230,239,145,67,208,193,7,68,129,152,59,67,208,193,7,68,129,152,59,67,108,123,223,13,68,224,67,46,67,99,109,124,25,39,68,68,109,213,67,98,51,5,57,68,107,151,196,67,35,221,60,68,137,132,175,67,218,102,61,68,105,239,171,67,98,8,106,
61,68,205,80,165,67,87,123,61,68,76,121,125,67,245,108,61,68,250,248,84,67,98,147,37,51,68,142,199,99,67,227,134,42,68,188,215,85,67,144,207,38,68,119,69,78,67,98,31,132,29,68,16,100,95,67,44,18,21,68,242,171,90,67,164,174,15,68,172,219,83,67,98,235,
158,15,68,196,153,117,67,218,138,15,68,85,128,153,67,167,178,15,68,106,93,171,67,98,66,185,15,68,161,74,174,67,203,110,16,68,215,52,177,67,253,106,17,68,17,36,180,67,98,155,179,18,68,188,246,183,67,213,124,20,68,252,175,187,67,16,121,22,68,14,54,191,
67,98,77,73,28,68,56,135,201,67,91,205,35,68,122,9,210,67,124,25,39,68,68,109,213,67,99,101,0,0 };


static const unsigned char Button[] = { 110,109,0,128,69,68,232,149,100,67,108,0,128,69,68,12,181,201,67,98,0,128,69,68,160,205,211,67,210,102,65,68,0,0,220,67,136,90,60,68,0,0,220,67,108,122,165,16,68,0,0,220,67,98,48,153,11,68,0,0,220,67,0,128,7,68,160,205,211,67,0,128,7,68,12,181,201,67,
108,0,128,7,68,232,149,100,67,98,0,128,7,68,200,100,80,67,48,153,11,68,8,0,64,67,122,165,16,68,8,0,64,67,108,136,90,60,68,8,0,64,67,98,210,102,65,68,8,0,64,67,0,128,69,68,200,100,80,67,0,128,69,68,232,149,100,67,99,109,162,217,64,68,136,178,113,67,98,
162,217,64,68,136,136,96,67,226,93,61,68,120,153,82,67,98,19,57,68,120,153,82,67,108,164,236,19,68,120,153,82,67,98,84,162,15,68,120,153,82,67,96,38,12,68,136,136,96,67,96,38,12,68,136,178,113,67,108,96,38,12,68,192,38,195,67,98,96,38,12,68,192,187,203,
67,84,162,15,68,72,179,210,67,164,236,19,68,72,179,210,67,108,98,19,57,68,72,179,210,67,98,226,93,61,68,72,179,210,67,162,217,64,68,192,187,203,67,162,217,64,68,192,38,195,67,108,162,217,64,68,136,178,113,67,99,109,210,54,58,68,176,52,130,67,108,210,
54,58,68,84,203,185,67,98,210,54,58,68,52,55,192,67,152,155,55,68,168,109,197,67,168,101,52,68,168,109,197,67,108,90,154,24,68,168,109,197,67,98,104,100,21,68,168,109,197,67,96,201,18,68,52,55,192,67,96,201,18,68,84,203,185,67,108,96,201,18,68,176,52,
130,67,98,96,201,18,68,160,145,119,67,104,100,21,68,128,37,109,67,90,154,24,68,128,37,109,67,108,168,101,52,68,128,37,109,67,98,152,155,55,68,128,37,109,67,210,54,58,68,160,145,119,67,210,54,58,68,176,52,130,67,99,101,0,0 };

static const unsigned char Choice[] = { 110,109,4,128,69,68,232,149,100,67,108,4,128,69,68,12,181,201,67,98,4,128,69,68,160,205,211,67,212,102,65,68,0,0,220,67,140,90,60,68,0,0,220,67,108,124,165,16,68,0,0,220,67,98,52,153,11,68,0,0,220,67,4,128,7,68,160,205,211,67,4,128,7,68,12,181,201,67,
108,4,128,7,68,232,149,100,67,98,4,128,7,68,200,100,80,67,52,153,11,68,8,0,64,67,124,165,16,68,8,0,64,67,108,140,90,60,68,8,0,64,67,98,212,102,65,68,8,0,64,67,4,128,69,68,200,100,80,67,4,128,69,68,232,149,100,67,99,109,164,217,64,68,136,178,113,67,98,
164,217,64,68,136,136,96,67,228,93,61,68,120,153,82,67,100,19,57,68,120,153,82,67,108,164,236,19,68,120,153,82,67,98,36,162,15,68,120,153,82,67,100,38,12,68,136,136,96,67,100,38,12,68,136,178,113,67,108,100,38,12,68,192,38,195,67,98,100,38,12,68,192,
187,203,67,36,162,15,68,72,179,210,67,164,236,19,68,72,179,210,67,108,100,19,57,68,72,179,210,67,98,228,93,61,68,72,179,210,67,164,217,64,68,192,187,203,67,164,217,64,68,192,38,195,67,108,164,217,64,68,136,178,113,67,99,109,236,15,41,68,216,112,179,67,
98,232,160,40,68,72,142,180,67,88,246,39,68,96,53,181,67,132,65,39,68,96,53,181,67,98,124,140,38,68,96,53,181,67,232,225,37,68,72,142,180,67,228,114,37,68,216,112,179,67,108,4,43,25,68,184,222,147,67,98,204,170,24,68,44,149,146,67,148,147,24,68,112,214,
144,67,64,239,24,68,160,95,143,67,98,32,75,25,68,108,232,141,67,160,9,26,68,140,250,140,67,76,218,26,68,140,250,140,67,108,152,100,50,68,140,250,140,67,98,204,113,51,68,140,250,140,67,152,103,52,68,68,45,142,67,196,221,52,68,44,17,144,67,98,32,84,53,
68,8,245,145,67,88,54,53,68,92,53,148,67,252,144,52,68,116,222,149,67,108,236,15,41,68,216,112,179,67,99,101,0,0 };


static const unsigned char TextInput[] = { 110,109,0,128,69,68,24,149,100,67,108,0,128,69,68,116,181,201,67,98,0,128,69,68,32,206,211,67,188,103,65,68,144,0,220,67,8,91,60,68,144,0,220,67,108,144,165,16,68,144,0,220,67,98,60,153,11,68,144,0,220,67,0,128,7,68,32,206,211,67,0,128,7,68,116,181,201,
67,108,0,128,7,68,24,149,100,67,98,0,128,7,68,192,99,80,67,60,153,11,68,224,254,63,67,144,165,16,68,224,254,63,67,108,8,91,60,68,224,254,63,67,98,188,103,65,68,224,254,63,67,0,128,69,68,192,99,80,67,0,128,69,68,24,149,100,67,99,109,144,218,64,68,208,
177,113,67,98,144,218,64,68,168,135,96,67,100,94,61,68,120,152,82,67,212,19,57,68,120,152,82,67,108,192,236,19,68,120,152,82,67,98,104,162,15,68,120,152,82,67,108,38,12,68,168,135,96,67,108,38,12,68,208,177,113,67,108,108,38,12,68,24,39,195,67,98,108,
38,12,68,44,188,203,67,104,162,15,68,196,179,210,67,192,236,19,68,196,179,210,67,108,212,19,57,68,196,179,210,67,98,100,94,61,68,196,179,210,67,144,218,64,68,44,188,203,67,144,218,64,68,24,39,195,67,108,144,218,64,68,208,177,113,67,99,109,240,125,53,
68,160,106,124,67,108,240,125,53,68,152,76,144,67,108,176,154,52,68,152,76,144,67,98,20,20,52,68,56,32,140,67,196,126,51,68,104,32,137,67,192,218,50,68,136,77,135,67,98,192,54,50,68,164,122,133,67,160,85,49,68,136,6,132,67,148,55,48,68,204,240,130,67,
98,212,151,47,68,144,89,130,67,40,128,46,68,192,13,130,67,152,240,44,68,192,13,130,67,108,84,115,42,68,192,13,130,67,108,84,115,42,68,224,156,181,67,98,84,115,42,68,220,7,185,67,128,139,42,68,200,42,187,67,216,187,42,68,84,5,188,67,98,52,236,42,68,72,
224,188,67,200,74,43,68,88,159,189,67,196,215,43,68,196,67,190,67,98,144,100,44,68,192,231,190,67,0,37,45,68,196,57,191,67,20,25,46,68,196,57,191,67,108,0,53,47,68,196,57,191,67,108,0,53,47,68,164,12,193,67,108,88,181,29,68,164,12,193,67,108,88,181,29,
68,196,57,191,67,108,68,209,30,68,196,57,191,67,98,104,201,31,68,196,57,191,67,48,145,32,68,48,225,190,67,160,40,33,68,204,48,190,67,98,232,149,33,68,212,186,189,67,40,236,33,68,232,240,188,67,52,43,34,68,16,211,187,67,98,164,89,34,68,36,9,187,67,164,
112,34,68,52,247,184,67,164,112,34,68,224,156,181,67,108,164,112,34,68,192,13,130,67,108,84,6,32,68,192,13,130,67,98,44,198,29,68,192,13,130,67,168,35,28,68,212,1,131,67,192,30,27,68,152,233,132,67,98,208,176,25,68,40,147,135,67,132,201,24,68,208,94,
139,67,208,104,24,68,152,76,144,67,108,252,120,23,68,152,76,144,67,108,252,120,23,68,160,106,124,67,108,240,125,53,68,160,106,124,67,99,101,0,0 };

static const unsigned char ColourChooser[] = { 110,109,192,128,38,68,136,65,208,67,98,136,86,35,68,48,120,212,67,160,138,31,68,208,236,214,67,104,117,27,68,208,236,214,67,98,136,113,16,68,208,236,214,67,0,128,7,68,192,10,197,67,0,128,7,68,0,3,175,67,98,0,128,7,68,184,95,159,67,128,1,12,68,56,211,
145,67,88,143,18,68,224,70,139,67,98,160,2,19,68,96,21,108,67,184,197,27,68,80,38,74,67,64,128,38,68,80,38,74,67,98,80,58,49,68,80,38,74,67,104,253,57,68,96,19,108,67,176,112,58,68,224,69,139,67,98,128,254,64,68,64,209,145,67,0,128,69,68,176,94,159,67,
0,128,69,68,0,1,175,67,98,0,128,69,68,192,8,197,67,128,142,60,68,200,235,214,67,152,138,49,68,200,235,214,67,98,224,117,45,68,200,235,214,67,0,170,41,68,40,119,212,67,192,128,38,68,136,65,208,67,99,109,88,163,35,68,24,161,203,67,98,80,24,32,68,240,190,
196,67,152,211,29,68,48,66,187,67,56,154,29,68,8,185,176,67,98,96,231,23,68,56,8,171,67,48,193,19,68,88,12,160,67,192,196,18,68,152,0,147,67,98,80,23,14,68,88,211,152,67,16,250,10,68,144,49,163,67,16,250,10,68,0,3,175,67,98,16,250,10,68,232,51,193,67,
120,92,18,68,176,248,207,67,104,117,27,68,176,248,207,67,98,64,111,30,68,176,248,207,67,48,58,33,68,136,100,206,67,88,163,35,68,24,161,203,67,99,109,88,43,22,68,104,131,136,67,98,232,218,23,68,232,150,135,67,240,160,25,68,56,24,135,67,104,117,27,68,56,
24,135,67,98,32,138,31,68,56,24,135,67,0,86,35,68,216,140,137,67,200,127,38,68,128,194,141,67,98,128,169,41,68,216,139,137,67,96,117,45,68,56,23,135,67,152,138,49,68,56,23,135,67,98,24,95,51,68,56,23,135,67,24,37,53,68,240,148,135,67,176,212,54,68,112,
130,136,67,98,72,188,53,68,192,220,112,67,24,214,46,68,144,14,88,67,64,128,38,68,144,14,88,67,98,224,41,30,68,144,14,88,67,64,67,23,68,176,222,112,67,88,43,22,68,104,131,136,67,99,109,200,59,58,68,144,255,146,67,98,208,62,57,68,88,12,160,67,168,24,53,
68,56,8,171,67,208,101,47,68,8,185,176,67,98,240,44,47,68,48,66,187,67,48,232,44,68,240,190,196,67,168,93,41,68,24,161,203,67,98,208,198,43,68,144,99,206,67,64,145,46,68,168,247,207,67,152,138,49,68,168,247,207,67,98,144,163,58,68,168,247,207,67,240,
5,66,68,224,50,193,67,240,5,66,68,0,1,175,67,98,240,5,66,68,152,48,163,67,168,232,62,68,88,209,152,67,200,59,58,68,144,255,146,67,99,109,96,20,22,68,88,216,143,67,98,32,137,22,68,112,118,154,67,120,131,25,68,80,166,163,67,24,207,29,68,88,255,168,67,98,
184,121,30,68,64,48,160,67,16,149,32,68,152,82,152,67,88,162,35,68,240,99,146,67,98,48,57,33,68,120,160,143,67,192,110,30,68,88,12,142,67,104,117,27,68,88,12,142,67,98,112,147,25,68,88,12,142,67,248,195,23,68,248,173,142,67,96,20,22,68,88,216,143,67,
99,109,64,128,38,68,96,120,199,67,98,48,81,41,68,64,97,194,67,152,64,43,68,40,96,187,67,208,201,43,68,128,124,179,67,98,192,26,42,68,248,104,180,67,192,84,40,68,184,231,180,67,64,128,38,68,184,231,180,67,98,72,171,36,68,184,231,180,67,72,229,34,68,248,
104,180,67,56,54,33,68,128,124,179,67,98,232,191,33,68,40,96,187,67,80,175,35,68,56,98,194,67,64,128,38,68,96,120,199,67,99,109,160,235,54,68,88,215,143,67,98,16,60,53,68,0,173,142,67,144,108,51,68,88,11,142,67,152,138,49,68,88,11,142,67,98,200,144,46,
68,88,11,142,67,80,198,43,68,120,159,143,67,40,93,41,68,240,98,146,67,98,112,106,44,68,160,81,152,67,72,134,46,68,64,48,160,67,232,48,47,68,88,255,168,67,98,136,124,51,68,80,166,163,67,224,118,54,68,112,118,154,67,160,235,54,68,88,215,143,67,99,109,200,
127,38,68,152,139,150,67,98,112,125,35,68,152,251,155,67,8,125,33,68,88,154,163,67,64,31,33,68,152,39,172,67,98,208,206,34,68,240,81,173,67,208,157,36,68,152,243,173,67,64,128,38,68,152,243,173,67,98,56,98,40,68,152,243,173,67,48,49,42,68,240,81,173,
67,192,224,43,68,152,39,172,67,98,248,130,43,68,88,154,163,67,24,130,41,68,152,250,155,67,200,127,38,68,152,139,150,67,99,101,0,0 };

static const unsigned char FileSelector[] = { 110,109,128,141,49,68,184,130,204,67,108,4,128,7,68,184,130,204,67,108,108,157,28,68,136,210,134,67,108,28,33,68,68,136,210,134,67,98,144,157,68,68,136,210,134,67,76,15,69,68,248,122,135,67,164,71,69,68,80,133,136,67,98,4,128,69,68,88,144,137,67,68,117,
69,68,96,208,138,67,36,44,69,68,232,193,139,67,98,244,90,64,68,120,168,155,67,128,141,49,68,184,130,204,67,128,141,49,68,184,130,204,67,99,109,128,141,49,68,80,171,126,67,108,108,42,26,68,80,171,126,67,108,4,128,7,68,0,240,188,67,108,4,128,7,68,32,145,
101,67,98,4,128,7,68,64,210,99,67,236,164,7,68,240,36,98,67,172,230,7,68,64,232,96,67,98,108,40,8,68,224,172,95,67,168,129,8,68,112,250,94,67,236,222,8,68,192,251,94,67,98,96,74,15,68,128,10,95,67,192,83,42,68,144,69,95,67,60,91,48,68,0,83,95,67,98,76,
4,49,68,176,85,95,67,128,141,49,68,224,233,97,67,128,141,49,68,0,23,101,67,98,128,141,49,68,16,35,110,67,128,141,49,68,80,171,126,67,128,141,49,68,80,171,126,67,99,101,0,0 };

static const unsigned char CodeEditor[] = { 110,109,212,43,30,68,208,206,92,67,98,120,21,28,68,208,206,92,67,148,140,26,68,64,239,93,67,108,145,25,68,32,48,96,67,98,72,150,24,68,16,113,98,67,32,46,24,68,224,147,102,67,172,88,24,68,64,150,108,67,108,80,73,25,68,192,212,136,67,98,212,83,25,68,16,
149,137,67,88,89,25,68,136,160,138,67,88,89,25,68,144,246,139,67,98,88,89,25,68,88,121,145,67,184,128,24,68,16,161,149,67,12,208,22,68,56,109,152,67,98,24,31,21,68,208,56,155,67,216,181,18,68,144,20,157,67,84,148,15,68,0,0,158,67,98,176,224,18,68,96,
0,159,67,48,87,21,68,168,214,160,67,32,248,22,68,192,130,163,67,98,196,152,24,68,224,46,166,67,96,105,25,68,0,70,170,67,96,105,25,68,200,200,175,67,98,96,105,25,68,136,94,176,67,152,94,25,68,136,127,177,67,80,73,25,68,168,42,179,67,108,172,88,24,68,80,
180,197,67,98,32,46,24,68,128,160,200,67,196,152,24,68,96,172,202,67,112,153,25,68,80,215,203,67,98,216,153,26,68,208,2,205,67,248,31,28,68,144,152,205,67,212,43,30,68,144,152,205,67,108,212,43,30,68,192,95,219,67,98,160,211,24,68,192,95,219,67,204,249,
20,68,160,233,217,67,212,157,18,68,112,253,214,67,98,216,65,16,68,56,17,212,67,220,19,15,68,184,46,207,67,220,19,15,68,232,85,200,67,98,220,19,15,68,16,128,199,67,160,30,15,68,0,42,198,67,228,51,15,68,192,83,196,67,108,144,36,16,68,88,74,178,67,108,148,
52,16,68,176,169,176,67,98,148,52,16,68,24,210,172,67,132,113,15,68,240,21,170,67,100,235,13,68,72,117,168,67,98,68,101,12,68,160,212,166,67,92,65,10,68,192,3,166,67,0,128,7,68,192,3,166,67,108,0,128,7,68,176,251,149,67,98,160,77,13,68,176,251,149,67,
148,52,16,68,64,111,146,67,148,52,16,68,80,86,139,67,108,144,36,16,68,24,181,137,67,108,228,51,15,68,144,152,111,67,98,160,30,15,68,208,149,107,67,220,19,15,68,192,191,104,67,220,19,15,68,0,20,103,67,98,220,19,15,68,96,98,89,67,152,68,16,68,96,167,79,
67,212,165,18,68,0,229,73,67,98,16,7,21,68,160,33,68,67,104,222,24,68,128,64,65,67,212,43,30,68,128,64,65,67,108,212,43,30,68,208,206,92,67,99,109,108,212,46,68,128,64,65,67,98,92,44,52,68,128,64,65,67,120,6,56,68,176,44,68,67,112,98,58,68,32,5,74,67,
98,36,190,60,68,144,221,79,67,32,236,61,68,144,162,89,67,32,236,61,68,48,84,103,67,98,32,236,61,68,208,255,104,67,92,225,61,68,240,171,107,67,20,204,61,68,128,88,111,67,108,180,219,60,68,24,181,137,67,108,172,203,60,68,80,86,139,67,98,172,203,60,68,64,
111,146,67,92,178,63,68,176,251,149,67,0,128,69,68,176,251,149,67,108,0,128,69,68,192,3,166,67,98,92,178,63,68,192,3,166,67,172,203,60,68,192,144,169,67,172,203,60,68,176,169,176,67,108,180,219,60,68,88,74,178,67,108,20,204,61,68,184,51,196,67,98,92,
225,61,68,128,52,198,67,32,236,61,68,32,160,199,67,32,236,61,68,248,117,200,67,98,32,236,61,68,200,78,207,67,168,187,60,68,192,43,212,67,108,90,58,68,112,13,215,67,98,48,249,55,68,160,238,217,67,224,33,52,68,192,95,219,67,108,212,46,68,192,95,219,67,
108,108,212,46,68,144,152,205,67,98,200,234,48,68,144,152,205,67,172,115,50,68,88,8,205,67,212,110,51,68,88,231,203,67,98,252,105,52,68,232,198,202,67,36,210,52,68,16,182,200,67,80,167,52,68,80,180,197,67,108,240,182,51,68,168,42,179,67,98,40,172,51,
68,96,106,178,67,236,166,51,68,240,94,177,67,236,166,51,68,112,9,176,67,98,236,166,51,68,32,134,170,67,68,127,52,68,240,94,166,67,52,48,54,68,192,146,163,67,98,40,225,55,68,160,198,160,67,164,79,58,68,216,234,158,67,244,123,61,68,0,0,158,67,98,156,47,
58,68,152,255,156,67,24,185,55,68,208,40,155,67,44,24,54,68,64,125,152,67,98,60,119,52,68,32,209,149,67,236,166,51,68,104,185,145,67,236,166,51,68,168,54,140,67,98,236,166,51,68,16,182,138,67,40,172,51,68,16,149,137,67,240,182,51,68,192,212,136,67,108,
80,167,52,68,64,150,108,67,98,36,210,52,68,208,189,102,67,56,103,52,68,64,167,98,67,208,102,51,68,64,80,96,67,98,36,102,50,68,48,249,93,67,4,224,48,68,208,206,92,67,108,212,46,68,208,206,92,67,108,108,212,46,68,128,64,65,67,99,101,0,0 };

static const unsigned char DownloadTask[] = { 110,109,0,128,38,68,186,255,63,67,98,254,155,55,68,186,255,63,67,0,128,69,68,192,143,119,67,0,128,69,68,220,255,157,67,98,0,128,69,68,32,56,192,67,254,155,55,68,35,0,220,67,0,128,38,68,35,0,220,67,98,2,100,21,68,35,0,220,67,0,128,7,68,32,56,192,67,0,
128,7,68,220,255,157,67,98,0,128,7,68,192,143,119,67,2,100,21,68,186,255,63,67,0,128,38,68,186,255,63,67,99,109,178,147,31,68,174,119,105,67,108,178,147,31,68,114,180,159,67,108,72,184,18,68,114,180,159,67,108,0,128,38,68,42,68,199,67,108,184,71,58,68,
114,180,159,67,108,78,108,45,68,114,180,159,67,108,78,108,45,68,174,119,105,67,108,178,147,31,68,174,119,105,67,99,101,0,0 };

static const unsigned char DummyWait[] = { 110,109,0,128,38,68,0,0,64,67,98,0,156,55,68,0,0,64,67,0,128,69,68,248,143,119,67,0,128,69,68,0,0,158,67,98,0,128,69,68,4,56,192,67,0,156,55,68,0,0,220,67,0,128,38,68,0,0,220,67,98,0,100,21,68,0,0,220,67,0,128,7,68,4,56,192,67,0,128,7,68,0,0,158,67,98,
0,128,7,68,248,143,119,67,0,100,21,68,0,0,64,67,0,128,38,68,0,0,64,67,99,109,0,128,38,68,204,57,87,67,98,96,152,24,68,204,57,87,67,116,78,13,68,196,48,130,67,116,78,13,68,0,0,158,67,98,116,78,13,68,58,207,185,67,96,152,24,68,26,99,208,67,0,128,38,68,
26,99,208,67,98,156,103,52,68,26,99,208,67,140,177,63,68,58,207,185,67,140,177,63,68,0,0,158,67,98,140,177,63,68,196,48,130,67,156,103,52,68,204,57,87,67,0,128,38,68,204,57,87,67,99,109,140,40,37,68,152,204,160,67,108,92,254,36,68,152,204,160,67,108,
92,254,36,68,48,46,103,67,108,60,39,42,68,48,46,103,67,108,60,39,42,68,252,43,151,67,108,32,149,55,68,196,7,178,67,108,240,116,50,68,36,72,188,67,108,220,239,36,68,254,61,161,67,108,140,40,37,68,152,204,160,67,99,101,0,0 };

static const unsigned char RelativeFileLoader[] = { 110,109,228,127,38,68,156,255,63,67,98,240,155,55,68,156,255,63,67,252,127,69,68,208,143,119,67,252,127,69,68,0,0,158,67,98,252,127,69,68,24,56,192,67,240,155,55,68,50,0,220,67,228,127,38,68,50,0,220,67,98,216,99,21,68,50,0,220,67,252,127,7,68,24,56,
192,67,252,127,7,68,0,0,158,67,98,252,127,7,68,208,143,119,67,216,99,21,68,156,255,63,67,228,127,38,68,156,255,63,67,99,109,228,127,38,68,60,58,80,67,98,68,161,23,68,60,58,80,67,164,142,11,68,90,66,128,67,164,142,11,68,0,0,158,67,98,164,142,11,68,66,
189,187,67,68,161,23,68,128,226,211,67,228,127,38,68,128,226,211,67,98,184,94,53,68,128,226,211,67,84,113,65,68,66,189,187,67,84,113,65,68,0,0,158,67,98,84,113,65,68,90,66,128,67,184,94,53,68,60,58,80,67,228,127,38,68,60,58,80,67,99,109,228,127,38,68,
8,8,103,67,98,64,57,50,68,8,8,103,67,228,189,59,68,74,141,134,67,228,189,59,68,0,0,158,67,98,228,189,59,68,180,114,181,67,64,57,50,68,252,123,200,67,228,127,38,68,252,123,200,67,98,184,198,26,68,252,123,200,67,232,65,17,68,180,114,181,67,232,65,17,68,
0,0,158,67,98,232,65,17,68,74,141,134,67,184,198,26,68,8,8,103,67,228,127,38,68,8,8,103,67,99,109,36,129,37,68,12,223,194,67,108,36,129,37,68,50,120,173,67,108,160,241,39,68,50,120,173,67,108,160,241,39,68,234,207,194,67,98,144,240,48,68,218,106,193,
67,248,34,56,68,110,26,179,67,52,229,56,68,70,37,161,67,108,24,27,46,68,70,37,161,67,108,24,27,46,68,82,68,156,67,108,28,241,56,68,82,68,156,67,98,220,130,56,68,216,166,137,67,176,44,49,68,116,60,117,67,160,241,39,68,44,96,114,67,108,160,241,39,68,102,
241,143,67,108,36,129,37,68,102,241,143,67,108,36,129,37,68,232,65,114,67,98,96,16,28,68,84,68,116,67,212,126,20,68,16,90,137,67,220,14,20,68,82,68,156,67,108,176,87,31,68,82,68,156,67,108,176,87,31,68,70,37,161,67,108,148,26,20,68,70,37,161,67,98,52,
224,20,68,120,102,179,67,228,76,28,68,228,227,193,67,36,129,37,68,12,223,194,67,99,101,0,0 };

static const unsigned char Skip[] = { 110,109,210,182,53,68,104,104,166,67,108,4,72,60,68,104,104,166,67,108,4,72,60,68,82,108,154,67,108,0,128,69,68,76,220,172,67,108,4,72,60,68,6,76,191,67,108,4,72,60,68,46,80,179,67,108,204,105,46,68,46,80,179,67,108,204,105,46,68,188,11,171,67,108,202,
74,46,68,188,11,171,67,98,202,74,46,68,166,198,153,67,202,74,39,68,166,198,139,67,64,168,30,68,166,198,139,67,98,20,60,22,68,166,198,139,67,116,83,15,68,142,30,153,67,64,8,15,68,120,244,169,67,108,0,128,7,68,204,109,169,67,98,140,239,7,68,112,122,144,
67,192,44,18,68,244,103,121,67,64,168,30,68,244,103,121,67,98,250,171,42,68,244,103,121,67,6,141,52,68,172,253,142,67,210,182,53,68,104,104,166,67,99,109,64,168,30,68,50,57,153,67,98,100,147,35,68,50,57,153,67,134,145,39,68,180,53,161,67,134,145,39,68,
188,11,171,67,98,134,145,39,68,196,225,180,67,100,147,35,68,70,222,188,67,64,168,30,68,70,222,188,67,98,60,189,25,68,70,222,188,67,252,190,21,68,196,225,180,67,252,190,21,68,188,11,171,67,98,252,190,21,68,180,53,161,67,60,189,25,68,50,57,153,67,64,168,
30,68,50,57,153,67,99,101,0,0 };

static const unsigned char HttpRequest[] = { 110,109,8,199,36,68,36,232,219,67,98,200,119,20,68,200,29,218,67,0,128,7,68,32,16,191,67,0,128,7,68,44,0,158,67,98,0,128,7,68,16,241,119,67,200,60,21,68,8,157,64,67,112,55,38,68,96,1,64,67,98,164,79,38,68,248,255,63,67,220,103,38,68,248,255,63,67,20,
128,38,68,248,255,63,67,98,116,138,38,68,248,255,63,67,212,148,38,68,248,255,63,67,56,159,38,68,248,255,63,67,98,152,169,38,68,248,255,63,67,248,179,38,68,248,255,63,67,88,190,38,68,248,255,63,67,98,144,214,38,68,248,255,63,67,200,238,38,68,248,255,63,
67,0,7,39,68,96,1,64,67,98,208,6,39,68,16,2,64,67,120,6,39,68,112,3,64,67,76,6,39,68,32,4,64,67,98,140,228,39,68,96,19,64,67,96,192,40,68,144,70,64,67,204,153,41,68,8,157,64,67,98,156,65,57,68,184,215,70,67,0,128,69,68,40,191,123,67,0,128,69,68,44,0,
158,67,98,0,128,69,68,4,232,190,67,172,167,56,68,16,220,217,67,72,116,40,68,52,225,219,67,98,80,117,40,68,76,227,219,67,92,118,40,68,180,229,219,67,100,119,40,68,36,232,219,67,98,104,229,39,68,16,248,219,67,104,82,39,68,4,0,220,67,88,190,38,68,4,0,220,
67,98,248,179,38,68,4,0,220,67,152,169,38,68,4,0,220,67,56,159,38,68,4,0,220,67,98,212,148,38,68,4,0,220,67,116,138,38,68,4,0,220,67,20,128,38,68,4,0,220,67,98,4,236,37,68,4,0,220,67,4,89,37,68,16,248,219,67,8,199,36,68,36,232,219,67,99,109,236,163,18,
68,208,99,125,67,108,80,152,28,68,208,99,125,67,98,212,111,29,68,144,4,113,67,228,136,30,68,248,73,101,67,172,218,31,68,168,95,90,67,98,172,142,26,68,168,34,96,67,20,243,21,68,248,158,108,67,236,163,18,68,208,99,125,67,99,109,164,75,13,68,104,205,153,
67,108,220,170,26,68,104,205,153,67,98,60,190,26,68,232,132,146,67,24,40,27,68,204,117,139,67,12,222,27,68,112,186,132,67,108,132,151,16,68,112,186,132,67,98,100,197,14,68,20,6,139,67,16,156,13,68,176,44,146,67,164,75,13,68,104,205,153,67,99,109,12,75,
17,68,44,142,185,67,108,48,144,28,68,44,142,185,67,98,72,169,27,68,56,222,178,67,44,15,27,68,112,206,171,67,196,204,26,68,52,123,164,67,108,108,106,13,68,52,123,164,67,98,176,232,13,68,232,45,172,67,220,68,15,68,240,86,179,67,12,75,17,68,44,142,185,67,
99,109,76,227,32,68,84,84,207,67,98,12,142,31,68,180,110,202,67,92,104,30,68,160,41,197,67,88,121,29,68,12,151,191,67,108,212,152,19,68,12,151,191,67,98,196,10,23,68,56,86,199,67,152,168,27,68,228,244,204,67,76,227,32,68,84,84,207,67,99,109,40,103,57,
68,12,151,191,67,108,20,197,47,68,12,151,191,67,98,32,216,46,68,44,29,197,67,144,181,45,68,216,86,202,67,28,100,44,68,28,51,207,67,98,108,129,49,68,84,193,204,67,248,4,54,68,232,50,199,67,40,103,57,68,12,151,191,67,99,109,188,149,63,68,52,123,164,67,
108,168,113,50,68,52,123,164,67,98,64,47,50,68,112,206,171,67,36,149,49,68,56,222,178,67,60,174,48,68,44,142,185,67,108,240,180,59,68,44,142,185,67,98,32,187,61,68,240,86,179,67,76,23,63,68,232,45,172,67,188,149,63,68,52,123,164,67,99,109,124,104,60,
68,112,186,132,67,108,100,96,49,68,112,186,132,67,98,88,22,50,68,204,117,139,67,52,128,50,68,232,132,146,67,144,147,50,68,104,205,153,67,108,88,180,63,68,104,205,153,67,98,24,100,63,68,176,44,146,67,152,58,62,68,20,6,139,67,124,104,60,68,112,186,132,
67,99,109,112,109,45,68,232,175,90,67,98,232,186,46,68,104,131,101,67,172,208,47,68,0,35,113,67,28,166,48,68,208,99,125,67,108,60,92,58,68,208,99,125,67,98,244,27,55,68,8,235,108,67,44,155,50,68,208,148,96,67,112,109,45,68,232,175,90,67,99,109,56,227,
45,68,208,99,125,67,98,36,238,44,68,192,246,111,67,240,167,43,68,104,93,99,67,228,28,42,68,240,220,87,67,98,188,238,40,68,248,47,87,67,0,186,39,68,184,214,86,67,20,128,38,68,184,214,86,67,98,84,93,37,68,184,214,86,67,24,63,36,68,136,35,87,67,140,38,35,
68,64,184,87,67,98,64,153,33,68,104,66,99,67,84,81,32,68,56,232,111,67,52,91,31,68,208,99,125,67,108,56,227,45,68,208,99,125,67,99,109,156,87,29,68,104,205,153,67,108,208,230,47,68,104,205,153,67,98,148,210,47,68,120,130,146,67,16,100,47,68,168,113,139,
67,84,166,46,68,112,186,132,67,108,24,152,30,68,112,186,132,67,98,92,218,29,68,168,113,139,67,216,107,29,68,120,130,146,67,156,87,29,68,104,205,153,67,99,109,140,82,31,68,44,142,185,67,108,224,235,45,68,44,142,185,67,98,248,220,46,68,116,228,178,67,44,
126,47,68,240,210,171,67,136,195,47,68,52,123,164,67,108,228,122,29,68,52,123,164,67,98,64,192,29,68,240,210,171,67,116,97,30,68,116,228,178,67,140,82,31,68,44,142,185,67,99,109,132,56,36,68,192,96,208,67,98,172,248,36,68,88,131,208,67,64,187,37,68,252,
148,208,67,20,128,38,68,252,148,208,67,98,56,91,39,68,252,148,208,67,28,52,40,68,216,126,208,67,184,9,41,68,76,84,208,67,98,116,149,42,68,164,54,203,67,196,231,43,68,0,153,197,67,32,247,44,68,12,151,191,67,108,76,71,32,68,12,151,191,67,98,136,87,33,68,
220,157,197,67,56,171,34,68,0,64,203,67,132,56,36,68,192,96,208,67,99,101,0,0 };

static const unsigned char AppDataFileWriter[] = { 110,109,158,99,29,68,224,199,127,67,108,80,140,41,68,54,238,141,67,98,114,235,47,68,174,73,149,67,50,27,50,68,130,155,165,67,116,109,46,68,198,89,178,67,98,160,191,42,68,10,24,191,67,208,150,34,68,136,119,195,67,148,55,28,68,14,28,188,67,108,224,14,16,
68,200,17,174,67,98,190,175,9,68,80,182,166,67,0,128,7,68,126,100,150,67,188,45,11,68,58,166,137,67,98,146,219,14,68,128,207,121,67,98,4,23,68,236,16,113,67,158,99,29,68,224,199,127,67,99,109,120,141,26,68,38,183,137,67,98,106,228,22,68,80,125,133,67,
120,52,18,68,102,0,136,67,112,23,16,68,82,82,143,67,98,108,250,13,68,108,164,150,67,248,59,15,68,134,4,160,67,6,229,18,68,94,62,164,67,108,186,13,31,68,164,72,178,67,98,200,182,34,68,176,130,182,67,186,102,39,68,152,255,179,67,192,131,41,68,124,173,172,
67,98,198,160,43,68,94,91,165,67,58,95,42,68,122,251,155,67,44,182,38,68,108,193,151,67,108,120,141,26,68,38,183,137,67,99,109,238,122,34,68,232,163,149,67,98,92,69,34,68,150,81,155,67,220,168,35,68,168,2,161,67,238,73,38,68,200,11,164,67,108,182,232,
40,68,134,18,167,67,98,32,204,40,68,136,236,168,67,36,128,40,68,244,194,170,67,98,1,40,68,128,122,172,67,98,60,126,39,68,200,64,174,67,118,209,38,68,104,185,175,67,50,10,38,68,250,219,176,67,108,200,115,35,68,50,223,173,67,98,252,18,30,68,126,169,167,
67,144,173,27,68,66,17,155,67,242,69,29,68,150,160,143,67,108,238,122,34,68,232,163,149,67,99,109,212,189,33,68,212,8,130,67,98,240,231,37,68,136,162,118,67,2,224,43,68,232,12,116,67,108,200,48,68,180,98,127,67,108,32,241,60,68,110,187,141,67,98,92,80,
67,68,24,23,149,67,0,128,69,68,184,104,165,67,66,210,65,68,46,39,178,67,98,134,36,62,68,116,229,190,67,158,251,53,68,242,68,195,67,124,156,47,68,120,233,187,67,108,0,237,45,68,44,247,185,67,98,108,189,46,68,46,72,184,67,12,119,47,68,214,95,182,67,176,
19,48,68,82,65,180,67,98,24,112,48,68,120,1,179,67,42,191,48,68,114,185,177,67,124,1,49,68,218,107,176,67,108,160,114,50,68,14,22,178,67,98,150,27,54,68,26,80,182,67,162,203,58,68,2,205,179,67,168,232,60,68,230,122,172,67,98,148,5,63,68,200,40,165,67,
34,196,61,68,226,200,155,67,20,27,58,68,214,142,151,67,108,96,242,45,68,144,132,137,67,98,162,219,43,68,42,27,135,67,86,111,41,68,102,227,134,67,176,88,39,68,168,129,136,67,108,212,189,33,68,212,8,130,67,99,101,0,0 };

static const unsigned char HlacDecoder[] = { 110,109,228,9,35,68,216,234,165,67,108,228,9,35,68,168,31,223,67,108,100,134,7,68,72,27,194,67,108,100,134,7,68,128,230,136,67,108,228,9,35,68,216,234,165,67,99,109,64,252,41,68,216,234,165,67,108,0,128,69,68,128,230,136,67,108,0,128,69,68,152,167,196,
67,108,64,252,41,68,120,172,225,67,108,64,252,41,68,216,234,165,67,99,109,64,222,37,68,32,167,52,67,108,200,60,68,68,240,164,114,67,108,64,222,37,68,104,81,152,67,108,0,128,7,68,240,164,114,67,108,64,222,37,68,32,167,52,67,99,101,0,0 };

static const unsigned char HiseActivator[] = { 110,109,16,133,38,68,128,182,39,67,98,48,106,54,68,64,59,83,67,96,72,69,68,128,20,36,67,96,72,69,68,128,20,36,67,98,96,72,69,68,128,20,36,67,0,128,69,68,64,209,145,67,96,72,69,68,224,107,173,67,98,224,21,69,68,96,144,199,67,64,123,41,68,192,174,230,67,
16,133,38,68,192,235,233,67,108,16,133,38,68,224,245,233,67,108,0,128,38,68,192,235,233,67,108,240,122,38,68,224,245,233,67,108,240,122,38,68,192,235,233,67,98,192,137,35,68,192,174,230,67,32,234,7,68,96,144,199,67,144,183,7,68,224,107,173,67,98,0,128,
7,68,64,209,145,67,144,183,7,68,128,20,36,67,144,183,7,68,128,20,36,67,98,144,183,7,68,128,20,36,67,224,154,22,68,64,59,83,67,240,122,38,68,128,182,39,67,108,240,122,38,68,64,162,39,67,108,0,128,38,68,128,182,39,67,108,16,133,38,68,64,162,39,67,108,16,
133,38,68,128,182,39,67,99,109,112,102,35,68,64,56,164,67,108,240,16,24,68,96,141,141,67,108,176,4,20,68,224,165,149,67,108,80,92,35,68,32,95,180,67,108,112,102,35,68,224,64,180,67,108,128,117,35,68,32,95,180,67,108,160,165,59,68,224,254,131,67,108,96,
148,55,68,128,184,119,67,108,112,102,35,68,64,56,164,67,99,101,0,0 };


static const unsigned char UnzipTask[] = { 110,109,240,160,44,68,40,158,33,67,108,208,154,68,68,216,194,128,67,98,136,45,69,68,72,232,129,67,0,128,69,68,132,118,131,67,0,128,69,68,172,21,133,67,108,0,128,69,68,20,222,230,67,98,0,128,69,68,148,62,234,67,120,33,68,68,68,251,236,67,56,113,66,68,
68,251,236,67,108,152,142,10,68,68,251,236,67,98,88,222,8,68,68,251,236,67,0,128,7,68,148,62,234,67,0,128,7,68,20,222,230,67,108,0,128,7,68,152,68,42,67,98,0,128,7,68,216,130,35,67,88,222,8,68,112,9,30,67,152,142,10,68,112,9,30,67,108,136,119,42,68,112,
9,30,67,98,28,71,43,68,112,9,30,67,60,14,44,68,80,83,31,67,240,160,44,68,40,158,33,67,99,109,204,152,60,68,120,10,130,67,108,80,250,43,68,192,155,65,67,108,80,250,43,68,120,10,130,67,108,204,152,60,68,120,10,130,67,99,109,176,195,13,68,68,108,217,67,
108,176,195,13,68,140,132,223,67,108,184,75,32,68,140,132,223,67,108,184,75,32,68,92,211,216,67,108,120,193,18,68,92,211,216,67,108,184,75,32,68,152,67,184,67,108,184,75,32,68,196,78,177,67,108,176,195,13,68,196,78,177,67,108,176,195,13,68,108,230,183,
67,108,120,125,27,68,108,230,183,67,108,176,195,13,68,68,108,217,67,99,109,136,164,40,68,196,78,177,67,108,204,169,36,68,196,78,177,67,108,204,169,36,68,140,132,223,67,108,136,164,40,68,140,132,223,67,108,136,164,40,68,196,78,177,67,99,109,180,156,46,
68,196,78,177,67,108,180,156,46,68,140,132,223,67,108,108,151,50,68,140,132,223,67,108,108,151,50,68,0,239,204,67,108,108,235,55,68,0,239,204,67,98,128,31,57,68,0,239,204,67,212,48,58,68,180,138,204,67,148,31,59,68,20,194,203,67,98,136,14,60,68,116,249,
202,67,232,215,60,68,212,239,201,67,232,123,61,68,212,164,200,67,98,24,32,62,68,56,90,199,67,112,156,62,68,128,223,197,67,88,241,62,68,180,52,196,67,98,68,70,63,68,232,137,194,67,184,112,63,68,244,212,192,67,184,112,63,68,160,22,191,67,98,184,112,63,
68,116,110,189,67,68,70,63,68,76,201,187,67,88,241,62,68,192,38,186,67,98,112,156,62,68,156,132,184,67,212,32,62,68,100,11,183,67,40,126,61,68,192,186,181,67,98,120,219,60,68,128,106,180,67,216,18,60,68,192,89,179,67,228,35,59,68,124,136,178,67,98,36,
53,58,68,56,183,177,67,80,34,57,68,196,78,177,67,108,235,55,68,196,78,177,67,108,180,156,46,68,196,78,177,67,99,109,108,151,50,68,252,95,198,67,108,108,151,50,68,108,230,183,67,108,148,19,55,68,108,230,183,67,98,216,200,55,68,108,230,183,67,120,105,56,
68,128,19,184,67,12,245,56,68,16,110,184,67,98,156,128,57,68,152,200,184,67,216,245,57,68,20,70,185,67,180,84,58,68,64,231,185,67,98,192,179,58,68,112,136,186,67,180,251,58,68,200,72,187,67,140,44,59,68,248,39,188,67,98,144,93,59,68,36,7,189,67,252,117,
59,68,24,249,189,67,252,117,59,68,20,253,190,67,98,252,117,59,68,176,6,192,67,164,98,59,68,236,253,192,67,192,59,59,68,24,227,193,67,98,224,20,59,68,228,199,194,67,144,213,58,68,100,143,195,67,216,125,58,68,212,56,196,67,98,236,37,58,68,172,226,196,67,
244,178,57,68,200,104,197,67,96,36,57,68,152,203,197,67,98,252,149,56,68,200,46,198,67,200,229,55,68,252,95,198,67,148,19,55,68,252,95,198,67,108,108,151,50,68,252,95,198,67,99,109,160,98,63,68,220,162,167,67,108,160,98,63,68,160,54,139,67,108,72,175,
41,68,160,54,139,67,98,56,107,40,68,160,54,139,67,60,100,39,68,252,40,137,67,60,100,39,68,140,160,134,67,108,60,100,39,68,232,122,56,67,98,60,100,39,68,144,204,55,67,32,105,39,68,192,34,55,67,36,114,39,68,232,126,54,67,108,96,157,13,68,232,126,54,67,
108,96,157,13,68,220,162,167,67,108,160,98,63,68,220,162,167,67,99,101,0,0 };

static const unsigned char Launch[] = { 110,109,204,254,59,68,44,63,184,67,108,204,254,59,68,192,64,201,67,98,204,254,59,68,140,179,210,67,213,40,56,68,120,95,218,67,113,111,51,68,120,95,218,67,108,92,15,16,68,120,95,218,67,98,247,85,11,68,120,95,218,67,0,128,7,68,140,179,210,67,0,128,7,68,
192,64,201,67,108,0,128,7,68,204,175,130,67,98,0,128,7,68,8,122,114,67,247,85,11,68,44,34,99,67,92,15,16,68,44,34,99,67,108,161,74,25,68,44,34,99,67,108,161,74,25,68,88,42,130,67,108,92,15,16,68,88,42,130,67,98,127,234,15,68,88,42,130,67,162,204,15,68,
20,102,130,67,162,204,15,68,204,175,130,67,108,162,204,15,68,192,64,201,67,98,162,204,15,68,48,138,201,67,127,234,15,68,236,197,201,67,92,15,16,68,236,197,201,67,108,113,111,51,68,236,197,201,67,98,40,148,51,68,236,197,201,67,43,178,51,68,48,138,201,
67,43,178,51,68,192,64,201,67,108,43,178,51,68,44,63,184,67,108,204,254,59,68,44,63,184,67,99,109,118,164,44,68,48,84,159,67,98,148,158,39,68,0,127,161,67,95,249,30,68,220,140,167,67,190,132,26,68,180,45,184,67,98,190,132,26,68,180,45,184,67,29,102,28,
68,0,164,121,67,10,97,42,68,56,210,120,67,98,214,50,43,68,96,197,120,67,150,243,43,68,248,197,120,67,118,164,44,68,160,209,120,67,108,118,164,44,68,20,65,67,67,108,0,128,69,68,188,3,138,67,108,118,164,44,68,236,102,178,67,108,118,164,44,68,48,84,159,
67,99,101,0,0 };

static const unsigned char LambdaTask[] = { 110,109,0,128,7,68,248,66,148,67,108,0,128,7,68,64,90,118,67,98,0,128,7,68,48,15,91,67,48,10,13,68,144,229,68,67,40,221,19,68,144,229,68,67,108,124,161,33,68,144,229,68,67,108,124,161,33,68,96,34,110,67,108,148,8,19,68,96,34,110,67,108,148,8,19,68,248,
66,148,67,108,0,128,7,68,248,66,148,67,99,109,132,94,43,68,144,229,68,67,108,212,34,57,68,144,229,68,67,98,208,245,63,68,144,229,68,67,0,128,69,68,48,15,91,67,0,128,69,68,64,90,118,67,108,0,128,69,68,248,66,148,67,108,104,247,57,68,248,66,148,67,108,
104,247,57,68,96,34,110,67,108,132,94,43,68,96,34,110,67,108,132,94,43,68,144,229,68,67,99,109,0,128,69,68,8,189,167,67,108,0,128,69,68,224,210,192,67,98,0,128,69,68,208,120,206,67,208,245,63,68,48,141,217,67,212,34,57,68,48,141,217,67,108,132,94,43,
68,48,141,217,67,108,132,94,43,68,208,238,196,67,108,104,247,57,68,208,238,196,67,108,104,247,57,68,8,189,167,67,108,0,128,69,68,8,189,167,67,99,109,124,161,33,68,48,141,217,67,108,40,221,19,68,48,141,217,67,98,48,10,13,68,48,141,217,67,0,128,7,68,208,
120,206,67,0,128,7,68,224,210,192,67,108,0,128,7,68,8,189,167,67,108,148,8,19,68,8,189,167,67,108,148,8,19,68,208,238,196,67,108,124,161,33,68,208,238,196,67,108,124,161,33,68,48,141,217,67,99,101,0,0 };

static const unsigned char Spacer[] = { 110,109,38,34,32,68,120,121,166,67,108,38,34,32,68,192,186,180,67,108,32,43,21,68,56,204,158,67,108,38,34,32,68,40,222,136,67,108,38,34,32,68,112,31,151,67,108,51,141,45,68,112,31,151,67,108,51,141,45,68,40,222,136,67,108,120,132,56,68,56,204,158,67,
108,51,141,45,68,192,186,180,67,108,51,141,45,68,120,121,166,67,108,38,34,32,68,120,121,166,67,99,109,28,153,15,68,80,216,189,67,108,0,128,7,68,80,216,189,67,108,0,128,7,68,96,79,124,67,108,28,153,15,68,96,79,124,67,108,28,153,15,68,80,216,189,67,99,
109,0,128,69,68,80,216,189,67,108,34,103,61,68,80,216,189,67,108,34,103,61,68,96,79,124,67,108,0,128,69,68,96,79,124,67,108,0,128,69,68,80,216,189,67,99,101,0,0 };

static const unsigned char PluginDirectories[] = { 110,109,123,123,40,68,60,36,65,67,108,123,123,40,68,52,214,76,67,98,123,123,40,68,136,255,93,67,231,255,36,68,220,237,107,67,146,181,32,68,220,237,107,67,98,61,107,28,68,220,237,107,67,168,239,24,68,136,255,93,67,168,239,24,68,52,214,76,67,108,168,239,
24,68,60,36,65,67,108,0,128,7,68,60,36,65,67,108,0,128,7,68,134,77,132,67,108,44,0,12,68,134,77,132,67,98,94,116,16,68,134,77,132,67,14,18,20,68,52,137,139,67,14,18,20,68,138,113,148,67,98,14,18,20,68,222,89,157,67,94,116,16,68,240,148,164,67,44,0,12,
68,240,148,164,67,108,0,128,7,68,240,148,164,67,108,0,128,7,68,148,190,192,67,108,32,118,24,68,148,190,192,67,108,32,118,24,68,238,238,202,67,98,32,118,24,68,150,9,212,67,62,40,28,68,224,109,219,67,146,181,32,68,224,109,219,67,98,230,66,37,68,224,109,
219,67,12,245,40,68,150,9,212,67,12,245,40,68,238,238,202,67,108,12,245,40,68,148,190,192,67,108,58,150,55,68,148,190,192,67,108,58,150,55,68,126,240,164,67,108,228,3,61,68,126,240,164,67,98,223,178,65,68,126,240,164,67,0,128,69,68,160,85,157,67,0,128,
69,68,170,247,147,67,98,0,128,69,68,82,154,138,67,223,178,65,68,114,255,130,67,228,3,61,68,114,255,130,67,108,58,150,55,68,114,255,130,67,108,58,150,55,68,60,36,65,67,108,123,123,40,68,60,36,65,67,99,101,0,0 };


static const unsigned char CopyAsset[] = { 110,109,104,71,48,68,48,191,113,67,108,0,128,7,68,48,191,113,67,108,0,128,7,68,212,145,227,67,98,0,128,7,68,80,23,233,67,10,189,9,68,154,145,237,67,227,127,12,68,154,145,237,67,108,104,71,48,68,154,145,237,67,108,104,71,48,68,48,191,113,67,99,109,152,
184,28,68,208,88,73,67,108,189,96,58,68,208,88,73,67,108,189,96,58,68,106,32,195,67,108,0,128,69,68,106,32,195,67,108,0,128,69,68,68,216,49,67,98,0,128,69,68,48,71,44,67,126,242,68,68,36,241,38,67,148,246,67,68,120,1,35,67,98,236,250,66,68,216,18,31,
67,104,165,65,68,208,220,28,67,35,65,64,68,208,220,28,67,98,18,62,53,68,208,220,28,67,152,184,28,68,208,220,28,67,152,184,28,68,208,220,28,67,108,152,184,28,68,208,88,73,67,99,101,0,0 };

static const unsigned char FileLogger[] = { 110,109,0,128,7,68,204,103,173,67,108,198,112,13,68,204,103,173,67,108,198,112,13,68,96,63,104,67,98,198,112,13,68,24,48,97,67,106,223,14,68,0,118,91,67,11,163,16,68,0,118,91,67,108,154,107,21,68,0,118,91,67,108,154,107,21,68,8,151,116,67,108,130,219,
27,68,8,151,116,67,108,130,219,27,68,0,118,91,67,108,72,84,35,68,0,118,91,67,108,72,84,35,68,8,151,116,67,108,48,196,41,68,8,151,116,67,108,48,196,41,68,0,118,91,67,108,0,61,49,68,0,118,91,67,108,0,61,49,68,8,151,116,67,108,223,172,55,68,8,151,116,67,
108,223,172,55,68,0,118,91,67,108,245,92,60,68,0,118,91,67,98,150,32,62,68,0,118,91,67,57,143,63,68,24,48,97,67,57,143,63,68,96,63,104,67,108,57,143,63,68,210,185,212,67,98,57,143,63,68,176,64,216,67,150,32,62,68,190,29,219,67,245,92,60,68,190,29,219,
67,108,11,163,16,68,190,29,219,67,98,106,223,14,68,190,29,219,67,198,112,13,68,176,64,216,67,198,112,13,68,210,185,212,67,108,198,112,13,68,192,153,173,67,108,0,128,7,68,192,153,173,67,108,0,128,7,68,210,185,212,67,98,0,128,7,68,166,207,222,67,24,152,
11,68,212,255,230,67,11,163,16,68,212,255,230,67,98,11,163,16,68,212,255,230,67,245,92,60,68,212,255,230,67,245,92,60,68,212,255,230,67,98,231,103,65,68,212,255,230,67,0,128,69,68,166,207,222,67,0,128,69,68,210,185,212,67,98,0,128,69,68,210,185,212,67,
0,128,69,68,96,63,104,67,0,128,69,68,96,63,104,67,98,0,128,69,68,188,19,84,67,231,103,65,68,92,179,67,67,245,92,60,68,92,179,67,67,108,223,172,55,68,92,179,67,67,108,223,172,55,68,84,0,42,67,108,0,61,49,68,84,0,42,67,108,0,61,49,68,92,179,67,67,108,48,
196,41,68,92,179,67,67,108,48,196,41,68,84,0,42,67,108,72,84,35,68,84,0,42,67,108,72,84,35,68,92,179,67,67,108,130,219,27,68,92,179,67,67,108,130,219,27,68,84,0,42,67,108,154,107,21,68,84,0,42,67,108,154,107,21,68,92,179,67,67,108,11,163,16,68,92,179,
67,67,98,24,152,11,68,92,179,67,67,0,128,7,68,188,19,84,67,0,128,7,68,96,63,104,67,108,0,128,7,68,204,103,173,67,99,109,176,74,58,68,140,144,204,67,108,176,74,58,68,44,1,189,67,108,137,156,18,68,44,1,189,67,108,137,156,18,68,140,144,204,67,108,176,74,
58,68,140,144,204,67,99,109,176,74,58,68,210,14,178,67,108,176,74,58,68,114,127,162,67,108,137,156,18,68,114,127,162,67,108,137,156,18,68,210,14,178,67,108,176,74,58,68,210,14,178,67,99,109,176,74,58,68,136,206,150,67,108,176,74,58,68,38,63,135,67,108,
137,156,18,68,38,63,135,67,108,137,156,18,68,136,206,150,67,108,176,74,58,68,136,206,150,67,99,101,0,0 };

static const unsigned char ClipboardLoader[] = { 110,109,0,128,7,68,204,103,173,67,108,198,112,13,68,204,103,173,67,108,198,112,13,68,96,63,104,67,98,198,112,13,68,24,48,97,67,106,223,14,68,0,118,91,67,11,163,16,68,0,118,91,67,108,154,107,21,68,0,118,91,67,108,154,107,21,68,8,151,116,67,108,130,219,
27,68,8,151,116,67,108,130,219,27,68,0,118,91,67,108,72,84,35,68,0,118,91,67,108,72,84,35,68,8,151,116,67,108,48,196,41,68,8,151,116,67,108,48,196,41,68,0,118,91,67,108,0,61,49,68,0,118,91,67,108,0,61,49,68,8,151,116,67,108,223,172,55,68,8,151,116,67,
108,223,172,55,68,0,118,91,67,108,245,92,60,68,0,118,91,67,98,150,32,62,68,0,118,91,67,57,143,63,68,24,48,97,67,57,143,63,68,96,63,104,67,108,57,143,63,68,210,185,212,67,98,57,143,63,68,176,64,216,67,150,32,62,68,190,29,219,67,245,92,60,68,190,29,219,
67,108,11,163,16,68,190,29,219,67,98,106,223,14,68,190,29,219,67,198,112,13,68,176,64,216,67,198,112,13,68,210,185,212,67,108,198,112,13,68,192,153,173,67,108,0,128,7,68,192,153,173,67,108,0,128,7,68,210,185,212,67,98,0,128,7,68,166,207,222,67,24,152,
11,68,212,255,230,67,11,163,16,68,212,255,230,67,98,11,163,16,68,212,255,230,67,245,92,60,68,212,255,230,67,245,92,60,68,212,255,230,67,98,231,103,65,68,212,255,230,67,0,128,69,68,166,207,222,67,0,128,69,68,210,185,212,67,98,0,128,69,68,210,185,212,67,
0,128,69,68,96,63,104,67,0,128,69,68,96,63,104,67,98,0,128,69,68,188,19,84,67,231,103,65,68,92,179,67,67,245,92,60,68,92,179,67,67,108,223,172,55,68,92,179,67,67,108,223,172,55,68,84,0,42,67,108,0,61,49,68,84,0,42,67,108,0,61,49,68,92,179,67,67,108,48,
196,41,68,92,179,67,67,108,48,196,41,68,84,0,42,67,108,72,84,35,68,84,0,42,67,108,72,84,35,68,92,179,67,67,108,130,219,27,68,92,179,67,67,108,130,219,27,68,84,0,42,67,108,154,107,21,68,84,0,42,67,108,154,107,21,68,92,179,67,67,108,11,163,16,68,92,179,
67,67,98,24,152,11,68,92,179,67,67,0,128,7,68,188,19,84,67,0,128,7,68,96,63,104,67,108,0,128,7,68,204,103,173,67,99,109,176,74,58,68,140,144,204,67,108,176,74,58,68,44,1,189,67,108,137,156,18,68,44,1,189,67,108,137,156,18,68,140,144,204,67,108,176,74,
58,68,140,144,204,67,99,109,176,74,58,68,210,14,178,67,108,176,74,58,68,114,127,162,67,108,137,156,18,68,114,127,162,67,108,137,156,18,68,210,14,178,67,108,176,74,58,68,210,14,178,67,99,109,176,74,58,68,136,206,150,67,108,176,74,58,68,38,63,135,67,108,
137,156,18,68,38,63,135,67,108,137,156,18,68,136,206,150,67,108,176,74,58,68,136,206,150,67,99,101,0,0 };

static const unsigned char OperatingSystem[] = { 110,109,148,176,35,68,180,70,188,67,108,192,125,10,68,180,70,188,67,98,192,214,8,68,180,70,188,67,0,128,7,68,53,153,185,67,0,128,7,68,53,75,182,67,108,0,128,7,68,59,179,95,67,98,0,128,7,68,137,23,89,67,192,214,8,68,59,188,83,67,192,125,10,68,59,188,83,
67,108,12,127,34,68,59,188,83,67,108,12,127,34,68,58,170,107,67,108,128,123,13,68,58,170,107,67,98,128,123,13,68,58,170,107,67,128,123,13,68,181,79,176,67,128,123,13,68,181,79,176,67,98,128,123,13,68,181,79,176,67,129,132,63,68,181,79,176,67,129,132,
63,68,181,79,176,67,108,129,132,63,68,58,170,107,67,108,86,165,34,68,58,170,107,67,108,86,165,34,68,59,188,83,67,108,64,130,66,68,59,188,83,67,98,65,41,68,68,59,188,83,67,0,128,69,68,137,23,89,67,0,128,69,68,59,179,95,67,108,0,128,69,68,53,75,182,67,
98,0,128,69,68,53,153,185,67,65,41,68,68,180,70,188,67,64,130,66,68,180,70,188,67,108,6,95,43,68,180,70,188,67,108,6,95,43,68,115,185,200,67,108,29,66,55,68,115,185,200,67,98,36,179,55,68,115,185,200,67,185,14,56,68,236,112,201,67,185,14,56,68,249,82,
202,67,108,185,14,56,68,93,136,208,67,98,185,14,56,68,106,106,209,67,36,179,55,68,227,33,210,67,29,66,55,68,227,33,210,67,108,126,205,23,68,227,33,210,67,98,119,92,23,68,227,33,210,67,186,0,23,68,106,106,209,67,186,0,23,68,93,136,208,67,108,186,0,23,
68,249,82,202,67,98,186,0,23,68,236,112,201,67,119,92,23,68,115,185,200,67,126,205,23,68,115,185,200,67,108,148,176,35,68,115,185,200,67,108,148,176,35,68,180,70,188,67,99,109,128,29,41,68,41,233,153,67,108,88,209,44,68,196,48,153,67,98,120,10,45,68,
244,172,155,67,24,126,45,68,94,128,157,67,136,44,46,68,180,170,158,67,98,248,218,46,68,88,213,159,67,92,198,47,68,131,106,160,67,142,238,48,68,131,106,160,67,98,92,40,50,68,131,106,160,67,210,20,51,68,148,229,159,67,202,179,51,68,82,220,158,67,98,234,
82,52,68,194,210,157,67,102,162,52,68,44,156,156,67,102,162,52,68,221,56,155,67,98,102,162,52,68,172,84,154,67,4,129,52,68,122,146,153,67,22,62,52,68,72,242,152,67,98,41,251,51,68,22,82,152,67,78,134,51,68,184,198,151,67,134,223,50,68,47,80,151,67,98,
110,109,50,68,40,1,151,67,97,105,49,68,223,116,150,67,136,211,47,68,5,171,149,67,98,74,201,45,68,50,168,148,67,245,90,44,68,236,105,147,67,56,136,43,68,135,240,145,67,98,8,96,42,68,135,221,143,67,240,203,41,68,65,86,141,67,240,203,41,68,182,90,138,67,
98,240,203,41,68,48,111,136,67,158,17,42,68,134,163,134,67,252,156,42,68,168,247,132,67,98,50,40,43,68,208,75,131,67,250,240,43,68,249,5,130,67,44,247,44,68,41,38,129,67,98,93,253,45,68,96,70,128,67,236,57,47,68,233,172,127,67,179,172,48,68,233,172,127,
67,98,104,10,51,68,233,172,127,67,46,210,52,68,244,223,128,67,84,4,54,68,245,242,130,67,98,82,54,55,68,238,5,133,67,34,215,55,68,165,202,135,67,114,230,55,68,20,65,139,67,108,66,24,52,68,168,150,139,67,98,154,238,51,68,194,166,137,67,42,149,51,68,58,
66,136,67,240,11,51,68,0,105,135,67,98,222,130,50,68,197,143,134,67,48,181,49,68,40,35,134,67,230,162,48,68,40,35,134,67,98,224,135,47,68,40,35,134,67,68,170,46,68,117,151,134,67,19,10,46,68,8,128,135,67,98,218,162,45,68,58,21,136,67,100,111,45,68,232,
220,136,67,100,111,45,68,8,215,137,67,98,100,111,45,68,58,187,138,67,162,159,45,68,133,126,139,67,30,0,46,68,228,32,140,67,98,25,123,46,68,31,239,140,67,111,165,47,68,44,198,141,67,111,127,49,68,246,165,142,67,98,72,89,51,68,198,133,143,67,215,183,52,
68,62,109,144,67,246,154,53,68,103,92,145,67,98,22,126,54,68,152,75,146,67,188,47,55,68,129,146,147,67,20,176,55,68,29,49,149,67,98,106,48,56,68,216,207,150,67,150,112,56,68,72,208,152,67,150,112,56,68,32,50,155,67,98,150,112,56,68,0,91,157,67,220,35,
56,68,243,96,159,67,64,138,55,68,173,67,161,67,98,164,240,54,68,104,38,163,67,121,23,54,68,22,141,164,67,152,254,52,68,4,120,165,67,98,184,229,51,68,165,98,166,67,198,135,50,68,244,215,166,67,192,228,48,68,244,215,166,67,98,194,130,46,68,244,215,166,
67,70,174,44,68,41,190,165,67,76,103,43,68,67,138,163,67,98,121,32,42,68,93,86,161,67,53,93,41,68,143,32,158,67,128,29,41,68,41,233,153,67,99,109,224,211,20,68,58,143,147,67,98,224,211,20,68,64,184,143,67,99,29,21,68,60,127,140,67,105,176,21,68,52,228,
137,67,98,56,30,22,68,182,248,135,67,217,179,22,68,180,63,134,67,193,113,23,68,36,185,132,67,98,130,47,24,68,148,50,131,67,84,255,24,68,240,16,130,67,98,225,25,68,66,84,129,67,98,4,14,27,68,185,85,128,67,191,104,28,68,233,172,127,67,108,241,29,68,233,
172,127,67,98,88,184,32,68,233,172,127,67,36,241,34,68,127,143,129,67,248,155,36,68,140,1,133,67,98,166,70,38,68,154,115,136,67,252,27,39,68,30,62,141,67,252,27,39,68,33,97,147,67,98,252,27,39,68,233,118,153,67,84,72,38,68,173,57,158,67,224,160,36,68,
174,169,161,67,98,106,249,34,68,173,25,165,67,95,195,32,68,134,209,166,67,152,254,29,68,134,209,166,67,98,22,49,27,68,134,209,166,67,154,246,24,68,210,27,165,67,36,79,23,68,106,176,161,67,98,174,167,21,68,181,68,158,67,224,211,20,68,27,143,153,67,224,
211,20,68,58,143,147,67,99,109,198,191,24,68,104,77,147,67,98,198,191,24,68,9,146,151,67,248,61,25,68,70,206,154,67,53,58,26,68,44,2,157,67,98,153,54,27,68,18,54,159,67,252,118,28,68,44,80,160,67,58,251,29,68,44,80,160,67,98,156,127,31,68,44,80,160,67,
81,190,32,68,133,56,159,67,86,183,33,68,232,8,157,67,98,132,176,34,68,76,217,154,67,223,44,35,68,9,146,151,67,223,44,35,68,16,51,147,67,98,223,44,35,68,50,225,142,67,186,179,34,68,54,168,139,67,75,193,33,68,17,136,137,67,98,220,206,32,68,229,103,135,
67,200,140,31,68,214,87,134,67,58,251,29,68,214,87,134,67,98,209,105,28,68,214,87,134,67,14,38,27,68,52,107,135,67,104,48,26,68,238,145,137,67,98,154,58,25,68,168,184,139,67,198,191,24,68,40,247,142,67,198,191,24,68,104,77,147,67,99,101,0,0 };



static const unsigned char JavascriptFunction[] = { 110,109,28,81,38,68,96,95,171,67,108,196,217,45,68,224,228,169,67,98,20,78,46,68,160,243,174,67,108,57,47,68,32,174,178,67,100,156,48,68,160,9,181,67,98,84,255,49,68,160,106,183,67,76,222,51,68,96,152,184,67,236,56,54,68,96,152,184,67,98,68,183,56,68,
96,152,184,67,100,152,58,68,128,139,183,67,20,220,59,68,224,113,181,67,98,196,31,61,68,192,82,179,67,156,193,61,68,224,219,176,67,156,193,61,68,160,7,174,67,98,156,193,61,68,224,58,172,67,148,125,61,68,224,175,170,67,60,245,60,68,192,102,169,67,98,44,
109,60,68,0,35,168,67,92,127,59,68,192,5,167,67,12,44,58,68,96,20,166,67,98,252,67,57,68,64,117,165,67,212,50,55,68,0,88,164,67,228,248,51,68,128,188,162,67,98,132,210,47,68,192,173,160,67,172,232,44,68,128,38,158,67,52,60,43,68,96,38,155,67,98,132,225,
40,68,160,237,150,67,20,180,39,68,240,200,145,67,20,180,39,68,80,184,139,67,98,20,180,39,68,96,204,135,67,228,65,40,68,192,39,132,67,60,93,41,68,112,191,128,67,98,220,120,42,68,64,174,122,67,84,17,44,68,160,126,117,67,220,38,46,68,224,250,113,67,98,100,
60,48,68,64,108,110,67,116,192,50,68,96,159,108,67,20,179,53,68,96,159,108,67,98,68,131,58,68,96,159,108,67,180,34,62,68,160,221,112,67,164,145,64,68,64,68,121,67,98,84,0,67,68,224,218,128,67,84,71,68,68,192,125,134,67,148,102,68,68,176,138,141,67,108,
116,168,60,68,64,58,142,67,98,100,83,60,68,208,72,138,67,132,157,59,68,144,116,135,67,140,134,58,68,64,184,133,67,98,148,111,57,68,208,251,131,67,252,204,55,68,112,32,131,67,196,158,53,68,112,32,131,67,98,252,94,51,68,112,32,131,67,12,156,49,68,80,12,
132,67,44,86,48,68,160,233,133,67,98,84,132,47,68,96,23,135,67,68,27,47,68,80,173,136,67,68,27,47,68,128,171,138,67,98,68,27,47,68,96,120,140,67,124,125,47,68,224,8,142,67,220,65,48,68,0,82,143,67,98,12,60,49,68,128,248,144,67,20,155,51,68,224,169,146,
67,140,95,55,68,64,113,148,67,98,4,36,59,68,144,56,150,67,52,237,61,68,96,16,152,67,36,187,63,68,160,248,153,67,98,92,137,65,68,240,224,155,67,228,242,66,68,192,120,158,67,12,248,67,68,160,197,161,67,98,116,253,68,68,0,13,165,67,4,128,69,68,96,31,169,
67,4,128,69,68,192,252,173,67,98,4,128,69,68,96,97,178,67,172,227,68,68,192,126,182,67,60,171,67,68,192,84,186,67,98,132,114,66,68,192,42,190,67,156,184,64,68,96,4,193,67,60,125,62,68,192,225,194,67,98,148,65,60,68,0,191,196,67,116,121,57,68,96,176,197,
67,236,36,54,68,96,176,197,67,98,172,75,49,68,96,176,197,67,116,146,45,68,96,112,195,67,68,249,42,68,192,245,190,67,98,12,96,40,68,32,123,186,67,148,210,38,68,224,241,179,67,28,81,38,68,96,95,171,67,99,109,96,82,24,68,160,60,111,67,108,240,9,32,68,160,
60,111,67,108,240,9,32,68,0,46,168,67,98,240,9,32,68,224,133,174,67,88,194,31,68,192,104,179,67,180,51,31,68,0,209,182,67,98,180,115,30,68,160,75,187,67,84,23,29,68,192,223,190,67,224,30,27,68,0,147,193,67,98,108,38,25,68,64,70,196,67,56,141,22,68,224,
159,197,67,72,83,19,68,224,159,197,67,98,108,138,15,68,224,159,197,67,220,160,12,68,192,128,195,67,80,150,10,68,0,72,191,67,98,8,140,8,68,192,9,187,67,176,132,7,68,192,210,180,67,4,128,7,68,160,157,172,67,108,156,204,14,68,192,241,170,67,98,208,226,14,
68,96,86,175,67,100,53,15,68,224,113,178,67,80,196,15,68,32,68,180,67,98,140,154,16,68,96,2,183,67,176,224,17,68,128,97,184,67,56,150,19,68,128,97,184,67,98,28,80,21,68,128,97,184,67,144,136,22,68,32,101,183,67,204,63,23,68,224,113,181,67,98,196,246,
23,68,32,121,179,67,96,82,24,68,64,97,175,67,96,82,24,68,96,42,169,67,108,96,82,24,68,160,60,111,67,99,101,0,0 };

static const unsigned char Table[] = { 110,109,2,85,37,68,64,67,80,67,108,2,85,37,68,196,89,142,67,108,128,133,7,68,196,89,142,67,108,128,133,7,68,72,143,108,67,98,128,133,7,68,2,14,101,67,89,68,8,68,200,219,93,67,231,151,9,68,52,141,88,67,98,140,235,10,68,162,62,83,67,50,184,12,68,64,67,
80,67,131,152,14,68,64,67,80,67,108,2,85,37,68,64,67,80,67,99,109,56,74,69,68,106,224,105,67,108,56,74,69,68,196,89,142,67,108,183,122,39,68,196,89,142,67,108,183,122,39,68,64,67,80,67,108,237,226,62,68,64,67,80,67,98,196,149,64,68,64,67,80,67,166,54,
66,68,22,246,82,67,21,106,67,68,204,195,87,67,98,131,157,68,68,132,145,92,67,56,74,69,68,106,21,99,67,56,74,69,68,106,224,105,67,99,109,66,124,37,68,151,85,148,67,108,66,124,37,68,137,39,177,67,108,0,128,7,68,137,39,177,67,108,0,128,7,68,151,85,148,67,
108,66,124,37,68,151,85,148,67,99,109,86,140,34,68,112,53,154,67,108,236,111,10,68,112,53,154,67,98,236,111,10,68,112,53,154,67,236,111,10,68,175,71,171,67,236,111,10,68,175,71,171,67,108,86,140,34,68,175,71,171,67,108,86,140,34,68,112,53,154,67,99,109,
66,124,37,68,112,12,183,67,108,66,124,37,68,97,222,211,67,108,63,139,14,68,97,222,211,67,98,1,173,12,68,97,222,211,67,87,226,10,68,126,98,210,67,36,144,9,68,24,190,207,67,98,241,61,8,68,178,25,205,67,0,128,7,68,95,132,201,67,0,128,7,68,226,199,197,67,
108,0,128,7,68,112,12,183,67,108,66,124,37,68,112,12,183,67,99,109,86,140,34,68,74,236,188,67,108,236,111,10,68,74,236,188,67,108,236,111,10,68,226,199,197,67,98,236,111,10,68,137,245,199,67,187,222,10,68,67,12,202,67,229,163,11,68,150,150,203,67,98,
15,105,12,68,235,32,205,67,108,116,13,68,136,254,205,67,63,139,14,68,136,254,205,67,108,86,140,34,68,136,254,205,67,108,86,140,34,68,74,236,188,67,99,109,0,128,69,68,151,85,148,67,108,0,128,69,68,137,39,177,67,108,190,131,39,68,137,39,177,67,108,190,
131,39,68,151,85,148,67,108,0,128,69,68,151,85,148,67,99,109,20,144,66,68,112,53,154,67,108,170,115,42,68,112,53,154,67,98,170,115,42,68,112,53,154,67,170,115,42,68,175,71,171,67,170,115,42,68,175,71,171,67,108,20,144,66,68,175,71,171,67,108,20,144,66,
68,112,53,154,67,99,109,0,128,69,68,112,12,183,67,108,0,128,69,68,193,34,197,67,98,0,128,69,68,24,11,201,67,78,185,68,68,121,202,204,67,172,87,67,68,186,141,207,67,98,244,245,65,68,252,80,210,67,92,22,64,68,97,222,211,67,48,34,62,68,97,222,211,67,108,
190,131,39,68,97,222,211,67,108,190,131,39,68,112,12,183,67,108,0,128,69,68,112,12,183,67,99,109,20,144,66,68,74,236,188,67,108,170,115,42,68,74,236,188,67,108,170,115,42,68,136,254,205,67,108,48,34,62,68,136,254,205,67,98,240,78,63,68,136,254,205,67,
84,111,64,68,151,15,205,67,236,67,65,68,58,102,203,67,98,155,24,66,68,9,189,201,67,20,144,66,68,66,124,199,67,20,144,66,68,193,34,197,67,108,20,144,66,68,74,236,188,67,99,101,0,0 };

static const unsigned char TagList[] = { 110,109,114,145,58,68,70,248,132,67,108,113,254,68,68,66,210,153,67,108,2,23,36,68,214,160,219,67,108,0,128,7,68,210,114,162,67,108,74,103,40,68,125,72,65,67,108,206,13,51,68,248,225,107,67,98,177,106,50,68,145,31,116,67,182,233,50,68,174,119,125,67,
222,138,52,68,37,254,129,67,98,5,44,54,68,116,64,133,67,12,130,56,68,127,62,134,67,114,145,58,68,70,248,132,67,99,109,162,26,51,68,99,66,107,67,108,58,172,40,68,198,136,65,67,108,85,88,57,68,10,223,64,67,98,231,145,60,68,84,190,64,67,7,172,63,68,227,
207,69,67,209,243,65,68,12,239,78,67,98,191,59,68,68,52,14,88,67,0,128,69,68,184,118,100,67,210,119,69,68,252,92,113,67,108,99,77,69,68,180,6,154,67,108,87,185,58,68,157,222,132,67,98,82,145,59,68,183,75,132,67,48,92,60,68,106,86,131,67,82,8,61,68,37,
254,129,67,98,47,96,63,68,106,157,122,67,47,96,63,68,90,101,107,67,82,8,61,68,120,6,98,67,98,154,176,58,68,6,167,88,67,150,226,54,68,6,167,88,67,222,138,52,68,120,6,98,67,98,187,222,51,68,113,182,100,67,240,99,51,68,121,226,103,67,162,26,51,68,99,66,
107,67,99,101,0,0 };

static const unsigned char CommandLineTask[] = { 110,109,254,127,69,68,134,49,130,67,98,254,127,69,68,52,147,113,67,160,174,65,68,220,75,98,67,170,250,60,68,220,75,98,67,108,203,5,16,68,220,75,98,67,98,213,81,11,68,220,75,98,67,255,127,7,68,52,147,113,67,255,127,7,68,134,49,130,67,108,255,127,7,68,
122,206,185,67,98,255,127,7,68,102,54,195,67,213,81,11,68,18,218,202,67,203,5,16,68,18,218,202,67,108,170,250,60,68,18,218,202,67,98,160,174,65,68,18,218,202,67,254,127,69,68,102,54,195,67,254,127,69,68,122,206,185,67,108,254,127,69,68,134,49,130,67,
99,109,249,35,43,68,98,75,161,67,108,55,240,13,68,46,65,186,67,108,55,240,13,68,110,124,175,67,108,214,16,37,68,172,76,156,67,108,55,240,13,68,254,74,137,67,108,55,240,13,68,92,14,125,67,108,249,35,43,68,224,47,151,67,108,249,35,43,68,98,75,161,67,99,
101,0,0 };

static const unsigned char PersistentSettings[] = { 110,109,0,128,7,68,120,160,175,67,108,79,94,18,68,120,160,175,67,98,70,238,17,68,116,156,172,67,153,169,17,68,248,129,169,67,35,146,17,68,52,94,166,67,108,78,201,22,68,108,203,162,67,98,140,219,22,68,40,212,158,67,111,77,23,68,104,239,154,67,4,25,24,
68,24,78,151,67,108,135,44,20,68,40,143,143,67,98,180,77,21,68,120,125,139,67,159,195,22,68,48,212,135,67,189,126,24,68,100,184,132,67,108,228,36,29,68,252,165,138,67,98,149,188,30,68,216,73,136,67,190,137,32,68,172,141,134,67,253,115,34,68,236,135,133,
67,108,215,8,35,68,224,34,117,67,98,46,84,37,68,240,152,115,67,192,171,39,68,240,152,115,67,25,247,41,68,224,34,117,67,108,242,139,42,68,236,135,133,67,98,49,118,44,68,172,141,134,67,91,67,46,68,216,73,136,67,28,219,47,68,252,165,138,67,108,50,129,52,
68,100,184,132,67,98,97,60,54,68,48,212,135,67,58,178,55,68,120,125,139,67,103,211,56,68,40,143,143,67,108,235,230,52,68,24,78,151,67,98,128,178,53,68,104,239,154,67,99,36,54,68,40,212,158,67,161,54,54,68,108,203,162,67,108,205,109,59,68,52,94,166,67,
98,25,75,59,68,164,2,171,67,166,197,58,68,64,147,175,67,0,227,57,68,196,225,179,67,108,136,105,52,68,76,147,178,67,98,43,125,51,68,232,20,182,67,13,62,50,68,176,52,185,67,8,189,48,68,168,201,187,67,108,225,151,50,68,28,44,198,67,98,101,177,48,68,132,
219,200,67,47,149,46,68,8,227,202,67,79,89,44,68,216,46,204,67,108,61,114,41,68,132,207,194,67,98,221,127,39,68,224,141,195,67,18,128,37,68,224,141,195,67,178,141,35,68,132,207,194,67,108,161,166,32,68,216,46,204,67,98,192,106,30,68,8,227,202,67,137,
78,28,68,132,219,200,67,15,104,26,68,28,44,198,67,108,231,66,28,68,168,201,187,67,98,227,193,26,68,176,52,185,67,196,130,25,68,232,20,182,67,102,150,24,68,76,147,178,67,108,239,28,19,68,196,225,179,67,98,74,58,18,68,64,147,175,67,213,180,17,68,164,2,
171,67,35,146,17,68,52,94,166,67,108,130,102,18,68,252,215,175,67,108,0,128,7,68,252,215,175,67,108,0,128,7,68,180,210,224,67,108,0,128,69,68,180,210,224,67,108,0,128,69,68,160,63,131,67,108,155,162,44,68,144,90,54,67,108,0,128,7,68,144,90,54,67,108,
0,128,7,68,120,160,175,67,99,109,0,128,38,68,208,46,152,67,98,100,189,41,68,208,46,152,67,186,94,44,68,144,112,157,67,186,94,44,68,156,235,163,67,98,186,94,44,68,168,102,170,67,100,189,41,68,184,169,175,67,0,128,38,68,184,169,175,67,98,155,66,35,68,184,
169,175,67,53,161,32,68,168,102,170,67,53,161,32,68,156,235,163,67,98,53,161,32,68,144,112,157,67,155,66,35,68,208,46,152,67,0,128,38,68,208,46,152,67,99,101,0,0 };

static const unsigned char Image[] = { 110,109,114,64,21,68,164,233,57,67,108,186,30,21,68,44,126,82,67,98,175,86,16,68,182,48,85,67,168,167,12,68,216,164,101,67,168,167,12,68,232,130,121,67,98,168,167,12,68,234,101,134,67,37,32,16,68,145,110,142,67,8,180,20,68,253,32,144,67,108,92,37,20,
68,136,30,196,67,108,168,167,12,68,94,158,213,67,108,88,245,19,68,94,158,213,67,108,126,219,19,68,46,11,223,67,108,196,117,12,68,46,11,223,67,98,242,184,9,68,46,11,223,67,0,128,7,68,58,153,218,67,0,128,7,68,166,31,213,67,108,0,128,7,68,180,192,77,67,
98,0,128,7,68,138,205,66,67,242,184,9,68,164,233,57,67,196,117,12,68,164,233,57,67,108,114,64,21,68,164,233,57,67,99,109,189,125,21,68,164,233,57,67,108,142,138,64,68,164,233,57,67,98,88,71,67,68,164,233,57,67,0,128,69,68,138,205,66,67,0,128,69,68,180,
192,77,67,108,0,128,69,68,166,31,213,67,98,0,128,69,68,58,153,218,67,88,71,67,68,46,11,223,67,142,138,64,68,46,11,223,67,108,192,24,20,68,46,11,223,67,108,163,50,20,68,94,158,213,67,108,144,88,64,68,94,158,213,67,108,106,8,48,68,57,72,150,67,108,111,
138,39,68,8,65,183,67,108,236,152,32,68,164,8,167,67,108,54,100,20,68,18,140,195,67,108,18,241,20,68,16,54,144,67,98,157,115,21,68,199,95,144,67,95,249,21,68,125,117,144,67,165,129,22,68,125,117,144,67,98,148,241,27,68,125,117,144,67,170,91,32,68,82,
161,135,67,170,91,32,68,232,130,121,67,98,170,91,32,68,44,195,99,67,148,241,27,68,246,26,82,67,165,129,22,68,246,26,82,67,98,102,30,22,68,246,26,82,67,126,188,21,68,226,49,82,67,46,92,21,68,150,94,82,67,108,189,125,21,68,164,233,57,67,99,101,0,0 };

static const unsigned char DirectoryScanner[] = { 110,109,36,206,58,68,232,191,148,67,98,36,206,58,68,136,117,158,67,24,115,57,68,216,138,167,67,140,27,55,68,232,72,175,67,108,0,128,69,68,80,19,204,67,108,180,66,62,68,56,142,218,67,108,16,40,48,68,192,90,190,67,98,24,239,43,68,216,116,196,67,252,192,
38,68,8,14,200,67,16,39,33,68,8,14,200,67,98,172,253,18,68,8,14,200,67,0,128,7,68,160,16,177,67,0,128,7,68,232,191,148,67,98,0,128,7,68,192,221,112,67,172,253,18,68,144,227,66,67,16,39,33,68,144,227,66,67,98,252,78,47,68,144,227,66,67,36,206,58,68,192,
221,112,67,36,206,58,68,232,191,148,67,108,168,204,58,68,8,25,148,67,108,180,92,49,68,8,25,148,67,108,44,94,49,68,232,191,148,67,98,44,94,49,68,72,217,130,67,124,25,42,68,48,162,104,67,16,39,33,68,48,162,104,67,98,40,51,24,68,48,162,104,67,244,239,16,
68,72,217,130,67,244,239,16,68,232,191,148,67,98,244,239,16,68,56,166,166,67,40,51,24,68,112,46,181,67,16,39,33,68,112,46,181,67,98,124,25,42,68,112,46,181,67,44,94,49,68,56,166,166,67,44,94,49,68,232,191,148,67,108,44,94,49,68,152,248,148,67,108,36,
206,58,68,152,248,148,67,108,36,206,58,68,232,191,148,67,99,101,0,0 };

static const unsigned char HtmlElement[] = { 110,109,48,44,40,68,128,141,126,67,108,184,99,44,68,128,141,126,67,108,72,237,36,68,64,185,188,67,108,136,194,32,68,64,185,188,67,108,48,44,40,68,128,141,126,67,99,109,40,139,30,68,96,33,183,67,108,0,128,7,68,248,2,164,67,108,0,128,7,68,232,250,151,67,
108,40,139,30,68,108,212,132,67,108,40,139,30,68,176,57,147,67,108,48,138,16,68,24,5,158,67,108,40,139,30,68,204,209,168,67,108,40,139,30,68,96,33,183,67,99,109,216,116,46,68,96,33,183,67,108,0,128,69,68,248,2,164,67,108,0,128,69,68,232,250,151,67,108,
216,116,46,68,108,212,132,67,108,216,116,46,68,176,57,147,67,108,208,117,60,68,24,5,158,67,108,216,116,46,68,204,209,168,67,108,216,116,46,68,96,33,183,67,99,101,0,0 };

} // Icons

#define LOAD_MULTIPAGE_PATH(Class) if(url == factory::Class::getStaticId().toString()) p.loadPathFromData(Icons::Class, sizeof(Icons::Class));
#else
#define LOAD_MULTIPAGE_PATH(Class);
#endif

Path Factory::createPath(const String& url) const
{
    Path p;
    
    LOAD_MULTIPAGE_PATH(Branch);
    LOAD_MULTIPAGE_PATH(FileSelector);
    LOAD_MULTIPAGE_PATH(Choice);
    LOAD_MULTIPAGE_PATH(ColourChooser);
    LOAD_MULTIPAGE_PATH(Column);
    LOAD_MULTIPAGE_PATH(List);
	LOAD_MULTIPAGE_PATH(Spacer);
    LOAD_MULTIPAGE_PATH(MarkdownText);

#if HISE_MULTIPAGE_INCLUDE_EDIT
	if(url == factory::SimpleText::getStaticId().toString()) p.loadPathFromData(Icons::MarkdownText, sizeof(Icons::MarkdownText));;
#endif

    LOAD_MULTIPAGE_PATH(Button);
    LOAD_MULTIPAGE_PATH(TextInput);
    LOAD_MULTIPAGE_PATH(Skip);
    LOAD_MULTIPAGE_PATH(Launch);
    LOAD_MULTIPAGE_PATH(DummyWait);
    LOAD_MULTIPAGE_PATH(LambdaTask);
    LOAD_MULTIPAGE_PATH(DownloadTask);
    LOAD_MULTIPAGE_PATH(UnzipTask);
    LOAD_MULTIPAGE_PATH(HlacDecoder);
	LOAD_MULTIPAGE_PATH(ClipboardLoader);
	LOAD_MULTIPAGE_PATH(Image);
	LOAD_MULTIPAGE_PATH(HtmlElement);
    LOAD_MULTIPAGE_PATH(AppDataFileWriter);
    LOAD_MULTIPAGE_PATH(RelativeFileLoader);
	LOAD_MULTIPAGE_PATH(HiseActivator);
    LOAD_MULTIPAGE_PATH(HttpRequest);
    LOAD_MULTIPAGE_PATH(CodeEditor);
    LOAD_MULTIPAGE_PATH(CopyProtection);
    LOAD_MULTIPAGE_PATH(ProjectInfo);
	LOAD_MULTIPAGE_PATH(PluginDirectories);
	LOAD_MULTIPAGE_PATH(CopyAsset);
	LOAD_MULTIPAGE_PATH(CommandLineTask);
	LOAD_MULTIPAGE_PATH(OperatingSystem);
	LOAD_MULTIPAGE_PATH(EventLogger);
	LOAD_MULTIPAGE_PATH(FileLogger);
	LOAD_MULTIPAGE_PATH(DirectoryScanner);
	LOAD_MULTIPAGE_PATH(JavascriptFunction);
	LOAD_MULTIPAGE_PATH(PersistentSettings);
    LOAD_MULTIPAGE_PATH(Table);
    LOAD_MULTIPAGE_PATH(TagList);
    
    return p;
    
}

#undef LOAD_MULTIPAGE_PATH

} // multipage
} // hise
