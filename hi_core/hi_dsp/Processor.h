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

#ifndef PROCESSOR_H_INCLUDED
#define PROCESSOR_H_INCLUDED

namespace hise { using namespace juce;

class Chain;

class ProcessorEditor;
class ProcessorEditorBody;

class FactoryType;

class BaseConstrainer
{

};



#define ADD_PARAMETER_DOC(parameter, description) addParameter({ parameter, #parameter, #parameter, description})
#define ADD_PARAMETER_DOC_WITH_NAME(parameter, name, description) addParameter({ parameter, #parameter, name, description})
#define ADD_CHAIN_DOC(chain, name, description) addChain({chain, #chain, name, description})

#define SET_DOC_NAME(classType) setName(classType::getClassName());

#define ADD_DOCUMENTATION_WITH_BASECLASS(baseClass) struct Documentation : public baseClass::Documentation { Documentation(); }; \
													ProcessorDocumentation* createDocumentation() const override { return new Documentation(); };
													

#define ADD_DOCUMENTATION() struct Documentation : public ProcessorDocumentation { Documentation(); }; \
							ProcessorDocumentation* createDocumentation() const override { return new Documentation(); };

#define SET_DOCUMENTATION(className) className::Documentation::Documentation()

class MarkdownHelpButton;

/** A object that holds all the documentation available for a certain processor.
*
*	In order to use it, subclass it as a inner class of your processor called ProcessorType::Documentation
*	and use the preprocessor macros `ADD_PARAMETER_DOC` and `ADD_CHAIN_DOC` and the addLine() method.
*
*	If you call the immediate base class constructor, it will make sure that all common parameters will be documented correctly.
*
*
*/
class ProcessorDocumentation
{
public:

	struct Entry
	{
		struct Sorter
		{
			int compareElements(Entry& first, Entry& second);
		};

		bool operator==(const Entry& other) const;;

		int index;
		Identifier id;
		String name;
		String helpText;
		String constrainer;

		String getMarkdownLine(bool usePrettyName) const;

		String createHelpText(int headLineLevel = 1) const;
	};

public:

	virtual ~ProcessorDocumentation();;

	int getNumAttributes() const;

	Identifier getAttributeId(int index) const;

	/** This creates and attaches a markdown help button to the given component.
	*
	*
	*/
	MarkdownHelpButton* createHelpButtonForParameter(int index, Component* componentToAttachTo);

	MarkdownHelpButton* createHelpButton();

	String createHelpText();

	void fillMissingParameters(Processor* p);

	void setOffset(int pOffset, int cOffset);

	ProcessorDocumentation();;

	void addParameter(Entry newParameter);

	void addChain(Entry newChain);

	void addLine(const String &l);

	void setName(const String& name_);

	String description;
	int parameterOffset = 0;
	int chainOffset = 0;

	String name;

	Array<Entry> parameters;

	Array<Entry> chains;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ProcessorDocumentation)
};

#define loadAttribute(name, nameAsString) (setAttribute(name, (float)v.getProperty(nameAsString, false), dontSendNotification))
#define saveAttribute(name, nameAsString) (v.setProperty(nameAsString, getAttribute(name), nullptr))
#define saveID(name) v.setProperty(#name, getAttribute(name), nullptr);
#define loadID(name) setAttribute(name, (float)v.getProperty(#name, false), dontSendNotification);
#define addAttributeID(name) parameterNames.add(Identifier(#name));

#define loadAttributeWithDefault(parameterId) setAttribute(parameterId, v.getProperty(getIdentifierForParameterIndex(parameterId), getDefaultValue(parameterId)), dontSendNotification);

// Handy macro to set the name of the processor (type = Identifier, name = Displayed processor name)
#define SET_PROCESSOR_NAME(type, name, description) static String getClassName() {return name;}; \
						  const String getName() const override {	return getClassName();}; \
						  static Identifier getClassType() {return Identifier(type);} \
						  const Identifier getType() const override {return getClassType();} \
						  String getDescription() const override { return description; }

