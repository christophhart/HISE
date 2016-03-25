
#pragma once

#include "../LuaState.h"
#include "../PluginProcessor.h"
#include "../vflib/FreeTypeAmalgam.h"

class AboutBox
{
public:
	static void launch(LuaProtoplugJuceAudioProcessor *processor, Component *colours)
	{
		protolua::LuaState ls(ProtoplugDir::Instance()->getLibDir());
		if (!ls.failed) {
			ls.openlibs();
			const char versionScript[] = "return (_VERSION..'\\n'..jit.version)";
			ls.loadbuffer(versionScript, strlen(versionScript), "vs");
			ls.pcall(0, 1, 0);
		}
		String arch;
		arch << 
		#ifdef JUCE_64BIT
			"64-bit "
		#else 
			"32-bit "
		#endif
		#ifdef JUCE_PPC
			<< "PPC"
		#endif
		#ifdef JUCE_ARM
			<< "ARM"
		#endif
		#ifdef JUCE_INTEL
			<< "Intel"
		#endif
		;
		String plugType;
		if (processor->wrapperType == AudioProcessor::wrapperType_AudioUnit)
			plugType = "AU";
		else if (processor->wrapperType == AudioProcessor::wrapperType_VST)
			plugType = "VST";
		else
			plugType = "error";
		String m;
		m << "Protoplug " << JucePlugin_VersionString << newLine
			<< "Author: Pierre Cusa" << newLine
			<< "Homepage: http://osar.fr/protoplug" << newLine
			<< "Build date: " << __DATE__ << newLine
			<< "Architecture: " << arch << newLine
			<< "Plugin type: " << plugType << newLine << newLine
			<< "Version info:" << newLine
			<< "JUCE " << JUCE_MAJOR_VERSION << "." << JUCE_MINOR_VERSION << "." << JUCE_BUILDNUMBER << newLine
			<< (ls.failed ? "LuaJIT not found" : ls.tostring(-1)) << newLine
			<< "Freetype " << FREETYPE_MAJOR << "." << FREETYPE_MINOR << "." << FREETYPE_PATCH;
		TextEditor *aboutBox = new TextEditor();
		aboutBox->setColour(TextEditor::backgroundColourId, colours->findColour(CodeEditorComponent::backgroundColourId));
		aboutBox->setColour(TextEditor::textColourId, colours->findColour(CodeEditorComponent::defaultTextColourId));
		aboutBox->setColour(TextEditor::highlightedTextColourId, colours->findColour(CodeEditorComponent::defaultTextColourId));
		aboutBox->setColour(TextEditor::highlightColourId, colours->findColour(CodeEditorComponent::highlightColourId));
		aboutBox->setMultiLine (true);
		aboutBox->setReadOnly (true);
		aboutBox->setScrollbarsShown (true);
		aboutBox->setCaretVisible (false);
		aboutBox->setPopupMenuEnabled (true);
		aboutBox->setText(m);
		DialogWindow::LaunchOptions options;
		options.content.setOwned (aboutBox);

		options.dialogTitle                   = "About Protoplug";
		options.escapeKeyTriggersCloseButton  = true;
		options.useNativeTitleBar             = false;
		options.resizable                     = true;

		DialogWindow* dw = options.launchAsync();
		dw->centreWithSize (300, 300);
	}
};
