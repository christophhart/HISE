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

namespace hise
{
using namespace juce;

void DAWClockController::LAF::drawRotarySlider(Graphics &g, int /*x*/, int /*y*/, int width, int height, float /*sliderPosProportional*/, float /*rotaryStartAngle*/, float /*rotaryEndAngle*/, Slider &s)
{
    g.setFont(GLOBAL_BOLD_FONT().withHeight(s.getHeight() - 2.0f));
    g.setColour(Colours::white.withAlpha(s.isMouseOverOrDragging() ? 0.9f : 0.7f));
    
    auto v = (int)s.getValue();
    
    if(s.getName() == "denom")
        v = nextPowerOfTwo(v);
    
    g.drawText(String(v), s.getLocalBounds().toFloat(), Justification::centred);
}


struct DAWClockController::Ruler: public Component
{
    static constexpr int LoopHeight = 17;
    
    Ruler(ExternalClockSimulator* clock_):
      clock(clock_)
    {
        setOpaque(true);
    };
    
    void setPositionFromEvent(const MouseEvent& e)
    {
        if(e.getPosition().getY() > LoopHeight)
        {
            clock->ppqPos = pixelToPPQ(e.getPosition().getX());
        }
        else
        {
            auto thisPos = pixelToPPQ(e.getPosition().getX());
            
            auto distToStart = hmath::abs(clock->ppqLoop.getStart() - thisPos);
            auto distToEnd = hmath::abs(clock->ppqLoop.getEnd() - thisPos);
            
            if(distToStart < distToEnd && thisPos < clock->ppqLoop.getEnd())
                clock->ppqLoop.setStart(thisPos);
            else
                clock->ppqLoop.setEnd(thisPos);
        }
    }
    
    void mouseDrag(const MouseEvent& e) override
    {
        setPositionFromEvent(e);
    }
    
    void mouseDown(const MouseEvent& e) override
    {
        setPositionFromEvent(e);
    }
    
    float PPQToPixel(double ppqPos) const
    {
        float numQuarters = numBars * clock->nom;
        
        return (float)ppqPos / (float)numQuarters * (float)getWidth();
    }
    
    float pixelToPPQ(int xPos) const
    {
        auto xNormalized = (double)xPos / (double)getWidth();
        
        float numQuarters = numBars * clock->nom;
        
        auto v = xNormalized * numQuarters;
        
        if(grid)
            v = hmath::round(v);
        
        return jmax<float>(0.0f, v);
    }
    
    void paint(Graphics& g) override
    {
        g.setGradientFill(ColourGradient(Colour(0xFF303030), 0.0f, 0.0f, Colour(0xFF262626), 0.0f, (float)getHeight(), false));
        g.fillAll();
        auto b = getLocalBounds().toFloat();
        
        auto top = b.removeFromTop((float)LoopHeight);
        
        g.setColour(Colours::white.withAlpha(0.1f));
        g.fillRect(top);
        g.setColour(Colour(0xFF555555));
        g.drawRect(b, 1.0f);
        
        g.setColour(Colours::black.withAlpha(0.6f));
        g.drawHorizontalLine(b.getY() + 1, 1.0f, (float)getWidth() - 1.0f);
        
        float barWidth = (float)getWidth() / (float)numBars;
        float beatWidth = barWidth / (float)clock->nom;
        
        float numQuarters = numBars * clock->nom;
        
        auto rulerPos = (float)PPQToPixel(clock->ppqPos);
        
        g.setColour(Colours::white.withAlpha(clock->isPlaying ? 1.0f : 0.7f));
        g.fillRect(rulerPos-0.5f, b.getY()+2, 2.0f, b.getHeight()-4);
        
        g.setColour(Colours::white.withAlpha(clock->isPlaying ? 0.2f : 0.1f));
        g.fillRect(rulerPos-3.5f, b.getY()+2, 8.0f, b.getHeight()-4);
        
        for(int i = 0; i < numQuarters; i++)
        {
            auto x = b.getX();
            
            g.setColour(Colour(0xFF555555).withAlpha(i % clock->nom == 0 ? 0.7f : 0.2f));
            b.removeFromLeft(beatWidth);
            g.drawVerticalLine(x, 0, b.getBottom());
        }
        
        DAWClockController::Icons f;
        
        auto ls = f.createPath("loopStart");
        auto le = f.createPath("loopEnd");
        
        auto lsX = PPQToPixel(clock->ppqLoop.getStart());
        auto leX = PPQToPixel(clock->ppqLoop.getEnd());
        
        f.scalePath(ls, top.withWidth(LoopHeight).withX(lsX - LoopHeight + 1.0f));
        f.scalePath(le, top.withWidth(LoopHeight).withX(leX - 1.0f));
        
        
        
        g.setColour(Colours::white.withAlpha(clock->isLooping ? 0.8f : 0.3f));
        
        g.fillPath(ls);
        g.fillPath(le);
        
        if(clock->isLooping)
        {
            g.setColour(Colours::white.withAlpha(0.05f));
            g.fillRect(lsX, b.getY(), leX - lsX, b.getHeight());
        }
    }
    
    
    bool grid = true;
    int numBars = 8;
    
