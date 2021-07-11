/*  ===========================================================================
*
*   This file is part of HISE.
*   Copyright 2016 Christoph Hart
*
*   HISE is free software: you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation, either version 3 of the License, or
*   (at your option any later version.
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


namespace hise {
using namespace juce;

void SnexTileBase::setCurrentFile(const File& f)
{
	currentFile = f;

	auto nh = SnexWorkbenchEditor::getNetworkHolderForNewFile(getMainController(), false);

	auto df = new hise::DspNetworkCodeProvider(nullptr, getMainController(), nh, f);

	auto wb = dynamic_cast<BackendProcessor*>(getMainController())->workbenches.getWorkbenchDataForCodeProvider(df, true);

	

	wb->setCompileHandler(new hise::DspNetworkCompileHandler(wb, getMainController(), nh));

	//wb->setCompileHandler(new snex::JitNodeCompileThread(wb, getMainController()->getGlobalUIUpdater()));

	workbenchChanged(wb);

	setCustomTitle(f.getFileName());

	getParentShell()->refreshRootLayout();

	resized();
}



SnexEditorPanel::SnexEditorPanel(FloatingTile* parent) :
	FloatingTileContent(parent)
{
	dynamic_cast<BackendProcessor*>(getMainController())->workbenches.addListener(this);
}

SnexEditorPanel::~SnexEditorPanel()
{
	dynamic_cast<BackendProcessor*>(getMainController())->workbenches.removeListener(this);

	if (wb != nullptr)
		wb->removeListener(this);
}

void SnexEditorPanel::recompiled(snex::ui::WorkbenchData::Ptr)
{
	if (auto dnp = dynamic_cast<DspNetworkCodeProvider*>(wb->getCodeProvider()))
	{
		if (dnp->source == DspNetworkCodeProvider::SourceMode::InterpretedNode)
		{
			//playground->updateTextFromCodeProvider();
		}
	}
}

void SnexEditorPanel::paint(Graphics& g)
{
	auto b = FloatingTileContent::getParentShell()->getContentBounds();

	g.fillAll(Colour(0xFF1d1d1d));

	if (playground == nullptr)
	{
		g.setColour(Colours::white.withAlpha(0.3f));
		g.setFont(GLOBAL_BOLD_FONT());
		g.drawText("No code to display", b.toFloat(), Justification::centred);
	}
}

}