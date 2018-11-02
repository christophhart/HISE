namespace hise
{
using namespace juce;


namespace raw
{


Processor* Builder::createFromBase64State(const String& base64EncodedString, Processor* parent, int chainIndex/*=-1*/)
{
	


	ValueTree v = ProcessorHelpers::ValueTreeHelpers::getValueTreeFromBase64String(base64EncodedString);

	// TODO
	// Create the processor somehow...

	Processor* p = nullptr;

	p->restoreFromValueTree(v);

	return p;
}

void Builder::setAttributes(Processor* p, const AttributeCollection& collection)
{
	for (const auto& i : collection)
	{
		p->setAttribute(i.index, i.value, dontSendNotification);
	}
}



}
}

