/*
  ==============================================================================

    This file was auto-generated!

  ==============================================================================
*/

#ifndef MAINCOMPONENT_H_INCLUDED
#define MAINCOMPONENT_H_INCLUDED



#include "../JuceLibraryCode/JuceHeader.h"

#include "../../../hi_native_jit/hi_native_jit_public.h"


class BufferDisplay : public Component
{
public:

	enum SignalType
	{
		Noise = 1,
		Static,
		Sine,
		Saw,
		Triangle,
		Square,
		DiracTrain,
		numSignalTypes
	};

	BufferDisplay(const String& name_):
		name(name_)
	{
		buffer.setSize(1, 0);
		p.clear();
	}

	void setBuffer(AudioSampleBuffer& bufferToDisplay)
	{
		buffer = bufferToDisplay;
		resetPath();
	}

	void setDataToDisplay(const float* data, int size)
	{
		ScopedLock sl(lock);

		buffer.setSize(1, size);

		FloatVectorOperations::copy(buffer.getWritePointer(0), data, size);

		resetPath();
	}

	void paint(Graphics& g) override
	{
		g.fillAll(Colours::lightgrey);

		ScopedLock sl(lock);

		g.setColour(Colours::darkgrey);

		float min = p.getBounds().getY();
		float max = p.getBounds().getHeight() + min;

		if (p.getBounds().getHeight() == 0.0f)
		{
			return;
		}

		p.scaleToFit(0.0f, 0.0f, (float)getWidth(), (float)getHeight(), false);

		g.strokePath(p, PathStrokeType(1.0f));

		g.drawRect(getLocalBounds(), 1);

		g.setColour(Colours::grey);

		g.fillPath(p);

		g.setColour(Colours::black);

		g.drawText(name, 5, 0, getWidth(), 15, Justification::centredLeft);
		g.drawText(String(max, 2), 0, 0, getWidth() - 5, 20, Justification::centredRight);
		g.drawText(String(min, 2), 0, getHeight() - 20, getWidth() - 5, 20, Justification::centredRight);
	}

	void resized() override
	{
		resetPath();
	}

	static void fillBufferWithTestSignal(AudioSampleBuffer& b, SignalType t, double freqMultiplier=1.0)
	{
		float* d = b.getWritePointer(0);
		int size = b.getNumSamples();

		switch (t)
		{
		case BufferDisplay::Noise:
		{
			Random r;
			r.setSeedRandomly();

			for (int i = 0; i < size; i++)
			{
				d[i] = r.nextFloat() * 2.0f - 1.0f;
			}
			break;
		}
		case BufferDisplay::Static:
		{
			for (int i = 0; i < size; i++)
			{
				d[i] = 1.0f;
			}
			break;
		}
		case BufferDisplay::Sine:
		{
			double uptime = 0.0;
			double uptimeDelta = freqMultiplier * double_Pi * 2.0 / (double)(size);

			for (int i = 0; i < size; i++)
			{
				d[i] = (float)sin(uptime);
				uptime += uptimeDelta;
			}

			break;
		}

		case BufferDisplay::Saw:
		{
			float step = (float)freqMultiplier * 1.0f / (size);

			for (int i = 0; i < size; i++)
			{
				const float value = (float)i * step;
				const float modValue = value - floor(value);

				d[i] = modValue * 2.0f - 1.0f;
			}

			break;
		}
		case BufferDisplay::DiracTrain:
		{
			FloatVectorOperations::clear(d, size);

			float step = (float)size / (float)freqMultiplier;

			for (float i = 0; i < size; i += step)
			{
				d[(int)i] = 1.0f;
			}

			break;
		}

		case BufferDisplay::Triangle:
			break;
		case BufferDisplay::Square:
		{
			float step = (float)freqMultiplier * 1.0f / (size);

			for (int i = 0; i < size; i++)
			{
				const float value = (float)i * step;
				const float modValue = value - floor(value);

				d[i] = modValue > 0.5f ? -1.0f : 1.0f;
			}


			break;
		}
		case BufferDisplay::numSignalTypes:
			break;
		default:
			break;
		}
	}

	void setIsFFT()
	{
		isFFT = true;
	}

private:

	bool isFFT = false;

	CriticalSection lock;

