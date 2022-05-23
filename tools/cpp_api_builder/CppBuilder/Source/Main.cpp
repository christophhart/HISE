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
DECLARE_ID(definition);
DECLARE_ID(derivedcompoundref);
DECLARE_ID(inheritancegraph);
DECLARE_ID(collaborationgraph);
DECLARE_ID(heading);
DECLARE_ID(level);
DECLARE_ID(title);
DECLARE_ID(innernamespace);
DECLARE_ID(templateparamlist);
DECLARE_ID(initializer);
DECLARE_ID(param);
DECLARE_ID(declname);


}

namespace JsonSettings
{
DECLARE_ID(build_settings);
DECLARE_ID(ShowBaseClasses);
DECLARE_ID(ShowDerivedClasses);
DECLARE_ID(Summary);
DECLARE_ID(CustomLinkPrefix);
DECLARE_ID(UseGroupFileAsReadme);
DECLARE_ID(StripNamespaceFromTitle);
DECLARE_ID(RenameMarkdownToTitle);
DECLARE_ID(RootKeyword);
DECLARE_ID(RootSummary);
DECLARE_ID(StripKeywords);

static Array<Identifier> getAllIds()
{
    Array<Identifier> ids;
    ids.add(ShowBaseClasses);
    ids.add(ShowDerivedClasses);
    ids.add(Summary);
    ids.add(CustomLinkPrefix);
    ids.add(UseGroupFileAsReadme);
    ids.add(StripNamespaceFromTitle);
    ids.add(RenameMarkdownToTitle);
    ids.add(RootKeyword);
    ids.add(RootSummary);
    ids.add(StripKeywords);
    
    return ids;
}

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
	static auto loadFile(const File& f)
	{
		return XmlDocument::parse(f);
	}
    
    static File inputRoot;
    
    
    static String sanitize(const String& s)
    {
        auto p = s.removeCharacters("():,;?").toLowerCase();

        if (!p.isEmpty() && p.endsWith("/"))
            p = p.upToLastOccurrenceOf("/", false, false);

        p = p.replace(".md", "");

        return p.replaceCharacter(' ', '-').toLowerCase();
    }
    static var getJSONSetting(const Identifier& id, var defaultValue)
    {
        jassert(inputRoot.isDirectory());
        
        auto f = inputRoot.getChildFile(JsonSettings::build_settings.toString()).withFileExtension("json");
        
        if(f.existsAsFile())
        {
            var v;
            auto r = JSON::parse(f.loadFileAsString(), v);
            
            if(!r.wasOk())
            {
                std::cout << "Error parsing JSON setting file: " << r.getErrorMessage();
                exit(1);
            }
            
            return v.getProperty(id, defaultValue);
        }
        
        return defaultValue;
    }

    
    
    static bool isGroupReadme(const File& f)
    {
        return f.getFileNameWithoutExtension().startsWith("group") && getJSONSetting(JsonSettings::UseGroupFileAsReadme, false);
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
        
        auto keywordsToStrip = Helpers::getJSONSetting(JsonSettings::StripKeywords, "").toString();
        
        if(!keywordsToStrip.isEmpty())
        {
            auto kw = StringArray::fromTokens(keywordsToStrip, " ,;", "");
            
            for(auto& k: kw)
                s = s.replace(k, "");
        }
        
		return s;
	}

	static ValueTree getApiDocTree(ValueTree v)
	{
		jassert(v.getType() == DoxygenTags::memberdef);

		String memberName;
		appendValueTree(memberName, v.getChildWithName(DoxygenTags::name), false);

		String description = findAllText(v, DoxygenTags::detaileddescription);

		ValueTree m("method");
		m.setProperty("name", memberName, nullptr);
		m.setProperty("arguments", findAllText(v, DoxygenTags::argsstring, true), nullptr);
		m.setProperty("returnType", findAllText(v, DoxygenTags::type, true), nullptr);

		auto d = findAllText(v, DoxygenTags::detaileddescription);

		if (d.isEmpty())
			return {};

		m.setProperty("description", d, nullptr);
		
		return m;
	}

