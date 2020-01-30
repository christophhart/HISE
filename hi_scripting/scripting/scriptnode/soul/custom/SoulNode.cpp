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
 *   which also must be licenced for commercial applications:
 *
 *   http://www.juce.com
 *
 *   ===========================================================================
 */

namespace scriptnode
{
using namespace juce;

juce::File SoulNode::getCacheFolder(MainController* mc)
{
	auto cacheFolder = mc->getCurrentFileHandler().getSubDirectory(FileHandlerBase::Binaries).getChildFile("soul_cache");

	if (!cacheFolder.isDirectory())
		cacheFolder.createDirectory();

	return cacheFolder;
}

#if 0
NodeComponent* SoulNode::createComponent()
{
	return new SoulNodeComponent(this);
}
#endif

SoulNode::SoulNode(DspNetwork* rootNetwork, ValueTree data) :
	WrapperNode(rootNetwork, data),
	compilerCache(new soul::patch::CompilerCacheFolder(getCacheFolder(rootNetwork->getScriptProcessor()->getMainController_()), 32)),
	debugHandler(new DebugHandler(*this)),
	codePath(PropertyIds::SoulPatch, "{PROJECT_FOLDER}soul/patch.soulpatch")
{
	codePath.init(this, nullptr);
	codePath.setAdditionalCallback([this](const Identifier& id, const var& newValue)
	{
		rebuild();
	});

	rebuild();
}

void SoulNode::logMessage(const String& s)
{

}

void SoulNode::rebuild()
{
	try
	{
		ok = false;

		auto desktop = File::getSpecialLocation(File::userDesktopDirectory);
		auto expectedLocation = desktop.getChildFile(soul::patch::SOULPatchLibrary::getLibraryFileName()).getFullPathName();

		library->ensureLibraryLoaded(expectedLocation);

		if (library->library == nullptr)
			throw String("Can't find Soul DLL at " + expectedLocation);

		auto mc = getScriptProcessor()->getMainController_();
		PoolReference ref(mc, codePath.getValue(), FileHandlerBase::AdditionalSourceCode);
		auto vptr = new VirtualHiseFile(mc, ref);
		
		currentFile = soul::patch::VirtualFile::Ptr(vptr);
		currentFileName = currentFile->getName().toString<String>();

		if (!vptr->isValid())
			throw String("Can't find soul patch " + currentFileName);
			
		patch = library->createInstance(currentFile);

		auto desc = patch->getDescription();

		if (patch == nullptr)
			throw String("Can't create SOUL patch from " + currentFileName);

		SimpleReadWriteLock::ScopedWriteLock sl(compileLock);

		patchPlayer = patch->compileNewPlayer(lastConfig, compilerCache.get(), nullptr, nullptr, debugHandler.get());

		if (patchPlayer == nullptr)
			throw String("Can't create SOUL patch player from " + currentFileName);

		auto compileMessages = patchPlayer->getCompileMessages();

		for (auto& m : compileMessages)
		{
			if (m.isError)
				logError(m.fullMessage);
		}

		int numInputChannelsInNewPatch = 0;
		int numOutputChannelsInNewPatch = 0;

		for (auto b : patchPlayer->getInputBuses())
			numInputChannelsInNewPatch += b.numChannels;

		for (auto b : patchPlayer->getOutputBuses())
			numOutputChannelsInNewPatch += b.numChannels;

		if(channelAmount != numOutputChannelsInNewPatch)
			
		{
			String s;

			s << "Channel mismatch: " << numInputChannelsInNewPatch << "/" << numOutputChannelsInNewPatch << ". Expected: " << channelAmount << "/" << channelAmount;

			throw s;
		}

		if (!patchPlayer->isPlayable())
			throw String("Can't play soul patch " + currentFileName);

		DynamicObject::Ptr oldValues = new DynamicObject();

		for (int i = 0; i < getNumParameters(); i++)
			oldValues->setProperty(Identifier(getParameter(i)->getId()), getParameter(i)->getValue());

		auto pTree = getParameterTree();
		pTree.getParent().removeChild(pTree, getUndoManager());
		while (getNumParameters() > 0)
			removeParameter(0);

		for (auto p : patchPlayer->getParameters())
		{
			ValueTree nt(PropertyIds::Parameter);

			NormalisableRange<double> d;
			d.start = p->minValue;
			d.end = p->maxValue;
			d.interval = p->step;
			
			auto pName = p->name.toString<String>();

			nt.setProperty(PropertyIds::ID, pName, nullptr);

			RangeHelpers::storeDoubleRange(nt, false, d, nullptr);

			auto hasValue = oldValues->hasProperty(Identifier(pName));

			nt.setProperty(PropertyIds::Value, hasValue ? oldValues->getProperty(pName) : p->initialValue, nullptr);

			getParameterTree().addChild(nt, -1, getUndoManager());

			auto np = new Parameter(this, nt);

			auto f = [p](double v)
			{
				p->setValue(v);
			};

			np->setCallback(f);

			addParameter(np);
		}

		ok = true;
	}
	catch (String& s)
	{
		ok = false;
		logError(s);
	}
	catch (...)
	{
		jassertfalse;
	}
}

#if 0
soul::patch::VirtualFile::Ptr SoulNode::getExternalFile(const char* externalVariableName)
{
	auto mc = getScriptProcessor()->getMainController_();
	PoolReference ref(mc, String(externalVariableName), FileHandlerBase::AudioFiles);
	soul::patch::VirtualFile::Ptr ptr(new VirtualHiseFile(mc, ref));
}
#endif

void SoulNode::logError(const String& s)
{
	debugError(dynamic_cast<Processor*>(getScriptProcessor()), s);
}

}


