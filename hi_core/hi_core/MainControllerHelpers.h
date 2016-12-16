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
*   Commercial licences for using HISE in an closed source project are
*   available on request. Please visit the project's website to get more
*   information about commercial licencing:
*
*   http://www.hartinstruments.net/hise/
*
*   HISE is based on the JUCE library,
*   which also must be licenced for commercial applications:
*
*   http://www.juce.com
*
*   ===========================================================================
*/


#ifndef MAINCONTROLLERHELPERS_H_INCLUDED
#define MAINCONTROLLERHELPERS_H_INCLUDED


// ====================================================================================================
// Extern class definitions

class PluginParameterModulator;
class PluginParameterAudioProcessor;
class ControlledObject;
class Processor;
class Console;
class ModulatorSamplerSound;
class ModulatorSamplerSoundPool;
class AudioSampleBufferPool;
class Plotter;
class ScriptWatchTable;
class ScriptComponentEditPanel;
class JavascriptMidiProcessor;
class Modulator;
class CustomKeyboardState;
class ModulatorSynthChain;
class FactoryType;

class MainController;


/** A base class for all objects that need access to a MainController.
*	@ingroup core
*
*	If you want to have access to the main controller object, derive the class from this object and pass a pointer to the MainController
*	instance in the constructor.
*/
class ControlledObject
{
public:

	/** Creates a new ControlledObject. The MainController must be supplied. */
	ControlledObject(MainController *m);

	virtual ~ControlledObject();

	/** Provides read-only access to the main controller. */
	const MainController *getMainController() const noexcept
	{
		jassert(controller != nullptr);
		return controller;
	};

	/** Provides write access to the main controller. Use this if you want to make changes. */
	MainController *getMainController() noexcept
	{
		jassert(controller != nullptr);
		return controller;
	}

private:

	friend class WeakReference<ControlledObject>;
	WeakReference<ControlledObject>::Master masterReference;

	MainController* const controller;

	friend class MainController;
	friend class ProcessorFactory;
};


/** This handles the MIDI automation for the frontend plugin.
*
*	For faster performance, one CC value can only control one parameter.
*
*/
class MidiControllerAutomationHandler : public RestorableObject
{
public:

	MidiControllerAutomationHandler(MainController *mc_);

	void addMidiControlledParameter(Processor *interfaceProcessor, int attributeIndex, NormalisableRange<double> parameterRange, int macroIndex);
	void removeMidiControlledParameter(Processor *interfaceProcessor, int attributeIndex);

	bool isLearningActive() const;

	ValueTree exportAsValueTree() const override;
	void restoreFromValueTree(const ValueTree &v) override;

	bool isLearningActive(Processor *interfaceProcessor, int attributeIndex) const;
	void deactivateMidiLearning();

	void setUnlearndedMidiControlNumber(int ccNumber);
	int getMidiControllerNumber(Processor *interfaceProcessor, int attributeIndex) const;

	void refreshAnyUsedState();
	void clear();

	/** The main routine. Call this for every MidiBuffer you want to process and it handles both setting parameters as well as MIDI learning. */
	void handleParameterData(MidiBuffer &b);

	struct AutomationData
	{
		AutomationData();

		WeakReference<Processor> processor;
		int attribute;
		NormalisableRange<double> parameterRange;
		int macroIndex;
		bool used;
	};


private:

	// ========================================================================================================

	CriticalSection lock;

	MainController *mc;
	bool anyUsed;
	MidiBuffer tempBuffer;

	AutomationData automationData[128];
	AutomationData unlearnedData;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MidiControllerAutomationHandler)

	// ========================================================================================================
};


class ConsoleLogger : public Logger
{
public:

	ConsoleLogger(Processor *p) :
		processor(p)
	{};

	void logMessage(const String &message) override;

private:

	Processor *processor;

};



#endif  // MAINCONTROLLERHELPERS_H_INCLUDED
