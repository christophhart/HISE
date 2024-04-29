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
namespace multipage {
using namespace juce;

struct ApiObject: public DynamicObject
{
	struct ScopedThisSetter
	{
		ScopedThisSetter(State& s_, DynamicObject::Ptr thisObject_):
		  s(s_),
		  thisObject(thisObject_)
		{
			s.createJavascriptEngine()->registerNativeObject("this", thisObject.get());
		}

		~ScopedThisSetter()
		{
			s.createJavascriptEngine()->registerNativeObject("this", nullptr);
		}

		DynamicObject* getThisObject() { return thisObject.get(); }

	private:
		DynamicObject::Ptr thisObject;
		State& s;
	};

	ApiObject(State&state_):
	  state(state_)
	{}

	struct Helpers
	{
		static bool callRecursive(const var& obj, const std::function<bool(const var&)>& f);
	};

	/** Call this whenever you need to update the given component. It will iterate the component tree and call the method for
	 *  the PageBase subclass that matches the given ID.
	 */
    void updateWithLambda(const var& infoObject, const Identifier& id, const std::function<void(Component*)>& f);

	void setMethodWithHelp(const Identifier& id, var::NativeFunction f, const String& helpText);
	void expectArguments(const var::NativeFunctionArgs& args, int numArgs, const String& customErrorMessage={});

	String getHelp(Identifier methodName) const;

	bool callForEachInfoObject(const std::function<bool(const var& obj)>& f) const;;

protected:

	State& state;

private:

    std::map<Identifier, String> help;
};

struct HtmlParser
{
    using DataProvider = simple_css::StyleSheet::Collection::DataProvider;

    struct HeaderInformation
    {
        enum class DataType
        {
            StyleSheet,
            ScriptCode,
            numDataTypes
        };

        HeaderInformation()
        {
	        rootObject = new DynamicObject();
        }

        void appendStyle(DataType t, const String& text);

        Result flush(DataProvider* d, State& state);

        simple_css::StyleSheet::Collection css;
        DynamicObject::Ptr rootObject;

    private:
        String code[(int)DataType::numDataTypes];
    };

    HtmlParser();

    HeaderInformation parseHeader(DataProvider* d, XmlElement* header);

    void parseTable(XmlElement* xml, DynamicObject::Ptr nv);

    var getElement(DataProvider* d, HeaderInformation& hi, XmlElement* xml);

	/** Use this if you need to process the value coming in from the HTML parser. */
	static var convertValue(const Identifier& id, const var& value, bool convertFromHtml)
	{
		if(id == mpid::Enabled)
		{
			return !(bool)value;
		}
		else
		{
			return value;
		}
	}

	struct IDConverter
	{
		enum class Type
		{
			HtmlId,
			MultipageId,
			Undefined,
			numTypes
		};

		struct Item
		{
			Identifier getHtmlId() { return htmlId; }
			Identifier getMultipageId() { return multipageId; }

			Identifier htmlId;
			Identifier multipageId;
		};

		Type getTypeForId(const Identifier& id) const;

		Identifier convert(const Identifier& id) const;

		void set(const Identifier& html, const Identifier& mp);

		Array<Item> items;
	};

	IDConverter types;
	IDConverter properties;
};



} // multipage	
} // hise