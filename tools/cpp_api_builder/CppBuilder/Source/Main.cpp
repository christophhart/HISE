/*
  ==============================================================================

    This file was auto-generated!

    It contains the basic startup code for a Juce application.

  ==============================================================================
*/

#include "../JuceLibraryCode/JuceHeader.h"

#define DECLARE_ID(x) static Identifier x(#x);

namespace DoxygenTags
{
DECLARE_ID(includes);
DECLARE_ID(innerclass);
DECLARE_ID(listofallmembers);
DECLARE_ID(text);
DECLARE_ID(inheritance);
DECLARE_ID(type);
DECLARE_ID(kind);
DECLARE_ID(name);
DECLARE_ID(enumvalue);
DECLARE_ID(briefdescription);
DECLARE_ID(detaileddescription);
DECLARE_ID(ref);
DECLARE_ID(argsstring);
DECLARE_ID(para);
DECLARE_ID(compoundname);
DECLARE_ID(compounddef);
DECLARE_ID(sectiondef);
DECLARE_ID(programlisting);
DECLARE_ID(verbatim);
DECLARE_ID(codeline);
DECLARE_ID(sp);
DECLARE_ID(memberdef);
DECLARE_ID(listitem);
DECLARE_ID(refid);
DECLARE_ID(baseclasses);
DECLARE_ID(derivedclasses);
DECLARE_ID(basecompoundref);
DECLARE_ID(derivedcompoundref);
DECLARE_ID(inheritancegraph);
DECLARE_ID(collaborationgraph);
DECLARE_ID(heading);
DECLARE_ID(level);
DECLARE_ID(title);
DECLARE_ID(innernamespace);

}

struct AnchorCache
{
	struct Item
	{
		Item() {};

		Item(const String& link_, const String& anchor_) :
			link(link_),
			anchor(anchor_)
		{
		}

		bool operator==(const Item& other) const
		{
			return toString() == other.toString();
		}

		bool operator==(const String& other) const
		{
			return toString() == other;
		}

		String toString() const
		{
			String s = link;

			if (anchor.isNotEmpty())
				s << "#" << anchor;

			return s;
		}

		String toResolvedLink() const
		{
			jassert(anchor.isEmpty() == resolvedAnchor.isEmpty());

			String s = link;

			if (resolvedAnchor.isNotEmpty())
				s << "#" << resolvedAnchor;

			return s;
		}

		explicit operator bool() const
		{
			return anchor.isNotEmpty();
		}

		String link;
		String anchor;
		String resolvedAnchor;
	};


	Array<Item> items;
};

namespace ReadmeFiles
{
DECLARE_ID(indexpage);
DECLARE_ID(namespacehise_1_1raw);
DECLARE_ID(namespacehise);

static bool isReadmeFile(const File& f)
{
	auto id = Identifier(f.getFileNameWithoutExtension());

	return id == indexpage || id == namespacehise_1_1raw || id == namespacehise;
}
}

struct Helpers
{
	static XmlElement* loadFile(const File& f)
	{
		return XmlDocument::parse(f);
	}

	static bool writeTextIntoAttribute(XmlElement* xml, XmlElement* parent)
	{


		if (xml->isTextElement())
		{
			ScopedPointer<XmlElement> textElement = new XmlElement("text");
			textElement->setAttribute(DoxygenTags::text, xml->getText());

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
			Array<Identifier> unusedTags = { DoxygenTags::includes,
				DoxygenTags::innerclass,
				DoxygenTags::listofallmembers,
				DoxygenTags::inheritancegraph,
				DoxygenTags::collaborationgraph,
			    DoxygenTags::title,
				DoxygenTags::innernamespace };

			Identifier id(xml->getTagName());

			if (unusedTags.contains(id))
			{
				parent->removeChildElement(xml, true);
				return true;
			}

			for (int i = 0; i < xml->getNumChildElements(); i++)
			{
				bool wasDeleted = writeTextIntoAttribute(xml->getChildElement(i), xml);

				if (wasDeleted)
					i--;
			}
		}



		return false;
	}


