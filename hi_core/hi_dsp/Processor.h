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

#ifndef PROCESSOR_H_INCLUDED
#define PROCESSOR_H_INCLUDED

class Chain;

class ProcessorEditor;
class ProcessorEditorBody;
class FactoryTypeConstrainer;

class FactoryType;

#define loadAttribute(name, nameAsString) (setAttribute(name, (float)v.getProperty(nameAsString, false), sendNotification))
#define saveAttribute(name, nameAsString) (v.setProperty(nameAsString, getAttribute(name), nullptr))

// Handy macro to set the name of the processor (type = Identifier, name = Displayed processor name)
#define SET_PROCESSOR_NAME(type, name) static String getClassName() {return name;}; \
						  const String getName() const override {	return getClassName();}; \
						  static Identifier getClassType() {return Identifier(type);} \
						  const Identifier getType() const override {return getClassType();}




/** If your Processor uses a Table object for anything, you can subclass it from this interface class and access its Table by script. */
class LookupTableProcessor
{
public:

	struct TableChangeBroadcaster : public SafeChangeBroadcaster
	{
		CriticalSection lock;

		WeakReference<Table> table;
		float tableIndex;
	};

    virtual ~LookupTableProcessor() {};
    
	/** Overwrite this method and return the table for the supplied index (or ignore this parameter if you only have one table).*/
	virtual Table *getTable(int tableIndex) const = 0;

	void addTableChangeListener(SafeChangeListener *listener)
	{
		tableChangeBroadcaster.addChangeListener(listener);
	};

	void removeTableChangeListener(SafeChangeListener *listener)
	{
		tableChangeBroadcaster.removeChangeListener(listener);
	};

	/** Call this method whenever the table index is changed and all connected tables will receive a change message from the internal broadcaster
	*
	*	You can dynamic_cast the broadcaster to a LookupTableProcessor::TableChangeBroadcaster and use this code:
	*
	*		if(dynamic_cast<LookupTableProcessor::TableChangeBroadcaster*>(b) != nullptr)
	*		{
	*			dynamic_cast<LookupTableProcessor::TableChangeBroadcaster*>(b)->table->setDisplayedIndex(tableValue);
	*		}
	*/
	void sendTableIndexChangeMessage(bool sendSynchronous, Table *table, float tableIndex)
	{
		ScopedLock sl(tableChangeBroadcaster.lock);

		tableChangeBroadcaster.table = table;
		tableChangeBroadcaster.tableIndex = tableIndex;

		if (sendSynchronous) tableChangeBroadcaster.sendSynchronousChangeMessage();
		else tableChangeBroadcaster.sendChangeMessage();
	}

	

private:

	

	TableChangeBroadcaster tableChangeBroadcaster;
};