/** The base class for all HISE modules in the signal path.
*	@ingroup core
*
*	Every object within HISE that processes audio or MIDI is derived from this base class to share the following features:
*
*	- handling of child processors (and special treatment for child processors which are Chains)
*	- bypassing & parameter management (using the 'float' type for compatibility with audio processing & plugin parameters): setAttribute() / getAttribute()
*	- set input / output values for metering and visualization (setInputValue(), setOutputValue())
*	- access to the global MainController object (see ControlledObject)
*	- import / export via ValueTree (see RestorableObject)
*	- methods for identification (getId(), getType(), getName(), getSymbol(), getColour())
*	- access to the console
*
*	The general architecture of a HISE patch is a tree of Processor objects, all residing in a main container of the type ModulatorSynthChain (which can be obtained using getMainController()->getMainSynthChain()).
*	There is a small helper class ProcessorHelpers, which contains some static methods for searching & checking the type of the Processor.
*
*	Normally, you would not derive from this class directly, but use some of its less generic subclasses (MidiProcessor, Modulator, EffectProcessor or ModulatorSynth). 
*	There are also some additional interface classes to extend the Processor class with specific features: \ref processor_interfaces
*/
class Processor: public ControlledObject,
                 public Dispatchable,
#if HISE_NEW_PROCESSOR_DISPATCH
                 public dispatch::SourceOwner,
