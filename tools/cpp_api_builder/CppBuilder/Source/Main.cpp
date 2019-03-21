/*
  ==============================================================================

    This file was auto-generated!

    It contains the basic startup code for a Juce application.

  ==============================================================================
*/

#include "../JuceLibraryCode/JuceHeader.h"


XmlElement* loadFile(const String& className)
{
	File root("D:\\Development\\HISE modules\\tools\\cpp_api_builder");

	String prefix = "classhise_1_1";

	auto f = root.getChildFile(prefix + className).withFileExtension(".xml");

	return XmlDocument::parse(f);
}

bool writeTextIntoAttribute(XmlElement* xml, XmlElement* parent)
{
	StringArray unusedTags = { "includes", "innerclass", "listofallmembers" };

	if (unusedTags.contains(xml->getTagName()))
	{
		parent->removeChildElement(xml, true);
		return true;
	}

	if (xml->isTextElement())
	{
		ScopedPointer<XmlElement> textElement = new XmlElement("text");
		textElement->setAttribute("text", xml->getText());

		for (int i = 0; i < parent->getNumChildElements(); i++)
		{
			if (parent->getChildElement(i) == xml)
			{
				parent->removeChildElement(xml, true);
				parent->insertChildElement(textElement.release(), i);
			}
		}
		
	}
	else
	{
		for (int i = 0; i < xml->getNumChildElements(); i++)
		{
			bool wasDeleted = writeTextIntoAttribute(xml->getChildElement(i), xml);

			if (wasDeleted)
				i--;
		}
	}

	

	return false;
}

struct Helpers
{
	static String findAllText(ValueTree v, Identifier id, bool noLinks=false)
	{
		if (id.isValid())
			v = v.getChildWithName(id);

		String s;
		appendValueTree(s, v, noLinks);
		return s;
	}

	static void appendMemberDef(String& s, ValueTree v)
	{
		auto memberType = v.getProperty("kind").toString();

		String memberName;
		appendValueTree(memberName, v.getChildWithName("name"), false);

		

		if (memberType == "enum")
		{
			s << "### " << memberType << " " << memberName << "\n";

			s << "| Name | Description |" << "\n";
			s << "| ---- | ------ |" << "\n";

			for (auto c : v)
			{
				if (c.getType() != Identifier("enumvalue"))
					continue;

				s << "| " << findAllText(c, "name") << " | " << findAllText(c, "briefdescription").trim() << " |\n";
			}
		}
		if (memberType == "function")
		{
			String description = findAllText(v, "detaileddescription");

			if (description.isEmpty())
				return;
			
			s << "### " << memberName << "\n";

			s << "```cpp" << "\n";
			s << findAllText(v, "type", true) << " " << memberName << findAllText(v, "argsstring", true) << "\n";
			s << "```\n";

			s << description << "\n";
		}
	}

	static void appendSection(String& s, ValueTree v)
	{
		auto sectionType = v.getProperty("kind").toString();

		if (sectionType == "public-type")
		{
			s << "## Public types\n";

			for (auto c : v)
				appendValueTree(s, c, false);
		}
		if (sectionType == "public-func")
		{
			s << "## Class methods\n";

			for (auto c : v)
				appendValueTree(s, c, false);
		}
	}

	static void appendValueTree(String& s, ValueTree v, bool noLinks)
	{
		if (v.getType() == Identifier("compoundname"))
		{
			s << "# ";
			appendValueTree(s, v.getChildWithName("text"), noLinks);
			s << "\n";
		}
		else if (v.getType() == Identifier("memberdef"))
		{
			appendMemberDef(s, v);
		}
		else if (v.getType() == Identifier("sectiondef"))
		{
			appendSection(s, v);
		}
		else if (v.getType() == Identifier("ref"))
		{
			if (noLinks)
			{
				s << v.getChildWithName("text").getProperty("text").toString();
			}
			else
			{
				s << "[" << v.getChildWithName("text").getProperty("text").toString();
				s << "](" << v.getProperty("refid").toString() << ")";
			}
			
		}
		else if (v.getType() == Identifier("text"))
		{
			s << v.getProperty("text").toString();
		}
		else if (v.getType() == Identifier("para"))
		{
			for (auto c : v)
				appendValueTree(s, c, noLinks);

			s << "  \n";
		}
		else if (v.getType() == Identifier("programlisting") || v.getType() == Identifier("verbatim"))
		{
			s << "\n```cpp\n";

			for (auto c : v)
				appendValueTree(s, c, true);


			s << "```\n\n";
		}
		else if (v.getType() == Identifier("codeline"))
		{
			for (auto c : v)
				appendValueTree(s, c, true);

			s << "\n";
		}
		else if (v.getType() == Identifier("sp"))
		{
			s << " ";
		}
		else if (v.getType() == Identifier("listitem"))
		{
			s << "- ";

			for (auto c : v)
				appendValueTree(s, c, false);
		}
		else
		{
			for (auto c : v)
				appendValueTree(s, c, noLinks);
		}


	}
};

//==============================================================================
int main (int argc, char* argv[])
{
	ScopedPointer<XmlElement> xml = loadFile("_hise_event");

	writeTextIntoAttribute(xml, nullptr);

	auto v = ValueTree::fromXml(*xml);
	auto c = v.getChildWithName("compounddef");

	auto description = c.getChildWithName("detaileddescription");

	c.removeChild(description, nullptr);
	c.addChild(description, 1, nullptr);

	File test("D:\\test.xml");
		
	test.replaceWithText(c.toXmlString());

	auto name = c.getChildWithName("compoundname").getProperty("text").toString();


	String d;

	for (auto child : c)
	{
		Helpers::appendValueTree(d, child, false);
	}
	
	DBG(d);

	

    // ..your code goes here!


    return 0;
}
