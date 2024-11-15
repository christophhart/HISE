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

#pragma once


namespace hise {
namespace dyncomp
{

using namespace hise;
using namespace juce;

#define DECLARE_ID(x) static const Identifier x(#x);

namespace dcid
{
	DECLARE_ID(ContentProperties);
	DECLARE_ID(Root);

	DECLARE_ID(id);
	DECLARE_ID(type);
	DECLARE_ID(text);
	DECLARE_ID(enabled);
	DECLARE_ID(visible);
	DECLARE_ID(tooltip);
	DECLARE_ID(defaultValue);
	DECLARE_ID(useUndoManager);

    static const Identifier class_("class");
    DECLARE_ID(elementStyle);

	DECLARE_ID(parentComponent);
	DECLARE_ID(x);
	DECLARE_ID(y);
	DECLARE_ID(width);
	DECLARE_ID(height);
	
	
	DECLARE_ID(isMomentary);
	DECLARE_ID(radioGroupId);
	DECLARE_ID(setValueOnClick);

	DECLARE_ID(useCustomPopup);
	DECLARE_ID(items);

	DECLARE_ID(editable);
	DECLARE_ID(multiline);
	DECLARE_ID(updateEachKey);

	DECLARE_ID(min);
	DECLARE_ID(max);
	DECLARE_ID(middlePosition);
	DECLARE_ID(stepSize);
	DECLARE_ID(mode);
	DECLARE_ID(suffix);
    DECLARE_ID(style);
	DECLARE_ID(showValuePopup);

	DECLARE_ID(filmstripImage);
    DECLARE_ID(numStrips);
	DECLARE_ID(isVertical);
	DECLARE_ID(scaleFactor);

    DECLARE_ID(animationSpeed);
    DECLARE_ID(dragMargin);

}

#undef DECLARE_ID

struct Factory;
struct Base;

/** This is the data model that is used by the dynamic container.
 *
 *  It holds a ValueTree for the data and one for the values.
 */	
struct Data: public ReferenceCountedObject
{
	enum class TreeType
	{
		Data,
		Values
	};

	using Ptr = ReferenceCountedObjectPtr<Data>;
	using ValueCallback = std::function<void(const Identifier&, const var&)>;

	using ImageProvider = std::function<Image(const String&)>;
	using FontProvider = std::function<Font(const String&, const String&)>;

	Data(const var& obj, Rectangle<int> position);

	void setValues(const var& valueObject);

	Image getImage(const String& ref)
	{
		if(ip)
		{
			return ip(ref);
		}

		return {};
	}

	simple_css::StyleSheet::Collection::DataProvider* createDataProvider() const
	{
		struct DP: public simple_css::StyleSheet::Collection::DataProvider
		{
			DP() = default;

			Font loadFont(const String& fontName, const String& url) override
			{
				if(fp)
					return fp(fontName, url);

				return Font(fontName, 13.0f, Font::plain);
			}

			String importStyleSheet(const String& url) override
			{
				return {};
			}

			Image loadImage(const String& imageURL) override
			{
				if(ip)
					return ip(imageURL);

				return {};
			}

			ImageProvider ip;
			FontProvider fp;
		};

		auto np = new DP();
		np->ip = ip;
		np->fp = fp;
		return np;
	}

	/** Converts a JSON object coming from HISE's interface designer to a ValueTree. */
	static ValueTree fromJSON(const var& obj, Rectangle<int> position);

	ReferenceCountedObjectPtr<Base> create(const ValueTree& v);

	void onValueChange(const Identifier& id, const var& newValue, bool useUndoManager);

	const ValueTree& getValueTree(TreeType t) const;

	void setValueCallback(const ValueCallback& v)
	{
		valueCallback = v;
	}

	void setDataProviderCallbacks(const ImageProvider& ip_, const FontProvider& fp_)
	{
		ip = ip_;
		fp = fp_;
	}

private:

	ValueCallback valueCallback;
	ImageProvider ip;
	FontProvider fp;
	UndoManager* um = nullptr;

	ValueTree data;
	ValueTree values;

