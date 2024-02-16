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
	auto visible = true;

	if(d.hasProperty(mpid::Visible))
		visible = (bool)d[mpid::Visible];

	setSize(width, visible ? 42 : 0);
	typeId = d[mpid::Type].toString();
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
	b.removeFromBottom(10);
	g.setColour(Colours::black.withAlpha(0.1f));
	g.fillRect(b);

	auto f = Dialog::getDefaultFont(*this);

	g.setColour(f.second);
	g.setFont(f.first);

	String m;
	m << "Type: " << typeId;

	g.drawText(m, b, Justification::centred);
}

String MarkdownText::getString(const String& markdownText, Dialog& parent)
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
							auto v = parent.getState().globalState[Identifier(variableId)].toString();

							if(v.isNotEmpty())
								other << v;

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
					auto v = parent.getState().globalState[Identifier(variableId)].toString();

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

MarkdownText::MarkdownText(Dialog& r, int width, const var& d):
	PageBase(r, width, d),
	r(getString(d[mpid::Text].toString(), r)),
	obj(d)
{


	padding = obj[mpid::Padding];

	setSize(width, 0);
	setInterceptsMouseClicks(false, false);
}

void MarkdownText::postInit()
{
	init();

	auto sd = findParentComponentOfClass<Dialog>()->getStyleData();

	sd.fromDynamicObject(obj, [](const String& name){ return Font(name, 13.0f, Font::plain); });

	r.setStyleData(sd);
	r.parse();

	auto width = getWidth();

	auto h = roundToInt(r.getHeightForWidth(width - 2 * padding));
	setSize(width, h + 2 * padding);
}

void MarkdownText::paint(Graphics& g)
{
	g.fillAll(r.getStyleData().backgroundColour);
	r.draw(g, getLocalBounds().toFloat().reduced(padding));
}

void MarkdownText::createEditor(Dialog::PageInfo& rootList)
{
    rootList[mpid::Padding] = 10;
    
    rootList.addChild<Type>({
        { mpid::ID, "Type"},
        { mpid::Type, "MarkdownText"},
		{ mpid::Help, "A field to display text messages with markdown support" }
    });

	rootList.addChild<TextInput>({
		{ mpid::ID, "Padding" },
		{ mpid::Text, "Padding" },
		{ mpid::Value, padding },
		{ mpid::Help, "The outer padding for the entire text render area" }
	});

    rootList.addChild<TextInput>({
        { mpid::ID, "Text" },
        { mpid::Text, "Text" },
        { mpid::Required, true },
        { mpid::Multiline, true },
        { mpid::Value, "Some markdown **text**." },
		{ mpid::Help, "The text content in markdown syntax." }
    });
}


void MarkdownText::editModeChanged(bool isEditMode)
{
#if HISE_MULTIPAGE_INCLUDE_EDIT
	PageBase::editModeChanged(isEditMode);

	if(overlay != nullptr)
	{
		overlay->localBoundFunction = [](Component* c)
		{
			return c->getLocalBounds();
		};

		overlay->setOnClick([this](bool isRightClick)
		{
			if(showDeletePopup(isRightClick))
				return;

			auto& list = rootDialog.createModalPopup<List>();
			list.setStateObject(infoObject);

			createEditor(list);

			list.setCustomCheckFunction([this](PageBase* b, const var& obj)
			{
				padding = obj[mpid::Padding];
				r.setNewText(obj[mpid::Text].toString());
				r.parse();

				auto width = getWidth();

				auto h = roundToInt(r.getHeightForWidth(width - 2 * padding));
				setSize(width, h + 2 * padding);

				Container::recalculateParentSize(this);

				return Result::ok();
			});


			rootDialog.showModalPopup(true);
		});
	}
#endif
}



Result MarkdownText::checkGlobalState(var)
{
	return Result::ok();
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
	registerPage<factory::List>();
    registerPage<factory::MarkdownText>();
	registerPage<factory::Button>();
	registerPage<factory::TextInput>();
	registerPage<factory::Skip>();
	registerPage<factory::Launch>();
	registerPage<factory::DummyWait>();
	registerPage<factory::LambdaTask>();
	registerPage<factory::DownloadTask>();
	registerPage<factory::UnzipTask>();
	registerPage<factory::HlacDecoder>();
	registerPage<factory::LinkFileWriter>();
	registerPage<factory::RelativeFileLoader>();
	registerPage<factory::HttpRequest>();
	registerPage<factory::CodeEditor>();
	registerPage<factory::CopyProtection>();
	registerPage<factory::ProjectInfo>();
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

StringArray Factory::getPopupMenuList() const
{
	StringArray sa;

	for(const auto& i: items)
	{
		String entry;
		entry << i.category << "::" << i.id;
		sa.add(entry);
	}

	return sa;
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


} // multipage
} // hise
