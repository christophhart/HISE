/** ============================================================================
 *
 * MCL Text Editor JUCE module 
 *
 * Copyright (C) Jonathan Zrake, Christoph Hart
 *
 * You may use, distribute and modify this code under the terms of the GPL3
 * license.
 * =============================================================================
 */


#pragma once

namespace mcl
{
using namespace juce;


//==============================================================================
class CaretComponent : public juce::Component, public juce::Timer
{
public:
	CaretComponent(const TextDocument& document);
	void setViewTransform(const juce::AffineTransform& transformToUse);
	void updateSelections();

	//==========================================================================
	void paint(juce::Graphics& g) override;

private:
	//==========================================================================
	float squareWave(float wt) const;
	void timerCallback() override;
	juce::Array<juce::Rectangle<float>> getCaretRectangles() const;
	//==========================================================================
	float phase = 0.f;
	const TextDocument& document;
	juce::AffineTransform transform;
};



}
