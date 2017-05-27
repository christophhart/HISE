
void SampleEditHandler::moveSamples(SamplerSoundMap::Neighbour direction)
{
	ModulatorSampler *s = sampler;

	s->getUndoManager()->beginNewTransaction("Moving Samples");

	switch (direction)
	{
	case SamplerSoundMap::Right:
	case SamplerSoundMap::Left:
	{
		for (int i = 0; i < selectedSamplerSounds.getNumSelected(); i++)
		{
			ModulatorSamplerSound *sound = selectedSamplerSounds.getSelectedItem(i);

			if (direction == SamplerSoundMap::Right)
			{
				changeProperty(sound, ModulatorSamplerSound::KeyHigh, 1);
				changeProperty(sound, ModulatorSamplerSound::KeyLow, 1);
				changeProperty(sound, ModulatorSamplerSound::RootNote, 1);
			}
			else
			{
				changeProperty(sound, ModulatorSamplerSound::KeyLow, -1);
				changeProperty(sound, ModulatorSamplerSound::KeyHigh, -1);
				changeProperty(sound, ModulatorSamplerSound::RootNote, -1);
			}
		}
		break;
	}
	case SamplerSoundMap::Up:
	case SamplerSoundMap::Down:
	{
		for (int i = 0; i < selectedSamplerSounds.getNumSelected(); i++)
		{
			ModulatorSamplerSound *sound = selectedSamplerSounds.getSelectedItem(i);

			changeProperty(sound, ModulatorSamplerSound::VeloHigh, direction == SamplerSoundMap::Up ? 1 : -1);
			changeProperty(sound, ModulatorSamplerSound::VeloLow, direction == SamplerSoundMap::Up ? 1 : -1);
		}
		break;
	}
	}
}

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

void SampleEditHandler::handleMidiSelection()
{
	auto& x = sampler->getSamplerDisplayValues();

	if (sampler->getEditorState(ModulatorSampler::MidiSelectActive) && newKeysPressed(x.currentNotes))
	{
		selectedSamplerSounds.deselectAll();

		SelectedItemSet<const ModulatorSamplerSound*> midiSounds;

		for (int i = 0; i < 127; i++)
		{
			if (x.currentNotes[i] != 0)
			{
				const int noteNumber = i;
				const int velocity = x.currentNotes[i];

				for (int j = 0; j < sampler->getNumSounds(); j++)
				{
					if (sampler->soundCanBePlayed(sampler->getSound(j), 1, noteNumber, (float)velocity / 127.0f))
					{
						selectedSamplerSounds.addToSelection(sampler->getSound(j));
					}
				}

			}
		}
	}
}

bool SampleEditHandler::newKeysPressed(const uint8 *currentNotes)
{
	for (int i = 0; i < 127; i++)
	{
		if (currentNotes[i] != 0) return true;
	}
	return false;
}

void SampleEditHandler::changeProperty(ModulatorSamplerSound *s, ModulatorSamplerSound::Property p, int delta)
{
	const int v = s->getProperty(p);

	s->setPropertyWithUndo(p, v + delta);
}
