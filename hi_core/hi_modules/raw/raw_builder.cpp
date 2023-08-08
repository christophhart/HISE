namespace hise
{
using namespace juce;


namespace raw
{
	Processor* Builder::create(Processor* parent, const Identifier& processorType, int chainIndex)
	{
		Chain* c = nullptr;

		if (chainIndex == -1)
			c = dynamic_cast<Chain*>(parent);
		else
			c = dynamic_cast<Chain*>(parent->getChildProcessor(chainIndex));

		if (c == nullptr)
		{
			jassertfalse;
			return nullptr;
		}

		auto f = c->getFactoryType();
		int index = f->getProcessorTypeIndex(processorType);

		if (index != -1)
		{
			auto p = f->createProcessor(index, processorType.toString());
			return addInternal<Processor>(p, c);
		}

		return nullptr;
	}

	Processor* Builder::createFromBase64State(const String& base64EncodedString, Processor* parent, int chainIndex)
{
	ignoreUnused(parent, chainIndex);

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