	ScopedPointer<Factory> factory;
};

struct Base: public Component,
		     public ReferenceCountedObject
{
	using Ptr = ReferenceCountedObjectPtr<Base>;
	using WeakPtr = WeakReference<Base>;
	using List = ReferenceCountedArray<Base>;
	using WeakList = Array<WeakPtr>;

	Base(Data::Ptr d, const ValueTree& v);
	~Base() override;
	

	Identifier getId() const;

	virtual void onValue(const var& newValue) = 0;
	virtual void updateChild(const ValueTree& v, bool wasAdded);

	void updateBasicProperties(const Identifier& id, const var& newValue);

	virtual void updatePosition(const Identifier&, const var&);

	void addChild(Base::Ptr c);

	bool operator==(const Base& other) const noexcept
	{
		return dataTree == other.dataTree;
	}

	bool operator==(const ValueTree& otherData) const noexcept
	{
		return dataTree == otherData;
	}

	void writePositionInValueTree(Rectangle<int> tb, bool useUndoManager)
	{
		dataTree.setProperty(dcid::x, tb.getX(), nullptr);
		dataTree.setProperty(dcid::y, tb.getY(), nullptr);
		dataTree.setProperty(dcid::width, tb.getWidth(), nullptr);
		dataTree.setProperty(dcid::height, tb.getHeight(), nullptr);
	}

protected:

    virtual Component* getCSSTarget() { return this; }
    
	Data::Ptr data;
	ValueTree dataTree;
	ValueTree valueReference;

private:

	valuetree::PropertyListener basicPropertyListener;
	valuetree::PropertyListener positionListener;
	valuetree::PropertyListener valueListener;
	valuetree::ChildListener childListener;

	List children;

	JUCE_DECLARE_WEAK_REFERENCEABLE(Base);
};




struct Root: public Base,
			 public simple_css::CSSRootComponent
{
	void setCSS(const String& styleSheetCode);

	Root(Data::Ptr d);

	void onValue(const var& newValue) override {}

	void updatePosition(const Identifier&, const var&) override
	{
		
	}
	
	simple_css::StyleSheet::Collection::DataProvider* createDataProvider() override
	{
		return data->createDataProvider();
	}

private:

	ScopedPointer<simple_css::StyleSheetLookAndFeel> laf;
};





struct TestComponent: public Component
{
	TestComponent():
	  cssDoc(cssDoc_),
	  jsonDoc(jsonDoc_),
	  cssEditor(cssDoc),
	  jsonEditor(jsonDoc),
	  console(consoleDoc, nullptr)
	{
		jsonDoc_.replaceAllContent(File::getSpecialLocation(File::userDesktopDirectory).getChildFile("funky.json").loadFileAsString());
		cssDoc_.replaceAllContent(File::getSpecialLocation(File::userDesktopDirectory).getChildFile("funky.css").loadFileAsString());
		
		cssEditor.tokenCollection = new mcl::TokenCollection("CSS");
		cssEditor.setLanguageManager(new simple_css::LanguageManager(cssDoc));
        
		addAndMakeVisible(cssEditor);
		addAndMakeVisible(jsonEditor);

		MessageManager::callAsync([this](){ this->recompile(); });

		console.setColour(CodeEditorComponent::ColourIds::backgroundColourId, Colour(0xFF222222));
		console.setColour(CodeEditorComponent::ColourIds::defaultTextColourId, Colour(0xFFBBBBBB));
		console.setLineNumbersShown(false);
		addAndMakeVisible(console);
	}

	juce::CodeDocument cssDoc_;
	juce::CodeDocument jsonDoc_;

	mcl::TextDocument cssDoc;
	mcl::TextDocument jsonDoc;

	mcl::TextEditor cssEditor;
	mcl::TextEditor jsonEditor;

