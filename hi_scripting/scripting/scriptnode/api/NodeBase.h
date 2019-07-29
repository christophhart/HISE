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
 *   which also must be licensed for commercial applications:
 *
 *   http://www.juce.com
 *
 *   ===========================================================================
 */

#pragma once

namespace scriptnode
{

using namespace juce;
using namespace hise;

class DspNetwork;
class NodeComponent;
class HardcodedNode;

struct NodeBase : public ConstScriptingObject
{
	struct HelpManager: ControlledObject
	{
		HelpManager(NodeBase& parent, ValueTree d);

		struct Listener
		{
			virtual ~Listener() {};

			virtual void helpChanged(float newWidth, float newHeight) = 0;

			virtual void repaintHelp() = 0;

		private:

			JUCE_DECLARE_WEAK_REFERENCEABLE(Listener);
		};

		void update(Identifier id, var newValue)
		{
			if (id == PropertyIds::NodeColour)
			{
				highlightColour = PropertyHelpers::getColourFromVar(newValue);
				if (highlightColour.isTransparent())
					highlightColour = Colour(SIGNAL_COLOUR);

				if (helpRenderer != nullptr)
				{
					helpRenderer->getStyleData().headlineColour = highlightColour;
					
					helpRenderer->setNewText(lastText);

					for (auto l : listeners)
					{
						if (l != nullptr)
							l->repaintHelp();
					}
				}
			}
			else if (id == PropertyIds::Comment)
			{
				lastText = newValue.toString();
				rebuild();
			}
			else if (id == PropertyIds::CommentWidth)
			{
				lastWidth = (float)newValue;

				rebuild();
			}
		}

		void render(Graphics& g, Rectangle<float> area)
		{
			if (helpRenderer != nullptr && !area.isEmpty())
			{
				area.removeFromLeft(10.0f);
				g.setColour(Colours::black.withAlpha(0.1f));
				g.fillRoundedRectangle(area, 2.0f);
				helpRenderer->draw(g, area.reduced(10.0f));
			}
				
		}

		void addHelpListener(Listener* l)
		{
			listeners.addIfNotAlreadyThere(l);
			l->helpChanged(lastWidth + 30.0f, lastHeight + 20.0f);
		}

		void removeHelpListener(Listener* l)
		{
			listeners.removeAllInstancesOf(l);
		}

		Rectangle<float> getHelpSize() const
		{
			return { 0.0f, 0.0f, lastHeight > 0.0f ? lastWidth + 30.0f : 0.0f, lastHeight + 20.0f };
		}

	private:

		void rebuild()
		{
			if (lastText.isNotEmpty())
			{
				helpRenderer = new MarkdownRenderer(lastText);
				helpRenderer->setDatabaseHolder(dynamic_cast<MarkdownDatabaseHolder*>(getMainController()));
				helpRenderer->getStyleData().headlineColour = highlightColour;
				helpRenderer->setDefaultTextSize(15.0f);
				helpRenderer->parse();
				lastHeight = helpRenderer->getHeightForWidth(lastWidth);
			}
			else
			{
				helpRenderer = nullptr;
				lastHeight = 0.0f;
			}

			for (auto l : listeners)
			{
				if (l != nullptr)
					l->helpChanged(lastWidth + 30.0f, lastHeight);
			}
		}

		String lastText;
		Colour highlightColour = Colour(SIGNAL_COLOUR);

		float lastWidth = 300.0f;
		float lastHeight = 0.0f;

		Array<WeakReference<HelpManager::Listener>> listeners;
		ScopedPointer<hise::MarkdownRenderer> helpRenderer;
		valuetree::PropertyListener commentListener;
		valuetree::PropertyListener colourListener;

		JUCE_DECLARE_WEAK_REFERENCEABLE(HelpManager);
	};

	using Ptr = WeakReference<NodeBase>;
	using List = Array<WeakReference<NodeBase>>;

	struct Parameter: public ConstScriptingObject
	{
		Identifier getObjectName() const override { return PropertyIds::Parameter; }

		Parameter(NodeBase* parent_, ValueTree& data_);;

		void setValueAndStoreAsync(double newValue);
		void addModulationValue(double newValue);
		void multiplyModulationValue(double newValue);

		void clearModulationValues();

		void addConnectionTo(var dragDetails);

		bool matchesConnection(const ValueTree& c) const;

		Array<Parameter*> getConnectedMacroParameters() const;

		String getId() const;
		double getValue() const;
		DspHelpers::ParameterCallback& getReferenceToCallback();
		DspHelpers::ParameterCallback getCallback() const;
		void setCallback(const DspHelpers::ParameterCallback& newCallback);
		double getModValue() const;