/** The base class for all modules.
*
*	Every object within HISE that processes audio or MIDI is derived from this base class to share the following features:
*
*	- handling of child processors (and special treatment for child processors which are Chains)
*	- bypassing & parameter management (using the 'float' type for compatibility with audio processing & plugin parameters): setAttribute() / getAttribute()
*	- set / restore view properties (folded, body shown etc.) using a NamedValueSet (setEditorState(), getEditorState())
*	- set input / output values for metering and stuff (setInputValue(), setOutputValue())
*	- access to the global MainController object (see ControlledObject)
*	- import / export via ValueTree (see RestorableObject)
*	- methods for identification (getId(), getType(), getName(), getSymbol(), getColour())
*	- access to the console
*	- a specially designed component (ProcessorEditor) which acts as interface for the Processor.
*
*	The general architecture of a HISE patch is a tree of Processors, with a main processor (which can be obtained using getMainController()->getMainSynthChain()).
*	There is a small helper class ProcessorHelpers, which contains some static methods for searching & checking the type of the Processor.
*
*	Normally, you would not derive from this class directly, but use some of its less generic subclasses (MidiProcessor, Modulator, EffectProcessor or ModulatorSynth). 
*	There are also some additional interface classes to extend the Processor class (Chain, LookupTableProcessor, AudioSampleProcessor) with specific features.
*/
class Processor: public SafeChangeBroadcaster,
				 public RestorableObject,
				 public ControlledObject
{
public:

	/**	Creates a new Processor with the given Identifier. */
	Processor(MainController *m, const String &id_):
		ControlledObject(m),
		id(id_),
		consoleEnabled(false),
		bypassed(false),
		visible(true),
		sampleRate(-1.0),
		samplesPerBlock(-1),
		inputValue(0.0f),
		outputValue(0.0f),
		editorState(0),
		symbol(Path())
	{
		editorStateIdentifiers.add("Folded");
		editorStateIdentifiers.add("BodyShown");
		editorStateIdentifiers.add("Visible");
		editorStateIdentifiers.add("Solo");
		
		setEditorState(Processor::BodyShown, true, dontSendNotification);
		setEditorState(Processor::Visible, true, dontSendNotification);
		setEditorState(Processor::Solo, false, dontSendNotification);
	};

	/** Overwrite this if you need custom destruction behaviour. */
	virtual ~Processor()
	{
		getMainController()->getMacroManager().removeMacroControlsFor(this);
		masterReference.clear();
		removeAllChangeListeners();	
	};

	/** Overwrite this enum and add new parameters. This is used by the set- / getAttribute methods. */
	enum SpecialParameters {};

	/** Overwrite this enum and list all internal chains. This can be used to access them by index. */
	enum InternalChains {};

	enum EditorState
	{
		Folded = 0,
		BodyShown,
		Visible,
		Solo,
		numEditorStates
	};

	/** Creates a ProcessorEditor for this Processor and returns the pointer.

		If you subclass this, just allocate a ProcessorEditor of the desired type on the heap and return it.
		Remember to pass the Processor as parameter to allow asynchronous GUI notification.
		The concept between Processor and ProcessorEditor is the same as AudioProcessor and AudioProcessorEditor.
	*/
	virtual ProcessorEditorBody *createEditor(ProcessorEditor* parentEditor) = 0;

	/** This saves the Processor.
	*
	*	It saves the ID, the bypassed state and the fold state. It also saves all child processors.
	*	You can overwrite this function and add more properties and their child processors but make sure you call the base class method.
	*
	*	For primitive values, you can use the macro saveAttribute(name, nameAsString):
	*	
	*	You don't need to save the editor states, as the first 32 bits of EditorState is saved.
	*
		@code
		ValueTree exportAsValueTree() const override
		{
			// must be named 'v' for the macros
			ValueTree v = BaseClass::exportAsValueTree(); 

			saveAttribute(attributeName, "AttributeName");

			// ...

			if(useTable) saveTable(tableVariableName, "TableVariableNameData");

			return v;
		};
		@endcode
	*
	*
	*	@see restoreFromValueTree()
	*
	*/
	virtual ValueTree exportAsValueTree() const override
	{
#if USE_OLD_FILE_FORMAT
		ValueTree v(getType());
#else
		ValueTree v("Processor");
		v.setProperty("Type", getType().toString(), nullptr);
#endif

		v.setProperty("ID", getId(), nullptr);
		v.setProperty("Bypassed", isBypassed(), nullptr);

		ScopedPointer<XmlElement> editorValueSet = new XmlElement("EditorStates");
		editorStateValueSet.copyToXmlAttributes(*editorValueSet);		

#if USE_OLD_FILE_FORMAT
		v.setProperty("EditorState", editorValueSet->createDocument(""), nullptr);

		for(int i = 0; i < getNumChildProcessors(); i++)
		{
			v.addChild(getChildProcessor(i)->exportAsValueTree(), i, nullptr);
		};

#else
		ValueTree editorStateValueTree = ValueTree::fromXml(*editorValueSet);
		v.addChild(editorStateValueTree, -1, nullptr);

		ValueTree childProcessors("ChildProcessors");

		for(int i = 0; i < getNumChildProcessors(); i++)
		{
			childProcessors.addChild(getChildProcessor(i)->exportAsValueTree(), i, nullptr);
		};

		v.addChild(childProcessors, -1, nullptr);
#endif
		return v;

	};
	
	/** Restores a previously saved ValueTree. 
	*
	*	The value tree must be created with exportAsValueTree or it will be unpredictable.
	*	The child processors are created automatically (both for chains and processors with fixed internal chains,
	*	but you should overwrite this method if your Processor uses parameters.
	*
	*	There is a handy macro saveAttribute(name, nameAsString) for this purpose.
	*
	*	@code
		restoreFromValueTree(const ValueTree &v) override // parameter must be named 'v' for the macros
		{
			// replace BaseClass with the class name of the immediate base class
			BaseClass::restoreFromValueTree(v); 
			
			loadAttribute(attributeName, "AttributeName");
			// ...
			
			// If your Processor uses tables: 
			if(useTable) loadTable(tableVariableName, "TableVariableData");
		}
		@endcode
	*
	*	@see exportAsValueTree() 
	*/
	virtual void restoreFromValueTree(const ValueTree &previouslyExportedProcessorState) override;
	
	/** Overwrite this method to specify the name. 
    *
    *   Use non-whitespace Strings (best practice is CamelCase). In most cases, this will be enough:
	*
    *       const String getType() const { return "ProcessorName";}; 
    */
	virtual const Identifier getType() const = 0;

	

	/** Returns the symbol of the Processor. 
	*
	*	It either checks if a special symbol is set for this particular Processor, or returns the default one defined in getSpecialSymbol()
	*/
	const Path getSymbol() const
	{
		if(symbol.isEmpty())
		{
			return getSpecialSymbol();
		}
		else return symbol;
	}

	/** Sets a special symbol for the Processor.
	*
	*	If this method is used, the getSymbol() method will return this Path instead of the default one defined in getSpecialSymbol();
	*/
	void setSymbol(Path newSymbol) {symbol = newSymbol;	};

	/** Changes a Processor parameter. 
	*
	*   This can be used in the audio thread eg. by other Processors. Overwrite the method setInternalAttribute() to store the
    *   parameters in private member variables.
	*   If the Processor is polyphonic, the attributes must stay the same for different voices. In this
    *   case store a multiplicator for the attribute in a ProcessorState.
    *
	*   \param parameterIndex the parameter index (use a enum from the derived class)
	*   \param newValue the new value between 0.0 and 1.0
	*	\param notifyEditor if sendNotification, then a asynchronous message is sent.
	*/
	void setAttribute(int parameterIndex, float newValue, juce::NotificationType notifyEditor )
					 
	{
		setInternalAttribute(parameterIndex, newValue);
		if(notifyEditor == sendNotification) sendChangeMessage();
	}

	/** returns the attribute with the specified index (use a enum in the derived class). */
	virtual float getAttribute(int parameterIndex) const = 0;

	/** Overwrite this and return the default value. */
	virtual float getDefaultValue(int /*parameterIndex*/) const { return 1.0f; }

	/** This must be overriden by every Processor and return the Chain with the Chain index.
	*
	*	You can either:
	*	- return nullptr, if the Processor uses no internal chains
	*	- one of the internal chains (it's recommended to use the InternalChains enum for this case)
	*	- return the Processor at the index if the Processor is a Chain. You can use it's handler's getProcessor() method.
	*/
	virtual Processor *getChildProcessor(int processorIndex) = 0;

	virtual const Processor *getChildProcessor(int processorIndex) const = 0;

	/** This must return the number of Child processors.
	*
	*	If this Processor is a Chain, you can use it's getHandler()->getNumProcessor() method.
	*/
	virtual int getNumChildProcessors() const = 0;

	/** If your processor uses internal chains, you can return the number here. 
	*
	*	This is used by the ProcessorEditor to make the chain button bar. 
	*/
	virtual int getNumInternalChains() const { return 0;};

	void setConstrainerForAllInternalChains(FactoryTypeConstrainer *constrainer);

	/** Enables the Processor to output messages to the Console.
    *
	*   This method itself does nothing, but you can use this to make a button or something else.
	*/
	void enableConsoleOutput(bool shouldBeEnabled) {consoleEnabled = shouldBeEnabled;};

	/** Overwrite this method if you want a special colour. 
	*
	*	This colour will be used in the debug console and in the editor.
	*/
	virtual Colour getColour() const
	{
		return Colours::grey;
	};

	/** Returns the unique id of the Processor instance (!= the Processor name). 
	*
	*	It must be a valid Identifier (so no whitespace and weird characters).
	*/
	const String & getId() const {return id;};

	/** Overwrite this and return a pretty name. If you don't overwrite it, the getType() method is used. */
	virtual const String getName() const 
	{
		return getType().toString();
	}

	

	void setId(const String &newId)
	{
		id = newId;
		sendChangeMessage();
	};

	/** This bypasses the processor. You don't have to check in the processors logic itself, normally the chain should do that for you. */
	virtual void setBypassed(bool shouldBeBypassed) noexcept 
	{ 
		bypassed = shouldBeBypassed; 
		currentValues.clear();
	};

	/** Returns true if the processor is bypassed. */
	bool isBypassed() const noexcept { return bypassed; };

	/** Sets the sample rate and the block size. */
	virtual void prepareToPlay(double sampleRate_, int samplesPerBlock_)
	{
		sampleRate = sampleRate_;
		samplesPerBlock = samplesPerBlock_;
	}

	/** Returns the sample rate. */
	double getSampleRate() const { return sampleRate; };

	/** Returns the block size. */
	int getBlockSize() const
    {   
        return samplesPerBlock;
    };

	
#if USE_BACKEND
	/** Prints a message to the console.
    *
	*	Use this internally to debug important information if the console output is enabled for the Processor.
	*	Never use this method directly, but use the macro debugMod() so the String creation can be skipped if compiling with the flag HI_DEBUG_MODULATORS set to 0
	*	
	*/
	void debugProcessor(const String &t);
#endif

	/** This can be used to display the Processors output value. */
	float getOutputValue() const {	return currentValues.outL;	};

	/** This can be used to display the Processors input value. */
	float getInputValue() const
	{
		return inputValue;
	};

	/** Saves the state of the Processor's editor. It must be saved within the Processor, because the Editor can be deleted. 
	*
	*	You can add more states in your subclass (they should be expressable as bool). Best use a enum:
	*
	*		enum EditorState
	*		{
	*			newState = Processor::numEditorStates,
	*		};
	*/
	void setEditorState(int state, bool isOn, NotificationType notifyView=sendNotification)
	{
		const Identifier stateId = getEditorStateForIndex(state);

		editorStateValueSet.set(stateId, isOn);

        if(notifyView)
		{
			getMainController()->setCurrentViewChanged();
		}
	};

	const var getEditorState(Identifier id) const
	{
		return editorStateValueSet[id];
	}

	void setEditorState(Identifier state, var stateValue, NotificationType notifyView=sendNotification)
	{
		jassert(state.isValid());

		editorStateValueSet.set(state, stateValue);

		if(notifyView)
		{
			getMainController()->setCurrentViewChanged();
		}
	}

	Identifier getEditorStateForIndex(int index) const
	{
		// Did you forget to add the identifier to the id list in your subtype constructor?
		jassert(index < editorStateIdentifiers.size());

		return editorStateIdentifiers[index];
	}

	void toggleEditorState(int index, NotificationType notifyEditor)
	{
		bool on = getEditorState(getEditorStateForIndex(index));

		setEditorState(getEditorStateForIndex(index), !on, notifyEditor);
	}

	/** Restores the state of the Processor's editor. It must be saved within the Processor, because the Editor can be deleted. */
	bool getEditorState(int state) const 
	{
		return editorStateValueSet[getEditorStateForIndex(state)];

		//return editorState[state];
	};

	XmlElement *getCompleteEditorState() const
	{
		XmlElement *state = new XmlElement("EditorState");

		editorStateValueSet.copyToXmlAttributes(*state);

		return state;
	};

	/** Restores the EditorState from a BigInteger that was retrieved using getCompleteEditorState. */
	void restoreCompleteEditorState(const XmlElement *storedState)
	{
		if(storedState != nullptr)
		{
			editorStateValueSet.setFromXmlAttributes(*storedState);
		}
	};

	struct DisplayValues
	{
		DisplayValues():
			inL(0.0f),
			inR(0.0f),
			outL(0.0f),
			outR(0.0f)
		{};

		void clear()
		{
			inL = 0.0f;
			inR = 0.0f;
			outL = 0.0f;
			outR = 0.0f;
		};

		float inL;
		float outL;
		float inR;
		float outR;
	};

	DisplayValues getDisplayValues() const { return currentValues;};

	/** A iterator over all child processors. 
	*
	*	You don't have to use a inherited class of Processor for the template argument, it works with all classes.
	*/
	template<class SubTypeProcessor=Processor>
	class Iterator
	{
	public:

		/** Creates a new iterator. Simply pass in the Processor which children you want to iterate.
		*
		*	It creates a list of all child processors (children before siblings).
		*	Call getNextProcessor() to get the next child processor. 
		*/
		Iterator(Processor *root_, bool useHierarchy=false):
			hierarchyUsed(useHierarchy),
			index(0)
		{
			if(useHierarchy)
			{
				internalHierarchyLevel = 0;
				addProcessorWithHierarchy(root_);

			}
			else
			{
				addProcessor(root_);
			}
		};

		Iterator(const Processor *root_, bool useHierarchy = false) :
			hierarchyUsed(useHierarchy),
			index(0)
		{
			if (useHierarchy)
			{
				internalHierarchyLevel = 0;
				addProcessorWithHierarchy(const_cast<Processor*>(root_));

			}
			else
			{
				addProcessor(const_cast<Processor*>(root_));
			}
		};

		/** returns the next processor. 
		*
		*	If a Processor gets deleted between creating the Iterator and calling this method, it will safely return nullptr.
		*	(in this case the remaining processors are not iterated, but this is acceptable for the edge case.
		*/
		SubTypeProcessor *getNextProcessor()
		{
			if(index == allProcessors.size()) return nullptr;

			return dynamic_cast<SubTypeProcessor*>(allProcessors[index++].get());
		};

		/** returns a const pointer to the next processor. 
		*
		*	If a Processor gets deleted between creating the Iterator and calling this method, it will safely return nullptr.
		*	(in this case the remaining processors are not iterated, but this is acceptable for the edge case.
		*/
		const SubTypeProcessor *getNextProcessor() const
		{
			if(index == allProcessors.size()) return nullptr;

			return dynamic_cast<const SubTypeProcessor*>(allProcessors[index++].get());
		}

		int getHierarchyForCurrentProcessor() const
		{

			// You must use the other method!
			jassert(hierarchyData.size() > index-1);

			return hierarchyData[index-1];

		}


		int getNumProcessors() const
		{
			return allProcessors.size();
		}

		SubTypeProcessor *getProcessor(int index)
		{
			return allProcessors[index];
		}

		const SubTypeProcessor* getProcessor(int index) const
		{
			return allProcessors[index];
		}

	private:


		void addProcessor(Processor *p)
		{
			jassert(p != nullptr);

			if(dynamic_cast<SubTypeProcessor*>(p) != nullptr)
			{
				allProcessors.add(p);
			}

			if (p == nullptr) return;

			for(int i = 0; i < p->getNumChildProcessors(); i++)
			{
				addProcessor(p->getChildProcessor(i));
			}
		};

		void addProcessorWithHierarchy(Processor *p)
		{
			jassert(p != nullptr);

			if (p == nullptr) return;

			const int thisHierarchy = internalHierarchyLevel;

			if(dynamic_cast<SubTypeProcessor*>(p) != nullptr)
			{
				allProcessors.add(p);
				hierarchyData.add(thisHierarchy);
			}

			internalHierarchyLevel++;

			for(int i = 0; i < p->getNumChildProcessors(); i++)
			{
				

				addProcessorWithHierarchy(p->getChildProcessor(i));

				internalHierarchyLevel = thisHierarchy + 1;

			}
		};

		const bool hierarchyUsed;

		int internalHierarchyLevel;
		
		mutable int index;

		Array<int> hierarchyData;

		Array<WeakReference<Processor>> allProcessors;
		
		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Iterator<SubTypeProcessor>)
	};

	/** This returns a Identifier with the name of the parameter.
	*
	*	If you want to use this feature (this lets you access Parameters with the script, you should add the parameter name
	*	for each parameter in your subtype constructor. */
	const Identifier getIdentifierForParameterIndex(int parameterIndex) const
	{
		if(parameterIndex > parameterNames.size()) return Identifier();

		return parameterNames[parameterIndex];
	};

	/** This returns the number of (named) parameters. */
	int getNumParameters() const {return parameterNames.size();}; 

	

