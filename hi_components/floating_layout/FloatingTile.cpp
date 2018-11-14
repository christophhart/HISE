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

FloatingTilePopup::FloatingTilePopup(Component* content_, Component* attachedComponent_, Point<int> localPoint) :
	content(content_),
	attachedComponent(attachedComponent_),
	localPointInComponent(localPoint)
{
	
	setColour((int)ColourIds::backgroundColourId, Colours::black.withAlpha(0.96f));

	setColour(Slider::ColourIds::textBoxOutlineColourId, Colours::white.withAlpha(0.6f));

	if (auto ftc = dynamic_cast<FloatingTile*>(content_))
	{
		auto c = ftc->getCurrentFloatingPanel()->findPanelColour(FloatingTileContent::PanelColourId::bgColour);

		setColour((int)ColourIds::backgroundColourId, c);

		auto c2 = ftc->getCurrentFloatingPanel()->findPanelColour(FloatingTileContent::PanelColourId::itemColour3);

		setColour(Slider::ColourIds::textBoxOutlineColourId, c2);
	}

	addAndMakeVisible(content);
	addAndMakeVisible(closeButton = new ImageButton("Close"));

	static const unsigned char popupData[] =
	{ 137,80,78,71,13,10,26,10,0,0,0,13,73,72,68,82,0,0,0,48,0,0,0,48,8,6,0,0,0,87,2,249,135,0,0,0,4,103,65,77,65,0,0,177,143,11,252,97,5,0,0,0,32,99,72,82,77,0,0,122,38,0,0,128,132,0,0,250,0,0,0,128,232,0,0,117,48,0,0,234,96,0,0,58,152,0,0,23,112,156,186,
		81,60,0,0,0,6,98,75,71,68,0,0,0,0,0,0,249,67,187,127,0,0,0,9,112,72,89,115,0,0,11,18,0,0,11,18,1,210,221,126,252,0,0,10,86,73,68,65,84,104,222,213,154,125,76,84,103,22,135,159,247,206,7,87,113,168,85,70,28,88,69,86,171,91,235,224,160,75,21,65,210,53,
		139,180,118,77,154,173,145,53,88,106,130,109,108,218,77,91,181,146,40,182,238,150,110,45,141,77,235,198,255,176,17,108,249,104,194,134,186,180,1,187,73,141,8,152,90,190,89,88,108,171,142,148,65,190,169,99,117,152,97,238,221,63,230,14,139,20,144,177,84,
		179,39,185,201,29,238,157,119,158,223,57,231,61,239,23,240,127,110,98,58,218,56,125,250,180,105,249,242,229,9,6,131,33,81,167,211,61,44,132,120,8,8,3,102,105,239,220,0,186,84,85,253,198,235,245,182,122,60,158,138,150,150,150,138,141,27,55,58,1,245,126,
		8,16,241,241,241,186,162,162,162,100,89,150,159,21,66,252,1,152,17,96,27,183,84,85,45,117,185,92,185,41,41,41,229,149,149,149,222,187,17,19,168,0,1,72,29,29,29,127,12,10,10,202,20,66,68,3,40,138,162,212,212,212,180,126,254,249,231,61,245,245,245,114,
		79,79,207,92,183,219,29,170,170,234,76,0,33,196,77,163,209,216,107,54,155,251,108,54,155,107,211,166,77,230,213,171,87,63,44,73,146,4,160,170,106,227,208,208,208,95,35,34,34,74,0,37,16,33,129,8,144,234,234,234,150,46,88,176,224,239,66,136,223,3,116,118,
		118,118,102,103,103,183,85,85,85,45,85,20,37,60,16,79,72,146,228,88,183,110,221,197,125,251,246,45,179,88,44,22,77,200,191,218,219,219,255,28,19,19,115,81,19,50,45,2,4,160,187,114,229,202,159,76,38,211,49,32,100,112,112,176,127,207,158,61,205,141,141,
		141,107,1,99,128,81,28,107,238,232,232,232,243,71,142,28,89,49,123,246,236,57,192,117,167,211,249,210,162,69,139,10,128,59,166,149,116,39,248,168,168,40,163,195,225,200,50,153,76,39,129,144,194,194,194,234,228,228,100,26,27,27,19,167,1,30,192,216,216,
		216,152,152,156,156,76,65,65,65,53,16,98,50,153,242,28,14,71,86,84,84,148,241,78,78,158,236,161,136,138,138,50,86,87,87,191,111,48,24,118,121,189,222,225,244,244,244,170,230,230,230,196,105,128,158,208,86,172,88,113,246,248,241,227,235,116,58,157,222,
		227,241,228,196,197,197,189,116,249,242,101,55,19,68,98,34,1,2,48,116,116,116,252,77,150,229,61,46,151,235,230,211,79,63,253,239,174,174,174,216,95,18,222,111,97,97,97,23,138,139,139,31,145,101,121,166,203,229,58,18,17,17,177,31,240,140,39,98,188,20,
		18,128,161,181,181,53,85,150,229,61,94,175,119,120,203,150,45,205,247,10,30,160,171,171,43,118,203,150,45,205,94,175,119,88,150,229,61,109,109,109,169,128,129,113,28,62,158,0,93,113,113,241,242,121,243,230,189,15,144,158,158,94,125,237,218,181,71,239,
		21,188,223,174,93,187,246,104,122,122,122,53,64,104,104,232,7,197,197,197,203,1,221,157,4,72,128,156,144,144,112,4,8,201,207,207,175,110,106,106,90,175,170,42,247,227,106,106,106,90,255,209,71,31,85,1,38,141,73,30,203,44,198,220,27,107,107,107,83,34,
		35,35,115,7,7,7,251,147,146,146,80,20,101,206,189,246,254,109,128,66,12,156,62,125,90,153,51,103,206,92,187,221,254,236,170,85,171,138,128,145,78,61,90,141,100,50,153,130,23,44,88,176,23,224,229,151,95,110,190,223,240,0,170,170,62,248,234,171,175,182,
		0,44,92,184,112,175,201,100,10,30,205,237,191,17,128,161,180,180,52,73,146,36,107,71,71,135,163,185,185,57,238,126,195,251,173,185,185,121,109,71,71,135,67,8,97,253,236,179,207,54,50,170,67,251,5,72,192,140,37,75,150,164,0,100,103,103,95,84,85,213,48,
		149,60,77,76,76,164,161,161,129,55,222,120,195,239,177,73,47,33,4,89,89,89,212,213,213,17,31,31,63,213,254,96,200,206,206,190,8,176,120,241,226,173,248,38,142,210,104,1,186,164,164,36,179,44,203,79,40,138,162,84,86,86,46,155,170,119,142,29,59,70,72,72,
		8,59,118,236,32,43,43,11,33,38,30,27,37,73,226,240,225,195,164,166,166,50,123,246,108,142,30,61,58,229,40,84,86,86,46,83,20,69,145,101,249,137,164,164,36,51,90,69,146,180,80,4,189,242,202,43,107,0,185,166,166,166,85,81,20,203,84,27,254,244,211,79,71,
		238,83,83,83,39,20,225,135,223,186,117,235,200,223,74,74,74,166,44,64,81,20,75,77,77,77,43,32,107,172,65,128,144,52,17,65,139,23,47,126,20,224,212,169,83,61,83,110,21,200,204,204,164,168,168,104,82,17,227,193,231,229,229,241,230,155,111,6,242,83,148,
		150,150,246,0,104,172,65,128,164,215,4,200,33,33,33,15,1,212,215,215,203,170,58,245,117,133,170,170,100,100,100,0,144,146,146,50,34,66,85,85,50,51,51,17,66,140,11,255,250,235,175,19,200,239,0,212,214,214,202,0,26,171,236,23,160,7,100,131,193,16,5,208,
		211,211,19,26,80,171,19,136,216,190,125,59,0,70,163,113,90,224,71,179,105,172,50,160,215,227,235,12,70,33,68,40,128,219,237,190,171,218,63,153,136,233,128,31,205,166,177,26,1,157,164,9,48,8,33,102,105,32,179,238,170,245,81,34,10,11,11,127,242,44,55,55,
		247,103,193,143,102,211,88,13,128,78,143,182,226,26,13,241,115,76,8,129,182,212,189,205,116,58,221,72,93,159,38,211,161,85,33,1,72,170,170,222,208,0,110,220,109,139,146,36,145,157,157,125,91,206,251,109,251,246,237,188,245,214,91,147,142,19,83,112,206,
		13,0,141,85,242,11,0,192,235,245,246,1,24,141,198,254,233,130,207,205,205,165,160,160,96,218,68,248,217,252,172,224,171,64,42,160,12,13,13,217,245,122,253,67,102,179,185,183,189,189,125,201,116,192,31,60,120,112,228,243,182,109,219,70,68,168,170,202,
		129,3,7,2,78,39,179,217,220,11,44,25,26,26,178,163,109,191,72,154,0,111,119,119,183,29,96,229,202,149,174,64,225,223,125,247,221,113,225,253,57,159,145,145,113,91,36,158,121,230,153,187,138,68,76,76,140,11,64,99,245,250,5,120,1,79,69,69,197,55,0,155,
		55,111,14,104,28,120,251,237,183,39,132,247,219,68,34,14,29,58,20,144,128,205,155,55,155,1,52,86,15,224,21,248,102,118,17,145,145,145,171,106,107,107,115,21,69,49,174,94,189,186,107,170,243,161,150,150,22,76,38,19,0,39,78,156,152,180,84,10,33,120,231,
		157,119,70,210,169,183,183,151,152,152,152,41,193,75,146,212,89,83,83,19,38,73,146,123,213,170,85,207,218,237,246,90,160,67,2,134,1,151,221,110,31,232,235,235,251,82,146,36,41,62,62,190,109,170,203,190,93,187,118,49,56,56,72,78,78,14,7,15,30,68,81,148,
		9,223,85,20,133,125,251,246,145,151,151,71,127,127,63,47,188,240,194,148,151,151,241,241,241,109,146,36,73,3,3,3,95,218,237,246,1,192,5,12,251,199,128,7,129,37,135,15,31,222,248,220,115,207,253,197,225,112,116,62,254,248,227,115,153,158,141,171,233,48,
		119,89,89,89,95,120,120,184,37,39,39,231,80,70,70,70,57,240,45,48,32,225,235,205,67,192,245,3,7,14,124,227,241,120,90,194,195,195,45,54,155,237,252,253,166,246,155,213,106,61,31,30,30,110,241,120,60,45,251,247,239,191,8,92,215,152,21,127,21,26,2,156,
		94,175,183,183,176,176,240,20,192,209,163,71,87,8,33,238,106,76,152,78,19,66,244,31,59,118,108,5,64,81,81,209,63,181,49,192,169,49,171,163,247,89,4,16,84,86,86,166,123,241,197,23,205,33,33,33,203,130,131,131,235,42,43,43,23,220,79,1,187,119,239,174,91,
		187,118,237,210,27,55,110,84,37,39,39,151,3,87,128,110,124,125,64,245,143,196,10,112,11,232,3,186,118,238,220,89,10,56,211,210,210,214,69,71,71,87,220,47,120,171,213,90,177,99,199,142,117,128,243,249,231,159,47,5,186,52,198,91,26,243,200,154,88,197,87,
		87,175,3,157,95,124,241,197,213,143,63,254,248,56,192,137,19,39,214,88,44,150,175,238,53,188,197,98,249,42,55,55,119,13,64,126,126,254,135,229,229,229,118,160,83,99,28,217,39,253,201,198,22,16,10,44,6,150,159,57,115,38,209,106,181,110,115,185,92,55,159,
		124,242,201,150,238,238,238,223,222,11,248,176,176,176,11,165,165,165,143,200,178,60,179,169,169,169,224,177,199,30,59,11,180,0,223,1,189,76,176,177,229,143,194,15,128,3,184,186,97,195,134,175,47,93,186,244,15,89,150,103,150,151,151,175,180,90,173,21,
		191,244,118,162,213,106,173,40,43,43,179,201,178,60,243,202,149,43,37,27,54,108,248,26,184,170,49,253,192,152,93,234,177,19,119,5,95,231,232,3,236,138,162,92,94,179,102,205,87,13,13,13,249,122,189,222,144,159,159,191,254,181,215,94,171,146,36,105,218,
		171,147,36,73,253,123,247,238,173,202,207,207,95,175,215,235,13,13,13,13,249,177,177,177,231,21,69,185,12,216,53,38,23,99,142,158,116,227,180,165,226,155,31,13,3,94,85,85,201,205,205,117,207,159,63,255,91,155,205,182,204,102,179,45,73,73,73,185,85,83,
		83,115,161,171,171,203,50,65,27,129,152,59,58,58,186,242,147,79,62,153,23,23,23,183,20,184,126,242,228,201,15,183,109,219,214,166,170,234,119,90,218,116,226,59,170,29,30,251,229,73,15,56,128,7,128,112,224,215,192,162,196,196,68,75,94,94,94,130,201,100,
		138,3,112,56,28,157,89,89,89,23,207,157,59,183,76,81,148,249,1,122,252,90,66,66,66,91,102,102,230,210,240,240,112,11,128,211,233,172,78,75,75,59,119,246,236,217,78,124,229,242,18,19,164,206,157,4,140,22,17,130,239,208,58,18,88,8,88,178,178,178,66,211,
		211,211,127,103,52,26,31,6,223,49,235,133,11,23,90,75,74,74,122,234,235,235,229,238,238,238,185,110,183,219,172,170,106,48,128,16,226,71,163,209,216,51,111,222,188,62,155,205,230,122,234,169,167,204,177,177,177,35,199,172,110,183,187,229,248,241,227,
		103,50,51,51,123,53,111,95,213,210,166,139,49,85,39,16,1,163,69,4,3,115,181,104,252,10,176,72,146,52,103,247,238,221,33,59,119,238,252,141,217,108,78,192,183,205,17,136,185,122,122,122,206,229,228,228,252,231,189,247,222,187,174,40,74,191,6,255,189,230,
		245,62,224,199,201,224,167,34,192,255,142,78,3,124,0,95,153,181,104,81,9,5,30,8,13,13,157,153,150,150,54,99,211,166,77,161,145,145,145,97,38,147,41,66,175,215,135,10,33,130,1,84,85,253,113,120,120,184,207,233,116,126,223,222,222,222,85,90,90,218,155,
		151,151,119,171,183,183,247,166,150,30,189,154,183,59,181,251,31,240,117,216,59,30,179,6,116,208,173,69,99,38,190,180,154,171,9,152,131,111,54,59,75,123,102,212,222,211,241,191,42,167,104,48,30,124,53,252,38,190,78,57,0,244,107,208,125,248,210,229,166,
		246,222,180,29,116,143,125,223,47,100,134,6,109,210,4,153,70,137,8,210,132,140,22,224,198,55,1,243,195,59,53,96,167,246,249,214,40,240,95,228,95,13,198,19,162,211,96,131,240,165,152,60,65,4,84,124,37,112,88,19,225,210,174,33,237,242,6,10,238,183,255,
		2,195,248,174,88,205,91,102,85,0,0,0,0,73,69,78,68,174,66,96,130,0,0 };

	Image img = ImageCache::getFromMemory(popupData, sizeof(popupData));

	closeButton->setImages(false, true, true, img, 1.0f, Colour(0),
		                                      img, 1.0f, Colours::white.withAlpha(0.05f),
											  img, 1.0f, Colours::white.withAlpha(0.1f));

	content->addComponentListener(this);

	attachedComponent->addComponentListener(this);

	
	closeButton->addListener(this);

	const int offset = 12;

	const int titleOffset = hasTitle() ? 20 : 0;

	setSize(content->getWidth() + 2*offset, content->getHeight() + 24 + titleOffset + 12);
}

