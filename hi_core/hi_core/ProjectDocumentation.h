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

namespace hise { using namespace juce;


/** A class that manages the content for the project documentation.

	In HISE it will use the files in the Documentation folder, but on compiled projects it will use the
	Documentation.dat file in the projects app data folder. */
class ProjectDocDatabaseHolder : public MarkdownDatabaseHolder,
								 public ControlledObject
{
public:

	ProjectDocDatabaseHolder(MainController* mc):
		ControlledObject(mc)
	{
		getDatabase().createFooter = false;
	}

	void registerContentProcessor(MarkdownContentProcessor* processor) override;
	void registerItemGenerators() override;
	File getCachedDocFolder() const override;
	File getDatabaseRootDirectory() const override;
	bool shouldUseCachedData() const override;

	void setProjectURL(URL newProjectURL);

	URL getBaseURL() const override
	{
		return projectURL;
	}

private:
	URL projectURL;
};


} // namespace hise