protected:

	/** Overwrite this method if you want to supply a custom symbol for the Processor. 
	*
	*	By default, it creates an empty Path, so you either have to set a custom Symbol using setSymbol(), or overwrite the getSpecialSymbol() method in a subclass.
	*/
	virtual Path getSpecialSymbol() const { return Path(); }

	DisplayValues currentValues;

	/** Call this from the baseclass whenever you want its editor to display a value change. */
	void setOutputValue(float newValue)
	{
		currentValues.outL = newValue;

		//outputValue = (double)newValue;
		//sendChangeMessage();
	};

	/** Call this from the baseclass whenever you want its editor to display a input value change. 
	*
	*	For most stuff a 0.0 ... 1.0 range is assumed.
	*/
	void setInputValue(float newValue, NotificationType notify=sendNotification)
	{
		inputValue = newValue;

		if(notify == sendNotification)
		{
			sendChangeMessage();
		}
	};

	

	/** Changes a Processor parameter. 
	*
	*   Overwrite this method to do your handling. Call the overloaded method with the notification type parameter for external changes.
    *
	*   \param parameterIndex the parameter index (use a enum from the derived class)
	*   \param newValue the new value between 0.0 and 1.0
	*/
	virtual void setInternalAttribute(int parameterIndex, float newValue) = 0;
	
	bool consoleEnabled;

	Array<Identifier> parameterNames;
	Array<Identifier> editorStateIdentifiers;