void FloatingTilePopup::paint(Graphics &g)
{
	g.setColour(findColour((int)ColourIds::backgroundColourId));

	if (arrowX > 0)
	{
		Path p;

		float l = (float)getWidth() - 12.0f;
		float yOff = 12.0f;
		float arc = 5.0f;
		float h = (float)getHeight() - 12.0f;

		p.startNewSubPath(arc + 1.0f, yOff);

		if (!arrowAtBottom)
		{
			p.lineTo((float)arrowX - 12.0f, yOff);
			p.lineTo((float)arrowX, 0.0f);
			p.lineTo((float)arrowX + 12.0f, yOff);
		}

		
		p.lineTo(l - arc, yOff);
		p.addArc(l - 2.0f*arc, yOff, 2.0f * arc, 2.f * arc, 0.0f, float_Pi / 2.0f);
		p.lineTo(l, h - arc);
		p.addArc(l - 2.0f*arc, h - 2.f*arc, 2.f * arc, 2.f * arc, float_Pi / 2.0f, float_Pi);
		

		if (arrowAtBottom)
		{
			p.lineTo((float)arrowX + 12.0f, h);
			p.lineTo((float)arrowX, (float)getHeight()-1.0f);
			p.lineTo((float)arrowX - 12.0f, h);
		}

		p.lineTo(arc + 1.0f, h);
		p.addArc(1.0f, h - 2.f*arc, 2.f * arc, 2.f * arc, float_Pi, float_Pi * 1.5f);

		p.lineTo(1.0f, yOff + arc);
		p.addArc(1.0f, yOff, 2.f * arc, 2.f * arc, float_Pi * 1.5f, float_Pi * 2.f);

		p.closeSubPath();

		g.fillPath(p);


		g.setColour(findColour(Slider::ColourIds::textBoxOutlineColourId));

		
		g.strokePath(p, PathStrokeType(2.0f));

	}
	else
	{
		Rectangle<float> area(0.0f, 12.0f, (float)getWidth() - 12.0f, (float)getHeight() - 24.0f);

		g.fillRoundedRectangle(area, 5.0f);
		g.setColour(Colours::white.withAlpha(0.6f));
		g.drawRoundedRectangle((float)area.getX() + 1.0f, (float)area.getY() + 1.0f, (float)area.getWidth() - 2.0f, (float)area.getHeight() - 2.0f, 5.0f, 2.0f);
	}

	if (hasTitle())
	{
		g.setColour(Colours::white);
		g.setFont(GLOBAL_BOLD_FONT());
		g.drawText(content->getName(), 0, 15, getWidth(), 20, Justification::centred, false);
	}

#if 0
	if (HiseDeviceSimulator::isMobileDevice())
	{
		g.setColour(Colours::black);
		g.fillEllipse((float)getWidth() - 26.0f, 2.0f, 24.0f, 24.0f);
		g.setColour(Colours::white.withAlpha(0.5f));
		g.drawEllipse((float)getWidth() - 26.0f, 2.0f, 24.0f, 24.0f, 2.0f);

	}
	else
	{
		g.setColour(Colours::black);
		g.fillEllipse((float)getWidth() - 22.0f, 2.0f, 20.0f, 20.0f);
		g.setColour(Colours::white.withAlpha(0.5f));
		g.drawEllipse((float)getWidth() - 22.0f, 2.0f, 20.0f, 20.0f, 2.0f);

	}
#endif


	
}