	void recompile()
	{
		counter = 2;
		auto css = cssEditor.getDocument().getAllContent();
		auto json = jsonEditor.getDocument().getAllContent();

		var obj;

		auto jsonOk = JSON::parse(json, obj);

		simple_css::Parser p(css);
		auto cssOk = p.parse();

		if(!cssOk.wasOk())
		{
			cssEditor.setError(cssOk.getErrorMessage());
			return;
		}

		if(!jsonOk.wasOk())
		{
			jsonEditor.setError(jsonOk.getErrorMessage());
		}

		cssEditor.setError("");
		jsonEditor.setError("");

		File::getSpecialLocation(File::userDesktopDirectory).getChildFile("funky.json").replaceWithText(json);
		File::getSpecialLocation(File::userDesktopDirectory).getChildFile("funky.css").replaceWithText(css);

		

		Data::Ptr newData = new Data(obj, getLocalBounds().removeFromRight(getWidth() / 2));

		

		auto newRoot = new Root(newData);

		

		newData->setDataProviderCallbacks([](const String& ref)
		{
			auto refPath = ref.fromLastOccurrenceOf("}", false, false);

#if JUCE_MAC
            auto imgFile = File("/Users/christophhart/Development/HiseSnippets/Assets/Images").getChildFile(refPath);

#else
			auto imgFile = File("D:/Development/HISE Snippets/Assets/Images").getChildFile(refPath);
#endif

			PNGImageFormat iff;
			return iff.loadFrom(imgFile);
		}, [this](const String& fontName, const String& url)
		{
			auto refPath = url.fromLastOccurrenceOf("}", false, false);
            

			
#if JUCE_MAC
            auto fontFile = File("/Users/christophhart/Development/HiseSnippets/Assets/Images").getChildFile(refPath);

#else
            auto fontFile = File("D:/Development/HISE Snippets/Assets/Images").getChildFile(refPath);
#endif

			MemoryBlock mb;

			FileInputStream fis(fontFile);

			fis.readIntoMemoryBlock(mb);

			auto tf = juce::Typeface::createSystemTypefaceFor(mb.getData(), mb.getSize());

			return Font(tf);
		});

		newRoot->setCSS(css);

		newData->setValueCallback([this](const Identifier& id, const var& newValue)
		{
			String m;
			m << "Value change: " << id << ": " << JSON::toString(newValue, true);
			this->writeToConsole(m);
		});

		root = newRoot;
		data = newData;

		addAndMakeVisible(root.get());
		resized();

	}

	void resized() override
	{
		auto b = getLocalBounds();

		jsonEditor.setBounds(b.removeFromLeft(getWidth()/3).reduced(20));
		cssEditor.setBounds(b.removeFromLeft(getWidth()/3).reduced(20));

		console.setBounds(b.removeFromBottom(getHeight() / 2).reduced(20));

		if(root != nullptr)
			root->setBounds(b.reduced(20));
	}

	void paint(Graphics& g) override
	{
		g.fillAll(Colour(0xFF333333));
		g.setFont(GLOBAL_BOLD_FONT());
		g.setColour(Colours::white.withAlpha(0.8f));
		g.drawText("CSS", cssEditor.getBoundsInParent().expanded(20).toFloat(), Justification::centredTop);
		g.drawText("JSON", jsonEditor.getBoundsInParent().expanded(20).toFloat(), Justification::centredTop);
		g.drawText("Console", console.getBoundsInParent().expanded(20).toFloat(), Justification::centredTop);

		if(root != nullptr)
		{
			g.drawText("Preview", root->getBoundsInParent().expanded(20).toFloat(), Justification::centredTop);
		}
	}

	bool keyPressed(const KeyPress& k) override
	{
		if(k == KeyPress::F5Key)
		{
			recompile();
			return true;
		}
		if(k == KeyPress::F4Key)
		{
			if(root != nullptr)
			{
				showInfo = !showInfo;
				dynamic_cast<Root*>(root.get())->showInfo(showInfo);
			}
		}
		if(k == KeyPress::F8Key)
		{
			auto v = data->getValueTree(Data::TreeType::Data).getChild(0);

			ValueTree c("Component");
			c.setProperty(dcid::type, "Panel", nullptr);
			c.setProperty(dcid::id, "P" + String(++counter), nullptr);
			c.setProperty(dcid::height, 50, nullptr);
			c.setProperty(dcid::y, 1000, nullptr);
			v.addChild(c, -1, nullptr);
			return true;
		}

		return false;
	}

	int counter = 4;

	Data::Ptr data;
	Base::Ptr root;

	void writeToConsole(const String& message)
	{
		auto m = consoleDoc.getAllContent();
		m << "\n" << message;
		consoleDoc.replaceAllContent(m);
	}

	bool showInfo = false;
	CodeDocument consoleDoc;
	juce::CodeEditorComponent console;
};



} // namespace dyncomp
} // namespace hise
