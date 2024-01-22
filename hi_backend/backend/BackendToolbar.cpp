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

#if 0
Path MainToolbarFactory::MainToolbarPaths::createPath(int id, bool isOn)
{
	Path path;

	switch(id)
	{
		case BackendCommandTarget::HamburgerMenu:
		{
			path.loadPathFromData(BackendBinaryData::ToolbarIcons::hamburgerIcon, sizeof(BackendBinaryData::ToolbarIcons::hamburgerIcon));
			break;
		}
        case BackendCommandTarget::MenuViewShowPluginPopupPreview:
		{
		
		path.loadPathFromData (BackendBinaryData::ToolbarIcons::customInterface, sizeof (BackendBinaryData::ToolbarIcons::customInterface));

		break;
		}
	case BackendCommandTarget::ModulatorList:
		{
			path.loadPathFromData (BackendBinaryData::ToolbarIcons::modulatorList, sizeof (BackendBinaryData::ToolbarIcons::modulatorList));

		break;

		}

	case BackendCommandTarget::ViewPanel:
		{
		
		path.loadPathFromData (BackendBinaryData::ToolbarIcons::viewPanel, sizeof (BackendBinaryData::ToolbarIcons::viewPanel));
		break;
		}
	case BackendCommandTarget::Mixer:
		{
		path.loadPathFromData (BackendBinaryData::ToolbarIcons::mixer, sizeof (BackendBinaryData::ToolbarIcons::mixer));
		break;

		}
	case BackendCommandTarget::Keyboard:
		{
		path.loadPathFromData (BackendBinaryData::ToolbarIcons::keyboard, sizeof (BackendBinaryData::ToolbarIcons::keyboard));
		break;

		}
	case BackendCommandTarget::DebugPanel:
		{
		path.loadPathFromData (BackendBinaryData::ToolbarIcons::debugPanel, sizeof (BackendBinaryData::ToolbarIcons::debugPanel));
		break;
		}
	case BackendCommandTarget::Settings:
		{
			path.loadPathFromData (BackendBinaryData::ToolbarIcons::settings, sizeof (BackendBinaryData::ToolbarIcons::settings));
		break;
		}
	case BackendCommandTarget::Macros:
		{
			path.loadPathFromData (BackendBinaryData::ToolbarIcons::macros, sizeof (BackendBinaryData::ToolbarIcons::macros));
		break;
		}
	
	default: jassertfalse;
	}
	
	return path;
};
#endif

juce::Path MainToolbarFactory::createPath(const String& id) const
{
	auto url = MarkdownLink::Helpers::getSanitizedFilename(id);
	Path p;

	LOAD_EPATH_IF_URL("back", MainToolbarIcons::back);
	LOAD_EPATH_IF_URL("forward", MainToolbarIcons::forward);
    LOAD_EPATH_IF_URL("custom-popup", MainToolbarIcons::customPopup);
	LOAD_EPATH_IF_URL("keyboard", BackendBinaryData::ToolbarIcons::keyboard);
	LOAD_EPATH_IF_URL("macro-controls", HiBinaryData::SpecialSymbols::macros);
	LOAD_EPATH_IF_URL("preset-browser", MainToolbarIcons::presetBrowser);
	LOAD_EPATH_IF_URL("plugin-preview", MainToolbarIcons::home);
	LOAD_EPATH_IF_URL("main-workspace", MainToolbarIcons::mainWorkspace);
	LOAD_EPATH_IF_URL("scripting-workspace", HiBinaryData::SpecialSymbols::scriptProcessor);
	LOAD_EPATH_IF_URL("sampler-workspace", MainToolbarIcons::samplerWorkspace);
	LOAD_PATH_IF_URL("custom-workspace", ColumnIcons::customizeIcon);
	LOAD_EPATH_IF_URL("settings", MainToolbarIcons::settings);
	LOAD_EPATH_IF_URL("help", MainToolbarIcons::help);
	LOAD_EPATH_IF_URL("hise", MainToolbarIcons::hise);
	LOAD_EPATH_IF_URL("quickplay", MainToolbarIcons::quickplay);
	LOAD_EPATH_IF_URL("quicknote", MainToolbarIcons::quicknote);

	return p;

}

} // namespace hise
