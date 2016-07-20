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

#ifndef MISCCOMPONENTS_H_INCLUDED
#define MISCCOMPONENTS_H_INCLUDED

class MouseCallbackComponent : public Component
{
	// ================================================================================================================

	enum EnterState
	{
		Nothing = 0,
		Entered,
		Exited,
		numEnterStates
	};

	enum class Action
	{
		Moved,
		Dragged,
		Clicked,
		Entered,
		Nothing
	};

public:

	enum class CallbackLevel
	{
		NoCallbacks = 0,
		ClicksOnly,
		ClicksAndEnter,
		Drag,
		AllCallbacks
	};

	// ================================================================================================================

	class Listener
	{
	public:

		Listener() {}
		virtual ~Listener() { masterReference.clear(); }
		virtual void mouseCallback(const var &mouseInformation) = 0;

	private:

		friend class WeakReference < Listener > ;
		WeakReference<Listener>::Master masterReference;

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Listener)
	};

	// ================================================================================================================

	MouseCallbackComponent();;
	virtual ~MouseCallbackComponent() {};

	static StringArray getCallbackLevels();

	void setPopupMenuItems(const StringArray &newItemList);
	void setUseRightClickForPopup(bool shouldUseRightClickForPopup);

	void addMouseCallbackListener(Listener *l);
	void removeCallbackListener(Listener *l);
	void removeAllCallbackListeners();

	void mouseDown(const MouseEvent& event) override;
	void mouseDrag(const MouseEvent& event) override;
	void mouseEnter(const MouseEvent &event) override;
	void mouseExit(const MouseEvent &event) override;

	void setAllowCallback(const String &newCallbackLevel) noexcept;
	CallbackLevel getCallbackLevel() const;

	// ================================================================================================================

private:

	void sendMessage(const MouseEvent &event, Action action, EnterState state = Nothing);
	void sendToListeners(var clickInformation);

	PopupLookAndFeel plaf;
	const StringArray callbackLevels;
	CallbackLevel callbackLevel;

	StringArray itemList;
	bool useRightClickForPopup = true;

	Array<WeakReference<Listener>> listenerList;
	DynamicObject::Ptr currentEvent;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MouseCallbackComponent);

	// ================================================================================================================
};



class BorderPanel : public MouseCallbackComponent
{
public:

	// ================================================================================================================

	BorderPanel();
	~BorderPanel() {}

	void paint(Graphics &g);
	Colour c1, c2, borderColour;

	// ================================================================================================================

	float borderRadius;
	float borderSize;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BorderPanel);

	// ================================================================================================================
};


class ImageComponentWithMouseCallback : public MouseCallbackComponent
{
public:

	// ================================================================================================================

	ImageComponentWithMouseCallback();

	void paint(Graphics &g) override;;

	void setImage(const Image &newImage);;
	void setAlpha(float newAlpha);;
	void setOffset(int newOffset);;
	void setScale(double newScale);;

private:

	AffineTransform scaler;
	Image image;
	float alpha;
	int offset;
	double scale;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ImageComponentWithMouseCallback);

	// ================================================================================================================
};


class MultilineLabel : public Label
{
public:

	// ================================================================================================================

	MultilineLabel(const String &name);;
	~MultilineLabel() {};

	void setMultiline(bool shouldBeMultiline);;
	TextEditor *createEditorComponent() override;

private:

	bool multiline;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MultilineLabel);

	// ================================================================================================================
};



#endif  // MISCCOMPONENTS_H_INCLUDED
