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

void FloatingTileContent::Factory::registerLayoutPanelTypes()
{
	registerType<SpacerPanel>(PopupMenuOptions::Spacer);
	registerType<VisibilityToggleBar>(PopupMenuOptions::VisibilityToggleBar);
	registerType<HorizontalTile>(PopupMenuOptions::HorizontalTile);
	registerType<VerticalTile>(PopupMenuOptions::VerticalTile);
	registerType<FloatingTabComponent>(PopupMenuOptions::Tabs);
}



void FloatingTileContent::Factory::registerAllPanelTypes()
{
	registerLayoutPanelTypes();
	
#if HISE_INCLUDE_SNEX_FLOATING_TILES

	registerType<SnexEditorPanel>(PopupMenuOptions::SnexEditor);
	registerType<SnexWorkbenchPanel<snex::ui::OptimizationProperties>>(PopupMenuOptions::SnexOptimisations);
	registerType<SnexWorkbenchPanel<snex::ui::TestGraph>>(PopupMenuOptions::SnexGraph);
	registerType<SnexWorkbenchPanel<snex::ui::ParameterList>>(PopupMenuOptions::SnexParameterList);
	registerType<SnexWorkbenchPanel<snex::ui::TestDataComponent>>(PopupMenuOptions::SnexTestDataInfo);
	registerType<SnexWorkbenchPanel<snex::ui::TestComplexDataManager>>(PopupMenuOptions::SnexComplexTestData);
	registerType<scriptnode::WorkbenchTestPlayer>(PopupMenuOptions::SnexWorkbenchPlayer);

	//registerType<snex::ui::Console>();
	//registerType<snex::ui::AssemblyViewer>();

#endif


	registerType<Note>(PopupMenuOptions::Note);
#if HISE_INCLUDE_RLOTTIE
	registerType<RLottieFloatingTile>(PopupMenuOptions::RLottieDevPanel);
#endif

	registerType<ExpansionEditBar>(PopupMenuOptions::ExpansionEditBar);

	registerType<PoolTableSubTypes::ImageFilePoolTable>(PopupMenuOptions::ImageTable);
	registerType<PoolTableSubTypes::MidiFilePoolTable>(PopupMenuOptions::MidiFilePoolTable);
	registerType<PoolTableSubTypes::AudioFilePoolTable>(PopupMenuOptions::AudioFileTable);
	registerType<PoolTableSubTypes::SampleMapPoolTable>(PopupMenuOptions::SampleMapPoolTable);

#if USE_BACKEND
	registerBackendPanelTypes();
#endif

	registerFrontendPanelTypes();

	registerType<InterfaceContentPanel>(PopupMenuOptions::InterfaceContent);
	registerType<SampleMapBrowser>(PopupMenuOptions::SampleMapBrowser);

	//MidiSourceList

	registerType<GlobalConnectorPanel<ModulatorSampler>>(PopupMenuOptions::SampleConnector);

#if HI_ENABLE_EXPANSION_EDITING

	registerType<SampleEditorPanel>(PopupMenuOptions::SampleEditor);
	registerType<SampleMapEditorPanel>(PopupMenuOptions::SampleMapEditor);

#endif

	registerType<TableEditorPanel>(PopupMenuOptions::TablePanel);
	registerType<SliderPackPanel>(PopupMenuOptions::SliderPackPanel);
}



void FloatingTileContent::Factory::registerFrontendPanelTypes()
{
	registerType<EmptyComponent>(PopupMenuOptions::Empty);
	registerType<PresetBrowserPanel>(PopupMenuOptions::PresetBrowser);
	registerType<AboutPagePanel>(PopupMenuOptions::AboutPage);
	registerType<MidiKeyboardPanel>(PopupMenuOptions::MidiKeyboard);
	registerType<PerformanceLabelPanel>(PopupMenuOptions::PerformanceStatistics);
	registerType<MidiOverlayPanel>(PopupMenuOptions::MidiPlayerOverlay);
	registerType<ActivityLedPanel>(PopupMenuOptions::ActivityLed);
    registerType<CustomSettingsWindowPanel>(PopupMenuOptions::PluginSettings);
	registerType<MidiSourcePanel>(PopupMenuOptions::MidiSourceList);
	registerType<MidiChannelPanel>(PopupMenuOptions::MidiChannelList);
	registerType<TooltipPanel>(PopupMenuOptions::TooltipPanel);
	registerType<MidiLearnPanel>(PopupMenuOptions::MidiLearnPanel);
	registerType<FrontendMacroPanel>(PopupMenuOptions::FrontendMacroPanel);
	registerType<PlotterPanel>(PopupMenuOptions::Plotter);
	registerType<AudioAnalyserComponent::Panel>(PopupMenuOptions::AudioAnalyser);
	registerType<WaveformComponent::Panel>(PopupMenuOptions::WavetablePreview);
	registerType<FilterGraph::Panel>(PopupMenuOptions::FilterGraphPanel);
	registerType<FilterDragOverlay::Panel>(PopupMenuOptions::DraggableFilterPanel);
	registerType<WaterfallComponent::Panel>(PopupMenuOptions::WavetableWaterfall);
	registerType<MPEPanel>(PopupMenuOptions::MPEPanel);
	
	registerType<AhdsrEnvelope::Panel>(PopupMenuOptions::AHDSRGraph);
	registerType<MarkdownPreviewPanel>(PopupMenuOptions::MarkdownPreviewPanel);
    registerType<MatrixPeakMeter>(PopupMenuOptions::MatrixPeakMeterPanel);
    
#if HI_ENABLE_EXTERNAL_CUSTOM_TILES
	registerExternalPanelTypes();
#endif
}




std::unique_ptr<Drawable> FloatingTileContent::Factory::getIcon(PopupMenuOptions type) const
{
	Path path = getPath(type);

	if (!path.isEmpty())
	{
		auto d = new DrawablePath();
		d->setPath(path);

		return std::unique_ptr<Drawable>(d);
	}
	else
		return {};
}

namespace FloatingTileIcons
{
static const unsigned char spacerIcon[] = { 110,109,0,120,111,66,255,167,242,66,108,0,120,111,66,0,84,4,67,108,0,134,111,66,0,84,4,67,108,0,134,111,66,128,86,4,67,108,0,195,141,66,128,86,4,67,108,0,195,141,66,128,86,1,67,108,0,120,123,66,128,86,1,67,108,0,120,123,66,255,167,242,66,108,0,120,111,
		66,255,167,242,66,99,109,0,150,20,66,0,180,242,66,108,0,150,20,66,128,93,1,67,108,0,24,233,65,128,93,1,67,108,0,24,233,65,128,93,4,67,108,0,140,32,66,128,93,4,67,108,0,140,32,66,0,90,4,67,108,0,150,32,66,0,90,4,67,108,0,150,32,66,0,180,242,66,108,0,150,
		20,66,0,180,242,66,99,109,0,120,232,65,0,61,24,67,108,0,120,232,65,0,61,27,67,108,0,74,20,66,0,61,27,67,108,0,74,20,66,128,63,35,67,108,0,74,32,66,128,63,35,67,108,0,74,32,66,128,63,24,67,108,0,60,32,66,128,63,24,67,108,0,60,32,66,0,61,24,67,108,0,120,
		232,65,0,61,24,67,99,109,0,84,112,66,0,86,24,67,108,0,84,112,66,128,89,24,67,108,0,74,112,66,128,89,24,67,108,0,74,112,66,128,89,35,67,108,0,74,124,66,128,89,35,67,108,0,74,124,66,0,86,27,67,108,0,42,142,66,0,86,27,67,108,0,42,142,66,0,86,24,67,108,0,
		84,112,66,0,86,24,67,99,101,0,0 };
}

