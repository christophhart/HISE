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

#include <JuceHeader.h>

namespace hise {
namespace multipage {
namespace library {



struct BroadcasterWizard: public HardcodedDialogWithState
{
    struct CustomResultPage: public Dialog::PageBase
	{
	    enum class SourceIndex
	    {
	        None,
	        ComplexData,
	        ComponentProperties,
	        ComponentVisibility,
	        ContextMenu,
	        EqEvents,
	        ModuleParameters,
	        MouseEvents,
	        ProcessingSpecs,
	        RadioGroup,
	        RoutingMatrix,
	        numSourceIndexTypes
	    };

	    enum class StringProcessor
	    {
		    None,
	        Unquote,
	        JoinToStringWithNewLines,
	        ParseInt,
	        numStringProcessors
	    };

	    enum class TargetIndex
	    {
	        None,
	        Callback,
	        CallbackDelayed,
	        ComponentProperty,
	        ComponentRefresh,
	        ComponentValue,
	        numTargetIndexTypes
	    };
	    
	    DEFAULT_PROPERTIES(CustomResultPage)
	    {
	        return {
	            { mpid::ID, "custom" }
	        };
	    }

	    CustomResultPage(Dialog& r, int width, const var& obj);;

	    static var getArgs(SourceIndex source);
	    static String createFunctionBodyIfAnonymous(const String& functionName, SourceIndex sourceIndex, bool createValueFunction);
	    static void appendLine(String& x, const var& state, const String& suffix, const Array<var>& args, Array<StringProcessor> sp={});
	    static String getTargetLine(TargetIndex target, const var& state);
	    static String getAttachLine(SourceIndex source, const var& state);

	    void postInit() override;
	    void resized() override;
	    Result checkGlobalState(var globalState) override;

	    CodeDocument doc;
	    mcl::TextDocument textDoc;
	    mcl::TextEditor codeEditor;

	};

    BroadcasterWizard();
    ~BroadcasterWizard() override;
	
    Dialog* createDialog(State& state) override;
};

struct NewProjectWizard: public HardcodedDialogWithState
{
	enum class ProjectType
	{
		Empty,
		HXI,
		Rhapsody,
		numProjectTypes
	};

	File getProjectDirectory() const
	{
		return File(getProperty("rootDirectory").toString());
	}

	File downloadLocation;

	File getHXIImportFile() const
	{
		if(getProjectType() == ProjectType::HXI)
			return File(getProperty("hxiDirectory").toString());
		else
			return downloadLocation;
	}

	ProjectType getProjectType() const
	{
		return (ProjectType)(int)getProperty("projectType");
	}

	Dialog* createDialog(State& state) override;

	var importHXIFile(State::Job& t, const var& stateObject)
	{
		auto pt = getProjectType();
		return var();
	}

	var downloadTemplate(State::Job& t, const var& stateObject)
	{
		return var();
	}

	void postInit() override
	{
		auto dir = File::getSpecialLocation(File::SpecialLocationType::userDocumentsDirectory).getChildFile("New HISE Project").getFullPathName();

		setProperty("rootDirectory", dir);
		setProperty("projectType", 0);
	}
	
};

} // namespace library
} // namespace multipage
} // namespace hise