    static void appendTemplateArguments(String& s, ValueTree v, bool includeTypes=false)
    {
        if(v.getType() == DoxygenTags::compounddef && !includeTypes)
            return;
        
        auto tp = v.getChildWithName(DoxygenTags::templateparamlist);
        
        
        
        if(tp.isValid())
        {
            if(!includeTypes)
                s << "template <";
            else
                s << "<";
            
            for(const auto& p: tp)
            {
                jassert(p.getType() == DoxygenTags::param);
                
                s << findAllText(p, DoxygenTags::type).trim();
                
                auto nn = findAllText(p, DoxygenTags::declname).trim();
                
                if(nn.isNotEmpty())
                {
                    s << " ";
                    s << nn;
                }
                
                auto isLast = tp.indexOf(p) == tp.getNumChildren() -1;
                
                if(!isLast)
                    s << ", ";
            }
            
            s << "> ";
        }
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
        if(memberType == "variable")
        {
            String description = findAllText(v, DoxygenTags::detaileddescription);

            if (description.isEmpty())
                return;
            
            s << "#### " << memberName;
            
            s << "\n```cpp\n " << findAllText(v, DoxygenTags::definition, true);
            
            String initValue = findAllText(v, DoxygenTags::initializer);
            
            if(initValue.isNotEmpty())
                s << " " << initValue;
            
            s << "\n```\n";
            
            s << description;
            
            
        }
		if (memberType == "function")
		{
			String description = findAllText(v, DoxygenTags::detaileddescription);

			if (description.isEmpty())
				return;
			
			s << "\n### " << memberName << "\n";

			s << "\n```cpp" << "\n";
            
            appendTemplateArguments(s, v);
            
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
            String sf;
            
            for (auto c : v)
                appendValueTree(sf, c, false);
            
            if(sf.isNotEmpty())
            {
                s << "\n## Class methods\n";
                s << sf;
            }
		}
        if(sectionType == "public-attrib")
        {
            
            
            String sf;
            
            for (auto c : v)
                appendValueTree(sf, c, false);
            
            if(sf.containsNonWhitespaceChars())
            {
                s << "\n## Class members\n";
                s << sf;
            }
        }
        if(sectionType == "public-static-func")
        {
            String sf;
            
            
            
            for(auto c: v)
                appendValueTree(sf, c, false);
            
            if(sf.containsNonWhitespaceChars())
            {
                s << "\n## Static functions\n";
                s << sf;
            }
        }
	}

