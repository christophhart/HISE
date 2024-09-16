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

#if 0
Path MainToolbarFactory::MainToolbarPaths::createPath(int id, bool isOn)
{
	Path path;

	switch(id)
	{
		case BackendCommandTarget::HamburgerMenu:
		{
			path.loadPathFromData(BackendBinaryData::ToolbarIcons::hamburgerIcon, sizeof(BackendBinaryData::ToolbarIcons::hamburgerIcon));
			break;
		}
        case BackendCommandTarget::MenuViewShowPluginPopupPreview:
		{
		
		path.loadPathFromData (BackendBinaryData::ToolbarIcons::customInterface, sizeof (BackendBinaryData::ToolbarIcons::customInterface));

		break;
		}
	case BackendCommandTarget::ModulatorList:
		{
			path.loadPathFromData (BackendBinaryData::ToolbarIcons::modulatorList, sizeof (BackendBinaryData::ToolbarIcons::modulatorList));

		break;

		}

	case BackendCommandTarget::ViewPanel:
		{
		
		path.loadPathFromData (BackendBinaryData::ToolbarIcons::viewPanel, sizeof (BackendBinaryData::ToolbarIcons::viewPanel));
		break;
		}
	case BackendCommandTarget::Mixer:
		{
		path.loadPathFromData (BackendBinaryData::ToolbarIcons::mixer, sizeof (BackendBinaryData::ToolbarIcons::mixer));
		break;

		}
	case BackendCommandTarget::Keyboard:
		{
		path.loadPathFromData (BackendBinaryData::ToolbarIcons::keyboard, sizeof (BackendBinaryData::ToolbarIcons::keyboard));
		break;

		}
	case BackendCommandTarget::DebugPanel:
		{
		path.loadPathFromData (BackendBinaryData::ToolbarIcons::debugPanel, sizeof (BackendBinaryData::ToolbarIcons::debugPanel));
		break;
		}
	case BackendCommandTarget::Settings:
		{
			path.loadPathFromData (BackendBinaryData::ToolbarIcons::settings, sizeof (BackendBinaryData::ToolbarIcons::settings));
		break;
		}
	case BackendCommandTarget::Macros:
		{
			path.loadPathFromData (BackendBinaryData::ToolbarIcons::macros, sizeof (BackendBinaryData::ToolbarIcons::macros));
		break;
		}
	
	default: jassertfalse;
	}
	
	return path;
};
#endif

juce::Path MainToolbarFactory::createPath(const String& id) const
{
	auto url = MarkdownLink::Helpers::getSanitizedFilename(id);
	Path p;

	LOAD_EPATH_IF_URL("back", MainToolbarIcons::back);
	LOAD_EPATH_IF_URL("forward", MainToolbarIcons::forward);
    LOAD_EPATH_IF_URL("custom-popup", MainToolbarIcons::customPopup);
	LOAD_EPATH_IF_URL("keyboard", BackendBinaryData::ToolbarIcons::keyboard);
	LOAD_EPATH_IF_URL("macro-controls", HiBinaryData::SpecialSymbols::macros);
	LOAD_EPATH_IF_URL("preset-browser", MainToolbarIcons::presetBrowser);
	LOAD_EPATH_IF_URL("plugin-preview", MainToolbarIcons::home);
	LOAD_EPATH_IF_URL("main-workspace", MainToolbarIcons::mainWorkspace);
	LOAD_EPATH_IF_URL("scripting-workspace", HiBinaryData::SpecialSymbols::scriptProcessor);
	LOAD_EPATH_IF_URL("sampler-workspace", MainToolbarIcons::samplerWorkspace);
	LOAD_PATH_IF_URL("custom-workspace", ColumnIcons::customizeIcon);
	LOAD_EPATH_IF_URL("settings", MainToolbarIcons::settings);
	LOAD_EPATH_IF_URL("help", MainToolbarIcons::help);
	LOAD_EPATH_IF_URL("hise", MainToolbarIcons::hise);
	LOAD_EPATH_IF_URL("quickplay", MainToolbarIcons::quickplay);
	LOAD_EPATH_IF_URL("quicknote", MainToolbarIcons::quicknote);

	return p;

}

void ImporterBase::logStatusMessage(const String& message)
{
	debugToConsole(bpe->getBackendProcessor()->getMainSynthChain(), message);
	showStatusMessageBase(message);
}