Path FloatingTileContent::Factory::getPath(PopupMenuOptions type) 
{
	Path path;


	switch (type)
	{
	case FloatingTileContent::Factory::PopupMenuOptions::Cancel:
		break;
	case FloatingTileContent::Factory::PopupMenuOptions::Empty:
		break;
	case FloatingTileContent::Factory::PopupMenuOptions::Spacer:
	{
		
		path.loadPathFromData(ColumnIcons::layoutIcon, sizeof(ColumnIcons::layoutIcon));
		//path.loadPathFromData(pathData, sizeof(pathData));

		return path;
	}
	case FloatingTileContent::Factory::PopupMenuOptions::BigResizer:
		break;
	case FloatingTileContent::Factory::PopupMenuOptions::HorizontalTile:
	{
		static const unsigned char pathData[] = { 110,109,0,191,29,67,129,235,111,67,108,0,51,205,66,129,235,111,67,108,0,51,205,66,128,254,121,67,108,0,191,29,67,128,254,121,67,108,0,191,29,67,129,235,111,67,99,109,0,159,29,67,1,209,91,67,108,1,242,204,66,1,209,91,67,108,1,242,204,66,129,228,101,67,
			108,0,159,29,67,129,228,101,67,108,0,159,29,67,1,209,91,67,99,109,3,151,29,67,0,247,129,67,108,6,227,204,66,0,247,129,67,108,6,227,204,66,192,0,135,67,108,3,151,29,67,192,0,135,67,108,3,151,29,67,0,247,129,67,99,101,0,0 };

		path.loadPathFromData(pathData, sizeof(pathData));
		return path;
	}
	case FloatingTileContent::Factory::PopupMenuOptions::VerticalTile:
	{
		static const unsigned char pathData[] = { 110,109,1,53,250,66,128,66,89,67,108,1,53,250,66,0,52,136,67,108,128,45,7,67,0,52,136,67,108,128,45,7,67,128,66,89,67,108,1,53,250,66,128,66,89,67,99,109,0,0,210,66,128,98,89,67,108,0,0,210,66,64,68,136,67,108,0,39,230,66,64,68,136,67,108,0,39,230,66,
			128,98,89,67,108,0,0,210,66,128,98,89,67,99,109,0,29,17,67,128,106,89,67,108,0,29,17,67,0,72,136,67,108,128,48,27,67,0,72,136,67,108,128,48,27,67,128,106,89,67,108,0,29,17,67,128,106,89,67,99,101,0,0 };

		path.loadPathFromData(pathData, sizeof(pathData));

		break;
	}
	case FloatingTileContent::Factory::PopupMenuOptions::Tabs:
	{
		static const unsigned char pathData[] = { 110,109,0,0,170,66,64,174,153,67,108,0,0,170,66,64,46,176,67,108,0,205,189,66,64,46,176,67,108,0,205,189,66,0,142,158,67,108,0,0,2,67,0,142,158,67,108,0,0,2,67,64,174,153,67,108,0,0,170,66,64,174,153,67,99,109,0,126,199,66,192,250,160,67,108,0,126,199,
			66,192,122,183,67,108,0,191,16,67,192,122,183,67,108,0,191,16,67,192,250,160,67,108,0,126,199,66,192,250,160,67,99,101,0,0 };


		path.loadPathFromData(pathData, sizeof(pathData));

		break;
	}
	case FloatingTileContent::Factory::PopupMenuOptions::Matrix2x2:
		break;
	case FloatingTileContent::Factory::PopupMenuOptions::ThreeColumns:
		break;
	case FloatingTileContent::Factory::PopupMenuOptions::ThreeRows:
		break;
	case FloatingTileContent::Factory::PopupMenuOptions::Note:
	{
		static const unsigned char pathData[] = { 110,109,0,160,54,67,64,174,213,67,108,0,160,54,67,192,178,217,67,108,0,137,42,67,192,178,217,67,108,0,137,42,67,192,178,218,67,108,0,137,42,67,192,57,243,67,108,0,55,101,67,192,57,243,67,108,0,55,101,67,192,178,217,67,108,0,128,89,67,192,178,217,67,108,
			0,128,89,67,64,174,213,67,108,0,128,84,67,64,174,213,67,108,0,128,84,67,192,178,217,67,108,0,128,79,67,192,178,217,67,108,0,128,79,67,64,174,213,67,108,0,128,74,67,64,174,213,67,108,0,128,74,67,192,178,217,67,108,0,224,69,67,192,178,217,67,108,0,224,
			69,67,64,174,213,67,108,0,224,64,67,64,174,213,67,108,0,224,64,67,192,178,217,67,108,0,160,59,67,192,178,217,67,108,0,160,59,67,64,174,213,67,108,0,160,54,67,64,174,213,67,99,109,0,137,46,67,192,178,219,67,108,0,160,54,67,192,178,219,67,108,0,160,54,
			67,64,46,221,67,108,0,160,59,67,64,46,221,67,108,0,160,59,67,192,178,219,67,108,0,224,64,67,192,178,219,67,108,0,224,64,67,64,46,221,67,108,0,224,69,67,64,46,221,67,108,0,224,69,67,192,178,219,67,108,0,128,74,67,192,178,219,67,108,0,128,74,67,64,46,221,
			67,108,0,128,79,67,64,46,221,67,108,0,128,79,67,192,178,219,67,108,0,128,84,67,192,178,219,67,108,0,128,84,67,64,46,221,67,108,0,128,89,67,64,46,221,67,108,0,128,89,67,192,178,219,67,108,0,55,97,67,192,178,219,67,108,0,55,97,67,192,57,241,67,108,0,137,
			46,67,192,57,241,67,108,0,137,46,67,192,178,219,67,99,109,0,167,54,67,64,238,222,67,108,0,167,54,67,64,110,224,67,108,128,179,89,67,64,110,224,67,108,128,179,89,67,64,238,222,67,108,0,167,54,67,64,238,222,67,99,109,0,167,54,67,64,238,227,67,108,0,167,
			54,67,64,110,229,67,108,128,179,89,67,64,110,229,67,108,128,179,89,67,64,238,227,67,108,0,167,54,67,64,238,227,67,99,109,0,167,54,67,64,238,232,67,108,0,167,54,67,64,110,234,67,108,128,179,89,67,64,110,234,67,108,128,179,89,67,64,238,232,67,108,0,167,
			54,67,64,238,232,67,99,109,0,167,54,67,64,238,237,67,108,0,167,54,67,64,110,239,67,108,128,179,89,67,64,110,239,67,108,128,179,89,67,64,238,237,67,108,0,167,54,67,64,238,237,67,99,101,0,0 };

		path.loadPathFromData(pathData, sizeof(pathData));

		break;
	}
	case PopupMenuOptions::MacroControls:
	{
		path.loadPathFromData(HiBinaryData::SpecialSymbols::macros, SIZE_OF_PATH(HiBinaryData::SpecialSymbols::macros));
		break;
	}
	case PopupMenuOptions::SnexParameterList:
	case PopupMenuOptions::MacroTable:
	{
		path.loadPathFromData(MainToolbarIcons::macroControlTable, SIZE_OF_PATH(MainToolbarIcons::macroControlTable));

		break;
	}
	case PopupMenuOptions::DspNetworkGraph:
	case PopupMenuOptions::DspNodeList:
	case PopupMenuOptions::DspNodeParameterEditor:
	{
		path.loadPathFromData(ScriptnodeIcons::pinIcon, SIZE_OF_PATH(ScriptnodeIcons::pinIcon));
		break;
	}
	case PopupMenuOptions::PresetBrowser:
	{
		path.loadPathFromData(MainToolbarIcons::presetBrowser, SIZE_OF_PATH(MainToolbarIcons::presetBrowser));
		break;
	}
	case FloatingTileContent::Factory::PopupMenuOptions::MidiKeyboard:
#if USE_BACKEND
		path.loadPathFromData(BackendBinaryData::ToolbarIcons::keyboard, SIZE_OF_PATH(BackendBinaryData::ToolbarIcons::keyboard));
#endif
		break;
	case PopupMenuOptions::SampleConnector:
	case FloatingTileContent::Factory::PopupMenuOptions::ScriptConnectorPanel:
	{
		path.loadPathFromData(EditorIcons::connectIcon, SIZE_OF_PATH(EditorIcons::connectIcon));
		break;
	}
    case PopupMenuOptions::MarkdownPreviewPanel:
	{
		static const unsigned char pathData[] = { 110,109,219,137,109,68,0,0,64,67,108,39,177,147,66,0,0,64,67,98,12,130,4,66,0,0,64,67,0,0,0,0,131,32,97,67,0,0,0,0,74,236,132,67,108,0,0,0,0,220,137,61,68,98,0,0,0,0,0,184,71,68,12,130,4,66,0,0,80,68,39,177,147,66,0,0,80,68,108,219,137,109,68,0,0,80,
68,98,0,184,119,68,0,0,80,68,0,0,128,68,0,184,71,68,0,0,128,68,219,137,61,68,108,0,0,128,68,74,236,132,67,98,0,0,128,68,131,32,97,67,0,184,119,68,0,0,64,67,219,137,109,68,0,0,64,67,99,109,0,0,16,68,0,248,47,68,108,0,0,224,67,0,0,48,68,108,0,0,224,67,
0,0,0,68,108,0,0,176,67,238,196,30,68,108,0,0,128,67,0,0,0,68,108,0,0,128,67,0,0,48,68,108,0,0,0,67,0,0,48,68,108,0,0,0,67,0,0,160,67,108,0,0,128,67,0,0,160,67,108,0,0,176,67,0,0,224,67,108,0,0,224,67,0,0,160,67,108,0,0,16,68,0,240,159,67,108,0,0,16,
68,0,248,47,68,99,109,211,197,63,68,0,248,55,68,108,0,0,24,68,0,0,0,68,108,0,0,48,68,0,0,0,68,108,0,0,48,68,0,0,160,67,108,0,0,80,68,0,0,160,67,108,0,0,80,68,0,0,0,68,108,0,0,104,68,0,0,0,68,108,211,197,63,68,0,248,55,68,99,101,0,0 };

		path.loadPathFromData(pathData, sizeof(pathData));
		break;
	}
	case PopupMenuOptions::MarkdownEditor:
	case FloatingTileContent::Factory::PopupMenuOptions::SnexEditor:
	case FloatingTileContent::Factory::PopupMenuOptions::ScriptEditor:
	{
		path.loadPathFromData(HiBinaryData::SpecialSymbols::scriptProcessor, SIZE_OF_PATH(HiBinaryData::SpecialSymbols::scriptProcessor));
		break;
	}
	case FloatingTileContent::Factory::PopupMenuOptions::ScriptContent:
	{
#if USE_BACKEND
		path.loadPathFromData(MainToolbarIcons::home, SIZE_OF_PATH(MainToolbarIcons::home));
#endif
		break;
	}
	case FloatingTileContent::Factory::PopupMenuOptions::ServerController:
	{
#if USE_BACKEND
		path.loadPathFromData(MainToolbarIcons::web, SIZE_OF_PATH(MainToolbarIcons::web));
#endif
		break;
	}
	case FloatingTileContent::Factory::PopupMenuOptions::ScriptComponentList:
	{
#if USE_BACKEND
		path.loadPathFromData(MainToolbarIcons::home, SIZE_OF_PATH(MainToolbarIcons::home));
#endif
		break;
	}
	case FloatingTileContent::Factory::PopupMenuOptions::TablePanel:
	{
		static const unsigned char pathData[] = { 110,109,0,208,241,65,0,192,141,66,108,0,208,241,65,0,192,144,66,108,0,208,241,65,0,38,222,66,108,0,219,140,66,0,38,222,66,108,0,219,140,66,0,192,141,66,108,0,208,241,65,0,192,141,66,99,109,0,232,4,66,0,192,147,66,108,0,88,134,66,0,192,147,66,98,169,221,
			125,66,48,89,190,66,241,53,32,66,226,20,210,66,0,232,4,66,0,249,214,66,108,0,232,4,66,0,192,147,66,99,109,0,219,134,66,0,85,166,66,108,0,219,134,66,0,38,216,66,108,0,66,31,66,0,38,216,66,98,108,248,64,66,21,141,208,66,248,121,117,66,254,2,193,66,0,219,
			134,66,0,85,166,66,99,101,0,0 };
		path.loadPathFromData(pathData, sizeof(pathData));

		break;
	}
	case FloatingTileContent::Factory::PopupMenuOptions::VisibilityToggleBar:
	{
		path.loadPathFromData(HiBinaryData::ProcessorEditorHeaderIcons::bypassShape, SIZE_OF_PATH(HiBinaryData::ProcessorEditorHeaderIcons::bypassShape));

		return path;
	}
	case FloatingTileContent::Factory::PopupMenuOptions::SnexTestDataInfo:
	{
		path.loadPathFromData(HnodeIcons::testIcon, SIZE_OF_PATH(HnodeIcons::testIcon));
		return path;
	}

	case FloatingTileContent::Factory::PopupMenuOptions::SliderPackPanel:
	{
		static const unsigned char pathData[] = { 110,109,0,128,157,66,0,185,141,66,108,0,128,157,66,0,185,144,66,108,0,128,157,66,0,185,227,66,108,0,128,243,66,0,185,227,66,108,0,128,243,66,0,185,141,66,108,0,128,157,66,0,185,141,66,99,109,0,128,163,66,0,185,147,66,108,0,128,237,66,0,185,147,66,108,
			0,128,237,66,0,185,221,66,108,0,0,233,66,0,185,221,66,108,0,0,233,66,0,185,154,66,108,0,0,227,66,0,185,154,66,108,0,0,227,66,0,185,221,66,108,0,0,223,66,0,185,221,66,108,0,0,223,66,0,185,164,66,108,0,0,217,66,0,185,164,66,108,0,0,217,66,0,185,221,66,
			108,0,0,213,66,0,185,221,66,108,0,0,213,66,0,185,174,66,108,0,0,207,66,0,185,174,66,108,0,0,207,66,0,185,221,66,108,0,0,203,66,0,185,221,66,108,0,0,203,66,0,185,194,66,108,0,0,197,66,0,185,194,66,108,0,0,197,66,0,185,221,66,108,0,0,193,66,0,185,221,66,
			108,0,0,193,66,0,185,184,66,108,0,0,187,66,0,185,184,66,108,0,0,187,66,0,185,221,66,108,0,0,183,66,0,185,221,66,108,0,0,183,66,0,185,194,66,108,0,0,177,66,0,185,194,66,108,0,0,177,66,0,185,221,66,108,0,0,173,66,0,185,221,66,108,0,0,173,66,0,185,204,66,
			108,0,0,167,66,0,185,204,66,108,0,0,167,66,0,185,221,66,108,0,128,163,66,0,185,221,66,108,0,128,163,66,0,185,147,66,99,101,0,0 };

		path.loadPathFromData(pathData, sizeof(pathData));


		break;
	}
	case FloatingTileContent::Factory::PopupMenuOptions::Console:
	{
#if USE_BACKEND
		path.loadPathFromData(BackendBinaryData::ToolbarIcons::debugPanel, SIZE_OF_PATH(BackendBinaryData::ToolbarIcons::debugPanel));
#endif
		break;
	}
	case FloatingTileContent::Factory::PopupMenuOptions::SnexGraph:
	case FloatingTileContent::Factory::PopupMenuOptions::Plotter:
	{
		static const unsigned char pathData[] = { 110,109,128,252,89,67,64,173,211,67,98,163,65,87,67,250,238,211,67,11,182,84,67,198,176,212,67,0,89,82,67,64,212,213,67,98,235,158,77,67,51,27,216,67,247,55,73,67,167,244,219,67,127,85,68,67,64,17,226,67,108,255,12,75,67,192,104,227,67,98,225,192,79,
			67,113,134,221,67,221,12,84,67,211,7,218,67,127,93,87,67,0,111,216,67,98,208,5,89,67,150,162,215,67,69,85,90,67,157,82,215,67,255,82,91,67,192,58,215,67,98,186,80,92,67,227,34,215,67,44,39,93,67,181,52,215,67,127,83,94,67,0,134,215,67,98,37,172,96,67,
			147,40,216,67,218,210,99,67,57,0,218,67,255,219,102,67,128,135,220,67,98,35,229,105,67,199,14,223,67,239,240,108,67,9,55,226,67,255,8,112,67,0,96,229,67,98,15,33,115,67,247,136,232,67,40,68,118,67,165,178,235,67,255,173,121,67,128,81,238,67,98,213,23,
			125,67,91,240,240,67,218,89,128,67,243,32,243,67,0,222,130,67,0,3,244,67,98,19,32,132,67,6,116,244,67,155,146,133,67,91,119,244,67,128,234,134,67,192,245,243,67,98,101,66,136,67,37,116,243,67,197,124,137,67,84,128,242,67,64,173,138,67,0,39,241,67,98,
			54,14,141,67,87,116,238,67,43,97,143,67,72,25,234,67,0,244,145,67,192,98,227,67,108,128,147,142,67,64,23,226,67,98,136,20,140,67,248,153,232,67,94,208,137,67,224,169,236,67,0,247,135,67,192,194,238,67,98,81,10,135,67,48,207,239,67,185,62,134,67,72,89,
			240,67,64,164,133,67,128,147,240,67,98,199,9,133,67,184,205,240,67,96,157,132,67,240,202,240,67,128,16,132,67,129,153,240,67,98,191,246,130,67,161,54,240,67,87,102,129,67,131,177,238,67,127,189,127,67,65,88,236,67,98,81,174,124,67,255,254,233,67,64,156,
			121,67,21,238,230,67,255,135,118,67,1,201,227,67,98,190,115,115,67,237,163,224,67,14,92,112,67,110,106,221,67,127,15,109,67,1,171,218,67,98,240,194,105,67,148,235,215,67,131,97,102,67,135,151,213,67,255,196,97,67,1,88,212,67,98,189,118,95,67,62,184,211,
			67,92,183,92,67,135,107,211,67,127,252,89,67,65,173,211,67,99,101,0,0 };

		path.loadPathFromData(pathData, sizeof(pathData));

		return path;
	}
	case PopupMenuOptions::SampleEditor:
	{
		path.loadPathFromData(MainToolbarIcons::samplerWorkspace , SIZE_OF_PATH(MainToolbarIcons::samplerWorkspace));
		return path;
	}
	case PopupMenuOptions::SampleMapEditor:
	{
		static const unsigned char pathData[] = { 110,109,0,0,27,67,64,174,163,67,108,0,0,27,67,64,174,168,67,108,0,0,37,67,64,174,168,67,108,0,0,37,67,64,174,163,67,108,0,0,27,67,64,174,163,67,99,109,0,0,42,67,64,174,163,67,108,0,0,42,67,64,174,168,67,108,0,0,52,67,64,174,168,67,108,0,0,52,67,64,174,
			163,67,108,0,0,42,67,64,174,163,67,99,109,0,0,57,67,64,174,163,67,108,0,0,57,67,64,174,168,67,108,0,0,67,67,64,174,168,67,108,0,0,67,67,64,174,163,67,108,0,0,57,67,64,174,163,67,99,109,0,0,72,67,64,174,163,67,108,0,0,72,67,64,174,168,67,108,0,0,82,67,
			64,174,168,67,108,0,0,82,67,64,174,163,67,108,0,0,72,67,64,174,163,67,99,109,0,0,27,67,64,46,171,67,108,0,0,27,67,64,174,178,67,108,0,0,37,67,64,174,178,67,108,0,0,37,67,64,46,171,67,108,0,0,27,67,64,46,171,67,99,109,0,0,42,67,64,46,171,67,108,0,0,42,
			67,64,174,178,67,108,0,0,52,67,64,174,178,67,108,0,0,52,67,64,46,171,67,108,0,0,42,67,64,46,171,67,99,109,0,0,57,67,64,46,171,67,108,0,0,57,67,64,174,178,67,108,0,0,67,67,64,174,178,67,108,0,0,67,67,64,46,171,67,108,0,0,57,67,64,46,171,67,99,109,0,0,
			72,67,64,46,171,67,108,0,0,72,67,64,174,178,67,108,0,0,82,67,64,174,178,67,108,0,0,82,67,64,46,171,67,108,0,0,72,67,64,46,171,67,99,109,0,0,27,67,64,46,181,67,108,0,0,27,67,64,46,186,67,108,0,0,37,67,64,46,186,67,108,0,0,37,67,64,46,181,67,108,0,0,27,
			67,64,46,181,67,99,109,0,0,42,67,64,46,181,67,108,0,0,42,67,64,46,186,67,108,0,0,52,67,64,46,186,67,108,0,0,52,67,64,46,181,67,108,0,0,42,67,64,46,181,67,99,109,0,0,57,67,64,46,181,67,108,0,0,57,67,64,46,186,67,108,0,0,67,67,64,46,186,67,108,0,0,67,67,
			64,46,181,67,108,0,0,57,67,64,46,181,67,99,109,0,0,72,67,64,46,181,67,108,0,0,72,67,64,46,186,67,108,0,0,82,67,64,46,186,67,108,0,0,82,67,64,46,181,67,108,0,0,72,67,64,46,181,67,99,109,0,0,27,67,64,174,188,67,108,0,0,27,67,64,46,191,67,108,0,0,82,67,
			64,46,191,67,108,0,0,82,67,64,174,188,67,108,0,0,27,67,64,174,188,67,99,101,0,0 };

		
		path.loadPathFromData(pathData, sizeof(pathData));

		return path;
	}
	case PopupMenuOptions::SamplerTable:
	{
		static const unsigned char pathData[] = { 110,109,0,128,142,67,64,46,186,67,108,0,128,142,67,64,174,188,67,108,0,0,145,67,64,174,188,67,108,0,0,145,67,64,46,186,67,108,0,128,142,67,64,46,186,67,99,109,0,128,147,67,64,46,186,67,108,0,128,147,67,64,174,188,67,108,0,0,170,67,64,174,188,67,108,0,
			0,170,67,64,46,186,67,108,0,128,147,67,64,46,186,67,99,109,0,128,172,67,64,46,186,67,108,0,128,172,67,64,174,188,67,108,0,0,175,67,64,174,188,67,108,0,0,175,67,64,46,186,67,108,0,128,172,67,64,46,186,67,99,109,0,128,177,67,64,46,186,67,108,0,128,177,
			67,64,174,188,67,108,0,0,180,67,64,174,188,67,108,0,0,180,67,64,46,186,67,108,0,128,177,67,64,46,186,67,99,109,0,128,182,67,64,46,186,67,108,0,128,182,67,64,174,188,67,108,0,0,185,67,64,174,188,67,108,0,0,185,67,64,46,186,67,108,0,128,182,67,64,46,186,
			67,99,109,0,128,142,67,64,69,191,67,108,0,128,142,67,64,197,193,67,108,0,0,145,67,64,197,193,67,108,0,0,145,67,64,69,191,67,108,0,128,142,67,64,69,191,67,99,109,0,128,147,67,64,69,191,67,108,0,128,147,67,64,197,193,67,108,0,0,170,67,64,197,193,67,108,
			0,0,170,67,64,69,191,67,108,0,128,147,67,64,69,191,67,99,109,0,128,172,67,64,69,191,67,108,0,128,172,67,64,197,193,67,108,0,0,175,67,64,197,193,67,108,0,0,175,67,64,69,191,67,108,0,128,172,67,64,69,191,67,99,109,0,128,177,67,64,69,191,67,108,0,128,177,
			67,64,197,193,67,108,0,0,180,67,64,197,193,67,108,0,0,180,67,64,69,191,67,108,0,128,177,67,64,69,191,67,99,109,0,128,182,67,64,69,191,67,108,0,128,182,67,64,197,193,67,108,0,0,185,67,64,197,193,67,108,0,0,185,67,64,69,191,67,108,0,128,182,67,64,69,191,
			67,99,109,192,150,142,67,128,23,196,67,108,192,150,142,67,128,151,198,67,108,192,22,145,67,128,151,198,67,108,192,22,145,67,128,23,196,67,108,192,150,142,67,128,23,196,67,99,109,192,150,147,67,128,23,196,67,108,192,150,147,67,128,151,198,67,108,192,22,
			170,67,128,151,198,67,108,192,22,170,67,128,23,196,67,108,192,150,147,67,128,23,196,67,99,109,192,150,172,67,128,23,196,67,108,192,150,172,67,128,151,198,67,108,192,22,175,67,128,151,198,67,108,192,22,175,67,128,23,196,67,108,192,150,172,67,128,23,196,
			67,99,109,192,150,177,67,128,23,196,67,108,192,150,177,67,128,151,198,67,108,192,22,180,67,128,151,198,67,108,192,22,180,67,128,23,196,67,108,192,150,177,67,128,23,196,67,99,109,192,150,182,67,128,23,196,67,108,192,150,182,67,128,151,198,67,108,192,22,
			185,67,128,151,198,67,108,192,22,185,67,128,23,196,67,108,192,150,182,67,128,23,196,67,99,109,192,150,142,67,64,46,201,67,108,192,150,142,67,64,174,203,67,108,192,22,145,67,64,174,203,67,108,192,22,145,67,64,46,201,67,108,192,150,142,67,64,46,201,67,
			99,109,192,150,147,67,64,46,201,67,108,192,150,147,67,64,174,203,67,108,192,22,170,67,64,174,203,67,108,192,22,170,67,64,46,201,67,108,192,150,147,67,64,46,201,67,99,109,192,150,172,67,64,46,201,67,108,192,150,172,67,64,174,203,67,108,192,22,175,67,64,
			174,203,67,108,192,22,175,67,64,46,201,67,108,192,150,172,67,64,46,201,67,99,109,192,150,177,67,64,46,201,67,108,192,150,177,67,64,174,203,67,108,192,22,180,67,64,174,203,67,108,192,22,180,67,64,46,201,67,108,192,150,177,67,64,46,201,67,99,109,192,150,
			182,67,64,46,201,67,108,192,150,182,67,64,174,203,67,108,192,22,185,67,64,174,203,67,108,192,22,185,67,64,46,201,67,108,192,150,182,67,64,46,201,67,99,109,192,150,142,67,64,46,206,67,108,192,150,142,67,64,174,208,67,108,192,22,145,67,64,174,208,67,108,
			192,22,145,67,64,46,206,67,108,192,150,142,67,64,46,206,67,99,109,192,150,147,67,64,46,206,67,108,192,150,147,67,64,174,208,67,108,192,22,170,67,64,174,208,67,108,192,22,170,67,64,46,206,67,108,192,150,147,67,64,46,206,67,99,109,192,150,172,67,64,46,
			206,67,108,192,150,172,67,64,174,208,67,108,192,22,175,67,64,174,208,67,108,192,22,175,67,64,46,206,67,108,192,150,172,67,64,46,206,67,99,109,192,150,177,67,64,46,206,67,108,192,150,177,67,64,174,208,67,108,192,22,180,67,64,174,208,67,108,192,22,180,
			67,64,46,206,67,108,192,150,177,67,64,46,206,67,99,109,192,150,182,67,64,46,206,67,108,192,150,182,67,64,174,208,67,108,192,22,185,67,64,174,208,67,108,192,22,185,67,64,46,206,67,108,192,150,182,67,64,46,206,67,99,109,192,150,142,67,64,69,211,67,108,
			192,150,142,67,64,197,213,67,108,192,22,145,67,64,197,213,67,108,192,22,145,67,64,69,211,67,108,192,150,142,67,64,69,211,67,99,109,192,150,147,67,64,69,211,67,108,192,150,147,67,64,197,213,67,108,192,22,170,67,64,197,213,67,108,192,22,170,67,64,69,211,
			67,108,192,150,147,67,64,69,211,67,99,109,192,150,172,67,64,69,211,67,108,192,150,172,67,64,197,213,67,108,192,22,175,67,64,197,213,67,108,192,22,175,67,64,69,211,67,108,192,150,172,67,64,69,211,67,99,109,192,150,177,67,64,69,211,67,108,192,150,177,67,
			64,197,213,67,108,192,22,180,67,64,197,213,67,108,192,22,180,67,64,69,211,67,108,192,150,177,67,64,69,211,67,99,109,192,150,182,67,64,69,211,67,108,192,150,182,67,64,197,213,67,108,192,22,185,67,64,197,213,67,108,192,22,185,67,64,69,211,67,108,192,150,
			182,67,64,69,211,67,99,109,0,128,142,67,64,69,216,67,108,0,128,142,67,64,197,218,67,108,0,0,145,67,64,197,218,67,108,0,0,145,67,64,69,216,67,108,0,128,142,67,64,69,216,67,99,109,0,128,147,67,64,69,216,67,108,0,128,147,67,64,197,218,67,108,0,0,170,67,
			64,197,218,67,108,0,0,170,67,64,69,216,67,108,0,128,147,67,64,69,216,67,99,109,0,128,172,67,64,69,216,67,108,0,128,172,67,64,197,218,67,108,0,0,175,67,64,197,218,67,108,0,0,175,67,64,69,216,67,108,0,128,172,67,64,69,216,67,99,109,0,128,177,67,64,69,216,
			67,108,0,128,177,67,64,197,218,67,108,0,0,180,67,64,197,218,67,108,0,0,180,67,64,69,216,67,108,0,128,177,67,64,69,216,67,99,109,0,128,182,67,64,69,216,67,108,0,128,182,67,64,197,218,67,108,0,0,185,67,64,197,218,67,108,0,0,185,67,64,69,216,67,108,0,128,
			182,67,64,69,216,67,99,101,0,0 };

		path.loadPathFromData(pathData, sizeof(pathData));

		return path;
	}
	case FloatingTileContent::Factory::PopupMenuOptions::ApiCollection:
	{
		BACKEND_ONLY(path.loadPathFromData(BackendBinaryData::ToolbarIcons::apiList, SIZE_OF_PATH(BackendBinaryData::ToolbarIcons::apiList)));
		break;
	}
	case FloatingTileContent::Factory::PopupMenuOptions::ScriptWatchTable:
	{
		BACKEND_ONLY(path.loadPathFromData(BackendBinaryData::ToolbarIcons::viewPanel, SIZE_OF_PATH(BackendBinaryData::ToolbarIcons::viewPanel)));

		break;
	}
	case FloatingTileContent::Factory::PopupMenuOptions::ScriptComponentEditPanel:
	{
		BACKEND_ONLY(path.loadPathFromData(BackendBinaryData::ToolbarIcons::mixer, SIZE_OF_PATH(BackendBinaryData::ToolbarIcons::mixer)));
		path.applyTransform(AffineTransform::rotation(float_Pi / 2.0f));

		break;
	}
	case FloatingTileContent::Factory::PopupMenuOptions::ModuleBrowser:
	{
		BACKEND_ONLY(path.loadPathFromData(BackendBinaryData::ToolbarIcons::modulatorList, SIZE_OF_PATH(BackendBinaryData::ToolbarIcons::modulatorList)));
		break;
	}
	case FloatingTileContent::Factory::PopupMenuOptions::PatchBrowser:
	{
		BACKEND_ONLY(path.loadPathFromData(BackendBinaryData::ToolbarIcons::modulatorList, SIZE_OF_PATH(BackendBinaryData::ToolbarIcons::modulatorList)));
		break;
	}
	break;
	case FloatingTileContent::Factory::PopupMenuOptions::ExpansionEditBar:
	case FloatingTileContent::Factory::PopupMenuOptions::FileBrowser:
	{
		BACKEND_ONLY(path.loadPathFromData(BackendBinaryData::ToolbarIcons::fileBrowser, SIZE_OF_PATH(BackendBinaryData::ToolbarIcons::fileBrowser)));
		break;
	}
	case FloatingTileContent::Factory::PopupMenuOptions::SamplePoolTable:
	{
		BACKEND_ONLY(path.loadPathFromData(BackendBinaryData::ToolbarIcons::sampleTable, SIZE_OF_PATH(BackendBinaryData::ToolbarIcons::sampleTable)));
		break;
	}
	case PopupMenuOptions::AudioFileTable:
	{
		BACKEND_ONLY(path.loadPathFromData(BackendBinaryData::ToolbarIcons::fileTable, SIZE_OF_PATH(BackendBinaryData::ToolbarIcons::fileTable)));
		break;
	}
	case PopupMenuOptions::ImageTable:
	{
		BACKEND_ONLY(path.loadPathFromData(BackendBinaryData::ToolbarIcons::imageTable, SIZE_OF_PATH(BackendBinaryData::ToolbarIcons::imageTable)));
		break;
	}
	case PopupMenuOptions::numOptions:
	{
		
		path.loadPathFromData(MainToolbarIcons::mainWorkspace, SIZE_OF_PATH(MainToolbarIcons::mainWorkspace));

		return path;
	}
	case FloatingTileContent::Factory::PopupMenuOptions::toggleLayoutMode:
		break;
	case FloatingTileContent::Factory::PopupMenuOptions::toggleGlobalLayoutMode:
		break;
	case FloatingTileContent::Factory::PopupMenuOptions::exportAsJSON:
		break;
	case FloatingTileContent::Factory::PopupMenuOptions::loadFromJSON:
		break;
	case FloatingTileContent::Factory::PopupMenuOptions::MenuCommandOffset:
		break;
	default:
		break;
	}

	return path;
}

