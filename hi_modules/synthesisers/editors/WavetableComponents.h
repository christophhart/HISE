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

#ifndef WAVETABLECOMPONENTS_H_INCLUDED
#define WAVETABLECOMPONENTS_H_INCLUDED

namespace hise {
using namespace juce;




#if USE_BACKEND


class WavetablePreviewBase : public Component,
							 private SafeChangeListener
{
protected:

	WavetablePreviewBase(SampleMapToWavetableConverter& parent_) :
		parent(parent_)
	{
		parent.addChangeListener(this);
	};

	virtual ~WavetablePreviewBase()
	{
		parent.removeChangeListener(this);
	};

	virtual void updateGraphics() = 0;

	void changeListenerCallback(SafeChangeBroadcaster* /*b*/) override
	{
		updateGraphics();
	}

	SampleMapToWavetableConverter& parent;
};



class SampleMapToWavetableConverter::SampleMapPreview : public WavetablePreviewBase,
														public juce::FileDragAndDropTarget
{
public:

	SampleMapPreview(SampleMapToWavetableConverter& parent) :
		WavetablePreviewBase(parent)
	{};

	void updateGraphics() override;

	void paint(Graphics& g) override;

	void mouseDown(const MouseEvent& e) override;

	void mouseMove(const MouseEvent& e) override;

	LambdaBroadcaster<int> indexBroadcaster;

	bool isInterestedInFileDrag(const StringArray& files) override { return files.size() == 1 && MultiChannelAudioBufferDisplay::isAudioFile(files[0]); }

	std::function<void(const ValueTree& v)> sampleMapLoadFunction;

	void fileDragEnter(const StringArray& files, int x, int y) override
	{
		insertPosition = (x * 128) / getWidth();
		repaint();
	}

	void fileDragMove(const StringArray& files, int x, int y) override
	{
		insertPosition = (x * 128) / getWidth();
		repaint();
	}

	void fileDragExit(const StringArray& files) override
	{
		insertPosition = -1;
		repaint();
	}

	void filesDropped(const StringArray& files, int x, int y) override;

private:

	struct Sample
	{
		Sample(const ValueTree& data, Rectangle<int> totalArea);

		Rectangle<int> area;
		int index;
		bool analysed = false;
		bool active = false;
		Range<int> keyRange;
		int rootNote;
	};

	int insertPosition = -1;

	int hoverIndex = -1;
	Array<Sample> samples;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SampleMapPreview);
};




class SampleMapToWavetableConverter::Preview : public WavetablePreviewBase,
											   public ControlledObject,
											   public PooledUIUpdater::SimpleTimer
{
public:

	Preview(SampleMapToWavetableConverter& parent_);
	~Preview();

	void mouseMove(const MouseEvent& event) override;
	void mouseEnter(const MouseEvent& /*event*/) override;
	void mouseExit(const MouseEvent& /*event*/) override;
	void mouseDown(const MouseEvent& event) override;

	void updateGraphics() override;

	void paint(Graphics& g);

	void timerCallback() override;

	void setImageToShow(const Image& img)
	{
		spectrumImage = img;
		repaint();
	}

private:

	double previewPosition = -1.0;
	Image spectrumImage;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Preview);
};


class CombinedPreview : public Component,
						public ButtonListener
{
public:

	struct LAF : public AlertWindowLookAndFeel
	{
		void drawButtonBackground(Graphics& g, Button& button, const Colour&, bool isMouseOverButton, bool isButtonDown) override
		{
			Path p;

			auto b = button.getLocalBounds().toFloat().reduced(1.0f);

			if (button.isConnectedOnLeft())
			{
				p.startNewSubPath(b.getX(), b.getY());
				p.lineTo(b.getRight() - b.getHeight() * 0.5f, b.getY());
				p.quadraticTo(b.getRight(), b.getY(), b.getRight(), b.getCentreY());
				p.quadraticTo(b.getRight(), b.getBottom(), b.getRight() - b.getHeight() * 0.5f, b.getBottom());
				p.lineTo(b.getX(), b.getBottom());
				p.closeSubPath();
			}
			else
			{
				p.startNewSubPath(b.getRight(), b.getY());
				p.lineTo(b.getX() + b.getHeight() * 0.5f, b.getY());
				p.quadraticTo(b.getX(), b.getY(), b.getX(), b.getCentreY());
				p.quadraticTo(b.getX(), b.getBottom(), b.getX() + b.getHeight() * 0.5f, b.getBottom());
				p.lineTo(b.getRight(), b.getBottom());
				p.closeSubPath();
			}

			g.setColour(bright);

			g.strokePath(p, PathStrokeType(2.0f));

			if (button.getToggleState())
			{
				g.setColour(bright.withAlpha(0.5f));
				g.fillPath(p);
			}
				
				
		}
	} laf;

	CombinedPreview(SampleMapToWavetableConverter& parent, MainController* mc)
	{
		addAndMakeVisible(spectrumButton = new TextButton("Spectrum"));
		addAndMakeVisible(waterfallButton = new TextButton("Waterfall"));
		addAndMakeVisible(spectrum = new SampleMapToWavetableConverter::Preview(parent));
		addAndMakeVisible(waterfall = new WaterfallComponent(mc, nullptr));

		spectrumButton->setClickingTogglesState(true);
		waterfallButton->setClickingTogglesState(true);
		spectrumButton->setRadioGroupId(9004242);
		waterfallButton->setRadioGroupId(9004242);

		spectrumButton->addListener(this);
		waterfallButton->addListener(this);

		spectrumButton->setConnectedEdges(Button::ConnectedEdgeFlags::ConnectedOnLeft);
		waterfallButton->setConnectedEdges(Button::ConnectedEdgeFlags::ConnectedOnRight);

		spectrumButton->setLookAndFeel(&laf);
		waterfallButton->setLookAndFeel(&laf);

		waterfallButton->setToggleState(true, dontSendNotification);
		spectrum->setVisible(false);

		waterfall->setColour(HiseColourScheme::ComponentFillTopColourId, Colours::white);
	}

	void buttonClicked(Button* b) override
	{
		spectrum->setVisible(b == spectrumButton);
		waterfall->setVisible(b == waterfallButton);
	}

	void resized() override
	{
		auto area = getLocalBounds();

		area.removeFromTop(10);

		auto topBar = area.removeFromTop(24).reduced(128, 0);
		
		area.removeFromTop(10);

		waterfallButton->setBounds(topBar.removeFromLeft(topBar.getWidth() / 2).reduced(2));
		spectrumButton->setBounds(topBar.reduced(2));

		spectrum->setBounds(area);
		waterfall->setBounds(area);
		waterfall->setColour(HiseColourScheme::ComponentFillTopColourId, Colours::white);
	}

	void setImageToShow(const Image& img)
	{
		spectrum->setImageToShow(img);
	}


private:

	friend class WavetableConverterDialog;

	ScopedPointer<TextButton> spectrumButton;
	ScopedPointer<TextButton> waterfallButton;
	ScopedPointer<SampleMapToWavetableConverter::Preview> spectrum;
	
	ScopedPointer<WaterfallComponent> waterfall;

	JUCE_DECLARE_WEAK_REFERENCEABLE(CombinedPreview);
	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CombinedPreview);
};

#endif

}

#endif