private:

	Path symbol;

	WeakReference<Processor>::Master masterReference;
    friend class WeakReference<Processor>;

	Array<bool> editorStateAsBoolList;

	BigInteger editorState;

	NamedValueSet editorStateValueSet;
	

	float inputValue;
	double outputValue;

	bool bypassed;
	bool visible;

	double sampleRate;
	int samplesPerBlock;

	OwnedArray<Chain> chains;

	/// the unique id of the Processor
	String id;

};

/** A interface class for all Processors that have a dynamic size of child processors.
*
*	Subclass this, put it in your subclassed Chain and return a member object of the chain in Chain::getHandler().
*/
class ChainHandler: public SafeChangeBroadcaster
{
public:

	virtual ~ChainHandler() {};

	/** Adds a new processor to the chain. It must be owned by the chain. */
	virtual void add(Processor *newProcessor, Processor *siblingToInsertBefore) = 0;

	/** Deletes a processor from the chain. */
	virtual void remove(Processor *processorToBeRemoved) = 0;

	/** Returns the processor at the index. */
	virtual Processor *getProcessor(int processorIndex) = 0;

	virtual const Processor *getProcessor(int processorIndex) const = 0;

	/** Returns the amount of processors. */
	virtual int getNumProcessors() const = 0;

	/** Deletes all Processors in the Chain. */
	virtual void clear() = 0;
	

	

};