	static void moveInheritanceToSubtree(ValueTree v)
	{
		v.removeChild(v.getChildWithName(DoxygenTags::briefdescription), nullptr);

		ValueTree inh(DoxygenTags::inheritance);

		ValueTree b(DoxygenTags::baseclasses);
		ValueTree d(DoxygenTags::derivedclasses);

		inh.addChild(b, -1, nullptr);
		inh.addChild(d, -1, nullptr);

		v.addChild(inh, 2, nullptr);
		
		for (int i = 0; i < v.getNumChildren(); i++)
		{
			auto t = v.getChild(i).getType();

			if (t == DoxygenTags::basecompoundref || t == DoxygenTags::derivedcompoundref)
			{
				auto c = v.getChild(i);
				v.removeChild(i, nullptr);

				auto& parent = t == DoxygenTags::basecompoundref ? b : d;

				parent.addChild(c, -1, nullptr);
				i--;
			}
		}
	}

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
		auto memberType = v.getProperty(DoxygenTags::kind).toString();

		String memberName;
		appendValueTree(memberName, v.getChildWithName(DoxygenTags::name), false);

		if (memberType == "enum")
		{
			if (v.getNumChildren() == 0)
				return;

			String e;

			e << "### " << memberType << " " << memberName << "\n";

			e << "| Name | Description |" << "\n";
			e << "| -- | ------ |" << "\n";

			bool didSomething = false;

			for (auto c : v)
			{
				if (c.getType() != DoxygenTags::enumvalue)
					continue;

				auto description = findAllText(c, DoxygenTags::briefdescription).trim();

				if (description.isEmpty())
					continue;

				didSomething = true;

				e << "| `" << findAllText(c, DoxygenTags::name).trim() << "` | " << description << " |\n";
			}

			if (didSomething)
				s << e;
			
		}
		if (memberType == "function")
		{
			String description = findAllText(v, DoxygenTags::detaileddescription);

			if (description.isEmpty())
				return;
			
			s << "\n### " << memberName << "\n";

			s << "\n```cpp" << "\n";
			s << findAllText(v, DoxygenTags::type, true) << " " << memberName << findAllText(v, DoxygenTags::argsstring, true) << "\n";
			s << "```\n\n";

			s << description;
		}
	}

	static void appendSection(String& s, ValueTree v)
	{
		auto sectionType = v.getProperty(DoxygenTags::kind).toString();

		if (sectionType == "public-type")
		{
			String content;

			for (auto c : v)
				appendValueTree(content, c, false);

			if (content.isNotEmpty())
			{
				s << "\n## Public types\n";

				s << content;
			}
		}
		if (sectionType == "public-func")
		{
			s << "\n## Class methods\n";

			for (auto c : v)
				appendValueTree(s, c, false);
		}
	}

	static void appendInheritance(String& s, ValueTree v)
	{
		

		String c;


		String b;

		for (auto c : v.getChildWithName(DoxygenTags::baseclasses))
			appendValueTree(b, c, false);

		if (b.isNotEmpty())
		{
			c << "### Base Classes\n\n";
			c << b;
		}
		

		String d;

		for (auto c : v.getChildWithName(DoxygenTags::derivedclasses))
			appendValueTree(d, c, false);

		if (d.isNotEmpty())
		{
			c << "\n### Derived Classes\n\n";
			c << d;
			c << "\n";
		}

		if (c.isNotEmpty())
		{
			s << "## Class Hierarchy\n\n";

			s << c;
		}
		

	}

	static String getProperLink(String link)
	{
		bool isRawLink = link.contains("raw");

		String s;

		if (isRawLink)
			s = "/cpp_api/raw/" + link;
		else
			s = "/cpp_api/hise/" + link;
		
		if (auto anchor = createAnchorItem(s))
			return anchor.toString();

		return s;
	}

	static AnchorCache::Item createAnchorItem(String link)
	{
		if (link.contains("#"))
			return { link.upToFirstOccurrenceOf("#", false, false), link.fromFirstOccurrenceOf("#", false, false) };

		auto lastPart = link.fromLastOccurrenceOf("_", false, false);

		bool hasAnchor = lastPart.startsWith("1a");

		if (hasAnchor)
			return { link.upToFirstOccurrenceOf("_" + lastPart, false, false), lastPart };
		
		return {};
	}

	static void appendValueTree(String& s, ValueTree v, bool noLinks)
	{
		if (v.getType() == Identifier(DoxygenTags::compoundname))
		{
			return;
#if 0
			s << "# ";
			appendValueTree(s, v.getChildWithName("text"), noLinks);
			s << "\n\n";
#endif
		}
		else if (v.getType() == DoxygenTags::inheritance)
		{
			appendInheritance(s, v);
		}
		else if (v.getType() == DoxygenTags::basecompoundref || v.getType() == DoxygenTags::derivedcompoundref)
		{
			auto resolved = resolveLinkWithAnchor(v.getProperty("refid").toString());

			if (resolved.isNotEmpty())
			{
				s << "- [`" << findAllText(v, DoxygenTags::text);
				s << "`](" << resolved << ")" << "  \n";
			}
			else
			{
				s << "- `" << findAllText(v, DoxygenTags::text) << "`  \n";
			}


			
		}
		else if (v.getType() == DoxygenTags::heading)
		{
			s << "\n";

			int l = (int)v.getProperty(DoxygenTags::level);

			while (--l >= 0)
				s << "#";

			s << " " << findAllText(v, DoxygenTags::text) << "\n";
		}
		else if (v.getType() == DoxygenTags::memberdef)
		{
			appendMemberDef(s, v);
		}
		else if (v.getType() == DoxygenTags::sectiondef)
		{
			appendSection(s, v);
		}
		else if (v.getType() == DoxygenTags::ref)
		{
			if (noLinks)
			{
				s << findAllText(v, DoxygenTags::text);
			}
			else
			{
				auto resolvedLink = resolveLinkWithAnchor(v.getProperty("refid").toString());

				if (resolvedLink.isNotEmpty())
				{
					s << "[" << findAllText(v, DoxygenTags::text);
					s << "](" << resolvedLink << ")";
				}
				else
					s << findAllText(v, DoxygenTags::text);
			}
			
		}
		else if (v.getType() == DoxygenTags::text)
		{
			s << v.getProperty("text").toString();
		}
		else if (v.getType() == DoxygenTags::para)
		{
			for (auto c : v)
				appendValueTree(s, c, noLinks);

			s << "  \n";
		}
		else if (v.getType() == DoxygenTags::programlisting || v.getType() == DoxygenTags::verbatim)
		{
			s << "\n```cpp\n";

			for (auto c : v)
				appendValueTree(s, c, true);


			s << "```\n\n";
		}
		else if (v.getType() == DoxygenTags::codeline)
		{
			for (auto c : v)
				appendValueTree(s, c, true);

			s << "\n";
		}
		else if (v.getType() == DoxygenTags::sp)
		{
			s << " ";
		}
		else if (v.getType() == DoxygenTags::listitem)
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

	static void collectAllAnchors(ValueTree v)
	{
		if (v.getType() == DoxygenTags::memberdef)
		{
			auto link = getProperLink(v.getProperty("id").toString());

			if (auto item = createAnchorItem(link))
			{
				item.resolvedAnchor = findAllText(v, DoxygenTags::name).toLowerCase();
				currentCache.items.addIfNotAlreadyThere(item);
			}
		}

		for (auto c : v)
			collectAllAnchors(c);
	}

	static String resolveLinkWithAnchor(const String& rawLink)
	{
		auto link = getProperLink(rawLink);

		if (link == currentPage)
		{
			return {};
		}

		for (const auto& c : currentCache.items)
		{
			if (c == link)
			{
				auto result = c.toResolvedLink();
				return result;
			}
		}

		std::cout << "stripped link: " + rawLink << "\n";

		return {};
	}

#if 0
	static void resolveAllAnchors(AnchorCache& cache, ValueTree v)
	{
		if (v.getType() == DoxygenTags::basecompoundref || v.getType() == DoxygenTags::derivedcompoundref || v.getType() == DoxygenTags::ref)
		{
			
		}

		for (auto c : v)
		{
			resolveAllAnchors(cache, c);
		}
	}
#endif

	static String currentPage;

	static AnchorCache currentCache;

};

