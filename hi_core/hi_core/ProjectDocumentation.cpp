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

namespace hise { using namespace juce;


void ProjectDocDatabaseHolder::registerContentProcessor(MarkdownContentProcessor* c)
{
	c->addLinkResolver(new MarkdownParser::DefaultLinkResolver(nullptr));

	if (shouldUseCachedData())
	{
		auto markdownRoot = c->getHolder().getCachedDocFolder();

		c->addLinkResolver(new DatabaseCrawler::Resolver(markdownRoot));
		c->addImageProvider(new DatabaseCrawler::Provider(markdownRoot, nullptr));
	}
	else
	{
		auto markdownRoot = c->getHolder().getDatabaseRootDirectory();

		c->addLinkResolver(new MarkdownParser::FileLinkResolver(markdownRoot));
		c->addImageProvider(new MarkdownParser::FileBasedImageProvider(nullptr, markdownRoot));
		c->addImageProvider(new MarkdownParser::URLImageProvider(markdownRoot.getChildFile("images/web/"), nullptr));
	}
}

void ProjectDocDatabaseHolder::registerItemGenerators()
{
	if (shouldUseCachedData())
	{
		return;
	}

	auto markdownRoot = getDatabaseRootDirectory();
	addItemGenerator(new MarkdownDataBase::DirectoryItemGenerator(markdownRoot, Colours::white));
}

juce::File ProjectDocDatabaseHolder::getCachedDocFolder() const
{
#if USE_BACKEND
	auto& settings = dynamic_cast<const GlobalSettingManager*>(getMainController())->getSettingsObject();

	String p = settings.getSetting(HiseSettings::User::Company).toString();
	
	p << "/" << settings.getSetting(HiseSettings::Project::Name).toString();

	auto appData = ProjectHandler::getAppDataDirectory(nullptr).getParentDirectory();
	auto projectDir = appData.getChildFile(p);
	return projectDir.getChildFile("Documentation");

#else
	return FrontendHandler::getAppDataDirectory().getChildFile("Documentation");
#endif
}

juce::File ProjectDocDatabaseHolder::getDatabaseRootDirectory() const
{
#if USE_BACKEND
	auto& handler = GET_PROJECT_HANDLER(getMainController()->getMainSynthChain());
	return handler.getSubDirectory(FileHandlerBase::Documentation);
#else

	return getCachedDocFolder();

#endif
}

bool ProjectDocDatabaseHolder::shouldUseCachedData() const
{
#if USE_BACKEND
	return false;
#else
	return true;
#endif
}

void ProjectDocDatabaseHolder::setProjectURL(URL newProjectURL)
{
	if (projectURL != newProjectURL)
	{
		projectURL = newProjectURL;
		DocUpdater::runSilently(*this);
	}
}

} // namespace hise