	static void appendInheritance(String& s, ValueTree v)
	{
		

		String c;


		String b;

		for (auto c : v.getChildWithName(DoxygenTags::baseclasses))
			appendValueTree(b, c, false);

		if (b.isNotEmpty() && Helpers::getJSONSetting(JsonSettings::ShowBaseClasses, true))
		{
			c << "### Base Classes\n\n";
			c << b;
		}
		

		String d;

		for (auto c : v.getChildWithName(DoxygenTags::derivedclasses))
			appendValueTree(d, c, false);

		if (d.isNotEmpty() && Helpers::getJSONSetting(JsonSettings::ShowDerivedClasses, true))
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

    static String getIdFromFile(const File& f)
    {
        jassert(Helpers::getJSONSetting(JsonSettings::RenameMarkdownToTitle, false));
        
        if(auto xml = XmlDocument::parse(f))
        {
            if(auto c = xml->getChildByName(DoxygenTags::compounddef))
            {
                if(auto c1 = c->getChildByName(DoxygenTags::compoundname))
                {
                    auto t = c1->getAllSubText().trim();
                    
                    if(getJSONSetting(JsonSettings::StripNamespaceFromTitle, false))
                    {
                        t = t.fromLastOccurrenceOf("::", false, false);
                        t = sanitize(t);
                    }
                    
                    return t;
                }
            }
        }
        
        return f.getFileNameWithoutExtension();
    }
    
	static String getProperLink(String link)
	{
        auto customLink = getJSONSetting(JsonSettings::CustomLinkPrefix, "").toString();
        
        if(Helpers::getJSONSetting(JsonSettings::RenameMarkdownToTitle, false))
        {
            auto allFiles = inputRoot.findChildFiles(File::findFiles, true);
            
            for(auto& f: allFiles)
            {
                if(link.startsWith(f.getFileNameWithoutExtension()))
                {
                    
                    auto r = f.getParentDirectory().getRelativePathFrom(inputRoot);
                    
                    auto l = getIdFromFile(f);
                    
                    String s;
                    
                    s << r << "/" << l << "/";
                    link = s;
                }
            }
        }
        
        String s;
        
        if(customLink.isNotEmpty())
        {
            s << "/";
            s << customLink.trimCharactersAtEnd("/").trimCharactersAtStart("/");
            s << "/";
            s << link;
        }
        else
        {
            bool isRawLink = link.contains("raw");

            if (isRawLink)
                s << "/cpp_api/raw/" << link;
            else
                s << "/cpp_api/hise/" << link;
        }
        
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
        if(v.getType() == DoxygenTags::templateparamlist)
            return;
		if (v.getType() == Identifier(DoxygenTags::compoundname))
		{
			return;
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


	static String currentPage;

	static AnchorCache currentCache;

};

File Helpers::inputRoot;

String Helpers::currentPage;
AnchorCache Helpers::currentCache;

struct XmlToMarkdownConverter
{
	XmlToMarkdownConverter(const File& f_, const File& root_, const File& target_):
		f(f_),
		root(root_),
		target(target_)
	{
		auto xml = Helpers::loadFile(f);

		path = f.getRelativePathFrom(root);

		Helpers::writeTextIntoAttribute(xml.get(), nullptr);

		auto v = ValueTree::fromXml(*xml);

		c = v.getChildWithName("compounddef");
	}

	
	void setValueTreeMode()
	{
		valueTreeMode = true;
	}

	bool valueTreeMode = false;

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

		if (ReadmeFiles::isReadmeFile(targetFile) || Helpers::isGroupReadme(targetFile))
		{
			targetFile = targetFile.getSiblingFile("readme.md");
		}
        else if(Helpers::getJSONSetting(JsonSettings::RenameMarkdownToTitle, false))
        {
            auto t = c.getChildWithName(DoxygenTags::compoundname).getChildWithName(DoxygenTags::text);
            auto title = Helpers::findAllText(t, {}).fromLastOccurrenceOf("::", false, false).trim();
            
            targetFile = targetFile.getSiblingFile(Helpers::sanitize(title)).withFileExtension("md");
        }

		targetFile.create();
		targetFile.replaceWithText(process());
		
		
	}

	ValueTree getProcessedValueTree()
	{
		ValueTree v(f.getFileNameWithoutExtension());

		String classDoc;

		Helpers::appendValueTree(classDoc, c.getChildWithName(DoxygenTags::detaileddescription), false);

		DBG(classDoc);

		v.setProperty("description", classDoc, nullptr);
		v.setProperty("detailed", classDoc, nullptr);

		for (auto c_ : c)
		{
			if (c_.getType() == DoxygenTags::sectiondef)
			{
				auto sectionType = c_.getProperty(DoxygenTags::kind).toString();

				if (sectionType == "public-func")
				{
					for (auto m : c_)
					{
						auto mm = Helpers::getApiDocTree(m);

						if(mm.isValid())
							v.addChild(mm, -1, nullptr);
					}
				}
			}
		}

		return v;
	}

	String process()
	{
		Helpers::currentPage = Helpers::getProperLink(f.getFileNameWithoutExtension());

        String briefSummary;
        
        if(c.isValid())
        {
            briefSummary = Helpers::findAllText(c, DoxygenTags::briefdescription).trim();
            
            Helpers::moveInheritanceToSubtree(c);

            auto description = c.getChildWithName(DoxygenTags::detaileddescription);

            c.removeChild(description, nullptr);
            c.addChild(description, 1, nullptr);
        }

		auto t = c.getChildWithName(DoxygenTags::compoundname).getChildWithName(DoxygenTags::text);

		String d;

		String keyword;
		String summary;

		if (ReadmeFiles::isReadmeFile(f))
		{
			auto id = Identifier(f.getFileNameWithoutExtension());

			if (id == ReadmeFiles::indexpage)
			{
				keyword = Helpers::getJSONSetting(JsonSettings::RootKeyword, "C++ API").toString().trim();
                summary = Helpers::getJSONSetting(JsonSettings::RootSummary, "The C++ API reference").toString().trim();
			}
				
			if (id == ReadmeFiles::namespacehise)
			{
				keyword = "namespace hise";
				summary = "A collection of important classes from the HISE codebase";
			}
				
			if (id == ReadmeFiles::namespacehise_1_1raw)
			{
				keyword = "namespace raw";
				summary = "A high-level API for creating projects using C++";
			}
		}
		else
		{
            if(Helpers::isGroupReadme(f))
            {
                DBG(c.createXml()->createDocument(""));
                keyword = Helpers::findAllText(c, DoxygenTags::title);
            }
            else
            {
                keyword = Helpers::findAllText(t, {}).fromLastOccurrenceOf("::", false, false).trim();
                
                Helpers::appendTemplateArguments(keyword, c, true);
            }
            
			summary = Helpers::getJSONSetting(JsonSettings::Summary, "C++ API Class reference").toString().trim();
		}

        if(briefSummary.isNotEmpty())
            summary = briefSummary;
        
		d << "---\n";
		d << "keywords: " << keyword << "\n";
		d << "summary:  " << summary << "\n";
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
	if (argc != 3 && argc != 4)
	{
		std::cout << "Doxygen XML2MarkdownConverter\n\n";
		std::cout << "Usage: cpp_api_builder [INPUT_DIRECTORY] [OUTPUT_DIRECTORY] [--valuetree]";
        std::cout << "In order to customize the build, supply a " << JsonSettings::build_settings.toString() << ".json\n";
        std::cout << "file in the root directory. Supported properties:\n";
        
        for(auto& id: JsonSettings::getAllIds())
        {
            std::cout << " - " << id.toString() << "\n";
        }
	}
	else
	{
		auto mode = String(argv[3]).compare("--valuetree") == 0;

		String inputString(argv[1]);
		String outputString(argv[2]);

		File root, target;

		if (File::isAbsolutePath(inputString))
			root = File(inputString);
		else
			root = File(argv[0]).getParentDirectory().getChildFile(inputString);

        Helpers::inputRoot = root;
        
		if (File::isAbsolutePath(outputString))
			target = File(outputString);
		else
			target = File(argv[0]).getParentDirectory().getChildFile(outputString);

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

		ValueTree api("Api");

		for (auto f : files)
		{
			XmlToMarkdownConverter c(f, root, target);

			if (mode)
			{
				c.setValueTreeMode();
				api.addChild(c.getProcessedValueTree(), -1, nullptr);
			}
			else
			{
				c.writeMarkdownFile();
			}
		}

		if (mode)
		{
			auto outputFile = target.getChildFile("apiValueTree.dat");
			outputFile.deleteFile();
			outputFile.create();
			FileOutputStream fos(outputFile);
			api.writeToStream(fos);
			return 0;
		}
	}

    // ..your code goes here!


    return 0;
}
