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


namespace hise {
namespace multipage {
namespace library {
using namespace juce;
struct CreateCSSFromTemplate: public HardcodedDialogWithState
{
    var createFile(State::Job& t, const var& state);
    CreateCSSFromTemplate() { setSize(700, 330); };
    
    Dialog* createDialog(State& state) override;
    
};
} // namespace library
} // namespace multipage
} // namespace hise



namespace hise {
namespace multipage {
namespace library {
using namespace juce;
struct ProjectExporter: public HardcodedDialogWithState
{
    var exportProjucerProject(State::Job& t, const var& state);
    ProjectExporter(File& rootDir_, State& appState_):
      rootDir(rootDir_),
      appState(appState_)
    {
        //closeFunction = BIND_MEMBER_FUNCTION_0(ProjectExporter::destroy);
        setSize(700, 400);
        
        ScopedSetting ss;

    }
    
    void postInit() override
    {
        ScopedSetting ss;
        setProperty("hisePath", ss.get("hisePath", ""));
        setProperty("teamID", ss.get("teamID"));
    }
    
    Dialog* createDialog(State& state) override;
    
    File rootDir;
    State& appState;
    
};
} // namespace library
} // namespace multipage
} // namespace hise



namespace hise {
namespace multipage {
namespace library {
using namespace juce;
struct AudioFolderCompressor: public HardcodedDialogWithState
{
    var createFolder(State::Job& t, const var& state);
    AudioFolderCompressor()
    {
        setOnCloseFunction([](){});
        setSize(700, 550);
    }
    
    Dialog* createDialog(State& state) override;
    
};
} // namespace library
} // namespace multipage
} // namespace hise


namespace hise {
namespace multipage {
namespace library {
using namespace juce;
struct ExportMonolithPayload: public HardcodedDialogWithState
{
	var exportMonolith(State::Job& t, const var& state);

	ExportMonolithPayload(State& s):
	  stateToExport(s)
	{
		setOnCloseFunction([](){});
		setSize(700, 500);
	}

	State& stateToExport;

	Dialog* createDialog(State& state) override;
	
};
} // namespace library
} // namespace multipage
} // namespace hise