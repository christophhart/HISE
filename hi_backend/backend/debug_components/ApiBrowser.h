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

#ifndef APIBROWSER_H_INCLUDED
#define APIBROWSER_H_INCLUDED

namespace hise { using namespace juce;


class ExtendedApiDocumentation
{
public:

	static void init();

	static String getMarkdownText(const Identifier& className, const Identifier& methodName);

private:

	class DocumentationBase
	{
	protected:

		DocumentationBase(const Identifier& id_) :
			id(id_)
		{};

		String description;

		Identifier id;

	public:

		virtual ~DocumentationBase() {};
		virtual String createMarkdownText() const = 0;

		void addDescriptionLine(const String& line)
		{
			description << line << "\n";
		}

		void setDescription(const String& description_)
		{
			description = description_;
		}
	};

	class MethodDocumentation : public DocumentationBase
	{
	public:

		MethodDocumentation(Identifier& className_, const Identifier& id);

		struct Parameter
		{
			bool operator == (const Parameter& other) const
			{
				return id == other.id;
			}

			String id;
			String type;
			String description;
		};

		String createMarkdownText() const override;

		using VarArray = Array<var>;

		using Object = DynamicObject;

		template <typename T> void addParameter(const String& id, const String& description)
		{
			parameters.add({ id, getTypeName<T>(), description });
		}

		void setCodeExample(const String& code)
		{
			codeExample = code;
		}

		void addCodeLine(const String& line)
		{
			codeExample << line << "\n";
		}

		template <typename T> void addReturnType(const String& returnDescription)
		{
			returnType.type = getTypeName<T>();
			returnType.description = returnDescription;
		}

	private:

		friend class ExtendedApiDocumentation;

		template <typename T> String getTypeName() const
		{
			String typeName;

			if      (typeid(T) == typeid(String))	typeName = "String";
			else if (typeid(T) == typeid(int))		typeName = "int";
			else if (typeid(T) == typeid(double))	typeName = "double";
			else if (typeid(T) == typeid(VarArray))	typeName = "Array";
			else if (typeid(T) == typeid(Object))	typeName = "Object";
			else									typeName = "Unknown";

			return typeName;
		}

		String className;
		String codeExample;

		Array<Parameter> parameters;
		Parameter returnType;

	};

	class ClassDocumentation : public DocumentationBase
	{
	public:

		ClassDocumentation(const Identifier& className);

		MethodDocumentation* addMethod(const Identifier& methodName);

		String createMarkdownText() const override;

		void addSubClass(Identifier subClassId)
		{
			subClassIds.add(subClassId);
		}

	private:

		Array<Identifier> subClassIds;

		friend class ExtendedApiDocumentation;

		Array<MethodDocumentation> methods;
	};

	static ClassDocumentation* addClass(const Identifier& name);

	

	

	static bool inititalised;
	static Array<ClassDocumentation> classes;
};

class ApiCollection : public SearchableListComponent
{
public:

	ApiCollection(BackendRootWindow *window);

	SET_GENERIC_PANEL_ID("ApiCollection");

	class MethodItem : public SearchableListComponent::Item
	{
	public:

		const int extendedWidth = 600;

		MethodItem(const ValueTree &methodTree_, const String &className_);

		int getPopupHeight() const override
		{ 
			if (parser != nullptr) 
				return (int)parser->getHeightForWidth((float)extendedWidth) + 20;
			else 
				return 150; 
		}

		int getPopupWidth() const override
		{
			if (parser != nullptr)
				return extendedWidth + 20;
			else
				return Item::getPopupWidth();
		}

		void paintPopupBox(Graphics &g) const
		{
			

			if (parser != nullptr)
			{
				auto bounds = Rectangle<float>(10.0f, 10.0f, (float)extendedWidth, (float)getPopupHeight() - 20);
				parser->draw(g, bounds);
			}
			else
			{
				auto bounds = Rectangle<float>(10.0f, 10.0f, 280.0f, (float)getPopupHeight() - 20);
				help.draw(g, bounds);
			}
			
		}

		void mouseEnter(const MouseEvent&) override { repaint(); }
		void mouseExit(const MouseEvent&) override { repaint(); }
		void mouseDoubleClick(const MouseEvent&) override;

		bool keyPressed(const KeyPress& key) override;

		void paint(Graphics& g) override;

	private:

		void insertIntoCodeEditor();

		AttributedString help;

		String name;
		String description;
		String className;
		String arguments;

		ScopedPointer<MarkdownRenderer> parser;

		const ValueTree methodTree;
	};

	class ClassCollection : public SearchableListComponent::Collection
	{
	public:
		ClassCollection(const ValueTree &api);;

		void paint(Graphics &g) override;
	private:

		String name;

		const ValueTree classApi;
	};

	int getNumCollectionsToCreate() const override { return apiTree.getNumChildren(); }

	Collection *createCollection(int index) override
	{
		return new ClassCollection(apiTree.getChild(index));
	}

	ValueTree apiTree;

private:

	BaseDebugArea *parentArea;

};

} // namespace hise

#endif  // APIBROWSER_H_INCLUDED