void ImporterBase::logVerboseMessage(const String& verboseMessage)
{
	debugToConsole(bpe->getBackendProcessor()->getMainSynthChain(), verboseMessage);
}

void ImporterBase::criticalErrorOccured(const String& message)
{
	showStatusMessageBase(message);
}

void ImporterBase::createSubDirectories()
{
	showStatusMessageBase("Create subdirectories");

	auto newProjectFolder = getProjectFolder();

	auto& handler = bpe->getBackendProcessor()->getSampleManager().getProjectHandler();
	auto ids = handler.getSubDirectoryIds();

	for (auto id : ids)
	{
		auto sub = newProjectFolder.getChildFile(handler.getIdentifier(id));
		sub.createDirectory();
	}
}

void ImporterBase::extractPools()
{
	showStatusMessageBase("Extract files from Pools...");

	writePool<ValueTree>(e->pool->getSampleMapPool(), [&](File outputFile, const ValueTree& data, const var&)
	{
		showStatusMessageBase("Write samplemap to " + outputFile.getFullPathName());
		auto xml = data.createXml();
		outputFile.replaceWithText(xml->createDocument(""));
	});

	writePool<Image>(e->pool->getImagePool(), [&](File outputFile, const Image& img, const var&)
	{
		showStatusMessageBase("Write image to " + outputFile.getFullPathName());

		if (auto format = ImageFileFormat::findImageFormatForFileExtension(outputFile))
		{
			FileOutputStream fos(outputFile);
			outputFile.getParentDirectory().createDirectory();
			format->writeImageToStream(img, fos);
		}
	});

	writePool<AudioSampleBuffer>(e->pool->getAudioSampleBufferPool(), [&](File outputFile, const AudioSampleBuffer& buffer, const var& additionalData)
	{
		showStatusMessageBase("Write audio file to " + outputFile.getFullPathName());

		OwnedArray<AudioFormat> formats;

		formats.add(new WavAudioFormat());
		formats.add(new AiffAudioFormat());

#if JUCE_USE_OGGVORBIS
		formats.add(new juce::OggVorbisAudioFormat());
#endif

		for (auto af : formats)
		{
			if (af->getFileExtensions().contains(outputFile.getFileExtension()))
			{
				auto fos = new FileOutputStream(outputFile);
				outputFile.getParentDirectory().createDirectory();

				auto sr = (int)additionalData["SampleRate"];

				AudioChannelSet channels;
				ScopedPointer<AudioFormatWriter> writer = af->createWriterFor(fos, sr, buffer.getNumChannels(), 24, {}, 5);

				writer->writeFromAudioSampleBuffer(buffer, 0, buffer.getNumSamples());
				writer->flush();
				writer = nullptr;
				return;
			}
		}
	});

	writePool<MidiFileReference>(e->pool->getMidiFilePool(), [&](File output, const MidiFileReference& mf, const var& data)
	{
		showStatusMessageBase("Write MIDI file " + output.getFullPathName());

		output.deleteFile();
		FileOutputStream fos(output);
		mf.getFile().writeTo(fos);
	});
}

void ImporterBase::extractFonts()
{
	showStatusMessageBase("Extract fonts");

	auto newProjectFolder = getProjectFolder();

	auto imageRoot = newProjectFolder.getChildFile(ProjectHandler::getIdentifier(FileHandlerBase::Images));

	for (auto f : bpe->getBackendProcessor()->exportCustomFontsAsValueTree())
	{
		auto fileName = f["Name"].toString().fromFirstOccurrenceOf("}", false, false);

		auto outputFile = imageRoot.getChildFile(fileName);
		outputFile.getParentDirectory().createDirectory();

		showStatusMessageBase("Write font " + outputFile.getFullPathName());

		auto size = (int)f["Size"];
			
		auto id = f["FontId"].toString();

		if (auto data = f["Data"].getBinaryData())
		{
			outputFile.deleteFile();
			FileOutputStream fos(outputFile);
			fos.write(data->getData(), size);
			fos.flush();
		}
	}
}

void ImporterBase::extractNetworks()
{
	showStatusMessageBase("Extract networks...");

	auto newProjectFolder = getProjectFolder();

	auto networkFolder = newProjectFolder.getChildFile(ProjectHandler::getIdentifier(FileHandlerBase::DspNetworks)).getChildFile("Networks");
	networkFolder.createDirectory();

	for (auto n : e->networks)
	{

		auto xml = n.createXml();
		auto xmlText = xml->createDocument("");

		auto filename = n[scriptnode::PropertyIds::ID].toString();

		auto outputFile = networkFolder.getChildFile(filename).withFileExtension("xml");
		outputFile.getParentDirectory().createDirectory();

		showStatusMessageBase("Write DSP network " + outputFile.getFullPathName());
		outputFile.replaceWithText(xmlText);
	}
}