/** A interface class for all Processors that have a dynamic size of child processors.
*	
*	If your Processor has more than a fixed amount of internal child processors, derive it from this class, write a ChainHandler subclass with all
*	needed operations and you can add / delete Processors on runtime.
*
*	You might want to overwrite the Processors functions getNumChildProcessors() and getChildProcessor() with the handlers methods (handle internal chains manually)
*	This allows the restoreState function to only clear the dynamic list of processors.
*
*	Every subclassed Chain must be derived from Processor, or the result will be unpredictable.
*/
class Chain
{
public:

	/** Restores a Chain from a ValueTree. It creates all processors and restores their values. It returns false, if anything went wrong. */
	bool restoreChain(const ValueTree &v);

	/** Overwrite this and return the processor that owns this chain if it exists. */
	virtual Processor *getParentProcessor() = 0;

	/** Overwrite this and return the processor that owns this chain if it exists. */
	virtual const Processor *getParentProcessor() const = 0;

	/** return your subclassed ChainHandler. */
	virtual ChainHandler *getHandler() = 0;

	/** read only access to the ChainHandler. */
	virtual const ChainHandler *getHandler() const = 0;

	virtual ~Chain() {};

	/** Sets the FactoryType that will be used. */
	virtual void setFactoryType(FactoryType *newType) = 0;

	/** Returns the Factory type this processor is using. */
	virtual FactoryType *getFactoryType() const = 0;

};

#define ADD_NAME_TO_TYPELIST(x) (typeNames.add(FactoryType::ProcessorEntry(x::getClassType(), x::getClassName())))

/** A Constrainer objects can impose restrictions on a particular FactoryType
*
*	If you want to restrict the selection of possible Processor types, you can
*	subclass this, overwrite allowType with your custom rules and call
*	setConstrainer() on the FactoryType you want to limit.
*/
class FactoryTypeConstrainer
{
public:

	virtual ~FactoryTypeConstrainer() {};

	/** Overwrite this and return true if the FactoryType can create this Processor and false, if not. */
	virtual bool allowType(const Identifier &typeName) = 0;

};