void FloatingTilePopup::resized()
{
	closeButton->setBounds(getWidth() - 24, 0, 24, 24);
	
	if (hasTitle())
	{
		content->setBounds(6, 18 + 20, content->getWidth(), content->getHeight());
	}
	else
	{
		content->setBounds(6, 18, content->getWidth(), content->getHeight());
	}

	
}

void FloatingTilePopup::deleteAndClose()
{
	if (attachedComponent.getComponent())
		attachedComponent->removeComponentListener(this);

	attachedComponent = nullptr;
	updatePosition();
}

void FloatingTilePopup::componentMovedOrResized(Component& component, bool /*moved*/, bool wasResized)
{
	if (&component == attachedComponent.getComponent())
	{
		updatePosition();
	}
	else
	{
		if (wasResized)
		{
			const int offset = 12;

			setSize(content->getWidth() + 2 * offset, content->getHeight() + 24 + 20 + 12);

			updatePosition();
		}
	}
}


void FloatingTilePopup::componentBeingDeleted(Component& component)
{
	if (&component == attachedComponent)
	{
		component.removeComponentListener(this);
		attachedComponent = nullptr;
		updatePosition();
	}
}

void FloatingTilePopup::buttonClicked(Button* /*b*/)
{
	deleteAndClose();
}

FloatingTilePopup::~FloatingTilePopup()
{
	if (content != nullptr) content->removeComponentListener(this);
	if (attachedComponent.getComponent()) attachedComponent->removeComponentListener(this);

	content = nullptr;
	closeButton = nullptr;
	attachedComponent = nullptr;
}


var FloatingTile::LayoutData::toDynamicObject() const
{
	return var(layoutDataObject);
}

void FloatingTile::LayoutData::fromDynamicObject(const var& objectData)
{
	layoutDataObject = objectData.getDynamicObject();
}

int FloatingTile::LayoutData::getNumDefaultableProperties() const
{
	return LayoutDataIds::numProperties;
}

