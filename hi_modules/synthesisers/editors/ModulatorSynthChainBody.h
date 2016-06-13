/*
  ==============================================================================

    ModulatorSynthChainBody.h
    Created: 29 May 2015 3:25:10pm
    Author:  Christoph

  ==============================================================================
*/

#ifndef MODULATORSYNTHCHAINBODY_H_INCLUDED
#define MODULATORSYNTHCHAINBODY_H_INCLUDED

class ScriptContentContainer;

class ModulatorSynthChainBody: public ProcessorEditorBody
{
public:

	ModulatorSynthChainBody(ProcessorEditor *parentEditor);

	~ModulatorSynthChainBody()
	{
		container = nullptr;
	}

	int getBodyHeight() const override;;

	void resized();;

	/** Refreshes the interface list. */
	void updateGui() override;

private:

	ModulatorSynthChain *chain;
	ScopedPointer<ScriptContentContainer> container;

};



#endif  // MODULATORSYNTHCHAINBODY_H_INCLUDED
