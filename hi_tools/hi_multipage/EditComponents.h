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

namespace hise
{


namespace multipage {
using namespace juce;

struct CodeGenerator
{
    enum class FileType
    {
	    AssetHeader,
        AssetData,
        DialogHeader,
        DialogImplementation,
        ProjucerFile,
        BatchFile,
        MainCpp,
        numFileTypes
    };

    CodeGenerator(const File& rootDirectory_, String className_, const var& totalData, int numTabs_=0):
      data(totalData),
      className(className_),
      rootDirectory(rootDirectory_),
      numTabs(numTabs_)
	{
        className = totalData[mpid::Properties][mpid::ProjectName].toString();
        classNameTrimmed = className.removeCharacters("_ ./-:");
        

    }

    void write(OutputStream& output, FileType t, State::Job* job) const;

    File getFile(FileType type) const
    {
        auto root = rootDirectory.getChildFile("Binaries");
        switch(type)
        {
        case FileType::AssetHeader: return root.getChildFile("Source/Assets.h");
        case FileType::AssetData:  return root.getChildFile("Source/Assets.cpp");
        case FileType::DialogHeader:  return root.getChildFile("Source/Dialog.h");
        case FileType::BatchFile:
#if JUCE_WINDOWS
            return root.getChildFile("batchCompile.bat");
#else
            return root.getChildFile("batchCompileOSX");
#endif
        case FileType::DialogImplementation:  return root.getChildFile("Source/Dialog.cpp");
        case FileType::ProjucerFile:  return root.getChildFile(className).withFileExtension(".jucer");
        case FileType::MainCpp:  return root.getChildFile("Source/Main.cpp");
        case FileType::numFileTypes: 
        default: return File();
        }
    }

    static void sanitizeData(var& obj)
    {
        if(auto dobj = obj.getDynamicObject())
        {
            NamedValueSet defaultValues;
		    defaultValues.set(mpid::Trigger, false);
		    defaultValues.set(mpid::Help, "");
		    defaultValues.set(mpid::Class, "");
		    defaultValues.set(mpid::Style, "");
		    defaultValues.set(mpid::Text, "LabelText");
		    defaultValues.set(mpid::UseInitValue, false);
		    defaultValues.set(mpid::InitValue, "");
		    defaultValues.set(mpid::Required, false);
		    defaultValues.set(mpid::Enabled, true);
		    defaultValues.set(mpid::Foldable, false);
		    defaultValues.set(mpid::Folded, false);
		    defaultValues.set(mpid::UseChildState, false);
		    defaultValues.set(mpid::EmptyText, "");
		    defaultValues.set(mpid::ParseArray, false);
		    defaultValues.set(mpid::Multiline, false);
		    defaultValues.set(mpid::EventTrigger, "OnPageLoad");
            
		    static const Array<Identifier> deprecatedIds = { Identifier("Padding"),
		        Identifier("LabelPosition"),
		        Identifier("UseFilter"),
		        Identifier("Visible"),
		        Identifier("Comment"),
				Identifier("UseOnValue"),
				Identifier("valueList"),
				Identifier("textList"),
		        Identifier("ManualAction"),
		        Identifier("CallOnNext")};

	        for(int i = 0; i < dobj->getProperties().size(); i++)
	        {
		        auto tid = dobj->getProperties().getName(i);

                if(deprecatedIds.contains(tid))
                {
	                dobj->removeProperty(tid);
                    i--;
                    continue;
                }

                if(obj[tid] == defaultValues[tid])
                {
	                dobj->removeProperty(tid);
                    i--;
                    continue;
                }
	        }

            if(obj[mpid::Code].toString() == "Console.print(value);")
                dobj->removeProperty(mpid::Code);

		    if(auto ar = obj[mpid::Children].getArray())
		    {
			    for(auto& a: *ar)
	                sanitizeData(a);
		    }
        }
    }

    void setUseRawMode(bool shouldUseRawMode)
    {
        rawMode = shouldUseRawMode;
    }
    
    String company;
    String version;
    String hisePath;
    String teamId;

private:

    mutable StringArray lambdaLocalIds;
    mutable StringArray lambdaIds;
    
    struct ScopedTabSetter
    {
	    ScopedTabSetter(const CodeGenerator& cg):
          parent(cg)
	    {
		    parent.numTabs++;
	    }

        ~ScopedTabSetter()
	    {
		    parent.numTabs--;
	    }

        const CodeGenerator& parent;
    };

    File rootDirectory;

    String generateRandomId(const String& prefix) const;
    String getNewLine() const;
    String createAddChild(const String& parentId, const var& childData, const String& itemType="Page", bool attachCustomFunction=false) const;
    
    static String arrayToCommaString(const var& value);
    
    mutable StringArray existingVariables;

    bool rawMode = false;
    
    mutable int idCounter = 0;
    mutable int numTabs;
    var data;
    String className;
    
    String classNameTrimmed;

};


struct AllEditor: public Component
{
    static void addRecursive(mcl::TokenCollection::List& tokens, const String& parentId, const var& obj);

    struct TokenProvider: public mcl::TokenCollection::Provider
    {
        TokenProvider(Component* p_):
          p(p_)
        {};

        Component::SafePointer<Component> p;
        
        void addTokens(mcl::TokenCollection::List& tokens) override;

        JavascriptEngine* engine = nullptr;
    };

    AllEditor(const String& syntax, const var& infoObject);

    ~AllEditor();

    bool keyPressed(const KeyPress& k)
	{
		if(k == KeyPress::F5Key)
		{
			auto ok = compile();

            if(!ok.wasOk())
            {
	            auto errorMessage = ok.getErrorMessage();
                editor->setError(errorMessage);
            }

			return true;
		}

		return false;
	}

    Result compile(bool useCompileCallback=true);

    void resized() override { editor->setBounds(getLocalBounds()); };

    std::function<Result()> compileCallback;

    String syntax;
    juce::CodeDocument doc;
    var infoObject;
    mcl::TextDocument codeDoc;
    ScopedPointer<mcl::TextEditor> editor;
};


} // multipage
} // hise
