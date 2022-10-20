namespace hise {using namespace juce;



DspBaseObject::DspBaseObject() {}
DspBaseObject::~DspBaseObject() {}

const char* DspBaseObject::getStringParameter(int index, size_t& textLength) { return nullptr; }
void DspBaseObject::setStringParameter(int index, const char* text, size_t textLength) {}

int DspBaseObject::getNumConstants() const { return 0; }
void DspBaseObject::getIdForConstant(int index, char* name, int &size) const noexcept {}
	  
bool DspBaseObject::getConstant(int index, int& value) const noexcept 
{	
	if (index < getNumParameters())
	{
		value = index; 
		return true;
	}

	return false;
}

bool DspBaseObject::getConstant(int index, char* text, size_t& size) const noexcept { return false; }
bool DspBaseObject::getConstant(int index, float** data, int &size) noexcept		{ return false; }
bool DspBaseObject::getConstant(int index, float& value) const noexcept				{ return false; }



} // namespace hise