Identifier FloatingTile::LayoutData::getDefaultablePropertyId(int index) const
{
	RETURN_DEFAULT_PROPERTY_ID(index, LayoutDataIds::ID, "ID");
	RETURN_DEFAULT_PROPERTY_ID(index, LayoutDataIds::Size, "Size");
	RETURN_DEFAULT_PROPERTY_ID(index, LayoutDataIds::Folded, "Folded");
	RETURN_DEFAULT_PROPERTY_ID(index, LayoutDataIds::ForceFoldButton, "ForceFoldButton");
	RETURN_DEFAULT_PROPERTY_ID(index, LayoutDataIds::Visible, "Visible");
	RETURN_DEFAULT_PROPERTY_ID(index, LayoutDataIds::MinSize, "MinSize");

	jassertfalse;

	return Identifier();
}

FloatingTile::CloseButton::CloseButton() :
	ShapeButton("Close", Colours::white.withAlpha(0.2f), Colours::white.withAlpha(0.8f), Colours::white)
{
	Path p;
	p.loadPathFromData(HiBinaryData::ProcessorEditorHeaderIcons::closeIcon, sizeof(HiBinaryData::ProcessorEditorHeaderIcons::closeIcon));
	setShape(p, false, true, true);

	addListener(this);

}

void FloatingTile::CloseButton::buttonClicked(Button* )
{
	auto ec = dynamic_cast<FloatingTile*>(getParentComponent());

	if (ec->closeTogglesVisibility)
	{
		ec->getLayoutData().setVisible(!ec->getLayoutData().isVisible());
		ec->getParentContainer()->refreshLayout();
		ec->getParentContainer()->notifySiblingChange();
	}
	else
	{
		Desktop::getInstance().getAnimator().fadeOut(ec->content.get(), 300);

		if (!ec->isEmpty())
		{
			if (auto tc = dynamic_cast<FloatingTileContainer*>(ec->content.get()))
				tc->clear();

			ec->addAndMakeVisible(ec->content = new EmptyComponent(ec));

			Desktop::getInstance().getAnimator().fadeIn(ec->content.get(), 300);

			ec->clear();
		}
		else
		{
			auto* cl = findParentComponentOfClass<FloatingTileContainer>();

			cl->removeFloatingTile(ec);
		}
	}
}

FloatingTile::MoveButton::MoveButton() :
	ShapeButton("Move", Colours::white.withAlpha(0.2f), Colours::white.withAlpha(0.8f), Colours::white)
{
	Path p;
	p.loadPathFromData(ColumnIcons::moveIcon, sizeof(ColumnIcons::moveIcon));

	setShape(p, false, true, true);

	addListener(this);
}

void FloatingTile::MoveButton::buttonClicked(Button* )
{
	auto ec = dynamic_cast<FloatingTile*>(getParentComponent());

	PopupMenu m;

	m.setLookAndFeel(&ec->plaf);

	m.addItem(1, "Swap Position", !ec->isVital(), ec->getLayoutData().swappingEnabled);
	m.addItem(2, "Edit JSON", !ec->isVital(), false, ec->getPanelFactory()->getIcon(FloatingTileContent::Factory::PopupMenuOptions::ScriptEditor));

	if (ec->hasChildren())
	{
		PopupMenu containerTypes;

		bool isTabs = dynamic_cast<FloatingTabComponent*>(ec->getCurrentFloatingPanel()) != nullptr;
		bool isHorizontal = dynamic_cast<HorizontalTile*>(ec->getCurrentFloatingPanel()) != nullptr;
		bool isVertical = dynamic_cast<VerticalTile*>(ec->getCurrentFloatingPanel()) != nullptr;

		ec->getPanelFactory()->addToPopupMenu(containerTypes, FloatingTileContent::Factory::PopupMenuOptions::Tabs, "Tabs", !isTabs, isTabs);
		ec->getPanelFactory()->addToPopupMenu(containerTypes, FloatingTileContent::Factory::PopupMenuOptions::HorizontalTile, "Horizontal Tile", !isHorizontal, isHorizontal);
		ec->getPanelFactory()->addToPopupMenu(containerTypes, FloatingTileContent::Factory::PopupMenuOptions::VerticalTile, "Vertical Tile", !isVertical, isVertical);

		m.addSubMenu("Swap Container Type", containerTypes, !ec->isVital());	
	}

	const int result = m.show();

	if (result == 1)
	{
		ec->getRootFloatingTile()->enableSwapMode(!ec->layoutData.swappingEnabled, ec);
	}
	else if (result == 2)
	{
		ec->editJSON();
	}
	if (result == (int)FloatingTileContent::Factory::PopupMenuOptions::Tabs)
	{
		ec->swapContainerType(FloatingTabComponent::getPanelId());
	}
	else if (result == (int)FloatingTileContent::Factory::PopupMenuOptions::HorizontalTile)
	{
		ec->swapContainerType(HorizontalTile::getPanelId());
	}
	else if (result == (int)FloatingTileContent::Factory::PopupMenuOptions::VerticalTile)
	{
		ec->swapContainerType(VerticalTile::getPanelId());
	}
}

FloatingTile::ResizeButton::ResizeButton() :
	ShapeButton("Move", Colours::white.withAlpha(0.2f), Colours::white.withAlpha(0.8f), Colours::white)
{
	addListener(this);
}


void FloatingTile::ResizeButton::buttonClicked(Button* )
{
	auto pc = findParentComponentOfClass<FloatingTile>();

	pc->toggleAbsoluteSize();
}

FloatingTile::FoldButton::FoldButton() :
	ShapeButton("Fold", Colours::white.withAlpha(0.2f), Colours::white.withAlpha(0.8f), Colours::white.withAlpha(0.8f))
{
	addListener(this);
}

void FloatingTile::FoldButton::buttonClicked(Button* )
{
	auto pc = findParentComponentOfClass<FloatingTile>();
	
	if (!pc->canBeFolded())
		return;

	pc->setFolded(!pc->isFolded());

	if (auto cl = dynamic_cast<ResizableFloatingTileContainer*>(pc->getParentContainer()))
	{
		cl->enableAnimationForNextLayout();
		cl->refreshLayout();
	}
}

FloatingTile::ParentType FloatingTile::getParentType() const
{
	if (getParentContainer() == nullptr)
		return ParentType::Root;
	else if (auto sl = dynamic_cast<const ResizableFloatingTileContainer*>(getParentContainer()))
	{
		return sl->isVertical() ? ParentType::Vertical : ParentType::Horizontal;
	}
	else if (dynamic_cast<const FloatingTabComponent*>(getParentContainer()) != nullptr)
	{
		return ParentType::Tabbed;
	}

	jassertfalse;

	return ParentType::numParentTypes;
}




FloatingTile::FloatingTile(MainController* mc_, FloatingTileContainer* parent, var data) :
	Component("Empty"),
	mc(mc_),
	parentContainer(parent)
{
	setOpaque(true);

	panelFactory.registerAllPanelTypes();

	addAndMakeVisible(closeButton = new CloseButton());
	addAndMakeVisible(moveButton = new MoveButton());
	addAndMakeVisible(foldButton = new FoldButton());
	addAndMakeVisible(resizeButton = new ResizeButton());

	//layoutIcon.loadPathFromData(ColumnIcons::layoutIcon, sizeof(ColumnIcons::layoutIcon));

	setContent(data);
}

bool FloatingTile::isEmpty() const
{
	return dynamic_cast<const EmptyComponent*>(getCurrentFloatingPanel()) != nullptr;
}

