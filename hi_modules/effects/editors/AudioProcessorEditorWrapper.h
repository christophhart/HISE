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

#ifndef AUDIOPROCESSOREDITORWRAPPER_H_INCLUDED
#define AUDIOPROCESSOREDITORWRAPPER_H_INCLUDED




class AudioProcessorEditorWrapper : public ProcessorEditorBody
{
public:

	class Content: public Component,
				   public ComboBox::Listener
	{
	public:

		Content(AudioProcessorWrapper *wrapper);

		~Content()
		{
			wrappedEditor = nullptr;

			if(getWrapper() != nullptr) getWrapper()->removeEditor(this);

			wrapper = nullptr;
		}

		void comboBoxChanged(ComboBox *c) override;

		void setAudioProcessor(AudioProcessor *newProcessor);

		void paint(Graphics &g) override
		{
			g.setColour(Colours::white.withAlpha(0.1f));
			g.drawRect(getLocalBounds(), 1);

			if (unconnected)
			{
				g.setColour(Colours::white);
				g.setFont(GLOBAL_BOLD_FONT());
				g.drawText("Unconnected", getLocalBounds(), Justification::centred);
			}
		}

		void resized()
		{
			if (wrappedEditor != nullptr)
			{
				wrappedEditor->setBounds((getWidth() - wrappedEditor->getWidth()) / 2, 5, wrappedEditor->getWidth(), wrappedEditor->getHeight());
			}
			else if (getWrapper() != nullptr)
			{
				registeredProcessorList->setBounds((getWidth() - registeredProcessorList->getWidth()) / 2, 0, 150, 30);
			}
			
		}

		void setUnconnected()
		{
			unconnected = true;

			wrappedEditor = nullptr;
			wrapper = nullptr;

			registeredProcessorList->setVisible(false);

			repaint();
		}

	private:

		bool unconnected;

		AudioProcessorWrapper *getWrapper() { return dynamic_cast<AudioProcessorWrapper*>(wrapper.get()); };

		const AudioProcessorWrapper *getWrapper() const { return dynamic_cast<AudioProcessorWrapper*>(wrapper.get()); };

		KnobLookAndFeel laf;

		friend class AudioProcessorEditorWrapper;

		ScopedPointer<AudioProcessorEditor> wrappedEditor;

		ScopedPointer<ComboBox> registeredProcessorList;

		WeakReference<Processor> wrapper;
	};

	AudioProcessorEditorWrapper(ProcessorEditor *p);;

	~AudioProcessorEditorWrapper();

	void updateGui()
	{

	};

	int getBodyHeight() const override
	{
		if (content->wrappedEditor != nullptr)
		{
			return content->wrappedEditor->getHeight() + 10;
		}
		else
		{
			return 30;
		}
	}

	

	void setAudioProcessor(AudioProcessor *newProcessor)
	{
		content->setAudioProcessor(newProcessor);
		
	}

	void resized()
	{
		content->setBounds(getLocalBounds());
	}

private:

	ScopedPointer<Content> content;
};



#endif  // AUDIOPROCESSOREDITORWRAPPER_H_INCLUDED
