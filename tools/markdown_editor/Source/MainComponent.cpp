/*
  ==============================================================================

    This file was auto-generated!

  ==============================================================================
*/

#include "MainComponent.h"

//==============================================================================
MainContentComponent::MainContentComponent() :
	bp(nullptr, nullptr),
	b(bp.getDocWindow()),
	ft(&bp, nullptr, {})
{
	bp.setAllowFlakyThreading(true);
	bp.prepareToPlay(44100.0, 512);
	
	bp.rebuildDatabase();


	setLookAndFeel(&laf);

	
	


	FloatingInterfaceBuilder ib(getRootFloatingTile());



	ib.setNewContentType<VerticalTile>(0);

	auto m = ib.addChild<VisibilityToggleBar>(0);

	ib.getContent(m)->setPanelColour(FloatingTileContent::PanelColourId::bgColour, Colour(0xFF444444));

	auto t = ib.addChild<FloatingTabComponent>(0);

	auto e1 = ib.addChild<MarkdownEditorPanel>(t);
	auto p = ib.addChild<MarkdownPreviewPanel>(0);

	ib.setCustomName(e1, "Editor");
	
	ib.setCustomName(p, "Preview");
	ib.setDynamic(0, false);
	ib.setSizes(0, { 32.0, -0.5, -0.5 });
	ib.setFoldable(0, false, { false, true, true });

	ib.setVisibility(0, true, { false, false, true });

	ib.finalizeAndReturnRoot();

	auto panel = dynamic_cast<MarkdownPreviewPanel*>(ib.getContent(p));
	auto editorPanel = dynamic_cast<MarkdownEditorPanel*>(ib.getContent(e1));
	
	preview = &panel->preview;
	jassert(preview != nullptr);

	editorPanel->setPreview(preview);

	preview->renderer.gotoLink({ {}, "/" });

	addAndMakeVisible(ft);

    setSize (1280, 800);
}

MainContentComponent::~MainContentComponent()
{
    
}

void MainContentComponent::paint (Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (Colour(0xFF393939));

    
}

void MainContentComponent::resized()
{
	ft.setBounds(getLocalBounds());
}
