
ModulatorSynthChainBody::ModulatorSynthChainBody(ProcessorEditor *parentEditor) :
ProcessorEditorBody(parentEditor),
chain(dynamic_cast<ModulatorSynthChain*>(parentEditor->getProcessor()))
{
	addAndMakeVisible(container = new ScriptContentContainer(chain, this));
}

int ModulatorSynthChainBody::getBodyHeight() const
{
	return container->getContentHeight();
}

void ModulatorSynthChainBody::resized()
{
	container->setBounds((int)(0.1f * (float)getWidth()), 0, (int)(0.8f * (float)getWidth()), getHeight());
}

void ModulatorSynthChainBody::updateGui()
{
	container->checkInterfaces();
}