/** This interface class lets the MainController do its work. 
*	@ingroup factory
*
*	You can tell a Processor (which should also be a Chain to make sense) to use a specific FactoryType with Processor::setFactoryType(), which will then use it in its popup menu
*	to create the possible Processors. Simply overwrite these two functions in your subclass:
*
*		Processor* createProcessor	(int typeIndex, const String &id);
*		const StringArray& getTypeNames() const;
*
*	A FactoryType constrains the number of creatable Processors by
*
*	- Type (will be defined by the subclass)
*	- Constrainer (can be added to a FactoryType and uses runtime information like parent processor etc.)
*
*/
class FactoryType
{
public:
	
	/** a simple POD which contains the id and the name of a Processor type. */
	struct ProcessorEntry
	{
		ProcessorEntry(const Identifier t, const String &n):
		type(t),
		name(n)
		{};

		ProcessorEntry() {};

		Identifier type;
		String name;
	};

	/** Creates a Factory type.  */
	FactoryType(Processor *owner_):
		owner(owner_),
		baseClassCalled(false),
		constrainer(nullptr),
		ownedConstrainer(nullptr)
	{};

	virtual ~FactoryType()
	{
		constrainer = nullptr;
		ownedConstrainer = nullptr;
	}

	

	/** Fills a popupmenu with all allowed processor types.
	*
	*	You can pass in a startIndex, if you overwrite this method for nested calls.
	*
	*	It returns the last index that can be used for the next menus.
	*/
	virtual int fillPopupMenu(PopupMenu &m, int startIndex=1)
	{
		Array<ProcessorEntry> types = getAllowedTypes();

		int index = startIndex;

		for(int i = 0; i < types.size(); i++)
		{
			m.addItem(i+startIndex, types[i].name);

			index++;
		}

		return index;
	};

	/** Overwrite this function and return a processor of the specific type index. */
	virtual Processor *createProcessor(int typeIndex, const String &ProcessorId) = 0;

	/** Returns the typeName using the result from the previously created popupmenu. */
	Identifier getTypeNameFromPopupMenuResult(int resultFromPopupMenu)
	{
		jassert(resultFromPopupMenu > 0);

		Array<ProcessorEntry> types = getAllowedTypes();

		return types[resultFromPopupMenu-1].type;
	}

	/** Returns the typeName using the result from the previously created popupmenu. */
	String getNameFromPopupMenuResult(int resultFromPopupMenu)
	{
		jassert(resultFromPopupMenu > 0);

		Array<ProcessorEntry> types = getAllowedTypes();

		return types[resultFromPopupMenu-1].name;
	}

	/** Returns the index of the type. */
	virtual int getProcessorTypeIndex(const Identifier &typeName) const
	{
		Array<ProcessorEntry> entries = getTypeNames();

		for(int i = 0; i < entries.size(); i++)
		{
			if(entries[i].type == typeName) return i;
		}

		return -1;
	};

	/** Returns the number of Processors that this factory can create. 
	*
	*	the rules defined in allowType are applied before counting the possible processors.
	*/
	virtual int getNumProcessors()
	{
		return getAllowedTypes().size();
	};

	const Processor *getOwnerProcessor() const {return owner.get();};
	Processor *getOwnerProcessor()			   {return owner.get();};

	/**	Checks if the type of the processor is found in the type name. 
	*
	*	You can overwrite this and add more conditions (in this case, call the base class method first to keep things safe!
	*/
	virtual bool allowType(const Identifier &typeName) const
	{
		baseClassCalled = true;

		bool isConstrained = (constrainer != nullptr) && !constrainer->allowType(typeName);

		if (isConstrained) return false;

		Array<ProcessorEntry> entries = getTypeNames();

		for(int i = 0; i < entries.size(); i++)
		{
			if(entries[i].type == typeName) return true;
		}
		
		return false;
	};

	/** Returns a unique ID for the new Processor.
	*
	*	It scans all existing Processors and returns something like "Processor12" if there are 11 other Processors with the same ID
	*/
	static String getUniqueName(Processor *id, String name=String::empty);
	

	/** Returns a string array with all allowed types that this factory can produce. */
	virtual Array<ProcessorEntry> getAllowedTypes()
	{
		Array<ProcessorEntry> allTypes = getTypeNames();
		
		Array<ProcessorEntry> allowedTypes;

		for(int i = 0; i < allTypes.size(); i++)
		{
			if(allowType(allTypes[i].type)) allowedTypes.add(allTypes[i]);

			// You have to call the base class' allowType!!!
			jassert(baseClassCalled);

			baseClassCalled = false;
		};
		return allowedTypes;
	};

	/** adds a Constrainer to a FactoryType. It will be owned by the FactoryType. You can pass nullptr. */
	virtual void setConstrainer(FactoryTypeConstrainer *newConstrainer, bool ownConstrainer=true)
	{

		constrainer = newConstrainer;

		if(ownConstrainer)
		{
			ownedConstrainer = newConstrainer;
		}
	}

	FactoryTypeConstrainer *getConstrainer()
	{
		return ownedConstrainer.get() != nullptr ? ownedConstrainer.get() : constrainer;
	}

protected:

	/** This should only be overwritten by the subclasses. For external usage, use getAllowedTypes(). */
	virtual const Array<ProcessorEntry> &getTypeNames() const = 0;