bool FloatingTile::showTitle() const
{
	/** Show the title if:
	
		- is folded and in horizontal container
		- is not container and the panel wants a title in presentation mode
		- is dynamic container and in layout mode
		- is container and has a custom title or is foldable

		Don't show the title if:

		- is folded and in vertical container
		- is root
		- is in tab & layout mode is deactivated
	
	
	*/

	auto pt = getParentType();

	const bool isRoot = pt == ParentType::Root;
	const bool isInTab = pt == ParentType::Tabbed;
	const bool isDynamicContainer = dynamic_cast<const FloatingTileContainer*>(getCurrentFloatingPanel()) != nullptr && dynamic_cast<const FloatingTileContainer*>(getCurrentFloatingPanel())->isDynamic();

	if (isRoot && !isDynamicContainer)
		return false;

	if (isInTab && !isLayoutModeEnabled())
		return false;

	if (getLayoutData().mustShowFoldButton() && !isFolded())
		return false;

	if (isFolded())
		return isInVerticalLayout();

	if (hasChildren())
	{
		

		if (isDynamicContainer && isLayoutModeEnabled())
		{
			return true;
		}
		else
		{
			if (getCurrentFloatingPanel()->hasCustomTitle())
				return true;

			if (canBeFolded())
				return true;
		
			if (getCurrentFloatingPanel()->hasCustomTitle())
				return true;

			return false;
		}
			
	}
	else
	{
		return getCurrentFloatingPanel()->showTitleInPresentationMode();
	}
}

Rectangle<int> FloatingTile::getContentBounds()
{
	if (isFolded())
		return Rectangle<int>();

	if (showTitle())
		return Rectangle<int>(0, 16, getWidth(), getHeight() - 16);
	else
		return getLocalBounds();
}

void FloatingTile::setFolded(bool shouldBeFolded)
{
	if (!canBeFolded())
		return;

	layoutData.setFoldState(shouldBeFolded ? 1 : 0);

	refreshFoldButton();
}

void FloatingTile::refreshFoldButton()
{
	Path p;
	p.loadPathFromData(HiBinaryData::ProcessorEditorHeaderIcons::foldedIcon, sizeof(HiBinaryData::ProcessorEditorHeaderIcons::foldedIcon));

	bool rotate = isFolded();

	foldButton->setTooltip((isFolded() ? "Expand " : "Fold ") + getCurrentFloatingPanel()->getBestTitle());

	if (getParentType() == ParentType::Vertical)
		rotate = !rotate;

	if (rotate)
		p.applyTransform(AffineTransform::rotation(float_Pi / 2.0f));

	foldButton->setShape(p, false, true, true);
}

void FloatingTile::setCanBeFolded(bool shouldBeFoldable)
{
	if (!shouldBeFoldable)
		layoutData.setFoldState(-1);
	else
		layoutData.setFoldState(0);

	resized();
}

bool FloatingTile::canBeFolded() const
{
	return layoutData.canBeFolded();
}

void FloatingTile::refreshPinButton()
{
	Path p;

	if (layoutData.isAbsolute())
		p.loadPathFromData(ColumnIcons::absoluteIcon, sizeof(ColumnIcons::absoluteIcon));
	else
		p.loadPathFromData(ColumnIcons::relativeIcon, sizeof(ColumnIcons::relativeIcon));
	
	resizeButton->setShape(p, false, true, true);
}

void FloatingTile::toggleAbsoluteSize()
{
	if (auto pl = dynamic_cast<ResizableFloatingTileContainer*>(getParentContainer()))
	{
		bool shouldBeAbsolute = !layoutData.isAbsolute();

		int totalWidth = pl->getDimensionSize(pl->getContainerBounds());

		if (shouldBeAbsolute)
		{
			layoutData.setCurrentSize(pl->getDimensionSize(getLocalBounds()));
		}
		else
		{
			double newAbsoluteSize = layoutData.getCurrentSize() / (double)totalWidth * -1.0;

			layoutData.setCurrentSize(newAbsoluteSize);
		}

		refreshPinButton();

		pl->refreshLayout();
	}
}

const BackendRootWindow* FloatingTile::getBackendRootWindow() const
{
	auto rw = getRootFloatingTile()->findParentComponentOfClass<ComponentWithBackendConnection>()->getBackendRootWindow();
	
	//auto rw = dynamic_cast<ComponentWithBackendConnection*>(getRootFloatingTile()->getParentComponent())->getBackendRootWindow();

	jassert(rw != nullptr);

	return rw;
}

BackendRootWindow* FloatingTile::getBackendRootWindow()
{

	auto rw = getRootFloatingTile()->findParentComponentOfClass<ComponentWithBackendConnection>()->getBackendRootWindow();

	//auto rw = dynamic_cast<ComponentWithBackendConnection*>(getRootFloatingTile()->getParentComponent())->getBackendRootWindow();

	jassert(rw != nullptr);

	return rw;
}

FloatingTile* FloatingTile::getRootFloatingTile()
{
	if (getParentType() == ParentType::Root)
		return this;

	auto parent = getParentContainer()->getParentShell(); //findParentComponentOfClass<FloatingTile>();

	if (parent == nullptr)
		return nullptr;

	return parent->getRootFloatingTile();
}

const FloatingTile* FloatingTile::getRootFloatingTile() const
{
	return const_cast<FloatingTile*>(this)->getRootFloatingTile();
}

void FloatingTile::clear()
{
	layoutData.reset();
	refreshPinButton();
	refreshFoldButton();
	refreshMouseClickTarget();
	refreshRootLayout();

	if (getParentContainer())
	{
		getParentContainer()->notifySiblingChange();
		getParentContainer()->refreshLayout();
	}

	resized();
}

void FloatingTile::refreshRootLayout()
{
	if (getRootFloatingTile() != nullptr)
	{
		auto rootContainer = dynamic_cast<FloatingTileContainer*>(getRootFloatingTile()->getCurrentFloatingPanel());

		if(rootContainer != nullptr)
			rootContainer->refreshLayout();
	}
}

void FloatingTile::setLayoutModeEnabled(bool shouldBeEnabled)
{
	if (getParentType() == ParentType::Root)
	{
		layoutModeEnabled = shouldBeEnabled;

		resized();
		repaint();
		refreshMouseClickTarget();

		if (hasChildren())
		{
			dynamic_cast<FloatingTileContainer*>(getCurrentFloatingPanel())->refreshLayout();
		}

		Iterator<FloatingTileContent> all(this);

		while (auto p = all.getNextPanel())
		{
			if (auto c = dynamic_cast<FloatingTileContainer*>(p))
				c->refreshLayout();
			
			p->getParentShell()->resized();
			p->getParentShell()->repaint();
			p->getParentShell()->refreshMouseClickTarget();
		}
	}
}

bool FloatingTile::isLayoutModeEnabled() const
{
	if (getParentType() == ParentType::Root)
		return layoutModeEnabled;

	return canDoLayoutMode() && getRootFloatingTile()->isLayoutModeEnabled();
}

bool FloatingTile::canDoLayoutMode() const
{
	if (getParentType() == ParentType::Root)
		return true;

	if (isVital())
		return false;

	return getParentContainer()->isDynamic();
}