String Helpers::currentPage;
AnchorCache Helpers::currentCache;

struct XmlToMarkdownConverter
{
	XmlToMarkdownConverter(const File& f_, const File& root_, const File& target_):
		f(f_),
		root(root_),
		target(target_)
	{
		ScopedPointer<XmlElement> xml = Helpers::loadFile(f);

		path = f.getRelativePathFrom(root);

		Helpers::writeTextIntoAttribute(xml, nullptr);

		auto v = ValueTree::fromXml(*xml);

		c = v.getChildWithName("compounddef");
	}

	

	void updateAnchorCache()
	{
		auto thisLink = Helpers::getProperLink(f.getFileNameWithoutExtension());

		Helpers::currentCache.items.add(AnchorCache::Item(thisLink, ""));
		Helpers::collectAllAnchors(c);
	}

	void writeMarkdownFile()
	{
		String path = f.getRelativePathFrom(root);
		auto targetFile = target.getChildFile(path).withFileExtension(".md");

		std::cout << "\ncreate " << targetFile.getFullPathName() << "\n";

		if (ReadmeFiles::isReadmeFile(targetFile))
		{
			targetFile = targetFile.getSiblingFile("readme.md");
		}

		targetFile.create();
		targetFile.replaceWithText(process());
		
		
	}

