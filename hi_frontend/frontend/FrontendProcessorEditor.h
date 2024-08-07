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
*   along with Foobar.  If not, see <http://www.gnu.org/licenses/>.
*
*   Commercial licenses for using HISE in an closed source project are
*   available on request. Please visit the project's website to get more
*   information about commercial licensing:
*
*   http://www.hartinstruments.net/hise/
*
*   HISE is based on the JUCE library,
*   which must be separately licensed for closed source applications:
*
*   http://www.juce.com
*
*   ===========================================================================
*/

#ifndef FRONTENDPROCESSOREDITOR_H_INCLUDED
#define FRONTENDPROCESSOREDITOR_H_INCLUDED

namespace hise { using namespace juce;

#define INCLUDE_BAR 1

class ScriptContentContainer;

class FrontendEditorHolder: public Component
{
    
};

class FrontendProcessorEditor: public AudioProcessorEditor,
							   public Timer,
							   public ModalBaseWindow,
							   public ComponentWithBackendConnection,
							   public GlobalSettingManager::ScaleFactorListener,
							   public TopLevelWindowWithOptionalOpenGL,
							   public MainController::LockFreeDispatcher::PresetLoadListener
{
public:

	FrontendProcessorEditor(FrontendProcessor *fp);;

	~FrontendProcessorEditor();

	BackendRootWindow* getBackendRootWindow() override
	{
		jassertfalse;
		return nullptr;
	}

	const BackendRootWindow* getBackendRootWindow() const override
	{
		jassertfalse;
		return nullptr;
	}

	FloatingTile* getRootFloatingTile() override
	{
		return rootTile;
	}

	void setSamplesCorrectlyInstalled(bool wasOK)
	{
		auto fp = dynamic_cast<FrontendProcessor*>(getAudioProcessor());

		if (!wasOK)
			fp->sendOverlayMessage(OverlayMessageBroadcaster::SamplesNotInstalled);
		
		if (wasOK)
		{
			
			
			GET_PROJECT_HANDLER(fp->getMainSynthChain()).setAllSampleReferencesCorrect();
			fp->loadSamplesAfterRegistration();
		}	
	}

#if USE_RAW_FRONTEND
	ScopedPointer<Component> rawEditor;
#endif

	Component* getContentComponent();

	void timerCallback()
	{
		dynamic_cast<FrontendProcessor*>(getAudioProcessor())->updateUnlockedSuspendStatus();

	}

	void newHisePresetLoaded() override;

	void scaleFactorChanged(float newScaleFactor) override;

	void paint(Graphics &g) override
	{
		g.fillAll(Colours::black);
        
        if(dynamic_cast<FrontendProcessor*>(getAudioProcessor())->deactivatedBecauseOfMemoryLimitation)
        {
            g.setColour(Colours::white);
            g.setFont(GLOBAL_BOLD_FONT());
            g.drawText("Deactivated because of AUv3 memory limitation", getLocalBounds(), Justification::centred);
            
        }
        
	};



    void setGlobalScaleFactor(float newScaleFactor, bool forceUpdate=false);

	void resized() override;

	void resetInterface()
	{
		//interfaceComponent->checkInterfaces();
	}

	

private:

    LookAndFeel_V3 globalLookAndFeel;
    
    ScopedPointer<FrontendEditorHolder> container;
    
	friend class BaseFrontendBar;

	ScopedPointer<FloatingTile> rootTile;
	ScopedPointer<DeactiveOverlay> deactiveOverlay;

	ScopedPointer<ThreadWithQuasiModalProgressWindow::Overlay> loaderOverlay;
	ScopedPointer<DebugLoggerComponent> debugLoggerComponent;
    
	float scaleFactor = 1.0f;

    int originalSizeX = 0;
    int originalSizeY = 0;

    bool overlayToolbar;


	bool usesOpenGl;
	OpenGLContext context;

	JUCE_DECLARE_WEAK_REFERENCEABLE(FrontendProcessorEditor);
};


} // namespace hise

#endif  // FRONTENDPROCESSOREDITOR_H_INCLUDED
