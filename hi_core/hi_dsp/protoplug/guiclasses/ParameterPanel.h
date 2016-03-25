#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "../PluginProcessor.h"

class ParamSlider	:	public Slider 
{
public:
	ParamSlider(LuaProtoplugJuceAudioProcessor *_pfx, int _index)
	{
		pfx = _pfx;
		index = _index;
		//Slider::setTextBoxIsEditable(false);
	}
	String getTextFromValue (double /*value must be set in pfx*/)
	{
		return pfx->getParameterText(index);
	}
	double getValueFromText (const String &text)
	{
		double d;
		if (pfx->parameterText2Double(index, text, d))
			return d;
		return Slider::getValueFromText(text);
	}
private:
	int index;
	LuaProtoplugJuceAudioProcessor *pfx;
};

class ParamPanelContent	:	public Component
{
public:
	void paint (Graphics& g) { g.fillAll (Colour(0xffffffff)); }
};

class ParameterPanel	:	public Viewport, public Slider::Listener
{
public:
	ParameterPanel (LuaProtoplugJuceAudioProcessor* _processor) 
	{
		processor = _processor;
		content = new ParamPanelContent();
		content->setBounds(0, 0, 220, NPARAMS*36+36);
		for (int i=0; i<NPARAMS; i++) {
			labels[i] = new Label();
			labels[i]->setEditable (false, false, false);
			labels[i]->setBounds(10, i*36, 100, 22);
			content->addAndMakeVisible(labels[i]);
			sliders[i] = new ParamSlider(processor, i);
			sliders[i]->setSliderStyle (Slider::LinearBar);
			sliders[i]->setBounds(110, i*36, getWidth()-130, 22);
			sliders[i]->setRange(0, 1.0);
			sliders[i]->setValue(processor->params[i], dontSendNotification);
			sliders[i]->updateText();
			sliders[i]->addListener(this);
			content->addAndMakeVisible(sliders[i]);
		}
		updateNames();
		setViewedComponent (content);
	}

	void resized()
	{
		content->setSize(std::max<int>(getWidth()-getLookAndFeel().getDefaultScrollbarWidth(),320), NPARAMS*36+36);
		for (int i=0; i<NPARAMS; i++) {
			sliders[i]->setSize(std::max<int>(getWidth()-130,200), 22);
		}
		// work around shit
		setViewPosition(getViewPosition().x, getViewPosition().y+1);
		setViewPosition(getViewPosition().x, getViewPosition().y-1);
	}

	void sliderDragStarted (Slider* sliderThatWasMoved)
	{
		for (int i=0; i<NPARAMS; i++)
			if (sliders[i]==sliderThatWasMoved) {
				processor->beginParameterChangeGesture(i);
				break;
			}
	}
	void sliderDragEnded (Slider* sliderThatWasMoved)
	{
		for (int i=0; i<NPARAMS; i++)
			if (sliders[i]==sliderThatWasMoved) {
				processor->endParameterChangeGesture(i);
				break;
			}
	}
	void sliderValueChanged (Slider* sliderThatWasMoved)
	{
		for (int i=0; i<NPARAMS; i++)
			if (sliders[i]==sliderThatWasMoved) {
				processor->setParameterNotifyingHost(i, (float)sliderThatWasMoved->getValue());
				sliders[i]->updateText();
				break;
			}
	}
	void updateNames()
	{
		for (int i=0; i<NPARAMS; i++) {
			String s = processor->luli->getParameterName(i);
			if (s==String::empty) {
				s = "nameless";
				labels[i]->setColour(Label::textColourId, Colours::grey);
			} else
				labels[i]->setColour(Label::textColourId, Colours::black);
			labels[i]->setText(String::formatted("%d. ",i) + s, dontSendNotification);
		}
	}
	void paramsChanged()
	{
		for (int i=0; i<NPARAMS; i++) {
			sliders[i]->setValue(processor->params[i], dontSendNotification);
			sliders[i]->updateText();
		}
	}
	void paint (Graphics& g) { g.fillAll (Colour(0xffffffff)); }

private:
    ScopedPointer<Component> content;
    ScopedPointer<Slider> sliders[NPARAMS];
    ScopedPointer<Label> labels[NPARAMS];
	LuaProtoplugJuceAudioProcessor* processor;
};