	WeakReference<Processor> owner;

private:

	// iterates all child processors and counts the number of same IDs.
	static bool countProcessorsWithSameId(int &index, const Processor *p, Processor *processorToLookFor, const String &nameToLookFor);

	FactoryTypeConstrainer *constrainer;

	ScopedPointer<FactoryTypeConstrainer> ownedConstrainer;

	mutable bool baseClassCalled;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FactoryType)
};


class ExternalFileProcessor
{
public:

    
    virtual ~ExternalFileProcessor() {};

	/** Call this to get the file for the string.
	*
	*	Whenever you want to read a file, use this method instead of the direct File constructor, so it will parse a global file expression to the local global folder.
	*/
	File getFile(const String &fileNameOrReference, PresetPlayerHandler::FolderType type = PresetPlayerHandler::GlobalSampleDirectory);

	bool isReference(const String &fileNameOrReference);

	/** Overwrite this method and replace all internal references with a global file expression (use getGlobalReferenceForFile())
	*
	*	You don't need to reload the data, it will be loaded from the global folder the next time you load the patch.
	*/
	virtual void replaceReferencesWithGlobalFolder() = 0;

	/** This will return a expression which can be stored instead of the actual filename. If you load the file using the getFile() method, it will look in the global sample folder. */
	String getGlobalReferenceForFile(const String &file, PresetPlayerHandler::FolderType type = PresetPlayerHandler::GlobalSampleDirectory);

private:

	File getFileForGlobalReference(const String &reference, PresetPlayerHandler::FolderType type = PresetPlayerHandler::GlobalSampleDirectory);
};


#define RESTORE_MATRIX() {getMatrix().restoreFromValueTree(v.getChildWithName("RoutingMatrix"));}

class SliderPackProcessor
{
public:
    
    virtual ~SliderPackProcessor() {};
    
    virtual SliderPackData *getSliderPackData(int index) = 0;

    virtual const SliderPackData *getSliderPackData(int index) const = 0;
};


/** If you want to use a audio sample from the pool, subclass your Processor from this and you will get some nice tools:
*
*	- automatic memory management (samples are reference counted and released on destruction)
*	- import / export sample properties in the RestorableObject methods
*	- handle the thumbnail cache
*	- designed to work with an AudioSampleBufferComponent
*
*	In order to use this class with a AudioSampleBufferComponent, just follow these steps:
*
*	1. Create a AudioSampleBufferComponent and use the method getCache() in the constructor.
*	2. Set the reference to the AudioSampleBuffer with AudioSampleBufferComponent::setAudioSampleBuffer();
*	3. Add the AudioSampleBuffer as ChangeListener (and remove it in the destructor!)
*	4. Add an AreaListener to the AudioSampleBufferComponent and call setRange() and setLoadedFile in the rangeChanged() callback
*/
class AudioSampleProcessor: public SafeChangeListener,
							public ExternalFileProcessor
{
public:

	enum SyncToHostMode
	{
		FreeRunning = 1,
		OneBeat,
		TwoBeats,
		OneBar,
		TwoBars,
		FourBars
	};

	/** Automatically releases the sample in the pool. */
	virtual ~AudioSampleProcessor()
	{
		mc->getSampleManager().getAudioSampleBufferPool()->releasePoolData(sampleBuffer);
	}

	void replaceReferencesWithGlobalFolder() override;

	/** Call this method within your exportAsValueTree method to store the sample settings. */
	void saveToValueTree(ValueTree &v) const
	{
		const Processor *thisAsProcessor = dynamic_cast<const Processor*>(this);

		const String fileName = GET_PROJECT_HANDLER(const_cast<Processor*>(thisAsProcessor)).getFileReference(loadedFileName, ProjectHandler::SubDirectories::AudioFiles);

		v.setProperty("FileName", fileName, nullptr);

		v.setProperty("min", sampleRange.getStart(), nullptr);
		v.setProperty("max", sampleRange.getEnd(), nullptr);
	};

	/** Call this method within your restoreFromValueTree() method to load the sample settings. */
	void restoreFromValueTree(const ValueTree &v)
	{
		const String savedFileName = v.getProperty("FileName", "");

#if USE_BACKEND

		String name = GET_PROJECT_HANDLER(dynamic_cast<Processor*>(this)).getFilePath(savedFileName, ProjectHandler::SubDirectories::AudioFiles);

#elif USE_FRONTEND

		String name = ProjectHandler::Frontend::getSanitiziedFileNameForPoolReference(savedFileName);

#endif

		setLoadedFile(name, true);

		Range<int> range = Range<int>(v.getProperty("min", 0), v.getProperty("max", 0));

		if(sampleBuffer != nullptr) setRange(range);
	}

	/** Returns the global thumbnail cache. Use this whenever you need a AudioSampleBufferComponent. */
	AudioThumbnailCache &getCache() {return *mc->getSampleManager().getAudioSampleBufferPool()->getCache();};

	/** This loads the file from disk (or from the pool, if existing and loadThisFile is false. */
	void setLoadedFile(const String &fileName, bool loadThisFile=false, bool forceReload=false);

	/** Sets the sample range that should be used in the plugin. 
	*
	*	This is called automatically if a AudioSampleBufferComponent is set up correctly.
	*/
	void setRange(Range<int> newSampleRange);

	/** Returns the range of the sample. */
	Range<int> getRange() const { return sampleRange; };

    int getTotalLength() const { return sampleBuffer != nullptr ? sampleBuffer->getNumSamples() : 0; };
    
	/** Returns a const pointer to the audio sample buffer. 
	*
	*	The pointer references a object from a AudioSamplePool and should be valid as long as the pool is not cleared. */
	const AudioSampleBuffer *getBuffer() {return sampleBuffer;};

	/** Returns the filename that was loaded.
	*
	*	It is possible that the file does not exist on your system:
	*	If you restore a pool completely from a ValueTree, it still uses the absolute filename as identification.
	*/
	String getFileName() const {return loadedFileName; };

	/** This callback sets the loaded file.
	*
	*	The AudioSampleBuffer should not change anything, but only send a message to the AudioSampleProcessor.
	*	This is where the actual reloading happens. 
	*/
	void changeListenerCallback(SafeChangeBroadcaster *b) override
	{
		AudioSampleBufferComponent *bc = dynamic_cast<AudioSampleBufferComponent*>(b);

		if(bc != nullptr)
		{
			setLoadedFile(bc->getCurrentlyLoadedFileName(), true);
			bc->setAudioSampleBuffer(sampleBuffer, loadedFileName);

			dynamic_cast<Processor*>(this)->sendSynchronousChangeMessage();
		}
		else jassertfalse;
	}

	/** Overwrite this method and do whatever needs to be done when the selected range changes. */
	virtual void rangeUpdated() {};

	/** Overwrite this method and do whatever needs to be done when a new file is loaded.
	*
	*	You don't need to call setLoadedFile(), but if you got some internal stuff going on, this is the place.
	*/
	virtual void newFileLoaded() {};

	double getSampleRateForLoadedFile() const { return sampleRateOfLoadedFile; }

protected:

	/** Call this constructor within your subclass constructor. */
	AudioSampleProcessor(Processor *p):
		length(0),
		sampleRateOfLoadedFile(-1.0),
		sampleBuffer(nullptr)
	{
		// A AudioSampleProcessor must be derived from Processor!
		jassert(p != nullptr);

		mc = p->getMainController();
	};


	String loadedFileName;
	Range<int> sampleRange;
	int length;
	
	const AudioSampleBuffer *getSampleBuffer() const { return sampleBuffer; };
	
	double sampleRateOfLoadedFile;

private:

	AudioSampleBuffer const *sampleBuffer;

	MainController *mc;

};