void FloatingTile::setAllowChildComponentCreation(bool shouldCreateChildComponents)
{
	jassert(getParentType() == ParentType::Root);

	allowChildComponentCreation = shouldCreateChildComponents;
}


bool FloatingTile::shouldCreateChildComponents() const
{
	if (getParentType() == ParentType::Root)
	{
		return allowChildComponentCreation;
	}
	else
	{
		return getRootFloatingTile()->shouldCreateChildComponents();
	}
}

bool FloatingTile::hasChildren() const
{
	return dynamic_cast<FloatingTileContainer*>(content.get()) != nullptr;
}

void FloatingTile::enableSwapMode(bool shouldBeEnabled, FloatingTile* source)
{
	currentSwapSource = source;

	layoutData.swappingEnabled = shouldBeEnabled;

	if (auto cl = dynamic_cast<FloatingTileContainer*>(content.get()))
	{
		cl->enableSwapMode(shouldBeEnabled, source);
	}

	repaint();
}

void FloatingTile::swapWith(FloatingTile* otherComponent)
{
	if (otherComponent->isParentOf(this) || isParentOf(otherComponent))
	{
		PresetHandler::showMessageWindow("Error", "Can't swap parents with their children", PresetHandler::IconType::Error);
		return;
	}

	removeChildComponent(content);
	otherComponent->removeChildComponent(otherComponent->content);

	content.swapWith(otherComponent->content);

	addAndMakeVisible(content);
	otherComponent->addAndMakeVisible(otherComponent->content);


	resized();
	otherComponent->resized();

	repaint();
	otherComponent->repaint();

	bringButtonsToFront();
	otherComponent->bringButtonsToFront();
}

void FloatingTile::bringButtonsToFront()
{
	if (getCurrentFloatingPanel())
	{
		closeButton->setTooltip("Delete " + getCurrentFloatingPanel()->getBestTitle());
		resizeButton->setTooltip("Toggle absolute size for " + getCurrentFloatingPanel()->getBestTitle());
		foldButton->setTooltip("Fold " + getCurrentFloatingPanel()->getBestTitle());
	}

	moveButton->toFront(false);
	foldButton->toFront(false);
	closeButton->toFront(false);
	resizeButton->toFront(false);
}

void FloatingTile::paint(Graphics& g)
{
	if (isOpaque())
	{
		

		if (findParentComponentOfClass<ScriptContentComponent>() || findParentComponentOfClass<FloatingTilePopup>())
		{
			g.fillAll(getCurrentFloatingPanel()->findPanelColour(FloatingTileContent::PanelColourId::bgColour));
		}
		else
		{
			if (getParentType() == ParentType::Root)
			{
				g.fillAll(HiseColourScheme::getColour(HiseColourScheme::ColourIds::EditorBackgroundColourIdBright));
			}
			else
			{
				g.fillAll(HiseColourScheme::getColour(HiseColourScheme::ColourIds::ModulatorSynthBackgroundColourId));
			}
		}
	}

	

    if(getLayoutData().isFolded() && getParentType() == ParentType::Horizontal)
    {
        g.setColour(Colours::white.withAlpha(0.2f));
        
        Path p = getIcon();
        p.scaleToFit(1.0f, 17.0f, 14.0f, 14.0f, true);
        g.fillPath(p);
    }
    

	if (showTitle())
	{
		g.setGradientFill(ColourGradient(Colour(0xFF222222), 0.0f, 0.0f,
			Colour(0xFF151515), 0.0f, 16.0f, false));

		g.fillRect(0, 0, getWidth(), 16);

		Rectangle<int> titleArea = Rectangle<int>(leftOffsetForTitleText, 0, rightOffsetForTitleText - leftOffsetForTitleText, 16);

		if (titleArea.getWidth() > 40)
		{
			g.setFont(GLOBAL_BOLD_FONT());
			g.setColour(Colours::white.withAlpha(0.8f));

			g.drawText(getCurrentFloatingPanel()->getBestTitle(), titleArea, Justification::centred);
		}
	}
}

void FloatingTile::paintOverChildren(Graphics& g)
{
	if (!hasChildren() && canDoLayoutMode() && isLayoutModeEnabled())
	{
		g.setColour(Colours::black.withAlpha(0.3f));
		g.fillAll();

		if (getWidth() > 80 && getHeight() > 80)
		{
			Path layoutPath;

			layoutPath.loadPathFromData(ColumnIcons::layoutIcon, sizeof(ColumnIcons::layoutIcon));

			g.setColour(Colours::white.withAlpha(0.1f));
			layoutPath.scaleToFit((float)(getWidth() - 40) / 2.0f, (float)(getHeight() - 40) / 2.0f, 40.0f, 40.0f, true);
			g.fillPath(layoutPath);
		}

	}
		

    if(deleteHover)
    {
        g.fillAll(Colours::red.withAlpha(0.1f));
        g.setColour(Colours::red.withAlpha(0.3f));
        g.drawRect(getLocalBounds(), 1);
    }
    
	if (currentSwapSource == this)
		g.fillAll(Colours::white.withAlpha(0.1f));

	if (isSwappable() && layoutData.swappingEnabled && !hasChildren())
	{
		if (isMouseOver(true))
		{
			g.fillAll(Colours::white.withAlpha(0.1f));
			g.setColour(Colours::white.withAlpha(0.4f));
			g.drawRect(getLocalBounds());
		}
		else
		{
			g.fillAll(Colours::green.withAlpha(0.1f));
			g.setColour(Colours::green.withAlpha(0.2f));
			g.drawRect(getLocalBounds());
		}
			
	}
}

void FloatingTile::refreshMouseClickTarget()
{
	if (isEmpty())
	{
		setInterceptsMouseClicks(true, false);
	}
	else if (!hasChildren())
	{
		bool allowClicksOnContent = !isLayoutModeEnabled();

		setInterceptsMouseClicks(!allowClicksOnContent, true);
		dynamic_cast<Component*>(getCurrentFloatingPanel())->setInterceptsMouseClicks(allowClicksOnContent, allowClicksOnContent);

	}
}

void FloatingTile::mouseEnter(const MouseEvent& )
{
	if(isLayoutModeEnabled())
		repaint();
}

void FloatingTile::mouseExit(const MouseEvent& )
{
	if (isLayoutModeEnabled())
		repaint();
}

void FloatingTile::mouseDown(const MouseEvent& event)
{
	if (event.mods.isRightButtonDown())
	{
		PopupMenu m;

		m.setLookAndFeel(&plaf);

		getPanelFactory()->handlePopupMenu(m, this);

	}
	else
	{
		if (layoutData.swappingEnabled && isSwappable())
		{
			currentSwapSource->swapWith(this);

			getRootFloatingTile()->enableSwapMode(false, nullptr);
		}
	}
}