		StringArray valueNames;
		NodeBase::Ptr parent;
		ValueTree data;

		void updateFromValueTree(Identifier, var newValue)
		{
			setValueAndStoreAsync((double)newValue);
		}

	private:

		

		static void nothing(double) {};
		void storeValue();

		valuetree::PropertyListener opTypeListener;
		valuetree::PropertyListener valuePropertyUpdater;
		valuetree::PropertyListener idUpdater;
		DspHelpers::ParameterCallback db;
		LockFreeUpdater valueUpdater;

		struct ParameterValue
		{
			double modAddValue = 0.0;
			double modMulValue = 1.0;
			double value = 0.0;

			double getModValue() const
			{
				return (value + modAddValue) * modMulValue;
			}

		} value;

		

		JUCE_DECLARE_WEAK_REFERENCEABLE(Parameter);
	};

	NodeBase(DspNetwork* rootNetwork, ValueTree data, int numConstants);;
	virtual ~NodeBase() {}

	/** Overwrite this method and implement the processing. */
	virtual void process(ProcessData& data) = 0;

	/** This will be called before calls to process. */
	virtual void prepare(PrepareSpecs specs) = 0;

	/** The default implementation calls process with a sample amount of 1, but you can override
	    this method if there's a faster way. */
	virtual void processSingle(float* frameData, int numChannels);

	virtual void handleHiseEvent(HiseEvent& e)
	{
		ignoreUnused(e);
	}

	/** Override this to reset your node. */
	virtual void reset() = 0;

	/** Override this method and create a Cpp class. */
	virtual String createCppClass(bool isOuterClass);

	/** Override this method and return a custom component. */
	virtual NodeComponent* createComponent();

	/** Override this method and return the bounds in the canvas for the topLeft position. */
	virtual Rectangle<int> getPositionInCanvas(Point<int> topLeft) const;
	
	virtual HardcodedNode* getAsHardcodedNode() { return nullptr; }

	void setBypassed(bool shouldBeBypassed);
	bool isBypassed() const noexcept;

	bool isConnected() const { return v_data.getParent().isValid(); }
	void setValueTreeProperty(const Identifier& id, const var value);

	void setDefaultValue(const Identifier& id, var newValue);

	virtual bool isPolyphonic() const { return false; }

	bool isBodyShown() const
	{
		if (v_data[PropertyIds::Folded])
			return false;

		if (auto p = getParentNode())
		{
			return p->isBodyShown();
		}

		return true;
	}

	HelpManager& getHelpManager() { return helpManager; }

	void addConnectionToBypass(var dragDetails);

	DspNetwork* getRootNetwork() const;
	ValueTree getParameterTree() { return v_data.getOrCreateChildWithName(PropertyIds::Parameters, getUndoManager()); }
	
	ValueTree getPropertyTree() { return v_data.getOrCreateChildWithName(PropertyIds::Properties, getUndoManager()); }

	NodeBase* getParentNode() const;
	ValueTree getValueTree() const;
	String getId() const;
	UndoManager* getUndoManager();

	Rectangle<int> getBoundsToDisplay(Rectangle<int> originalHeight) const;

	Rectangle<int> getBoundsWithoutHelp(Rectangle<int> originalHeight) const;

	void setNumChannels(int newNumChannels)
	{
		if (!v_data[PropertyIds::LockNumChannels])
			setValueTreeProperty(PropertyIds::NumChannels, newNumChannels);
	}

	bool hasFixChannelAmount() const;

	int getNumChannelsToProcess() const { return (int)v_data[PropertyIds::NumChannels]; };

	int getNumParameters() const;;
	Parameter* getParameter(const String& id) const;
	Parameter* getParameter(int index) const;
	void addParameter(Parameter* p);
	void removeParameter(int index);

	void setParentNode(Ptr newParentNode)
	{
		parentNode = newParentNode;
	}

	void setCurrentId(const String& newId)
	{
		currentId = newId;
	}

	String getCurrentId() const { return currentId; }


private:

	WeakReference<ConstScriptingObject> parent;

protected:

	ValueTree v_data;

private:

	String currentId;

	HelpManager helpManager;

	CachedValue<bool> bypassed;
	bool pendingBypassState;
	LockFreeUpdater bypassUpdater;

	ReferenceCountedArray<Parameter> parameters;

	
	WeakReference<NodeBase> parentNode;

	JUCE_DECLARE_WEAK_REFERENCEABLE(NodeBase);
};


}