/** Some handy helper functions that are using mainly the Iterator. */
class ProcessorHelpers
{
public:

	/** Small helper function that returns the parent processor of the given child processor. 
	*
	*	@param childProcessor the processor which parent should be found. It must be within the normal tree structure.
	*	@param getParentSynth if true, then the synth where the processor resides will be looked for. 
	*	If false, it will return the chain where the Processor resides (either ModulatorChain, MidiProcessorChain or EffectChain)
	*/
	static const Processor *findParentProcessor(const Processor *childProcessor, bool getParentSynth);

	static Processor *findParentProcessor(Processor *childProcessor, bool getParentSynth);

	/** Returns the first Processor with the given name (It skips all InternalChains). If there are multiple Processors with the same name, it will always return the first one.
	*
	*	To avoid this, use PresetHandler::findProcessorsWithDuplicateId...
	*/
	static Processor *getFirstProcessorWithName(const Processor *rootProcessor, const String &name);

	template <class ProcessorType> static int getAmountOf(const Processor *rootProcessor, const Processor *upTochildProcessor = nullptr);

	template <class ProcessorType> static StringArray getAllIdsForType(const Processor *rootProcessor)
	{
		StringArray sa;

		Processor::Iterator<const Processor> iter(rootProcessor);

		const Processor *p;

		while ((p = iter.getNextProcessor()) != nullptr)
		{
			if (dynamic_cast<const ProcessorType*>(p) != nullptr) sa.add(p->getId());
		}

		return sa;
	}

	/** Small helper function that checks if the given processor is of the supplied type. */
	template <class ProcessorType> static bool is(const Processor *p)
	{
		return dynamic_cast<const ProcessorType*>(p) != nullptr;
	}

	/** Small helper function that checks if the given processor is of the supplied type. */
	template <class ProcessorType> static bool is(Processor *p)
	{
		return dynamic_cast<ProcessorType*>(p) != nullptr;
	}

	/** Checks if the Processor can be hidden. This returns true for all processors that show up in the popup list. */
	static bool isHiddableProcessor(const Processor *p);

	/** Returns a string that declares a variable to be copied into a script.
	*
	*	For a given Processor of type "Type" and id "name" it will return:
	*		
	*		id = Synth.getType("id");
	*
	*	Currently supported types (including all subclasses):
	*	- MidiProcessors
	*	- Modulators
	*	- ModulatorSynths
	*/
	static String getScriptVariableDeclaration(const Processor *p, bool copyToClipboard=true);
};


#endif  // PROCESSOR_H_INCLUDED
