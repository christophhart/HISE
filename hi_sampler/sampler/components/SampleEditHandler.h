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

#ifndef SAMPLEEDITHANDLER_H_INCLUDED
#define SAMPLEEDITHANDLER_H_INCLUDED
namespace hise { using namespace juce;




class SampleEditHandler: public KeyListener
{
public:

	

	class SubEditorTraverser: public juce::KeyboardFocusTraverser
	{
	public:
		
		SubEditorTraverser(Component* sub);;
		
		Component::SafePointer<Component> component;

		Component* getNextComponent(Component* current) override
		{
			return component;
		}

		Component* getPreviousComponent(Component* current) override
		{
			return component;
		}

		Component* getDefaultComponent(Component* parentComponent) override
		{
			return component;
		}
	};



	SampleEditHandler(ModulatorSampler* sampler_);

	~SampleEditHandler()
	{
	}

	ModulatorSampler* getSampler() { return sampler; }

	void moveSamples(SamplerSoundMap::Neighbour direction);

	void resizeSamples(SamplerSoundMap::Neighbour direction);

	static void handleMidiSelection(SampleEditHandler& handler, int noteNumber, int velocity);

	struct SampleEditingActions
	{
		static void toggleFirstScriptButton(SampleEditHandler* handler);
		static void deleteSelectedSounds(SampleEditHandler *body);
		static void duplicateSelectedSounds(SampleEditHandler *body);
		static void removeDuplicateSounds(SampleEditHandler *body);
		static void removeNormalisationInfo(SampleEditHandler* body);
		static void cutSelectedSounds(SampleEditHandler *body);
		static void copySelectedSounds(SampleEditHandler *body);
		static void automapVelocity(SampleEditHandler *body);
		static void pasteSelectedSounds(SampleEditHandler *body);

		static void checkMicPositionAmountBeforePasting(const ValueTree &v, ModulatorSampler * s);

		static void refreshCrossfades(SampleEditHandler * body);
		static void selectAllSamples(SampleEditHandler * body);
		static void mergeIntoMultiSamples(SampleEditHandler * body, Component* childOfRoot);
		static void extractToSingleMicSamples(SampleEditHandler * body);
		static void normalizeSamples(SampleEditHandler *handler, Component* childOfRoot);
		static void automapUsingMetadata(ModulatorSampler* sampler);

		static bool metadataWasFound(ModulatorSampler* sampler);

		static void trimSampleStart(Component* childComponentOfMainEditor, SampleEditHandler * body);
		static void createMultimicSampleMap(SampleEditHandler* handler);
		static void deselectAllSamples(SampleEditHandler* handler);
		static void reencodeMonolith(Component* childComponentOfMainEditor, SampleEditHandler* handler);
		static void encodeAllMonoliths(Component* comp, SampleEditHandler* handler);


		static void writeSamplesWithAiffData(ModulatorSampler* sampler);


		static ModulatorSamplerSound* getNeighbourSample(SampleEditHandler* handler, SamplerSoundMap::Neighbour direction);

		static void selectNeighbourSample(SampleEditHandler* handler, SamplerSoundMap::Neighbour direction, ModifierKeys mods);
	};

	LambdaBroadcaster<ModulatorSamplerSound::Ptr, int> selectionBroadcaster;
	LambdaBroadcaster<int, int> noteBroadcaster;
	LambdaBroadcaster<int, BigInteger*> groupBroadcaster;
	LambdaBroadcaster<int> allSelectionBroadcaster;
    SamplerTools toolBroadcaster;
    
	const ModulatorSamplerSound::Ptr* begin() const
	{
		return selectedSamplerSounds.begin();
	}

	ModulatorSamplerSound::Ptr* begin()
	{
		return selectedSamplerSounds.begin();
	}

	const ModulatorSamplerSound::Ptr* end() const
	{
		return selectedSamplerSounds.end();
	}

	ModulatorSamplerSound::Ptr* end()
	{
		return selectedSamplerSounds.end();
	}

	ModulatorSamplerSound::Ptr getMainSelection() { return currentMainSound; };

	int getNumSelected() const { return selectedSamplerSounds.getNumSelected(); }

	SelectedItemSet<ModulatorSamplerSound::Ptr>& getSelectionReference()
	{
		return selectedSamplerSounds;
	}

	void cycleMainSelection(int indexToUse=-1, int currentMicIndex=-1, bool back=false);

	bool applyToMainSelection = false;

	SampleSelection getSelectionOrMainOnlyInTabMode();

	bool keyPressed(const KeyPress& key, Component* originatingComponent) override;

	void setMainSelectionToLast();

	

	void setPreviewStart(int start)
	{
		previewer.setPreviewStart(start);
	}

	void togglePreview()
	{
		previewer.previewSample(currentMainSound, currentMicIndex);
	}

	const SamplePreviewer& getPreviewer() const { return previewer; }

private:

	SamplerSoundMap::Neighbour currentDirection = SamplerSoundMap::Right;

	SamplePreviewer previewer;

	static void updateMainSound(SampleEditHandler& s, ModulatorSamplerSound::Ptr sound, int micIndex);

	ModulatorSamplerSound::Ptr currentMainSound;
	int currentMicIndex = 0;

	ModulatorSampler* sampler;

	SelectedItemSet<ModulatorSamplerSound::Ptr> selectedSamplerSounds;

	struct PrivateSelectionUpdater: public ChangeListener
	{
		PrivateSelectionUpdater(SampleEditHandler& parent_, MainController* mc);

		void changeListenerCallback(ChangeBroadcaster* );

		~PrivateSelectionUpdater()
		{
            MessageManagerLock mm;
			parent.selectedSamplerSounds.removeChangeListener(this);
		}

		SampleEditHandler& parent;

		JUCE_DECLARE_WEAK_REFERENCEABLE(PrivateSelectionUpdater);
	} internalSelectionListener;

	bool newKeysPressed(const uint8 *currentNotes);

	void changeProperty(ModulatorSamplerSound::Ptr s, const Identifier& p, int delta);;

public:
	File getCurrentSampleMapDirectory() const;

	JUCE_DECLARE_WEAK_REFERENCEABLE(SampleEditHandler);
};


} // namespace hise

#endif  // SAMPLEEDITHANDLER_H_INCLUDED