	void resetPath()
	{
		p.clear();

		if (buffer.getNumSamples() == 0)
			return;

		ScopedLock sl(lock);

		p.startNewSubPath(0.0f, 0.0f);

		if (!isFFT)
		{
			p.lineTo(0.0f, -1.0f);
			p.lineTo(0.0f, 1.0f);
			p.lineTo(0.0f, 0.0f);
		}

		const float stepsPerPixel = (float)buffer.getNumSamples() / (float)getWidth();

		for (float i = 0; i < (float)buffer.getNumSamples(); i += 1.0f)
		{
			const int index = (int)i;

			float value = (float)buffer.getSample(0, index);

			if (std::isnan<float>(value))
			{
				continue;
			}

			if (isFFT)
			{
				value = fabsf(value);
				value = sqrt(value);
			}

			p.lineTo(i, -1.0f * value);
		}

		p.lineTo((float)buffer.getNumSamples(), 0.0f);
		p.closeSubPath();


		repaint();

	}

	Path p;

	const String name;

	AudioSampleBuffer buffer;
};



//==============================================================================
/*
    This component lives inside our window, and this is where you should put all
    your controls and content.
*/
class MainContentComponent   : public Component,
							   public ButtonListener,
							   public Timer,
							   public ApplicationCommandTarget,
							   public MenuBarModel
{
public:
    //==============================================================================
    MainContentComponent();
    ~MainContentComponent();

	enum NativeJITCommandID
	{
		FileNew = 1,
		FileSave,
		FileLoad,
		ViewShowAnalysis,
		ViewShowInput,
		ViewShowFFT,
		TestSignalNoise,
		TestSignalSine,
		TestSignalSaw,
		TestSignalSquare,
		TestSignalDirac,
		Compile,
		numCommands
	};

    void paint (Graphics&) override;
    void resized() override;

	void buttonClicked(Button* b) override;

	void timerCallback() override
	{
		table->updateContent();
		table->repaint();
	}


	ApplicationCommandTarget* getNextCommandTarget() override
	{
		return findFirstTargetParentComponent();
	};

	void setCommandTarget(ApplicationCommandInfo &result, const String &name, bool active, bool ticked, char shortcut, bool useShortCut = true, ModifierKeys mod = ModifierKeys::commandModifier) {
		result.setInfo(name, name, "Target", 0);
		result.setActive(active);
		result.setTicked(ticked);

		if (useShortCut) result.addDefaultKeypress(shortcut, mod);
	};

	

	void getAllCommands(Array<CommandID>& commands) override;;

	void getCommandInfo(CommandID commandID, ApplicationCommandInfo &result) override;;

	bool perform(const InvocationInfo &info) override;

	void calcFFTAndSetBuffers(bool input=true);

	void compileAndRun();

	StringArray getMenuBarNames() override
	{
		StringArray sa;

		sa.add("File");
		sa.add("Edit");
		sa.add("View");
		sa.add("Analysis");

		return sa;
	}

	PopupMenu getMenuForIndex(int topLevelMenuIndex, const String &menuName)
	{
		PopupMenu m;

		if (menuName == "File")
		{
			m.addCommandItem(commandManager, NativeJITCommandID::FileNew);
			m.addCommandItem(commandManager, NativeJITCommandID::FileLoad);
			m.addCommandItem(commandManager, NativeJITCommandID::FileSave);
		}
		else if (menuName == "Edit")
		{
			m.addCommandItem(commandManager, NativeJITCommandID::Compile);

			m.addSeparator();

			Array<int> editorCommands;
			
			editor->getAllCommands(editorCommands);

			for (int i = 0; i < editorCommands.size(); i++)
			{
				m.addCommandItem(commandManager, editorCommands[i]);
			}

			
		}
		else if (menuName == "View")
		{
			m.addCommandItem(commandManager, NativeJITCommandID::ViewShowAnalysis);
			m.addCommandItem(commandManager, NativeJITCommandID::ViewShowFFT);
			m.addCommandItem(commandManager, NativeJITCommandID::ViewShowInput);
		}
		else if (menuName == "Analysis")
		{
			PopupMenu freq;

			freq.addItem(1000 + 1, "x1", true, currentFreq == 1);
			freq.addItem(1000 + 2, "x2", true, currentFreq == 2);
			freq.addItem(1000 + 3, "x3", true, currentFreq == 3);
			freq.addItem(1000 + 4, "x4", true, currentFreq == 4);
			freq.addItem(1000 + 8, "x8", true, currentFreq == 8);
			freq.addItem(1000 + 16, "x16", true, currentFreq == 16);
			freq.addItem(1000 + 32, "x32", true, currentFreq == 32);

			m.addSubMenu("Set Frequency Multiplier", freq);


			m.addCommandItem(commandManager, NativeJITCommandID::TestSignalNoise);
			m.addCommandItem(commandManager, NativeJITCommandID::TestSignalSine);
			m.addCommandItem(commandManager, NativeJITCommandID::TestSignalSaw);
			m.addCommandItem(commandManager, NativeJITCommandID::TestSignalSquare);
			m.addCommandItem(commandManager, NativeJITCommandID::TestSignalDirac);
		}
		
		return m;
	}

	void menuItemSelected(int menuItemID, int topLevelMenuIndex)
	{
		if (topLevelMenuIndex == 3 && menuItemID > 1000)
		{
			currentFreq = menuItemID - 1000;
			calcFFTAndSetBuffers();
		}
	};

	void createFFT(bool createForInput)
	{
		FFT fft(11, false);

		float *w = tempBuffer.getWritePointer(0);

		FloatVectorOperations::clear(w, 4096);

		if (createForInput)
		{
			FloatVectorOperations::copy(w, inBuffer.getReadPointer(0), 2048);
		}
		else
		{
			FloatVectorOperations::copy(w, outBuffer.getReadPointer(0), 2048);
		}

		fft.performRealOnlyForwardTransform(w);

		float* o;
		
		if (createForInput)
		{
			o = inFFTBuffer.getWritePointer(0);
		}
		else
		{
			o = outFFTBuffer.getWritePointer(0);
		}
		
		int oIndex = 0;

		for (int i = 0; i < 2048; i+= 2)
		{
			o[oIndex] = sqrt(w[i] * w[i] + w[i+1] * w[i+1]);
			oIndex++;
		}
	}

	void run();

private:

	struct VariableTableModel : public TableListBoxModel
	{
		VariableTableModel(MainContentComponent* parent_) :
			parent(parent_)
		{};

		int getNumRows() override
		{
			if (parent->module != nullptr)
			{
				return parent->module->getScope()->getNumGlobalVariables();
			}
			return 0;
		}

		void paintRowBackground(Graphics& g, int /*rowNumber*/, int /*width*/, int /*height*/, bool rowIsSelected)
		{
			g.fillAll(rowIsSelected ? Colours::lightblue : Colours::lightgrey);
		}

		void paintCell(Graphics& g, int rowNumber, int columnId, int width, int height, bool /*rowIsSelected*/)
		{
			if (parent->module == nullptr) return;

			g.setColour(Colours::black);

			if (columnId == 1)
			{
				String t = (parent->module->getScope()->getGlobalVariableType(rowNumber).name());
				g.drawText(t, 0, 0, width, height, Justification::centred);
			}

			if (columnId == 2)
			{
				String t = (parent->module->getScope()->getGlobalVariableName(rowNumber).toString());
				g.drawText(t, 0, 0, width, height, Justification::centred);
			}

			if (columnId == 3)
			{
				String t = (parent->module->getScope()->getGlobalVariableValue(rowNumber).toString());
				g.drawText(t, 0, 0, width, height, Justification::centred);
			}
		}

		MainContentComponent* parent;
	};

	int currentFreq = 1;
	BufferDisplay::SignalType currentSignal;

	AudioSampleBuffer inBuffer;
	AudioSampleBuffer inFFTBuffer;
	AudioSampleBuffer outFFTBuffer;
	AudioSampleBuffer tempBuffer;
	AudioSampleBuffer outBuffer;

	ScopedPointer<CodeDocument> doc;
	ScopedPointer<CodeTokeniser> tokeniser;
	ScopedPointer<CodeEditorComponent> editor;
	ScopedPointer<Label> messageBox;
	
	ScopedPointer<NativeJITCompiler> compiler;

	ScopedPointer<NativeJITDspModule> module;

	ScopedPointer<TableListBox> table;
	ScopedPointer<TableListBoxModel> tableModel;

	ScopedPointer<FileLogger> fileLogger;

	ScopedPointer<BufferDisplay> inDisplay;
	ScopedPointer<BufferDisplay> inFFTDisplay;
	ScopedPointer<BufferDisplay> outFFTDisplay;
	ScopedPointer<BufferDisplay> outDisplay;

	ScopedPointer<ComboBox> testSignalSelector;

	ScopedPointer<ApplicationCommandManager> commandManager;

	ScopedPointer<MenuBarComponent> menuBar;


    
	bool showAnalysis = true;
	bool showFFT = true;
	bool showInput = true;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainContentComponent)
};


#endif  // MAINCOMPONENT_H_INCLUDED
