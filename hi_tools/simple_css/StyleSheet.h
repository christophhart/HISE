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
namespace simple_css
{
using namespace juce;

struct Animator;
struct CSSRootComponent;

struct StyleSheet: public ReferenceCountedObject
{
	using Ptr = ReferenceCountedObjectPtr<StyleSheet>;
	using List = ReferenceCountedArray<StyleSheet>;

	/** A collection is a list of multiple StyleSheet objects that can be used to query a style sheet for a given list of selectors. */
	struct Collection
	{
		Collection() = default;

		explicit Collection(List l);

		bool operator==(const Collection& other) const { return list.getFirst() == other.list.getFirst(); }
		bool operator!=(const Collection& other) const { return !(*this == other); }

		operator bool() const { return !list.isEmpty(); }

		void setAnimator(Animator* a);

		Ptr getFirst() const { return list.getFirst(); }


        String getDebugLogForComponent(Component* c) const;

		Ptr getForComponent(Component* c);
		
		String toString() const;

		Ptr getWithAllStates(Component* c, const Selector& s);

		void addComponentToSetup(Component* c);

		void setPropertyVariable(const Identifier& id, const var& newValue);

		MarkdownLayout::StyleData getMarkdownStyleData(Component* c);

		bool clearCache(Component* c = nullptr);

		void setCreateStackTrace(bool shouldCreateStackTrace)
		{
			createStackTrace = shouldCreateStackTrace;
			clearCache();
		}

		void addIsolatedCollection(Component* c, const String& fileName, const Collection& other)
		{
			setUseIsolatedCollections(true);
			addCollectionForComponent(c, other);

			if(fileName.isNotEmpty())
			{
				for(auto& f: isolatedStyleSheetFileNames)
				{
					if(f.first == c)
					{
						f.second = fileName;
						return;
					}
				}

				isolatedStyleSheetFileNames.add({ c, fileName});
			}
			else
			{
				for(const auto& f: isolatedStyleSheetFileNames)
				{
					if(f.first == c)
					{
						isolatedStyleSheetFileNames.remove(&f);
						return;
					}
				}
			}
		}

		void updateIsolatedCollection(const String& fileName, const Collection& other);

		void addCollectionForComponent(Component* c, const Collection& other);

		struct DataProvider
		{
			virtual ~DataProvider() {};
			virtual Font loadFont(const String& fontName, const String& url) = 0;
			virtual String importStyleSheet(const String& url) = 0;
			virtual Image loadImage(const String& imageURL) = 0;
		};

		Result performAtRules(DataProvider* d);

		/** If this is true, then getForComponent() will look only in the child collection for matches. */
		void setUseIsolatedCollections(bool shouldUseIsolatedCollections)
		{
			useIsolatedCollections = shouldUseIsolatedCollections;
		}
		
	private:

		static bool sameOrParent(Component* possibleParent, Component* componentToLookFor);

		bool useIsolatedCollections = false;
		
		Array<std::pair<Component::SafePointer<Component>, String>> isolatedStyleSheetFileNames;
		Array<std::pair<Component::SafePointer<Component>, List>> childCollections;

		bool createStackTrace = true;

		Ptr operator[](const Selector& s) const;

		void forEach(const std::function<void(Ptr)>& f);

        struct CachedStyleSheet
        {
            Component::SafePointer<Component> first;
            StyleSheet::Ptr second;
            String debugLog;
        };
        
		Array<std::pair<std::pair<Component::SafePointer<Component>, Selector>, StyleSheet::Ptr>> cachedMapForAllStates;
		Array<CachedStyleSheet> cachedMaps;

		Animator* animator = nullptr;

		Array<std::pair<String, Font>> customFonts;

		List list;
	};

	StyleSheet(ComplexSelector::List cs):
	  complexSelectors(cs)
	{};

	StyleSheet(const Selector& s)
	{
		complexSelectors.add(new ComplexSelector(s));
	}
	
	TransitionValue getTransitionValue(const PropertyKey& key) const;
	PropertyValue getPropertyValue(const PropertyKey& key) const;

	String getPropertyValueString(const PropertyKey& key) const;

	void copyPropertiesFrom(Ptr other, bool overwriteExisting=true, const StringArray& propertiesToCopy={});

	void copyPropertiesFromParent(Ptr parent);

	void copyVarProperties(Ptr other)
	{
		if(other->varProperties != nullptr)
		{
			if(varProperties == nullptr)
			{
				varProperties = other->varProperties->clone();
			}
			else
			{
				for(const auto& nv: other->varProperties->getProperties())
					varProperties->setProperty(nv.name, nv.value);
			}
		}
	}