void ImporterBase::extractScripts()
{
	showStatusMessageBase("Extract scripts");

	auto newProjectFolder = getProjectFolder();

	auto scriptRoot = newProjectFolder.getChildFile(ProjectHandler::getIdentifier(FileHandlerBase::Scripts));

	valuetree::Helpers::forEach(e->presetToLoad, [&](ValueTree& v)
	{
		if (v.hasProperty("Script"))
		{
			auto code = v["Script"].toString();

			auto includedFiles = JavascriptProcessor::Helpers::desolveIncludeStatements(code, scriptRoot, bpe->getBackendProcessor());

			for (auto f : includedFiles)
			{
				showStatusMessageBase("Extract script " + f.f.getFullPathName());
				f.f.getParentDirectory().createDirectory();
				f.f.replaceWithText(f.content);
			}

			v.setProperty("Script", code, nullptr);
		}
		return false;
	});

		
}

void ImporterBase::createProjectSettings()
{
	auto allData = e->getValueTreeFromFile(e->getExpansionType());
		
	auto iconData = allData.getChildWithName(ExpansionIds::HeaderData).getChildWithName(ExpansionIds::Icon)[ExpansionIds::Data].toString();

	auto newProjectFolder = getProjectFolder();

	if (iconData.isNotEmpty())
	{
		showStatusMessageBase("Write Icon.png image file");
		MemoryBlock mb;
		mb.fromBase64Encoding(iconData);

		auto output = newProjectFolder.getChildFile(ProjectHandler::getIdentifier(FileHandlerBase::Images)).getChildFile("Icon.png");

		juce::PNGImageFormat format;
		auto icon = format.loadFrom(mb.getData(), mb.getSize());

		FileOutputStream fos(output);

		format.writeImageToStream(icon, fos);
	}

	showStatusMessageBase("Create project setting files");

	auto headerData = allData.getChildWithName(ExpansionIds::ExpansionInfo).createCopy();

	auto projectSettings = headerData.getChildWithName("ProjectSettings");

	if (!projectSettings.isValid())
		projectSettings = ValueTree("ProjectSettings");

	auto userSettings = headerData.getChildWithName("UserSettings");

	if (!userSettings.isValid())
		userSettings = ValueTree("UserSettings");

	std::map<Identifier, ValueTree> map;
	map[HiseSettings::SettingFiles::ProjectSettings] = projectSettings;
	map[HiseSettings::SettingFiles::UserSettings] = userSettings;
	map[HiseSettings::SettingFiles::ExpansionSettings] = ValueTree("ExpansionInfo");

	std::map<Identifier, File> fileMap;
	fileMap[HiseSettings::SettingFiles::ProjectSettings] = newProjectFolder.getChildFile("project_info.xml");
	fileMap[HiseSettings::SettingFiles::UserSettings] = newProjectFolder.getChildFile("user_info.xml");
	fileMap[HiseSettings::SettingFiles::ExpansionSettings] = newProjectFolder.getChildFile("expansion_info.xml");

	auto writeSetting = [&map](const Identifier& target, const Identifier& p, const String& value)
	{
		ValueTree c(p);
		c.setProperty("value", value, nullptr);
		map[target].addChild(c, -1, nullptr);
	};

	auto saveSetting = [&map, &fileMap](const Identifier& id)
	{
		auto xml = map[id].createXml();
		auto xmlText = xml->createDocument("");
		auto f = fileMap[id];
		f.replaceWithText(xmlText);
	};

	if (projectSettings.getNumChildren() == 0)
	{
		writeSetting(HiseSettings::SettingFiles::ProjectSettings, HiseSettings::Project::Name, headerData["Name"]);
		writeSetting(HiseSettings::SettingFiles::ProjectSettings, HiseSettings::Project::Version, headerData["Version"]);
		writeSetting(HiseSettings::SettingFiles::ProjectSettings, HiseSettings::Project::EncryptionKey, "1234");
	}
		
	if (userSettings.getNumChildren() == 0)
	{
		writeSetting(HiseSettings::SettingFiles::UserSettings, HiseSettings::User::Company, headerData["Company"]);
		writeSetting(HiseSettings::SettingFiles::UserSettings, HiseSettings::User::CompanyURL, headerData["CompanyURL"]);
	}
		
	writeSetting(HiseSettings::SettingFiles::ExpansionSettings, HiseSettings::ExpansionSettings::Description, headerData["Description"]);
	writeSetting(HiseSettings::SettingFiles::ExpansionSettings, HiseSettings::ExpansionSettings::Tags, headerData["Tags"]);
	writeSetting(HiseSettings::SettingFiles::ExpansionSettings, HiseSettings::ExpansionSettings::UUID, headerData["UUID"]);
		
	saveSetting(HiseSettings::SettingFiles::ProjectSettings);
	saveSetting(HiseSettings::SettingFiles::UserSettings);
	saveSetting(HiseSettings::SettingFiles::ExpansionSettings);
}