void FloatingTile::resized()
{
	if (content.get() == nullptr)
		return;

	
	LayoutHelpers::setContentBounds(this);

	if (LayoutHelpers::showFoldButton(this))
	{
		leftOffsetForTitleText = 16;
		foldButton->setBounds(1, 1, 14, 14);
		foldButton->setVisible(LayoutHelpers::showFoldButton(this));
	}
	else
		foldButton->setVisible(false);


	rightOffsetForTitleText = getWidth();

	if (LayoutHelpers::showCloseButton(this))
	{
		rightOffsetForTitleText -= 18;

		closeButton->setVisible(rightOffsetForTitleText > 16);
		closeButton->setBounds(rightOffsetForTitleText, 0, 16, 16);
	}
	else
	{
		closeButton->setVisible(false);
	}

	if (LayoutHelpers::showMoveButton(this))
	{
		rightOffsetForTitleText -= 18;
		moveButton->setVisible(rightOffsetForTitleText > 16);
		moveButton->setBounds(rightOffsetForTitleText, 0, 16, 16);
		
	}
	else
		moveButton->setVisible(false);
	
	if (LayoutHelpers::showPinButton(this))
	{
		rightOffsetForTitleText -= 18;
		resizeButton->setVisible(rightOffsetForTitleText > 16);
		resizeButton->setBounds(rightOffsetForTitleText, 0, 16, 16);
		
	}
	else
		resizeButton->setVisible(false);
}


double FloatingTile::getCurrentSizeInContainer()
{
	if (isFolded())
	{
		return layoutData.getCurrentSize();
	}
	else
	{
		if (layoutData.isAbsolute())
		{
			return getParentType() == ParentType::Horizontal ? getWidth() : getHeight();
		}
		else
			return layoutData.getCurrentSize();
	}
}

const FloatingTileContent* FloatingTile::getCurrentFloatingPanel() const
{
	auto c = content.get();

	if(c != nullptr)
		return dynamic_cast<FloatingTileContent*>(content.get());

	return nullptr;
}

FloatingTileContent* FloatingTile::getCurrentFloatingPanel()
{
	auto c = content.get();

	if (c != nullptr)
		return dynamic_cast<FloatingTileContent*>(content.get());

	return nullptr;
}

void FloatingTile::editJSON()
{
	auto codeEditor = new JSONEditor(getCurrentFloatingPanel());

	codeEditor->setSize(300, 400);
	showComponentInRootPopup(codeEditor, moveButton, moveButton->getLocalBounds().getCentre());
}

FloatingTilePopup* FloatingTile::showComponentInRootPopup(Component* newComponent, Component* attachedComponent, Point<int> localPoint)
{
	if (getParentType() != ParentType::Root)
	{
		return getRootFloatingTile()->showComponentInRootPopup(newComponent, attachedComponent, localPoint);
	}
	else
	{
		if (newComponent != nullptr)
		{
			if (currentPopup != nullptr)
				Desktop::getInstance().getAnimator().fadeOut(currentPopup, 150);

			addAndMakeVisible(currentPopup = new FloatingTilePopup(newComponent, attachedComponent, localPoint));


			currentPopup->updatePosition();

			currentPopup->setVisible(false);

			Desktop::getInstance().getAnimator().fadeIn(currentPopup, 150);

		}
		else
		{
			if(currentPopup != nullptr)
				Desktop::getInstance().getAnimator().fadeOut(currentPopup, 150);

			currentPopup = nullptr;
		}

		for (int i = 0; i < listeners.size(); i++)
		{
			if (listeners[i].get() != nullptr)
			{
				listeners[i]->popupChanged(newComponent);
			}
			else
			{
				listeners.remove(i--);
			}
		}

		return currentPopup;
	}
}


void FloatingTilePopup::updatePosition()
{
	auto root = findParentComponentOfClass<FloatingTile>();

	if (root == nullptr)
		return;

	jassert(root != nullptr && root->getParentType() == FloatingTile::ParentType::Root);

	jassert(content != nullptr);

	if (attachedComponent.getComponent() != nullptr)
	{
		Point<int> pointInRoot = root->getLocalPoint(attachedComponent.getComponent(), localPointInComponent);

		int desiredWidth = getWidth();
		int desiredHeight = getHeight();

		int distanceToBottom = root->getHeight() - pointInRoot.getY();
		int distanceToRight = root->getWidth() - pointInRoot.getX();

		int xToUse = -1;
		int yToUse = -1;

		if ((desiredWidth / 2) < distanceToRight)
		{
			xToUse = jmax<int>(0, pointInRoot.getX() - desiredWidth / 2);
		}
		else
		{
			xToUse = jmax<int>(0, root->getWidth() - desiredWidth);
		}

		arrowX = pointInRoot.getX() - xToUse;

		if (desiredHeight < distanceToBottom)
		{
			yToUse = pointInRoot.getY();
			arrowAtBottom = false;
		}
		else
		{
			yToUse = jmax<int>(0, pointInRoot.getY() - desiredHeight - 30);
			arrowAtBottom = true;

			if (yToUse == 0)
				arrowX = -1;
		}

		setTopLeftPosition(xToUse, yToUse);
		resized();
		repaint();
	}
	else
	{
		root->showComponentInRootPopup(nullptr, nullptr, {});
	}
}

bool FloatingTile::canBeDeleted() const
{
#if USE_BACKEND
	const bool isInPopout = findParentComponentOfClass<FloatingTileDocumentWindow>() != nullptr;
#else
    const bool isInPopout = false;
#endif

	if (isVital())
		return false;

	if (getParentType() == ParentType::Root)
		return isInPopout;

	return getParentContainer()->isDynamic();

	//return layoutData.deletable;
}

bool FloatingTile::isFolded() const
{
	return layoutData.isFolded();
}

bool FloatingTile::isInVerticalLayout() const
{
	return getParentType() == ParentType::Vertical;
}



String FloatingTile::exportAsJSON() const
{
	var obj = getCurrentFloatingPanel()->toDynamicObject();

	auto json = JSON::toString(obj, false, DOUBLE_TO_STRING_DIGITS);

	return json;
}


void FloatingTile::loadFromJSON(const String& jsonData)
{
	var obj;

	auto result = JSON::parse(jsonData, obj);

	if (result.wasOk())
		setContent(obj);
}


void FloatingTile::swapContainerType(const Identifier& containerId)
{
	var v = getCurrentFloatingPanel()->toDynamicObject();

	v.getDynamicObject()->setProperty("Type", containerId.toString());

	if (auto list = v.getDynamicObject()->getProperty("Content").getArray())
	{
		for (int i = 0; i < list->size(); i++)
		{
			var c = list->getUnchecked(i);

			var layoutDataObj = c.getDynamicObject()->getProperty("LayoutData");

			layoutDataObj.getDynamicObject()->setProperty("Size", -0.5);
		}
	}

	setContent(v);
}


Path FloatingTile::getIcon() const
{
	if (iconId != -1)
	{
		return getPanelFactory()->getPath((FloatingTileContent::Factory::PopupMenuOptions)iconId);
	}

	if (hasChildren())
	{
		auto firstPanel = dynamic_cast<const FloatingTileContainer*>(getCurrentFloatingPanel())->getComponent(0);

		if (firstPanel != nullptr)
		{
			return firstPanel->getIcon();
		}
	}

	auto index = getPanelFactory()->getOption(this);
	return getPanelFactory()->getPath(index);
}

