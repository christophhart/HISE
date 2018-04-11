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

BackendProcessor::BackendProcessor(AudioDeviceManager *deviceManager_/*=nullptr*/, AudioProcessorPlayer *callback_/*=nullptr*/) :
MainController(),
AudioProcessorDriver(deviceManager_, callback_),
viewUndoManager(new UndoManager())
{
	
	ExtendedApiDocumentation::init();

    synthChain = new ModulatorSynthChain(this, "Master Chain", NUM_POLYPHONIC_VOICES, viewUndoManager);

    
	synthChain->addProcessorsWhenEmpty();

	getSampleManager().getModulatorSamplerSoundPool()->setDebugProcessor(synthChain);

	getMacroManager().setMacroChain(synthChain);

	

	handleEditorData(false);

	restoreGlobalSettings(this);

	initData(this);

	getAutoSaver().updateAutosaving();


	createUserPresetData();
}


BackendProcessor::~BackendProcessor()
{
	clearPreset();

	synthChain = nullptr;

	handleEditorData(true);
}



void BackendProcessor::processBlock(AudioSampleBuffer& buffer, MidiBuffer& midiMessages)
{
	getDelayedRenderer().processWrapped(buffer, midiMessages);
};

void BackendProcessor::handleControllersForMacroKnobs(const MidiBuffer &/*midiMessages*/)
{
	
}


void BackendProcessor::prepareToPlay(double newSampleRate, int samplesPerBlock)
{
	setRateAndBufferSizeDetails(newSampleRate, samplesPerBlock);
 
	getDelayedRenderer().prepareToPlayWrapped(newSampleRate, samplesPerBlock);
};

void BackendProcessor::getStateInformation(MemoryBlock &destData)
{
	MemoryOutputStream output(destData, false);

	ValueTree v = synthChain->exportAsValueTree();

	v.setProperty("ProjectRootFolder", GET_PROJECT_HANDLER(synthChain).getWorkDirectory().getFullPathName(), nullptr);

	if (auto root = dynamic_cast<BackendRootWindow*>(getActiveEditor()))
	{
		root->saveInterfaceData();
	}

	v.setProperty("InterfaceData", JSON::toString(editorInformation, true), nullptr);

	v.writeToStream(output);
}

void BackendProcessor::setStateInformation(const void *data, int sizeInBytes)
{
	tempLoadingData.setSize(sizeInBytes);

	tempLoadingData.copyFrom(data, 0, sizeInBytes);

	

	auto f = [](Processor* p)
	{
		auto bp = dynamic_cast<BackendProcessor*>(p->getMainController());

		auto& tmp = bp->tempLoadingData;

		ValueTree v = ValueTree::readFromData(tmp.getData(), tmp.getSize());

		String fileName = v.getProperty("ProjectRootFolder", String());

		if (fileName.isNotEmpty())
		{
			File root(fileName);
			if (root.exists() && root.isDirectory())
			{
				GET_PROJECT_HANDLER(p).setWorkingProject(root, nullptr);

				bp->getSettingsObject().refreshProjectData();

			}
		}

		p->getMainController()->loadPresetFromValueTree(v);

		

		bp->editorInformation = JSON::parse(v.getProperty("InterfaceData", ""));

		tmp.reset();

		return true;
	};

	getKillStateHandler().killVoicesAndCall(getMainSynthChain(), f, MainController::KillStateHandler::SampleLoadingThread);
}

AudioProcessorEditor* BackendProcessor::createEditor()
{
	return new BackendRootWindow(this, editorInformation);
}

void BackendProcessor::setEditorData(var editorState)
{
	editorInformation = editorState;
}

} // namespace hise