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



namespace hise {
using namespace juce;



void ExpansionHandler::Helpers::createFrontendLayoutWithExpansionEditing(FloatingTile* root)
{
#if HI_ENABLE_EXPANSION_EDITING
	FloatingInterfaceBuilder ib(root);

	ib.setNewContentType<FloatingTabComponent>(0);

	int mainTabs = 0;

	auto interface = ib.addChild<InterfaceContentPanel>(mainTabs);
	auto expansionRoot = ib.addChild<HorizontalTile>(mainTabs);

	auto topBar = ib.addChild<VerticalTile>(expansionRoot);

	ib.addChild<ExpansionEditBar>(topBar);
	ib.addChild<TooltipPanel>(topBar);

	ib.getPanel(topBar)->getLayoutData().setCurrentSize(28.0);

	ib.setDynamic(topBar, false);
	ib.setFoldable(topBar, false, { false, false });
	ib.setSizes(topBar, { -0.5, 300.0 });

	auto connector = ib.addChild<GlobalConnectorPanel<ModulatorSampler>>(expansionRoot);
	auto editor = ib.addChild<SampleEditorPanel>(expansionRoot);
	auto mapEditor = ib.addChild<SampleMapEditorPanel>(expansionRoot);
	auto keyboard = ib.addChild<MidiKeyboardPanel>(expansionRoot);

	ib.getContent(editor)->setStyleProperty("showConnectionBar", false);
	ib.getContent(mapEditor)->setStyleProperty("showConnectionBar", false);

	ib.setCustomName(interface, "Interface");
	ib.setCustomName(expansionRoot, "Expansion Editing");

	auto c = ib.getContent<GlobalConnectorPanel<ModulatorSampler>>(connector);

	if (auto s = ProcessorHelpers::getFirstProcessorWithType<ModulatorSampler>(root->getMainController()->getMainSynthChain()))
	{
		c->setContentWithUndo(s, 0);
	}
	

	ib.getPanel(keyboard)->setCanBeFolded(true);

	ib.finalizeAndReturnRoot();

	ib.setDynamic(mainTabs, false);
	ib.setDynamic(expansionRoot, false);
#else
	root->setNewContent(GET_PANEL_NAME(InterfaceContentPanel));
#endif
}

bool ExpansionHandler::Helpers::isValidExpansion(const File& directory)
{
	return Expansion::Helpers::getExpansionInfoFile(directory, false).existsAsFile() ||
		   Expansion::Helpers::getExpansionInfoFile(directory, true).existsAsFile();
}

juce::Identifier ExpansionHandler::Helpers::getSanitizedIdentifier(const String& idToSanitize)
{
	String s = idToSanitize.replaceCharacters(".:/\\ ()", "_______");

	return Identifier(s);
}

ExpansionHandler::ExpansionHandler(MainController* mc_):
	mc(mc_),
	notifier(*this)
{
	createAvailableExpansions();
}

void ExpansionHandler::createNewExpansion(const File& expansionFolder)
{
	if (Helpers::isValidExpansion(expansionFolder))
		return;

	expansionList.add(new Expansion(mc, expansionFolder));
	notifier.sendNotification(Notifier::EventType::ExpansionCreated);
}

juce::File ExpansionHandler::getExpansionFolder() const
{
	auto f = mc->getSampleManager().getProjectHandler().getRootFolder().getChildFile("Expansions");

	if (!f.isDirectory())
		f.createDirectory();

	return f;
}

void ExpansionHandler::createAvailableExpansions()
{
	Array<File> folders;

	getExpansionFolder().findChildFiles(folders, File::findDirectories, false);

	for (auto f : folders)
	{
		if (Helpers::isValidExpansion(f))
		{
			expansionList.add(new Expansion(mc, f));
		}
	}

	notifier.sendNotification(Notifier::EventType::ExpansionCreated);
}

var ExpansionHandler::getListOfAvailableExpansions() const
{
	Array<var> ar;

	for (auto e : expansionList)
	{
		ar.add(e->name.get());
	}

	return var(ar);
}

bool ExpansionHandler::setCurrentExpansion(const String& expansionName)
{
	currentExpansion = nullptr;

	if (expansionName.isEmpty())
	{
		notifier.sendNotification(Notifier::EventType::ExpansionLoaded);
		return true;
	}

	for (auto e : expansionList)
	{
		if (e->name == expansionName)
		{
			currentExpansion = e;

			notifier.sendNotification(Notifier::EventType::ExpansionLoaded);

			return true;
		}
	}

	return false;
}

PooledAudioFile ExpansionHandler::loadAudioFileReference(const PoolReference& sampleId)
{
	AudioSampleBufferPool* pool = nullptr;
	getPoolForReferenceString(sampleId, &pool);
	return pool->loadFromReference(sampleId, PoolHelpers::LoadAndCacheWeak);
}

double ExpansionHandler::getSampleRateForFileReference(const PoolReference& sampleId)
{
	return (double)getMetadata(sampleId).getProperty(MetadataIDs::SampleRate, 0.0);
}

const var ExpansionHandler::getMetadata(const PoolReference& sampleId)
{
	AudioSampleBufferPool* pool = nullptr;
	getPoolForReferenceString(sampleId, &pool);

	return pool->getAdditionalData(sampleId);
}

hise::PooledImage ExpansionHandler::loadImageReference(const PoolReference& imageId, PoolHelpers::LoadingType loadingType /*= PoolHelpers::LoadAndCacheWeak*/)
{
	ImagePool* pool = nullptr;
	getPoolForReferenceString(imageId, &pool);
	return pool->loadFromReference(imageId, loadingType);
}

hise::PooledSampleMap ExpansionHandler::loadSampleMap(const PoolReference& sampleMapId)
{
	SampleMapPool* pool = nullptr;
	getPoolForReferenceString(sampleMapId, &pool);
	return pool->loadFromReference(sampleMapId, PoolHelpers::LoadAndCacheWeak);
}

hise::Expansion* ExpansionHandler::getExpansionForWildcardReference(const String& poolReferenceString) const
{
	auto id = Expansion::Helpers::getExpansionIdFromReference(poolReferenceString);

	if (id.isNotEmpty())
	{
		for (auto e : expansionList)
		{
			if (e->name == id)
				return e;
		}

		throw String(id + " was not found");
	}

	return nullptr;
}

void ExpansionHandler::clearPools()
{
	for (auto e : expansionList)
	{
		e->pool->clear();
	}
		
}

PoolCollection* ExpansionHandler::getCurrentPoolCollection()
    {
        return mc->getCurrentFileHandler().pool;
    }
    
var Expansion::getSampleMapList()
{
	if (isEncrypted)
	{
		jassertfalse;
		return var();
	}
	else
	{
		return getFileListInternal(ProjectHandler::SubDirectories::SampleMaps, "*.xml", false);
	}
}

var Expansion::getImageList()
{
	if (isEncrypted)
	{
		jassertfalse;
		return var();
	}
	else
	{
		return getFileListInternal(ProjectHandler::SubDirectories::Images, "*.png;*.jpg;*.PNG;*.JPG", true);
	}
}

var Expansion::getAudioFileList()
{
	if (isEncrypted)
	{
		jassertfalse;
		return var();
	}
	else
	{
		return getFileListInternal(ProjectHandler::SubDirectories::AudioFiles, afm.getWildcardForAllFormats(), true);
	}
}

var Expansion::getFileListInternal(ProjectHandler::SubDirectories type, const String& wildcards, bool addExtension)
{
	Array<var> list;

	Array<File> childFiles;

	auto subRoot = getSubDirectory(type);

	subRoot.findChildFiles(childFiles, File::findFiles, true, wildcards);

	childFiles.sort();

	for (int i = 0; i < childFiles.size(); i++)
	{
		auto n = childFiles[i].getRelativePathFrom(subRoot);
		
		n = n.replace(File::getSeparatorString(), "/");
		
		if (!addExtension)
		{
			n = n.upToLastOccurrenceOf(".", false, true);
		}
		
		list.add(n);
	}

	return var(list);
}



PooledAudioFile Expansion::loadAudioFile(const PoolReference& audioFileId)
{
	jassert(Helpers::getExpansionIdFromReference(audioFileId.getReferenceString()).isNotEmpty());

	return pool->getAudioSampleBufferPool().loadFromReference(audioFileId, PoolHelpers::LoadAndCacheWeak);
}

PooledImage Expansion::loadImageFile(const PoolReference& imageId)
{
	jassert(Helpers::getExpansionIdFromReference(imageId.getReferenceString()).isNotEmpty());

	return pool->getImagePool().loadFromReference(imageId, PoolHelpers::LoadAndCacheWeak);
}

juce::String Expansion::getSampleMapReference(const String& sampleMapId)
{
	if (isEncrypted)
	{
		jassertfalse;
		return {};
	}
	else
	{
		Array<File> sampleMapFiles;
		getSubDirectory(ProjectHandler::SubDirectories::SampleMaps).findChildFiles(sampleMapFiles, File::findFiles, true, "*.xml");

		String expStart = "{EXP::" + name.get() + "}";

		Array<var> list;

		for (auto f : sampleMapFiles)
		{
			auto thisId = f.getFileNameWithoutExtension();

			if (thisId == sampleMapId)
				return expStart + thisId;
		}

		return {};
	}
}

juce::ValueTree Expansion::getSampleMap(const String& sampleMapId)
{
	if (isEncrypted)
	{
		jassertfalse;
		return ValueTree();
	}
	else
	{
		Array<File> sampleMapFiles;
		getSubDirectory(ProjectHandler::SubDirectories::SampleMaps).findChildFiles(sampleMapFiles, File::findFiles, true, "*.xml");

		String expStart = "{EXP::" + name.get() + "}";

		for (auto f : sampleMapFiles)
		{
			String thisId = expStart + f.getFileNameWithoutExtension();

			if (thisId == sampleMapId)
			{
				ScopedPointer<XmlElement> xml = XmlDocument::parse(f);

				if (xml != nullptr)
				{
					return ValueTree::fromXml(*xml);
				}
			}
		}

		throw String("!Sample Map with id " + sampleMapId.fromFirstOccurrenceOf("}", false, false) + " not found");
	}
}

juce::String Expansion::Helpers::getExpansionIdFromReference(const String& referenceId)
{
	

	static const String expStart = "{EXP::";

	if (!referenceId.startsWith(expStart))
		return {};

	static const String regexp = R"(^\{EXP::(.*)\})";

	auto matches = RegexFunctions::getFirstMatch(regexp, referenceId);

	if (matches.size() == 2)
	{
		return matches[1];
	}

	return {};
}

}