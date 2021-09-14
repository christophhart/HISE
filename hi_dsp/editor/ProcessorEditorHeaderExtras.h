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

#ifndef PROCESSOREDITORHEADEREXTRAS_H_INCLUDED
#define PROCESSOREDITORHEADEREXTRAS_H_INCLUDED

namespace hise { using namespace juce;

class Modulation;
class Modulator;
class ModulatorSynth;
class EffectProcessor;
class DummyModulatorEditorBody;
class FilterGraph;
class ProcessorEditorHeader;

class HeaderButton : public Component,
	public ButtonListener,
	public SettableTooltipClient,
	public Learnable
{
public:

	HeaderButton(const String &name, const unsigned char *path, size_t pathSize, ProcessorEditorHeader *parentHeader_);

	void buttonClicked(Button *) override;
	bool getToggleState() const { return button->getToggleState(); };
	void setToggleState(bool on, NotificationType notify = dontSendNotification) { button->setToggleState(on, notify); };

	void resized()  override{ button->setBounds(getLocalBounds().reduced(1)); }
	void refresh();
	void paint(Graphics &g) override;

	ScopedPointer<ShapeButton> button;

	ProcessorEditorHeader *parentHeader;
};

struct IntensitySlider : public Slider,
	public Learnable
{
	IntensitySlider(const String& name) :
		Slider(name)
	{};

	void mouseDrag(const MouseEvent& e);
};

class ChainIcon : public Component,
	public ChangeListener
{
public:

	enum ChainIcons
	{
		PolyphonicEffect = 5,
		MonophonicEffect,
		MasterEffect,
		VoiceModulator,
		TimeModulator,
		Envelope,
		ScriptingProcessor,
		MacroMod,
		Filter,
		ModulatorSynthIcon
	};

	ChainIcon(Processor *p);

	~ChainIcon();
	void changeListenerCallback(ChangeBroadcaster *b) override;

	bool drawIcon() const { return chainType != -1; };

	void paint(Graphics &g) override;

	void resized() override;

	int chainType;

	void mouseDown(const MouseEvent &m) override;

	ScopedPointer<FilterGraph> filterGraph;

	Processor *p;

};


struct ButtonShapes
{

public:

	enum Symbol
	{
		Fold = 0,
		Bypass,
		Debug,
		Plot,
		Add,
		Delete,
		Routing
	};

	static Drawable *createSymbol(Symbol s, bool on, bool enabled);;

private:

	static Drawable *foldShape(bool on, bool enabled);;
	static Drawable *deleteShape(bool /*on*/, bool enabled);;
	static Drawable *plotShape(bool /*on*/, bool enabled);;
	static Drawable *addShape(bool /*on*/, bool enabled);;
	static Drawable *bypassShape(bool /*on*/, bool enabled);
	static Drawable *bipolarShape(bool /*on*/, bool enabled);
	static Drawable *routingShape(bool, bool enabled);
};


} // namespace hise

#endif  // PROCESSOREDITORHEADEREXTRAS_H_INCLUDED