	NonUniformBorderData getNonUniformBorder(Rectangle<float> totalArea, PseudoState stateFlag) const;
	Path getBorderPath(Rectangle<float> totalArea, PseudoState stateFlag) const;

	String getCodeGeneratorPixelValueString(const String& areaName, const PropertyKey& key,
	                                        float defaultValue = 0.0f) const;

	float getPixelValue(Rectangle<float> totalArea, const PropertyKey& key, float defaultValue=0.0f) const;
	StringArray getCodeGeneratorArea(const String& rectangleName, const PropertyKey& key) const;

	Rectangle<float> getArea(Rectangle<float> totalArea, const PropertyKey& key) const;

	void setFullArea(Rectangle<float> fullArea);

	Rectangle<float> expandArea(Rectangle<float> sourceArea, const PropertyKey& key) const;
	Rectangle<float> getBounds(Rectangle<float> sourceArea, PseudoState state) const;

	Rectangle<float> getPseudoArea(Rectangle<float> sourceArea, int currentState, PseudoElementType area) const;
	Rectangle<float> truncateBeforeAndAfter(Rectangle<float> sourceArea, int currentState) const;

	void setupComponent(CSSRootComponent* cssRoot, Component* c, int currentState);

	Justification getJustification(PseudoState currentState, int defaultXFlag=Justification::horizontallyCentred, int defaultYFlag=Justification::verticallyCentred) const;
    float getOpacity(int state) const;
	MouseCursor getMouseCursor() const;
	String getText(const String& t, PseudoState currentState) const;
	Font getFont(PseudoState currentState, Rectangle<float> totalArea) const;
	AffineTransform getTransform(Rectangle<float> totalArea, PseudoState currentState) const;
	std::vector<melatonin::ShadowParameters> getShadow(Rectangle<float> totalArea, const PropertyKey& key, bool wantsInset) const;
	std::pair<Colour, ColourGradient> getColourOrGradient(Rectangle<float> area, PropertyKey key, Colour defaultColour=Colours::transparentBlack) const;

	String getCodeGeneratorColour(const String& rectangleName, PropertyKey key, Colour defaultColour=Colours::transparentBlack) const;

	String getAtRuleName() const;

	PositionType getPositionType(PseudoState state) const;

	FlexBox getFlexBox() const;

	juce::RectanglePlacement getRectanglePlacement() const;

	FlexItem getFlexItem(Component* c, Rectangle<float> fullArea) const;

	String getURLFromProperty(const PropertyKey& key) const;

	Rectangle<float> getLocalBoundsFromText(const String& text) const;

	std::pair<bool, PseudoState> matchesRawList(const Selector::RawList& blockSelectors) const;

	String toString() const;

	void setDefaultTransition(PseudoElementType elementType, const Transition& t);

	Transition getTransitionOrDefault(PseudoElementType elementType, const Transition& t) const;

	bool matchesComplexSelectorList(ComplexSelector::List list) const;
	
	bool matchesSelectorList(const Array<Selector>& otherSelectors);
	bool forEachProperty(PseudoElementType type, const std::function<bool(PseudoElementType, Property& v)>& f);
	void setDefaultColour(const String& key, Colour c);
	
	template <typename EnumType> EnumType getAsEnum(const PropertyKey& key, EnumType defaultValue) const
	{
		if(auto pv = getPropertyValue(key))
		{
			return keywords->getAsEnum(key.name, pv.getValue(varProperties), defaultValue);
		}

		return defaultValue;
	}

	void setPropertyVariable(const Identifier& id, const String& newValue);

	ComplexSelector::List complexSelectors;

	bool isAll() const;

	void setCustomFonts(const Array<std::pair<String, Font>>& cf)
	{
		customFonts = cf;
	}

private:

	Array<std::pair<String, Font>> customFonts;

	friend class ComponentUpdaters;

	DynamicObject::Ptr varProperties;

	SharedResourcePointer<KeywordDataBase> keywords;

	Rectangle<float> currentFullArea;
	std::map<String, Colour> defaultColours;
	float defaultFontSize = 16.0f;
	friend class Parser;

	//Array<Selector> selectors;
	std::array<std::vector<Property>, (int)PseudoElementType::All> properties;
	std::array<Transition, (int)PseudoElementType::All> defaultTransitions;
	Animator* animator = nullptr;
};


} // namespace simple_css
} // namespace hise