	String process()
	{
		Helpers::currentPage = Helpers::getProperLink(f.getFileNameWithoutExtension());

		Helpers::moveInheritanceToSubtree(c);

		auto description = c.getChildWithName(DoxygenTags::detaileddescription);

		c.removeChild(description, nullptr);
		c.addChild(description, 1, nullptr);

		File test("D:\\test.xml");

		test.replaceWithText(c.toXmlString());

		auto t = c.getChildWithName(DoxygenTags::compoundname).getChildWithName(DoxygenTags::text);

		String d;

		String keyword;
		String summary;

		if (ReadmeFiles::isReadmeFile(f))
		{
			auto id = Identifier(f.getFileNameWithoutExtension());

			if (id == ReadmeFiles::indexpage)
			{
				keyword = "C++ API";
				summary = "The C++ API reference\n";
			}
				
			if (id == ReadmeFiles::namespacehise)
			{
				keyword = "namespace hise";
				summary = "A collection of important classes from the HISE codebase\n";
			}
				
			if (id == ReadmeFiles::namespacehise_1_1raw)
			{
				keyword = "namespace raw";
				summary = "A high-level API for creating projects using C++\n";
			}
		}
		else
		{
			keyword = Helpers::findAllText(t, {}).fromFirstOccurrenceOf("::", false, false).trim();
			summary = " C++ API Class reference\n";
		}

		d << "---\n";
		d << "keywords: " << keyword << "\n";
		d << "summary:  " << summary;
		d << "author:   Christoph Hart\n";
		d << "---\n\n";

		for (auto child : c)
		{
			Helpers::appendValueTree(d, child, false);
		}

		return d;
	}

	String path;
	File root;
	File f;
	File target;
	ValueTree c;
};

//==============================================================================
int main (int argc, char* argv[])
{
	if (argc != 3)
	{
		std::cout << "Doxygen XML2MarkdownConverter\n\n";
		std::cout << "Usage: cpp_api_builder [INPUT_DIRECTORY] [OUTPUT_DIRECTORY]";
	}
	else
	{
		File root(argv[1]);

		File target(argv[2]);

		Array<File> files;

		root.findChildFiles(files, File::findFiles, true, "*.xml");

		std::cout << "Resolving links\n";

		for (auto f : files)
		{
			XmlToMarkdownConverter c(f, root, target);
			c.updateAnchorCache();
		}

		for (auto d : Helpers::currentCache.items)
			DBG(d.toString());

		std::cout << "Creating files\n";

		for (auto f : files)
		{
			XmlToMarkdownConverter c(f, root, target);
			c.writeMarkdownFile();
		}
	}

    // ..your code goes here!


    return 0;
}