    WeakReference<ExternalClockSimulator> clock;
};



DAWClockController::DAWClockController(MainController* mc):
  SimpleTimer(mc->getGlobalUIUpdater()),
  ControlledObject(mc),
  clock(&dynamic_cast<BackendProcessor*>(mc)->externalClockSim),
  play("play", nullptr, f),
  stop("stop", nullptr, f),
  loop("loop", nullptr, f),
  grid("grid", nullptr, f),
  rewind("rewind", nullptr, f)
{
    addAndMakeVisible(play);
    addAndMakeVisible(stop);
    addAndMakeVisible(rewind);
    addAndMakeVisible(loop);
    addAndMakeVisible(bpm);
    addAndMakeVisible(nom);
    addAndMakeVisible(denom);
    addAndMakeVisible(position);
    addAndMakeVisible(ruler = new Ruler(clock));
    
    addAndMakeVisible(grid);
    addAndMakeVisible(length);
    
    play.setToggleModeWithColourChange(true);
    stop.setToggleModeWithColourChange(true);
    loop.setToggleModeWithColourChange(true);
    grid.setToggleModeWithColourChange(true);
    
    denom.setName("denom");
    
    auto setupSlider = [this](Slider& s)
    {
        s.setSliderStyle(Slider::SliderStyle::RotaryVerticalDrag);
        s.setTextBoxStyle(Slider::TextEntryBoxPosition::NoTextBox, false, 0, 0);
        s.setLookAndFeel(&laf);
        s.addListener(this);
    };
    
    nom.setRange(1.0, 16.0, 1.0);
    denom.setRange(1, 16.0, 1.0);
    bpm.setRange(30.0, 240.0, 1.0);
    length.setRange(1, 128.0, 1.0);
    
    length.setValue(dynamic_cast<Ruler*>(ruler.get())->numBars, dontSendNotification);
    
    play.onClick = [this]()
    {
        clock->isPlaying = true;
    };
    
    stop.onClick = [this]()
    {
        clock->isPlaying = false;
    };
    
    loop.onClick = [this]()
    {
        clock->isLooping = loop.getToggleState();
    };
    
    rewind.onClick = [this]()
    {
        clock->ppqPos = 0.0;
    };
    
    grid.onClick = [this]()
    {
        dynamic_cast<Ruler*>(ruler.get())->grid = grid.getToggleState();
    };
    
    grid.setToggleStateAndUpdateIcon(true);
    
    setupSlider(length);
    setupSlider(bpm);
    setupSlider(nom);
    setupSlider(denom);
    
    position.setEditable(false);
    position.setFont(GLOBAL_BOLD_FONT().withHeight(17.0f));
    position.setColour(Label::ColourIds::textColourId, Colours::white.withAlpha(0.5f));
    
    play.setTooltip("Start the external DAW playback simulator [Space]");
    stop.setTooltip("Stop the external DAW playback simulator [Space]");
    loop.setTooltip("Toggle the loop playback");
    bpm.setTooltip("Set the external DAW tempo");
    rewind.setTooltip("Rewind to 1|1|0 [Backspace]");
    grid.setTooltip("Enable the magnetic grid for the playback ruler");
    length.setTooltip("Set the length of the playback ruler");
    
}

bool DAWClockController::keyPressed(const KeyPress& k)
{
    if(k == KeyPress::spaceKey)
    {
        clock->isPlaying = !clock->isPlaying;
        return true;
    }
    if(k == KeyPress::backspaceKey)
    {
        rewind.triggerClick();
        return true;
    }
    if(k == KeyPress::leftKey)
    {
        clock->ppqPos = jmax(0.0, clock->ppqPos - 1.0f);
        return true;
    }
    if(k == KeyPress::rightKey)
    {
        clock->ppqPos += 1.0;
        return true;
    }
    
    
    return false;
}

void DAWClockController::sliderValueChanged(Slider* s)
{
    if(s == &bpm)
    {
        clock->bpm = roundToInt(s->getValue());
        getMainController()->setHostBpm(clock->bpm);
    }
    if(s == &nom)
        clock->nom = roundToInt(s->getValue());
    if(s == &denom)
        clock->denom = nextPowerOfTwo(roundToInt(s->getValue()));
    if(s == &length)
        dynamic_cast<Ruler*>(ruler.get())->numBars = (int)s->getValue();
}

void DAWClockController::timerCallback()
{
    if(!bpm.isMouseButtonDown())
        bpm.setValue(clock->bpm, dontSendNotification);
    
    if(!nom.isMouseButtonDown())
        nom.setValue(clock->nom, dontSendNotification);
    
    if(!denom.isMouseButtonDown())
        denom.setValue(clock->denom, dontSendNotification);
    
    auto pos = clock->ppqPos;
    
    auto ticks = roundToInt(hmath::fmod(pos, 1.0) * 960.0);
    auto quarters = (int)hmath::floor(hmath::fmod(pos, (double)clock->denom));
    auto bars = (int)hmath::floor(pos / clock->denom);
    
    String posString;
    posString << String(bars+1) << " | " << String(quarters+1) << " | " << String(ticks);
    
    position.setText(posString, dontSendNotification);
    
    play.setToggleStateAndUpdateIcon(clock->isPlaying);
    stop.setToggleStateAndUpdateIcon(!clock->isPlaying);
    loop.setToggleStateAndUpdateIcon(clock->isLooping);
    
    ruler->repaint();
}

void DAWClockController::resized()
{
    auto b = getLocalBounds();
    
    static constexpr int TopHeight = 28;
    static constexpr int Margin = 5;
    
    auto top = b.removeFromTop(TopHeight);
    
    top.removeFromLeft(TopHeight);
    
    b.removeFromTop(5);
    
    play.setBounds(top.removeFromLeft(TopHeight).reduced(Margin));
    stop.setBounds(top.removeFromLeft(TopHeight).reduced(Margin));
    rewind.setBounds(top.removeFromLeft(TopHeight).reduced(Margin));
    top.removeFromLeft(Margin);
    loop.setBounds(top.removeFromLeft(TopHeight).reduced(Margin));
    bpm.setBounds(top.removeFromLeft(TopHeight * 2).reduced(Margin));
    
    auto ts = top.removeFromLeft(TopHeight);
    
    nom.setBounds(ts.removeFromTop(ts.getHeight()/2));
    denom.setBounds(ts);
    
    position.setBounds(top.removeFromLeft(TopHeight * 4));
    
    auto r = b.removeFromLeft(TopHeight);
    
    length.setBounds(r.removeFromTop(Ruler::LoopHeight));
    
    grid.setBounds(r.reduced(Margin));
    
    b.removeFromLeft(10);
    ruler->setBounds(b);
}



juce::Image PoolTableHelpers::getPreviewImage(const AudioSampleBuffer* buffer, float width)
{
	if (buffer == nullptr)
		return PoolHelpers::getEmptyImage((int)width, 150);

	return HiseAudioThumbnail::createPreview(buffer, (int)width);
}

juce::Image PoolTableHelpers::getPreviewImage(const Image* img, float width)
{
	if (img == nullptr)
		return PoolHelpers::getEmptyImage((int)width, 150);

	auto ratio = (float)img->getWidth() / (float)img->getHeight();

	if (img->getWidth() > width)
	{
		return img->rescaled((int)width, (int)(width / ratio));
	}
	else
	{
		if (img->getHeight() < 1600)
		{
			int heightToUse = jmin<int>(500, img->getHeight());

			return img->rescaled((int)((float)heightToUse * ratio), heightToUse);
		}
		else
		{
			// most likely a filmstrip, so crop it to show the first two strips...
			return img->getClippedImage({ 0, 0, img->getWidth(), img->getWidth() * 2 });

		}
	}
}

juce::Image PoolTableHelpers::getPreviewImage(const ValueTree* v, float width)
{
	if (v == nullptr)
		return PoolHelpers::getEmptyImage((int)width, 150);

	Array<Rectangle<int>> zones;

	auto totalArea = Rectangle<int>(0, 0, (int)width, 128);

	for (const auto& data : *v)
	{
		auto d = StreamingHelpers::getBasicMappingDataFromSample(data);

		int x = jmap((int)d.lowKey, 0, 128, 0, totalArea.getWidth());
		int w = jmap((int)(1 + d.highKey - d.lowKey), 0, 128, 0, totalArea.getWidth());
		int y = jmap((int)d.highVelocity, 128, 0, 0, totalArea.getHeight());
		int h = jmap((int)(1 + d.highVelocity - d.lowVelocity), 0, 128, 0, totalArea.getHeight() - 1);

		zones.add({ x, y, w, h });
	}

	Image img(Image::ARGB, (int)width, 128, true);

	Graphics g(img);

	g.setColour(Colours::white.withAlpha(0.2f));
	g.drawRect(totalArea, 1);

	for (auto z : zones)
	{
		g.fillRect(z);
		g.drawRect(z);
	}

	return img;
}

juce::Image PoolTableHelpers::getPreviewImage(const MidiFileReference* v, float width)
{
	auto f = v->getFile();

#if 0

	MemoryOutputStream mos;
	f.writeTo(mos);

	MemoryBlock mb = mos.getMemoryBlock();
	MemoryInputStream mis(mb, true);
#endif

	HiseMidiSequence seq;
	seq.loadFrom(f);

	auto l = seq.getRectangleList({ 0.0f, 0.0f, width, 200.0f });

	Image img(Image::PixelFormat::ARGB, (int)width, 200, true);
	Graphics g(img);

	g.setColour(Colours::white);

	for (auto note : l)
		g.fillRect(note);

	return img;
}

juce::Path PoolTableHelpers::Factory::createPath(const String& name) const
{
	auto url = MarkdownLink::Helpers::getSanitizedFilename(name);

	Path p;

	LOAD_PATH_IF_URL("preview", HiBinaryData::FrontendBinaryData::infoButtonShape);
	LOAD_PATH_IF_URL("reload", ColumnIcons::moveIcon);

	return p;
}

namespace ClockIcons
{
static const unsigned char play[] = { 110,109,48,200,67,68,112,182,151,67,98,184,219,68,68,40,55,153,67,4,128,69,68,60,173,155,67,4,128,69,68,240,76,158,67,98,4,128,69,68,180,238,160,67,184,219,68,68,184,98,163,67,48,200,67,68,132,229,164,67,98,192,23,56,68,196,67,181,67,224,196,25,68,160,
189,223,67,72,37,13,68,216,110,241,67,98,142,12,12,68,220,247,242,67,140,158,10,68,176,39,243,67,230,109,9,68,156,235,241,67,98,74,62,8,68,132,175,240,67,4,128,7,68,124,59,238,67,4,128,7,68,64,141,235,67,98,4,128,7,68,140,67,201,67,4,128,7,68,112,9,97,
67,4,128,7,68,0,107,31,67,98,4,128,7,68,160,147,26,67,148,43,8,68,88,40,22,67,18,62,9,68,136,238,19,67,98,136,79,10,68,152,176,17,67,46,154,11,68,240,7,18,67,224,151,12,68,32,207,20,67,98,0,221,24,68,176,47,55,67,20,236,55,68,184,23,135,67,48,200,67,
68,112,182,151,67,99,101,0,0 };

static const unsigned char stop[] = { 110,109,0,128,69,68,4,15,89,67,98,0,128,69,68,236,58,75,67,70,177,66,68,0,0,64,67,62,60,63,68,0,0,64,67,108,118,196,13,68,0,0,64,67,98,112,79,10,68,0,0,64,67,0,128,7,68,236,58,75,67,0,128,7,68,4,15,89,67,108,0,128,7,68,18,119,207,67,98,0,128,7,68,32,
97,214,67,112,79,10,68,0,0,220,67,118,196,13,68,0,0,220,67,108,62,60,63,68,0,0,220,67,98,70,177,66,68,0,0,220,67,0,128,69,68,32,97,214,67,0,128,69,68,18,119,207,67,108,0,128,69,68,4,15,89,67,99,101,0,0 };

static const unsigned char loop[] = { 110,109,100,113,22,68,130,145,190,67,98,28,151,26,68,168,120,198,67,24,27,32,68,82,230,202,67,244,217,37,68,82,230,202,67,98,252,8,50,68,82,230,202,67,44,249,59,68,146,94,183,67,172,47,60,68,134,0,159,67,108,32,85,69,68,234,81,159,67,98,28,8,69,68,158,
169,193,67,136,6,55,68,56,49,221,67,244,217,37,68,56,49,221,67,98,196,173,29,68,56,49,221,67,224,213,21,68,76,216,214,67,128,248,15,68,214,129,203,67,108,216,218,7,68,152,190,219,67,108,0,128,7,68,146,118,172,67,108,76,35,31,68,64,44,173,67,108,100,113,
22,68,130,145,190,67,99,109,104,196,54,68,248,165,121,67,98,32,168,50,68,104,154,106,67,192,71,45,68,92,51,98,67,104,175,39,68,92,51,98,67,98,168,127,27,68,92,51,98,67,116,143,17,68,224,162,132,67,244,88,17,68,238,0,157,67,108,132,51,8,68,22,174,156,
67,98,140,128,8,68,196,172,116,67,28,130,22,68,144,157,61,67,104,175,39,68,144,157,61,67,98,92,180,47,68,144,157,61,67,92,105,55,68,36,219,73,67,72,61,61,68,104,194,95,67,108,44,37,69,68,228,34,64,67,108,0,128,69,68,120,89,143,67,108,184,220,45,68,88,
162,142,67,108,104,196,54,68,248,165,121,67,99,101,0,0 };

static const unsigned char grid[] = { 110,109,96,140,22,68,64,118,86,67,108,96,140,22,68,144,107,68,67,108,120,41,17,68,144,107,68,67,108,120,41,17,68,240,165,102,67,108,248,127,7,68,240,165,102,67,108,248,127,7,68,136,49,124,67,108,120,41,17,68,136,49,124,67,108,120,41,17,68,24,157,152,
67,108,216,154,8,68,24,157,152,67,108,216,154,8,68,228,98,163,67,108,120,41,17,68,228,98,163,67,108,120,41,17,68,52,231,189,67,108,216,154,8,68,52,231,189,67,108,216,154,8,68,0,173,200,67,108,120,41,17,68,0,173,200,67,108,120,41,17,68,0,0,220,67,108,
96,140,22,68,0,0,220,67,108,96,140,22,68,12,173,204,67,108,152,123,22,68,12,173,204,67,108,152,123,22,68,64,118,86,67,108,96,140,22,68,64,118,86,67,99,109,104,49,41,68,168,178,85,67,108,104,49,41,68,240,255,63,67,108,128,206,35,68,240,255,63,67,108,128,
206,35,68,240,165,102,67,108,80,22,24,68,240,165,102,67,108,80,22,24,68,136,49,124,67,108,128,206,35,68,136,49,124,67,108,128,206,35,68,24,157,152,67,108,80,22,24,68,24,157,152,67,108,80,22,24,68,228,98,163,67,108,128,206,35,68,228,98,163,67,108,128,
206,35,68,52,231,189,67,108,80,22,24,68,52,231,189,67,108,80,22,24,68,0,173,200,67,108,128,206,35,68,0,173,200,67,108,128,206,35,68,48,202,217,67,108,104,49,41,68,48,202,217,67,108,104,49,41,68,220,73,204,67,108,184,48,41,68,220,73,204,67,108,184,48,
41,68,168,178,85,67,108,104,49,41,68,168,178,85,67,99,109,40,204,42,68,240,165,102,67,108,40,204,42,68,136,49,124,67,108,144,115,54,68,136,49,124,67,108,144,115,54,68,24,157,152,67,108,40,204,42,68,24,157,152,67,108,40,204,42,68,228,98,163,67,108,144,
115,54,68,228,98,163,67,108,144,115,54,68,52,231,189,67,108,40,204,42,68,52,231,189,67,108,40,204,42,68,0,173,200,67,108,144,115,54,68,0,173,200,67,108,144,115,54,68,48,202,217,67,108,120,214,59,68,48,202,217,67,108,120,214,59,68,0,173,200,67,108,96,
219,59,68,0,173,200,67,108,96,219,59,68,52,231,189,67,108,120,214,59,68,52,231,189,67,108,120,214,59,68,228,98,163,67,108,96,219,59,68,228,98,163,67,108,96,219,59,68,24,157,152,67,108,120,214,59,68,24,157,152,67,108,120,214,59,68,136,49,124,67,108,96,
219,59,68,136,49,124,67,108,96,219,59,68,240,165,102,67,108,120,214,59,68,240,165,102,67,108,120,214,59,68,240,255,63,67,108,144,115,54,68,240,255,63,67,108,144,115,54,68,240,165,102,67,108,40,204,42,68,240,165,102,67,99,109,32,118,61,68,52,231,189,67,
108,32,118,61,68,0,173,200,67,108,248,127,69,68,0,173,200,67,108,248,127,69,68,52,231,189,67,108,32,118,61,68,52,231,189,67,99,109,32,118,61,68,24,157,152,67,108,32,118,61,68,228,98,163,67,108,248,127,69,68,228,98,163,67,108,248,127,69,68,24,157,152,
67,108,32,118,61,68,24,157,152,67,99,109,32,118,61,68,240,165,102,67,108,32,118,61,68,136,49,124,67,108,8,101,68,68,136,49,124,67,108,8,101,68,68,240,165,102,67,108,32,118,61,68,240,165,102,67,99,101,0,0 };

static const unsigned char loopEnd[] = { 110,109,0,128,7,68,64,40,54,67,108,0,128,69,68,128,42,58,67,108,0,111,8,68,224,235,224,67,108,0,128,7,68,64,40,54,67,99,101,0,0 };

static const unsigned char loopStart[] = { 110,109,0,128,69,68,160,37,54,67,108,0,128,7,68,64,45,58,67,108,248,144,68,68,64,237,224,67,108,0,128,69,68,160,37,54,67,99,101,0,0 };

static const unsigned char rewind[] = { 110,109,212,254,24,68,202,91,153,67,98,48,52,24,68,204,119,154,67,232,186,23,68,240,72,156,67,232,186,23,68,206,56,158,67,98,232,186,23,68,50,42,160,67,48,52,24,68,208,249,161,67,212,254,24,68,90,23,163,67,98,236,159,33,68,176,44,175,67,172,2,56,68,10,
136,206,67,18,85,65,68,158,151,219,67,98,82,36,66,68,194,185,220,67,128,50,67,68,18,221,220,67,162,18,68,68,186,243,219,67,98,138,243,68,68,98,10,219,67,0,128,69,68,194,58,217,67,0,128,69,68,38,64,215,67,98,0,128,69,68,66,240,189,67,0,128,69,68,76,217,
120,67,0,128,69,68,76,104,72,67,98,0,128,69,68,88,213,68,67,92,1,69,68,56,146,65,67,182,54,68,68,148,237,63,67,98,20,108,67,68,224,69,62,67,250,119,66,68,92,134,62,67,118,189,65,68,96,147,64,67,98,156,174,56,68,40,244,89,67,238,192,33,68,218,22,141,67,
212,254,24,68,202,91,153,67,99,109,150,181,19,68,192,76,64,67,98,150,181,19,68,200,71,63,67,100,129,19,68,236,115,62,67,38,64,19,68,236,115,62,67,108,112,245,7,68,236,115,62,67,98,246,180,7,68,236,115,62,67,0,128,7,68,200,71,63,67,0,128,7,68,192,76,64,
67,108,0,128,7,68,102,195,217,67,98,0,128,7,68,90,68,218,67,246,180,7,68,70,174,218,67,112,245,7,68,70,174,218,67,108,38,64,19,68,70,174,218,67,98,100,129,19,68,70,174,218,67,150,181,19,68,90,68,218,67,150,181,19,68,102,195,217,67,108,150,181,19,68,192,
76,64,67,99,101,0,0 };

}

Path DAWClockController::Icons::createPath(const String& url) const
{
    Path p;
    
    LOAD_PATH_IF_URL("play", ClockIcons::play);
    LOAD_PATH_IF_URL("stop", ClockIcons::stop);
    LOAD_PATH_IF_URL("loop", ClockIcons::loop);
    LOAD_PATH_IF_URL("grid", ClockIcons::grid);
    LOAD_PATH_IF_URL("loopStart", ClockIcons::loopStart);
    LOAD_PATH_IF_URL("loopEnd", ClockIcons::loopEnd);
    LOAD_PATH_IF_URL("rewind", ClockIcons::rewind);
    
    return p;
}

}