void FloatingTileContent::Factory::addToPopupMenu(PopupMenu& m, PopupMenuOptions type, const String& name, bool isEnabled, bool isTicked)
{
	m.addItem((int)type, name, isEnabled, isTicked, std::unique_ptr<Drawable>(getIcon(type)));
}

void addCommandIcon(FloatingTile* /*parent*/, PopupMenu& , int /*commandID*/)
{
	
}

FloatingTileContent::Factory::PopupMenuOptions FloatingTileContent::Factory::getOption(const FloatingTile* t) const
{
	if (t->getCurrentFloatingPanel())
	{
		Identifier id = t->getCurrentFloatingPanel()->getIdentifierForBaseClass();

		int index = ids.indexOf(id);

		return idIndexes[index];
	}

	return PopupMenuOptions::Empty;
}

void FloatingTileContent::Factory::handlePopupMenu(PopupMenu& m, FloatingTile* parent)
{
#if USE_BACKEND
	if (parent->canBeDeleted())
	{
		if (parent->isLayoutModeEnabled())
		{
			m.addSectionHeader("Layout Elements");

			addToPopupMenu(m, PopupMenuOptions::Tabs, "Tabs");
			addToPopupMenu(m, PopupMenuOptions::HorizontalTile, "Horizontal Tile");
			addToPopupMenu(m, PopupMenuOptions::VerticalTile, "Vertical Tile");
			addToPopupMenu(m, PopupMenuOptions::Spacer, "Spacer");
			addToPopupMenu(m, PopupMenuOptions::VisibilityToggleBar, "Visibility Toggle Bar");

			PopupMenu combinedLayouts;

			addToPopupMenu(combinedLayouts, PopupMenuOptions::Matrix2x2, "2x2 Matrix");
			addToPopupMenu(combinedLayouts, PopupMenuOptions::ThreeColumns, "3 Columns");
			addToPopupMenu(combinedLayouts, PopupMenuOptions::ThreeRows, "3 Rows");

			m.addSubMenu("Combined Layouts", combinedLayouts);
		}
		else
		{
			m.addSectionHeader("Scripting Tools");

			addToPopupMenu(m, PopupMenuOptions::ScriptConnectorPanel, "Global Script Connector");
			addToPopupMenu(m, PopupMenuOptions::ScriptEditor, "Script Editor");
            addToPopupMenu(m, PopupMenuOptions::MarkdownEditor, "Markdown Editor");
			addToPopupMenu(m, PopupMenuOptions::ScriptContent, "Script Content");
			addToPopupMenu(m, PopupMenuOptions::ScriptComponentEditPanel, "Script Interface Property Editor");
			addToPopupMenu(m, PopupMenuOptions::ScriptComponentList, "Script Component List");
			addToPopupMenu(m, PopupMenuOptions::ApiCollection, "API Browser");
			addToPopupMenu(m, PopupMenuOptions::ScriptWatchTable, "Live Variable View");
			addToPopupMenu(m, PopupMenuOptions::ComplexDataManager, "Complex Data Manager");
			addToPopupMenu(m, PopupMenuOptions::Console, "Console");
			addToPopupMenu(m, PopupMenuOptions::OSCLogger, "OSC Logger");
			addToPopupMenu(m, PopupMenuOptions::DspNodeList, "DSP Node list");
			addToPopupMenu(m, PopupMenuOptions::DspNetworkGraph, "DSP Network Graph");
			addToPopupMenu(m, PopupMenuOptions::DspNodeParameterEditor, "DSP Network Node Editor");
            addToPopupMenu(m, PopupMenuOptions::DspFaustEditorPanel, "Faust Editor");
			addToPopupMenu(m, PopupMenuOptions::RLottieDevPanel, "Lottie Dev Panel");
			addToPopupMenu(m, PopupMenuOptions::ServerController, "Server Controller");
			addToPopupMenu(m, PopupMenuOptions::ScriptBroadcasterMap, "ScriptBroadcaster Map");
			addToPopupMenu(m, PopupMenuOptions::SnexEditor, "SNEX Editor");

			m.addSectionHeader("Sampler Tools");

			addToPopupMenu(m, PopupMenuOptions::SampleConnector, "Global Sampler Connector");
			addToPopupMenu(m, PopupMenuOptions::SampleEditor, "Sample Editor");
			addToPopupMenu(m, PopupMenuOptions::SampleMapEditor, "Sample Map Editor");
			addToPopupMenu(m, PopupMenuOptions::SamplerTable, "Sampler Table");
			addToPopupMenu(m, PopupMenuOptions::SamplePoolTable, "Global Sample Pool");


			m.addSectionHeader("Misc Tools");

			addToPopupMenu(m, PopupMenuOptions::MacroControls, "8 Macro Controls");
			addToPopupMenu(m, PopupMenuOptions::MacroTable, "Macro Control Editor");
			addToPopupMenu(m, PopupMenuOptions::FrontendMacroPanel, "All Macro Edit Table");
			addToPopupMenu(m, PopupMenuOptions::Plotter, "Plotter");
			addToPopupMenu(m, PopupMenuOptions::AudioAnalyser, "Audio Analyser");
			addToPopupMenu(m, PopupMenuOptions::TablePanel, "Table Editor");
			addToPopupMenu(m, PopupMenuOptions::PresetBrowser, "Preset Browser");
			addToPopupMenu(m, PopupMenuOptions::ModuleBrowser, "Module Browser");

			addToPopupMenu(m, PopupMenuOptions::PatchBrowser, "Patch Browser");
			addToPopupMenu(m, PopupMenuOptions::FileBrowser, "File Browser");
			addToPopupMenu(m, PopupMenuOptions::AutomationDataBrowser, "Automation Data Browser");
			addToPopupMenu(m, PopupMenuOptions::SamplePoolTable, "SamplePoolTable");
			addToPopupMenu(m, PopupMenuOptions::SliderPackPanel, "Array Editor");
			addToPopupMenu(m, PopupMenuOptions::MidiKeyboard, "Virtual Keyboard");
			addToPopupMenu(m, PopupMenuOptions::PerfettoViewer, "Perfetto Viewer");

			addToPopupMenu(m, PopupMenuOptions::Note, "Note");
			addToPopupMenu(m, PopupMenuOptions::AudioFileTable, "Audio File Pool Table");
			addToPopupMenu(m, PopupMenuOptions::ImageTable, "Image Pool Table");
			addToPopupMenu(m, PopupMenuOptions::SampleMapPoolTable, "SampleMap Pool Table");
			addToPopupMenu(m, PopupMenuOptions::MidiFilePoolTable, "MidiFile Pool Table");

			PopupMenu fm;

			addToPopupMenu(fm, PopupMenuOptions::InterfaceContent, "Main Interface");
			addToPopupMenu(fm, PopupMenuOptions::PerformanceStatistics, "Performance Statistics");
			addToPopupMenu(fm, PopupMenuOptions::ActivityLed, "MIDI Activity LED");
			addToPopupMenu(fm, PopupMenuOptions::PluginSettings, "Plugin Settings");
			addToPopupMenu(fm, PopupMenuOptions::MidiSourceList, "Midi Source List");
			addToPopupMenu(fm, PopupMenuOptions::MidiChannelList, "Midi Channel List");
			addToPopupMenu(fm, PopupMenuOptions::TooltipPanel, "Tooltip Bar");
			addToPopupMenu(fm, PopupMenuOptions::MidiLearnPanel, "MIDI Learn Panel");
			addToPopupMenu(fm, PopupMenuOptions::WavetableWaterfall, "WavetableWaterfall");
			addToPopupMenu(fm, PopupMenuOptions::MidiPlayerOverlay, "MIDI Player Overlay");
			addToPopupMenu(fm, PopupMenuOptions::SampleMapBrowser, "Sample Map Browser");
			addToPopupMenu(fm, PopupMenuOptions::AudioAnalyser, "Audio Analyser");
            addToPopupMenu(fm, PopupMenuOptions::MatrixPeakMeterPanel, "MatrixPeakMeter");
			addToPopupMenu(fm, PopupMenuOptions::MPEPanel, "MPE Panel");
			addToPopupMenu(fm, PopupMenuOptions::MarkdownPreviewPanel, "Markdown Panel");

			m.addSubMenu("Frontend Panels", fm);

			m.addSeparator();

			PopupMenu icons;

			m.addSubMenu("Icons", icons);


		}

		
	}

	m.addItem((int)PopupMenuOptions::toggleGlobalLayoutMode, "Toggle Global Layout Mode", true, parent->isLayoutModeEnabled());
	m.addItem((int)PopupMenuOptions::exportAsJSON, "Export as JSON", true, false);
	m.addItem((int)PopupMenuOptions::loadFromJSON, "Load JSON from clipboard", parent->canBeDeleted(), false);

	auto popupDir = ProjectHandler::getAppDataDirectory(nullptr).getChildFile("custom_popups");

	auto fileList = popupDir.findChildFiles(File::findFiles, false, "*.json");
	fileList.sort();

	PopupMenu customPopups;

	static constexpr int CustomOffset = 90000;
	int cIndex = CustomOffset;

	for (auto f : fileList)
		customPopups.addItem(cIndex++, f.getFileNameWithoutExtension());

	m.addSubMenu("Custom Popups", customPopups);

	const int result = m.show();

	if (result >= CustomOffset)
	{
		auto f = fileList[result - CustomOffset];
		auto obj = JSON::parse(f);

		if (obj.isObject())
		{
			parent->loadFromJSON(f.loadFileAsString());
			
		}

		return;
	}

	if (result > (int)PopupMenuOptions::MenuCommandOffset)
	{
		parent->setNewContent("Icon");
		return;
	}

	PopupMenuOptions resultOption = (PopupMenuOptions)result;

	if(handleBackendMenu(resultOption, parent))
		return;


	switch (resultOption)
	{
	case PopupMenuOptions::Cancel:				return;
	case PopupMenuOptions::Empty:				parent->setNewContent(GET_PANEL_NAME(EmptyComponent)); break;
	case PopupMenuOptions::Spacer:				parent->setNewContent(GET_PANEL_NAME(SpacerPanel)); break;
	case PopupMenuOptions::HorizontalTile:		parent->setNewContent(GET_PANEL_NAME(HorizontalTile)); break;
	case PopupMenuOptions::VerticalTile:		parent->setNewContent(GET_PANEL_NAME(VerticalTile)); break;
	case PopupMenuOptions::Tabs:				parent->setNewContent(GET_PANEL_NAME(FloatingTabComponent)); break;
	case PopupMenuOptions::VisibilityToggleBar:	parent->setNewContent(GET_PANEL_NAME(VisibilityToggleBar)); break;

	case PopupMenuOptions::Matrix2x2:			FloatingPanelTemplates::create2x2Matrix(parent); break;
	case PopupMenuOptions::ThreeColumns:		FloatingPanelTemplates::create3Columns(parent); break;
	case PopupMenuOptions::ThreeRows:			FloatingPanelTemplates::create3Rows(parent); break;
	case PopupMenuOptions::Note:				parent->setNewContent(GET_PANEL_NAME(Note)); break;
	case PopupMenuOptions::MacroControls:		parent->setNewContent(GET_PANEL_NAME(GenericPanel<MacroComponent>)); break;
	case PopupMenuOptions::FrontendMacroPanel:   parent->setNewContent(GET_PANEL_NAME(FrontendMacroPanel)); break;
	case PopupMenuOptions::MidiKeyboard:		parent->setNewContent(GET_PANEL_NAME(MidiKeyboardPanel)); break;
	case PopupMenuOptions::TablePanel:			parent->setNewContent(GET_PANEL_NAME(TableEditorPanel)); break;
	case PopupMenuOptions::SampleConnector:		parent->setNewContent(GET_PANEL_NAME(GlobalConnectorPanel<ModulatorSampler>)); break;
	case PopupMenuOptions::SampleEditor:		parent->setNewContent(GET_PANEL_NAME(SampleEditorPanel)); break;
	case PopupMenuOptions::SampleMapEditor:		parent->setNewContent(GET_PANEL_NAME(SampleMapEditorPanel)); break;
	case PopupMenuOptions::SamplerTable:		parent->setNewContent(GET_PANEL_NAME(SamplerTablePanel)); break;
	case PopupMenuOptions::WavetablePreview:	parent->setNewContent(GET_PANEL_NAME(WaveformComponent::Panel)); break;
	case PopupMenuOptions::AHDSRGraph:			parent->setNewContent(GET_PANEL_NAME(AhdsrEnvelope::Panel)); break;
	case PopupMenuOptions::MarkdownPreviewPanel:parent->setNewContent(GET_PANEL_NAME(MarkdownPreviewPanel)); break;
	case PopupMenuOptions::MarkdownEditor:		parent->setNewContent(GET_PANEL_NAME(MarkdownEditorPanel)); break;
	case PopupMenuOptions::FilterGraphPanel:	parent->setNewContent(GET_PANEL_NAME(FilterGraph::Panel)); break;
	case PopupMenuOptions::DraggableFilterPanel:parent->setNewContent(GET_PANEL_NAME(FilterDragOverlay::Panel)); break;
	case PopupMenuOptions::MPEPanel:			parent->setNewContent(GET_PANEL_NAME(MPEPanel)); break;
	case PopupMenuOptions::ScriptEditor:		parent->setNewContent(GET_PANEL_NAME(CodeEditorPanel)); break;
	case PopupMenuOptions::ScriptContent:		parent->setNewContent(GET_PANEL_NAME(ScriptContentPanel)); break;
	case PopupMenuOptions::OSCLogger:			parent->setNewContent(GET_PANEL_NAME(OSCLogger)); break;

	


	case PopupMenuOptions::InterfaceContent:	parent->setNewContent(GET_PANEL_NAME(InterfaceContentPanel)); break;
	case PopupMenuOptions::Plotter:				parent->setNewContent(GET_PANEL_NAME(PlotterPanel)); break;
	case PopupMenuOptions::AudioAnalyser:		parent->setNewContent(GET_PANEL_NAME(AudioAnalyserComponent::Panel)); break;
	
        
    case PopupMenuOptions::MatrixPeakMeterPanel: parent->setNewContent(GET_PANEL_NAME(MatrixPeakMeter)); break;
	case PopupMenuOptions::ComplexDataManager:  parent->setNewContent(GET_PANEL_NAME(ComplexDataManager)); break;
	case PopupMenuOptions::DspNetworkGraph:		parent->setNewContent(GET_PANEL_NAME(scriptnode::DspNetworkGraphPanel)); break;
	case PopupMenuOptions::SliderPackPanel:		parent->setNewContent(GET_PANEL_NAME(SliderPackPanel)); break;
	case PopupMenuOptions::ScriptConnectorPanel:parent->setNewContent(GET_PANEL_NAME(GlobalConnectorPanel<JavascriptProcessor>)); break;
	case PopupMenuOptions::Console:				parent->setNewContent(GET_PANEL_NAME(ConsolePanel)); break;
	case PopupMenuOptions::PresetBrowser:		parent->setNewContent(GET_PANEL_NAME(PresetBrowserPanel)); break;
	case PopupMenuOptions::ActivityLed:		    parent->setNewContent(GET_PANEL_NAME(ActivityLedPanel)); break;
	case PopupMenuOptions::PluginSettings:		parent->setNewContent(GET_PANEL_NAME(CustomSettingsWindowPanel)); break;
	case PopupMenuOptions::PerformanceStatistics: parent->setNewContent(GET_PANEL_NAME(PerformanceLabelPanel)); break;
	case PopupMenuOptions::MidiSourceList:		parent->setNewContent(GET_PANEL_NAME(MidiSourcePanel)); break;
	case PopupMenuOptions::MidiChannelList:		parent->setNewContent(GET_PANEL_NAME(MidiChannelPanel)); break;
	case PopupMenuOptions::MidiLearnPanel:		parent->setNewContent(GET_PANEL_NAME(MidiLearnPanel)); break;
	case PopupMenuOptions::MidiPlayerOverlay:	parent->setNewContent(GET_PANEL_NAME(MidiOverlayPanel)); break;
	case PopupMenuOptions::TooltipPanel:		parent->setNewContent(GET_PANEL_NAME(TooltipPanel)); break;
	case PopupMenuOptions::WavetableWaterfall:	parent->setNewContent(GET_PANEL_NAME(WaterfallComponent::Panel)); break;
	
	case PopupMenuOptions::DspNodeParameterEditor: parent->setNewContent(GET_PANEL_NAME(scriptnode::NodePropertyPanel)); break;
    case PopupMenuOptions::DspFaustEditorPanel: parent->setNewContent(GET_PANEL_NAME(scriptnode::FaustEditorPanel)); break;
	
	case PopupMenuOptions::SampleMapBrowser:	parent->setNewContent(GET_PANEL_NAME(SampleMapBrowser)); break;
	case PopupMenuOptions::ServerController:	parent->setNewContent(GET_PANEL_NAME(ServerControllerPanel)); break;
	case PopupMenuOptions::AboutPage:			parent->setNewContent(GET_PANEL_NAME(AboutPagePanel)); break;
	case PopupMenuOptions::SnexEditor:			parent->setNewContent(GET_PANEL_NAME(SnexEditorPanel)); break;
	case PopupMenuOptions::ScriptBroadcasterMap:			parent->setNewContent(GET_PANEL_NAME(ScriptingObjects::ScriptBroadcasterPanel)); break;
#if HISE_INCLUDE_RLOTTIE
	case PopupMenuOptions::RLottieDevPanel:		parent->setNewContent(GET_PANEL_NAME(RLottieFloatingTile));
		break;
#endif
    case PopupMenuOptions::PerfettoViewer:      parent->setNewContent(GET_PANEL_NAME(GenericPanel<PerfettoWebviewer>)); break;
	case PopupMenuOptions::AudioFileTable:		parent->setNewContent(GET_PANEL_NAME(PoolTableSubTypes::AudioFilePoolTable)); break;
	case PopupMenuOptions::SampleMapPoolTable:  parent->setNewContent(GET_PANEL_NAME(PoolTableSubTypes::SampleMapPoolTable)); break;
	case PopupMenuOptions::ImageTable:			parent->setNewContent(GET_PANEL_NAME(PoolTableSubTypes::ImageFilePoolTable)); break;
	case PopupMenuOptions::MidiFilePoolTable:  parent->setNewContent(GET_PANEL_NAME( PoolTableSubTypes::MidiFilePoolTable)); break;
	case PopupMenuOptions::ScriptWatchTable:		parent->setNewContent(GET_PANEL_NAME(GenericPanel<ScriptWatchTable>)); break;
	case PopupMenuOptions::toggleGlobalLayoutMode:    parent->getRootFloatingTile()->setLayoutModeEnabled(!parent->isLayoutModeEnabled()); break;
	case PopupMenuOptions::exportAsJSON:		SystemClipboard::copyTextToClipboard(parent->exportAsJSON()); break;
	case PopupMenuOptions::loadFromJSON:		parent->loadFromJSON(SystemClipboard::getTextFromClipboard()); break;
	case PopupMenuOptions::numOptions:
	default:
		jassertfalse;
		break;
	}
#else

	ignoreUnused(m, parent);
	
#endif
}


} // namespace hise