void ImporterBase::extractPreset()
{
	showStatusMessageBase("Extract main preset");
	auto xml = e->presetToLoad.createXml();

	auto xmlFolder = e->getRootFolder().getChildFile(e->getIdentifier(FileHandlerBase::Presets));

	FileOutputStream fos(xmlFolder.getChildFile("Preset.hip"));

	e->presetToLoad.writeToStream(fos);
}

void ImporterBase::extractUserPresets()
{
	auto allData = e->getValueTreeFromFile(e->getExpansionType());

	showStatusMessageBase("Extracting user presets...");
	e->extractUserPresetsIfEmpty(allData, true);
}

void ImporterBase::extractWebResources()
{
	for (auto id : bpe->getBackendProcessor()->getAllWebViewIds())
	{
		auto wv = bpe->getBackendProcessor()->getOrCreateWebView(id);
		auto ok = wv->explode();
		ignoreUnused(ok);
	}
		
}

void ImporterBase::extractHxi(const File& archive)
{
	auto hxiFile = getProjectFolder().getChildFile("info.hxi");

	if (archive.getFileExtension() == ".hxi")
		archive.copyFileTo(hxiFile);
	else if (archive.getFileExtension() == ".hiseproject")
	{
		FileInputStream fis(archive);

		hxiFile.deleteFile();

		FileOutputStream fos(hxiFile);

		auto numHeaderBytes = fis.readInt64();

		fos.writeFromInputStream(fis, numHeaderBytes);

		int archiveIndex = 0;

		while (!fis.isExhausted())
		{
			auto numBytes = fis.readInt64();

			FileOutputStream so(hxiFile.getSiblingFile("Samples" + String(++archiveIndex)).withFileExtension(".hr1"));
			so.writeFromInputStream(fis, numBytes);

			sampleArchives.add(so.getFile());
		}
	}
	else
	{
		ZipFile zf(archive);
		zf.uncompressTo(getProjectFolder());
	}
}

void ImporterBase::createProjectData()
{
	auto key = "1234";

	auto newProjectFolder = getProjectFolder();

	auto mc = dynamic_cast<MainController*>(bpe->getBackendProcessor());

	mc->getExpansionHandler().setExpansionType<FullInstrumentExpansion>();
	mc->getExpansionHandler().setEncryptionKey(key);
		
	e = new FullInstrumentExpansion(mc, newProjectFolder);
	createSubDirectories();
	mc->setWebViewRoot(newProjectFolder);

	ok = e->initialise();

	if (ok.failed())
		return;

	ok = e->lazyLoad();

	if (ok.failed())
		return;

	extractFonts();

	createProjectSettings();
	extractScripts();
	extractPreset();
	extractPools();
	extractNetworks();
	extractUserPresets();
	extractWebResources();

	for (auto s : sampleArchives)
	{
		showStatusMessageBase("Extract Sample Archive " + s.getFileName());

		hlac::HlacArchiver::DecompressData data;

		data.option = hlac::HlacArchiver::OverwriteOption::ForceOverwrite;

		data.supportFullDynamics = true;
		data.sourceFile = s;
		data.targetDirectory = e->getSubDirectory(FileHandlerBase::Samples);
		data.progress = &getLogData()->progress;
		data.partProgress = &(getLogData()->progress);
		data.totalProgress = &getLogData()->progress;

		hlac::HlacArchiver decompressor(getThreadToUse());

		decompressor.setListener(this);

		bool ok = decompressor.extractSampleData(data);
		ignoreUnused(ok);

	}
}
} // namespace hise