#endif
                 public RestorableObject
{
public:
    
    /**	Creates a new Processor with the given Identifier. */
    Processor(MainController *m, const String &id_, int numVoices);;
    
    /** Overwrite this if you need custom destruction behaviour. */
    virtual ~Processor();;
    
    /** Overwrite this enum and add new parameters. This is used by the set- / getAttribute methods. */
    enum SpecialParameters
    {
        numParameters = 0
    };
    
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
    virtual ValueTree exportAsValueTree() const override;;
    
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
    
    virtual ProcessorDocumentation* createDocumentation() const;
    
    /** Returns the symbol of the Processor.
     *
     *	It either checks if a special symbol is set for this particular Processor, or returns the default one defined in getSpecialSymbol()
     */
    const Path getSymbol() const;
    
    /** Sets a special symbol for the Processor.
     *
     *	If this method is used, the getSymbol() method will return this Path instead of the default one defined in getSpecialSymbol();
     */
    void setSymbol(Path newSymbol);;
    
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
    void setAttribute(int parameterIndex, float newValue, dispatch::DispatchType notifyEditor );

    void setAttribute(int parameterIndex, float newValue, NotificationType notifyEditor )
    {
	    auto nt = static_cast<dispatch::DispatchType>(notifyEditor);
        setAttribute(parameterIndex, newValue, nt);
    }

    /** returns the attribute with the specified index (use a enum in the derived class). */
    virtual float getAttribute(int parameterIndex) const = 0;
    
    /** Overwrite this and return the default value. */
    virtual float getDefaultValue(int /*parameterIndex*/) const;
    
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
    
    /** Return a one-line description of the processor. */
    virtual String getDescription() const = 0;
    
    /** If your processor uses internal chains, you can return the number here.
     *
     *	This is used by the ProcessorEditor to make the chain button bar.
     */
    virtual int getNumInternalChains() const;;
    
    void setConstrainerForAllInternalChains(BaseConstrainer *constrainer);
    
    /** Enables the Processor to output messages to the Console.
     *
     *   This method itself does nothing, but you can use this to make a button or something else.
     */
    void enableConsoleOutput(bool shouldBeEnabled);;
    
    /** Overwrite this method if you want a special colour.
     *
     *	This colour will be used in the debug console and in the editor.
     */
    virtual Colour getColour() const;;
    
    /** getNumVoices() is occupied by the Synthesiser class, d'oh! */
    int getVoiceAmount() const noexcept;;
    
    /** Returns the unique id of the Processor instance (!= the Processor name).
     *
     *	It must be a valid Identifier (so no whitespace and weird characters).
     */
    const String & getId() const;;
    
    /** Overwrite this and return a pretty name. If you don't overwrite it, the getType() method is used. */
    virtual const String getName() const;
    
    void setId(const String &newId, NotificationType notifyChangeHandler=dontSendNotification);;
    
    const Identifier& getIDAsIdentifier() const;
    
    /** This bypasses the processor. You don't have to check in the processors logic itself, normally the chain should do that for you. */
    virtual void setBypassed(bool shouldBeBypassed, NotificationType notifyChangeHandler=dontSendNotification) noexcept;
    ;
    
    /** Returns true if the processor is bypassed. For checking the bypass state of ModulatorSynths, better use isSoftBypassed(). */
    bool isBypassed() const noexcept;;
    
    void sendSynchronousBypassChangeMessage();
    
    /** Sets the sample rate and the block size. */
    virtual void prepareToPlay(double sampleRate_, int samplesPerBlock_);
    
    /** Returns the sample rate. */
    double getSampleRate() const;;
    
    /** Returns the block size. */
    int getLargestBlockSize() const;;
    
    
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
    float getOutputValue() const;;
    
    /** This can be used to display the Processors input value. */
    float getInputValue() const;;
    
    bool isValidAndInitialised(bool checkOnAir=false) const;
    
    /** Saves the state of the Processor's editor. It must be saved within the Processor, because the Editor can be deleted.
     *
     *	You can add more states in your subclass (they should be expressable as bool). Best use a enum:
     *
     *		enum EditorState
     *		{
     *			newState = Processor::numEditorStates,
     *		};
     */
    void setEditorState(int state, bool isOn, NotificationType notifyView=sendNotification);;
    
    const var getEditorState(Identifier editorStateId) const;
    
    void setEditorState(Identifier state, var stateValue, NotificationType notifyView=sendNotification);
    
    Identifier getEditorStateForIndex(int index) const;
    
    void toggleEditorState(int index, NotificationType notifyEditor);
    
    /** Restores the state of the Processor's editor. It must be saved within the Processor, because the Editor can be deleted. */
    bool getEditorState(int state) const;;
    
    XmlElement *getCompleteEditorState() const;;
    
    /** Restores the EditorState from a BigInteger that was retrieved using getCompleteEditorState. */
    void restoreCompleteEditorState(const XmlElement *storedState);;
    
    struct DisplayValues
    {
        DisplayValues();;
        
        void clear();;
        
        float inL;
        float outL;
        float inR;
        float outR;
    };
    
    DisplayValues getDisplayValues() const;;
    
    struct BypassListener: public dispatch::ListenerOwner
    {
        BypassListener(dispatch::RootObject& r);
        virtual ~BypassListener();;
        
        virtual void bypassStateChanged(Processor* p, bool bypassState) = 0;
        
        void onBypassUpdate(dispatch::library::Processor* p, bool state);
        
        
        NEW_PROCESSOR_DISPATCH(dispatch::library::Processor::BypassListener dispatcher);
        
        JUCE_DECLARE_WEAK_REFERENCEABLE(BypassListener)
    };

    struct AttributeListener: public dispatch::ListenerOwner
    {
	    AttributeListener(dispatch::RootObject& r)
          NEW_PROCESSOR_DISPATCH(:dispatcher(r, *this, BIND_MEMBER_FUNCTION_2(AttributeListener::internalUpdate)))
	    {}

        virtual ~AttributeListener()
	    {
		    removeFromProcessor();
	    }


        virtual void onAttributeUpdate(Processor* p, uint16 index) = 0;

        void addToProcessor(Processor* p, const uint16* indexes, int numIndexes, dispatch::DispatchType n)
        {
            cp = p;
	        p->addAttributeListener(&dispatcher, indexes, numIndexes, n);
        }

        void removeFromProcessor()
        {
	        if(cp != nullptr)
                cp->removeAttributeListener(&dispatcher);
        }


    private:

        void internalUpdate(dispatch::library::Processor* p, uint16 index)
        {
			NEW_PROCESSOR_DISPATCH(onAttributeUpdate(&p->getOwner<hise::Processor>(), index));
        }

        using Callback = void(*)(dispatch::library::Processor*, uint8);
        
        NEW_PROCESSOR_DISPATCH(dispatch::library::Processor::AttributeListener dispatcher);

        WeakReference<Processor> cp;
        
        JUCE_DECLARE_WEAK_REFERENCEABLE(AttributeListener)
    };

    struct OtherListener: public dispatch::ListenerOwner,
						  public SafeChangeListener
    {
        OtherListener(Processor* p, dispatch::library::ProcessorChangeEvent eventToListenTo):
#if HISE_NEW_PROCESSOR_DISPATCH
          dispatcher(p->dispatcher.getRootObject(), *this, BIND_MEMBER_FUNCTION_1(OtherListener::onChange), eventToListenTo),
#endif
          processor(p)
        {
	        OLD_PROCESSOR_DISPATCH(processor->otherBroadcaster.addChangeListener(this));
            NEW_PROCESSOR_DISPATCH(processor->addOtherChangeListener(&dispatcher, dispatch::sendNotificationAsync));
        }

        ~OtherListener()
        {
	        if(auto p = processor.get())
	        {
		        OLD_PROCESSOR_DISPATCH(processor->otherBroadcaster.removeChangeListener(this));
                NEW_PROCESSOR_DISPATCH(p->removeOtherChangeListener(&dispatcher));
	        }
        }
        virtual void otherChange(Processor* p) = 0;

    	void changeListenerCallback(SafeChangeBroadcaster* b) override
	    {
#if HISE_OLD_PROCESSOR_DISPATCH
		    auto ob = dynamic_cast<OldBroadcaster*>(b);
            jassert(ob != nullptr);
            otherChange(&ob->parent);
#endif
	    }

    private:

        void onChange(dispatch::library::Processor* p)
        {
	        NEW_PROCESSOR_DISPATCH(otherChange(&p->getOwner<hise::Processor>()));
        }

        NEW_PROCESSOR_DISPATCH(dispatch::library::Processor::OtherChangeListener dispatcher);
        WeakReference<Processor> processor;
    };
    
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
        Iterator(const Processor *root, bool useHierarchy = false) :
        hierarchyUsed(useHierarchy),
        index(0)
        {
            jassert(root->isValidAndInitialised());
            WARN_IF_AUDIO_THREAD(true, MainController::KillStateHandler::IllegalOps::IteratorCreation);
            
            LockHelpers::SafeLock sl(root->getMainController(), LockHelpers::Type::IteratorLock, !root->getMainController()->isFlakyThreadingAllowed());
            
            if (useHierarchy)
            {
                internalHierarchyLevel = 0;
                addProcessorWithHierarchy(const_cast<Processor*>(root));
            }
            else
                addProcessor(const_cast<Processor*>(root));
            
        };
        
        /** returns the next processor.
         *
         *	If a Processor gets deleted between creating the Iterator and calling this method, it will safely return nullptr.
         *	(in this case the remaining processors are not iterated, but this is acceptable for the edge case.
         */
        SubTypeProcessor *getNextProcessor()
        {
            if(index == allProcessors.size()) return nullptr;
            
            auto thisProcessor = dynamic_cast<SubTypeProcessor*>(allProcessors[index++].get());
            
            if (thisProcessor != nullptr)
                return thisProcessor;
            
            return getNextProcessor();
        };
        
        /** returns a const pointer to the next processor.
         *
         *	If a Processor gets deleted between creating the Iterator and calling this method, it will safely return nullptr.
         *	(in this case the remaining processors are not iterated, but this is acceptable for the edge case.
         */
        const SubTypeProcessor *getNextProcessor() const
        {
            if(index == allProcessors.size()) return nullptr;
            
            auto thisProcessor = dynamic_cast<SubTypeProcessor*>(allProcessors[index++].get());
            
            if (thisProcessor != nullptr)
                return thisProcessor;
            
            return getNextProcessor();
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
            if (p == nullptr)
            {
                jassertfalse;
                return;
            }
            
            if (dynamic_cast<SubTypeProcessor*>(p) != nullptr)
                allProcessors.add(p);
            
            // If you hit this assertion, it means that somehow
            // a uninitialised processor got inserted into the processing
            // chain, which is bad. Initialise all processors BEFORE
            // adding them there...
            jassert(p->isValidAndInitialised());
            
            for(int i = 0; i < p->getNumChildProcessors(); i++)
            {
                addProcessor(p->getChildProcessor(i));
            }
        };
        
        void addProcessorWithHierarchy(Processor *p)
        {
            if (p == nullptr)
            {
                jassertfalse;
                return;
            }
            
            const int thisHierarchy = internalHierarchyLevel;
            
            // If you hit this assertion, it means that somehow
            // a uninitialised processor got inserted into the processing
            // chain, which is bad. Initialise all processors BEFORE
            // adding them there...
            jassert(p->isValidAndInitialised());
            
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
    virtual Identifier getIdentifierForParameterIndex(int parameterIndex) const;
    virtual int getParameterIndexForIdentifier(const Identifier& id) const;

    virtual int getNumAttributes() const { return parameterNames.size(); }

    

    String getDescriptionForParameters(int parameterIndex);
    
    /** This returns the number of (named) parameters. */
    virtual int getNumParameters() const;;
    
    /** Call this method after inserting the processor in the signal chain.
     *
     *	If you call prepareToPlay before calling this method, it won't lock which makes inserting new processors nicer.
     */
    void setIsOnAir(bool isBeingProcessedInAudioThread);
    
    bool isOnAir() const noexcept;
    
    struct DeleteListener
    {
        virtual ~DeleteListener();
        
        virtual void processorDeleted(Processor* deletedProcessor) = 0;
        
        virtual void updateChildEditorList(bool forceUpdate) = 0;
        
    private:
        
        friend class WeakReference<DeleteListener>;
        WeakReference<DeleteListener>::Master masterReference;
    };
    
    void addDeleteListener(DeleteListener* listener);
    
    void setIsWaitingForDeletion();
    
    bool isWaitingForDeletion() const noexcept;
    
    void removeDeleteListener(DeleteListener* listener);
    
    void sendDeleteMessage();
    
    void addBypassListener(BypassListener* l, dispatch::DispatchType n);
    
    void removeBypassListener(BypassListener* l);

    void sendOtherChangeMessage(dispatch::library::ProcessorChangeEvent eventType, dispatch::DispatchType n=dispatch::sendNotificationAsync);

    void addOtherChangeListener(dispatch::library::Processor::OtherChangeListener* l, dispatch::DispatchType n)
    {
        NEW_PROCESSOR_DISPATCH(dispatcher.addOtherChangeListener(l, n));
    }
    
    void removeOtherChangeListener(dispatch::library::Processor::OtherChangeListener* l)
    {
        NEW_PROCESSOR_DISPATCH(dispatcher.removeOtherChangeListener(l));
    }

    void addAttributeListener(dispatch::library::Processor::AttributeListener* l, const uint16* indexes, size_t numAttributes, dispatch::DispatchType n)
    {
        NEW_PROCESSOR_DISPATCH(dispatcher.addAttributeListener(l, indexes, numAttributes, n));
    }
    
    void removeAttributeListener(dispatch::library::Processor::AttributeListener* l)
    {
        NEW_PROCESSOR_DISPATCH(dispatcher.removeAttributeListener(l));
    }

    void addNameAndColourListener(dispatch::library::Processor::NameAndColourListener* l, dispatch::DispatchType n=dispatch::sendNotificationSync)
    {
        NEW_PROCESSOR_DISPATCH(dispatcher.addNameAndColourListener(l, n));
    }
    
    void removeNameAndColourListener(dispatch::library::Processor::NameAndColourListener* l)
    {
		NEW_PROCESSOR_DISPATCH(dispatcher.removeNameAndColourListener(l));
    }
    
	bool isRebuildMessagePending() const noexcept;

	void cleanRebuildFlagForThisAndParents();

	void sendRebuildMessage(bool forceUpdate = false);

	Processor* getParentProcessor(bool getOwnerSynth, bool assertIfFalse=true);

	const Processor* getParentProcessor(bool getOwnerSynth, bool assertIfFalse=true) const;

	void setParentProcessor(Processor* newParent);

	Array<Identifier> parameterNames;

    void updateParameterSlots(int numForced = -1)
	{
        if(numForced == -1)
            numForced = getNumAttributes();

		NEW_PROCESSOR_DISPATCH(dispatcher.setNumAttributes(numForced));
	}
    
protected:

	/** Overwrite this method if you want to supply a custom symbol for the Processor. 
	*
	*	By default, it creates an empty Path, so you either have to set a custom Symbol using setSymbol(), or overwrite the getSpecialSymbol() method in a subclass.
	*/
	virtual Path getSpecialSymbol() const;

	DisplayValues currentValues;

	/** Call this from the baseclass whenever you want its editor to display a value change. */
	void setOutputValue(float newValue);;

	/** Call this from the baseclass whenever you want its editor to display a input value change. 
	*
	*	For most stuff a 0.0 ... 1.0 range is assumed.
	*/
	void setInputValue(float newValue, NotificationType notify=sendNotification);;

	

	/** Changes a Processor parameter. 
	*
	*   Overwrite this method to do your handling. Call the overloaded method with the notification type parameter for external changes.
    *
	*   \param parameterIndex the parameter index (use a enum from the derived class)
	*   \param newValue the new value between 0.0 and 1.0
	*/
	virtual void setInternalAttribute(int parameterIndex, float newValue) = 0;
	
	bool consoleEnabled;

	
	StringArray parameterDescriptions;
	Array<Identifier> editorStateIdentifiers;

	NEW_PROCESSOR_DISPATCH(dispatch::library::Processor dispatcher);

private:

    struct OldBroadcaster: public SafeChangeBroadcaster
    {
	    OldBroadcaster(Processor& parent_):
          parent(parent_)
        {};

        Processor& parent;
    } otherBroadcaster;
    
	bool rebuildMessagePending = false;

	Array<WeakReference<DeleteListener>> deleteListeners;

	Array<WeakReference<BypassListener>> bypassListeners;

	CriticalSection dummyLock;

	bool onAir = false;

	bool pendingDelete = false;

	WeakReference<Processor> parentProcessor;

	Path symbol;

	WeakReference<Processor>::Master masterReference;
    friend class WeakReference<Processor>;

	Array<bool> editorStateAsBoolList;

	BigInteger editorState;

	NamedValueSet editorStateValueSet;
	
	const int numVoices;

	float inputValue;
	double outputValue;

	bool bypassed;
	bool visible;

	double samplerate;

	int largestBlockSize;

	OwnedArray<Chain> chains;

	/// the unique id of the Processor
	String id;

	Identifier idAsIdentifier;
};


/** Some handy helper functions that are using mainly the Iterator. */
class ProcessorHelpers
{
public:

	struct ObjectWithProcessor
	{
		virtual ~ObjectWithProcessor() {};

		virtual Processor* getProcessor() = 0;
	};

	/** Small helper function that returns the parent processor of the given child processor. 
	*
	*	@param childProcessor the processor which parent should be found. It must be within the normal tree structure.
	*	@param getParentSynth if true, then the synth where the processor resides will be looked for. 
	*	If false, it will return the chain where the Processor resides (either ModulatorChain, MidiProcessorChain or EffectChain)
	*/
	static const Processor *findParentProcessor(const Processor *childProcessor, bool getParentSynth);

	static Processor *findParentProcessor(Processor *childProcessor, bool getParentSynth);

    static juce::NotificationType getAttributeNotificationType();

	/** Returns the first Processor with the given name (It skips all InternalChains). If there are multiple Processors with the same name, it will always return the first one.
	*
	*	To avoid this, use PresetHandler::findProcessorsWithDuplicateId...
	*/
	static Processor *getFirstProcessorWithName(const Processor *rootProcessor, const String &name);

	static Array<WeakReference<Processor>> getListOfAllGlobalModulators(const Processor* rootProcessor);

	template <class ProcessorType> static int getAmountOf(const Processor *rootProcessor, const Processor *upTochildProcessor = nullptr);

	static StringArray getAllIdsForDataType(const Processor* rootProcessor, snex::ExternalData::DataType dataType);

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

	template <class ProcessorType> static Array<WeakReference<ProcessorType>> getListOfAllProcessors(const Processor* rootProcessor)
	{
		Array<WeakReference<ProcessorType>> list;

		Processor::Iterator<ProcessorType> iter(rootProcessor, false);

		while (auto t = iter.getNextProcessor())
		{
			list.add(t);
		}

		return list;
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

	template <class ProcessorType> static ProcessorType* getFirstProcessorWithType(const Processor *root)
	{
		Processor::Iterator<ProcessorType> iter(root);

		if (auto p = iter.getNextProcessor())
			return p;

		return nullptr;
	}

	static MarkdownLink getMarkdownLink(const Processor* p);

	/** Checks if the Processor can be hidden. This returns true for all processors that show up in the popup list. */
	static bool isHiddableProcessor(const Processor *p);

	static String getPrettyNameForAutomatedParameter(const Processor* p, int parameterIndex);

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

	static String getTypedScriptVariableDeclaration(const Processor* p, String typeName, bool copyToClipboard=true);

	static String getBase64String(const Processor* p, bool copyToClipboard=true, bool exportContentOnly=false);

	static void restoreFromBase64String(Processor* p, const String& base64String, bool restoreScriptContentOnly=false);

	static void increaseBufferIfNeeded(AudioSampleBuffer& b, int numSamplesNeeded);

	static void increaseBufferIfNeeded(hlac::HiseSampleBuffer& b, int numSamplesNeeded);

	struct ValueTreeHelpers
	{
		static String getBase64StringFromValueTree(const ValueTree& v);;

		static ValueTree getValueTreeFromBase64String(const String& base64State);
	};

    static void connectTableEditor(TableEditor& t, Processor* p, int index=0);


	/** Returns a list of all processors that can be connected to a parameter. */
	static StringArray getListOfAllConnectableProcessors(const Processor* processorToSkip);

	static StringArray getListOfAllParametersForProcessor(Processor* p);

	static int getParameterIndexFromProcessor(Processor* p, const Identifier& id);

};

} // namespace hise

#endif  // PROCESSOR_H_INCLUDED