void FloatingTile::setContent(const var& data)
{
	if (data.isUndefined() || data.isVoid())
	{
		addAndMakeVisible(content = new EmptyComponent(this));
		
	}
	else
	{
		layoutData.fromDynamicObject(data);
		addAndMakeVisible(content = dynamic_cast<Component*>(FloatingTileContent::createPanel(data, this)));
		getCurrentFloatingPanel()->fromDynamicObject(data);
	}

	refreshFixedSizeForNewContent();

	refreshFoldButton();
	refreshPinButton();

	if (getParentContainer())
	{
		getParentContainer()->notifySiblingChange();
		getParentContainer()->refreshLayout();
	}
		
	bringButtonsToFront();
	refreshMouseClickTarget();
	resized();
	repaint();
}


void FloatingTile::setContent(NamedValueSet&& data)
{
	DynamicObject::Ptr d = new DynamicObject();
	d->swapProperties(std::move(data));
	var newData(d);

	setContent(newData);
}

void FloatingTile::setNewContent(const Identifier& newId)
{
	addAndMakeVisible(content = dynamic_cast<Component*>(FloatingTileContent::createNewPanel(newId, this)));

	refreshFixedSizeForNewContent();
	
	if (hasChildren())
		setCanBeFolded(false);

	if (getParentContainer())
	{
		getParentContainer()->notifySiblingChange();
		getParentContainer()->refreshLayout();
	}

	refreshRootLayout();

	bringButtonsToFront();
	refreshMouseClickTarget();

	resized();
}

bool FloatingTile::isSwappable() const
{
	if (getParentType() == ParentType::Root)
		return false;

	if (isVital())
		return false;

	return getParentContainer()->isDynamic();

}

void FloatingTile::refreshFixedSizeForNewContent()
{
	int fixedSize = getCurrentFloatingPanel()->getFixedSizeForOrientation();

	if (fixedSize != 0)
	{
		layoutData.setCurrentSize(fixedSize);
	}
}

void FloatingTile::LayoutHelpers::setContentBounds(FloatingTile* t)
{
	auto scaleFactor = t->content->getTransform().getScaleFactor();

	const int width = (int)((float)t->getWidth() / scaleFactor);
	const int height = (int)((float)t->getHeight() / scaleFactor);

	t->content->setVisible(!t->isFolded());
	t->content->setBounds(0, 0, width, height);
	t->content->resized();
}

bool FloatingTile::LayoutHelpers::showCloseButton(const FloatingTile* t)
{
	auto pt = t->getParentType();

	if (t->closeTogglesVisibility)
		return true;

	if (t->hasChildren() && !t->isLayoutModeEnabled())
		return false;

	if (pt != ParentType::Root && t->isEmpty() && t->getParentContainer()->getNumComponents() == 1)
		return false;

	if (!t->canBeDeleted())
		return false;

	switch (pt)
	{
	case ParentType::Root:
		return !t->isEmpty();
	case ParentType::Horizontal:
		return !t->isFolded() && t->canBeDeleted();
	case ParentType::Vertical:
		return t->canBeDeleted();
	case ParentType::Tabbed:
		return false;
    case ParentType::numParentTypes:
        return false;
	}

	return true;

#if 0
	if (!t->getCurrentFloatingPanel()->showTitleInPresentationMode() && !t->isLayoutModeEnabled())
		return false;

	if (!t->canBeDeleted())
		return false;

	if (!t->canDoLayoutMode())
		return false;

	if (!t->isLayoutModeEnabled() && (t->isEmpty() || t->hasChildren()))
		return false;

	auto pt = t->getParentType();

	

	return false;
#endif

}

bool FloatingTile::LayoutHelpers::showMoveButton(const FloatingTile* t)
{
	if (t->hasChildren() && dynamic_cast<const FloatingTileContainer*>(t->getCurrentFloatingPanel())->isDynamic() && t->isLayoutModeEnabled())
		return true;

	return showPinButton(t);
}

bool FloatingTile::LayoutHelpers::showPinButton(const FloatingTile* t)
{
	if (!t->isSwappable())
		return false;

	if (t->getParentType() == ParentType::Tabbed)
		return false;

	if (!t->isLayoutModeEnabled())
		return false;

	if (!t->canDoLayoutMode())
		return false;

	auto pt = t->getParentType();

	if (pt == ParentType::Root)
		return false;

	if (!t->isInVerticalLayout() && t->isFolded())
		return false;

	return true;
}

bool FloatingTile::LayoutHelpers::showFoldButton(const FloatingTile* t)
{
	if (t->getLayoutData().mustShowFoldButton())
		return true;

	if (!t->canBeFolded())
		return false;

	if (t->getParentType() == ParentType::Tabbed)
		return false;

	if (t->getParentType() == ParentType::Horizontal)
		return true;

	if (t->showTitle())
		return true;

	return false;
}

void FloatingTile::TilePopupLookAndFeel::getIdealPopupMenuItemSize(const String &text, bool isSeparator, int standardMenuItemHeight, int &idealWidth, int &idealHeight)
{
	if (isSeparator)
	{
		idealWidth = 50;
		idealHeight = standardMenuItemHeight > 0 ? standardMenuItemHeight / 2 : 10;
	}
	else
	{
		Font font(getPopupMenuFont());

		if (standardMenuItemHeight > 0 && font.getHeight() > standardMenuItemHeight / 1.3f)
			font.setHeight(standardMenuItemHeight / 1.3f);

		idealHeight = JUCE_LIVE_CONSTANT_OFF(26);

		idealWidth = font.getStringWidth(text) + idealHeight * 2;
	}
}

void FloatingTile::TilePopupLookAndFeel::drawPopupMenuSectionHeader(Graphics& g, const Rectangle<int>& area, const String& sectionName)
{
	g.fillAll(JUCE_LIVE_CONSTANT_OFF(Colour(0x1AFFFFFF)));

	g.setFont(getPopupMenuFont());
	g.setColour(Colours::white);

	g.drawFittedText(sectionName,
		area.getX() + 12, area.getY(), area.getWidth() - 16, (int)(area.getHeight() * 0.8f),
		Justification::bottomLeft, 1);
}

#if USE_BACKEND

FloatingTileDocumentWindow::FloatingTileDocumentWindow(BackendRootWindow* parentRoot) :
	DocumentWindow("Popout", HiseColourScheme::getColour(HiseColourScheme::EditorBackgroundColourId), DocumentWindow::TitleBarButtons::allButtons, true),
	parent(parentRoot)
{
	setContentOwned(new FloatingTile(parentRoot->getBackendProcessor(), nullptr), false);
	setVisible(true);
	setUsingNativeTitleBar(true);
	setResizable(true, true);

	centreWithSize(500, 500);
}

void FloatingTileDocumentWindow::closeButtonPressed()
{
	parent->removeFloatingWindow(this);
}

bool FloatingTileDocumentWindow::keyPressed(const KeyPress& key)
{
	if (key == KeyPress::F6Key)
	{
		getBackendRootWindow()->getBackendProcessor()->getCommandManager()->invokeDirectly(BackendCommandTarget::MenuViewEnableGlobalLayoutMode, false);
		return true;
	}

	return false;
}

FloatingTile* FloatingTileDocumentWindow::getRootFloatingTile()
{
	return dynamic_cast<FloatingTile*>(getContentComponent());
}

#endif

} // namespace hise