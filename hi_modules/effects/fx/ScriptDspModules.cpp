


#define REGISTER_TYPE(x) instance->factory.registerType<x>();

var DspFactory::createModule(String name)
{
	return var(factory.createFromId(Identifier(name)));
}

void DspFactory::registerTypes(DspFactory *instance)
{
	REGISTER_TYPE(ScriptingDsp::Buffer);
	REGISTER_TYPE(ScriptingDsp::Gain);
	REGISTER_TYPE(ScriptingDsp::Biquad);
	REGISTER_TYPE(ScriptingDsp::MoogFilter);
	REGISTER_TYPE(ScriptingDsp::Delay);
}




