namespace snex

{

VariableStorage::VariableStorage() :
	data({})
{}

VariableStorage::VariableStorage(Types::ID type_, const var& value)
{
	data.i.type = type_;

	if (type_ == Types::ID::Integer)
		data.i.value = (int64)value;
	else if (type_ == Types::ID::Float)
		data.f.value = (float)value;
	else if (type_ == Types::ID::Double)
		data.d.value = static_cast<double>(value);
	else if (type_ == Types::ID::Pointer)
		data.p.data = reinterpret_cast<void*>((int64)value);
	else if (type_ == Types::ID::Event)
		data.e = HiseEvent();
	else if (type_ == Types::ID::Block)
		data.b = {};
	else
		jassertfalse;
}

VariableStorage::VariableStorage(FloatType s)
{
	data.d.type = Types::ID::Float;
	data.f.value = s;
}

VariableStorage::VariableStorage(int s) 
{
	data.i.type = Types::ID::Integer;
	data.i.value = (int64)s;
}

VariableStorage::VariableStorage(const block& b)
{
	data.b.referTo(b);
}

VariableStorage::VariableStorage(const HiseEvent& m_)
{
	data.e = m_;
}

VariableStorage::VariableStorage(double d)
{
	data.d.type = Types::ID::Double;
	data.d.value = d;
}

VariableStorage::VariableStorage(void* objectPointer, int objectSize)
{
	data.p.type = Types::Pointer;
	data.p.data = objectPointer;
	data.p.size = objectSize;
}

snex::VariableStorage& VariableStorage::operator=(const VariableStorage& other)
{
	data.e = HiseEvent(other.data.e);
	return *this;
}

bool VariableStorage::operator==(const VariableStorage& other) const
{
	if (other.getType() != getType())
		return false;

	if (getType() == Types::ID::Float)
		return std::abs(data.f.value - (float)other) < 1e-4;
	if (getType() == Types::ID::Double)
		return std::abs(data.d.value - (double)other) < 1e-4;
	if (getType() == Types::ID::Integer)
		return data.i.value == (int)other;
	if (getType() == Types::ID::Block)
		return (data.b.begin() == other.data.b.begin()) && (data.b.size() == other.data.b.size());
	if (getType() == Types::ID::Pointer)
		return data.p.data == other.data.p.data && data.p.size == other.data.p.size;
	if (getType() == Types::ID::Void)
		return true;

	return false;
}

void VariableStorage::setWithType(Types::ID newType, double value)
{
	switch (newType)
	{
	case snex::Types::Float:	data.f.type = Types::ID::Float;
								data.f.value = static_cast<float>(value);
								break;
	case snex::Types::Double:	data.d.type = Types::ID::Double;
								data.d.value = value; 
								break;
	case snex::Types::Integer:	data.i.type = Types::ID::Integer;
								data.i.value = static_cast<int>(value);
								break;
	case snex::Types::Dynamic:	data.d.type = Types::ID::Dynamic;
								data.d.value = value;
								break;
	default:
		jassertfalse;
		break;
	}
}

void VariableStorage::set(FloatType s)
{
	data.f.type = Types::ID::Float;
	data.f.value = s;
}

void VariableStorage::set(double s)
{
	data.d.type = Types::ID::Double;
	data.d.value = s;
}
    
void VariableStorage::set(int s)
{
	data.i.type = Types::ID::Integer;
	data.i.value = s;
}

void VariableStorage::set(block&& s)
{
	data.b = std::forward<block>(s);
}

void VariableStorage::set(block& b)
{
	data.i.type = Types::ID::Block;
	data.b = b;
}



void VariableStorage::set(void* objectPointer, int newSize)
{
	data.p.type = Types::ID::Pointer;
	data.p.data = objectPointer;
	data.p.size = newSize;
}

void VariableStorage::setDouble(double newValue)
{
	set(newValue);
}

void VariableStorage::clear()
{
	data.e = {};
}

VariableStorage::operator FloatType() const noexcept
{
	//jassert(Types::Helpers::isFloatingPoint((Types::ID)getTypeValue()));

	if (getTypeValue() == Types::ID::Float)
		return data.f.value;
	if (getTypeValue() == Types::ID::Double)
		return static_cast<float>(data.d.value);

	return static_cast<FloatType>(data.d.value);
}

VariableStorage::operator double() const noexcept
{
	//jassert(Types::Helpers::isFloatingPoint((Types::ID)getTypeValue()));

	if (getTypeValue() == Types::ID::Float)
		return static_cast<double>(data.f.value);
	if (getTypeValue() == Types::ID::Double)
		return data.d.value;

	return data.d.value;
}
    
VariableStorage::operator int() const
{
	jassert(getTypeValue() == Types::ID::Integer);
	return static_cast<int>(data.i.value);
}

VariableStorage::operator HiseEvent() const
{
	jassert(getTypeValue() < (int)HiseEvent::Type::numTypes);
	return data.e;
}

VariableStorage::operator void*() const
{
	jassert(getTypeValue() == Types::ID::Pointer);
	return data.p.data;
}

void* VariableStorage::toPtr() const
{
	jassert(!isVoid());

	return data.p.data;
}

double VariableStorage::toDouble() const
{
	if (getTypeValue() == Types::ID::Double)
		return data.d.value;
	else if (getTypeValue() == Types::ID::Float)
		return static_cast<double>(data.f.value);
	else if (getTypeValue() == Types::ID::Integer)
		return static_cast<double>(data.i.value);

	return 0.0;
}

snex::FloatType VariableStorage::toFloat() const
{
	if (getTypeValue() == Types::ID::Float)
		return data.f.value;
	else if (getTypeValue() == Types::ID::Double)
		return static_cast<float>(data.d.value);
	else if (getTypeValue() == Types::ID::Integer)
		return static_cast<float>(data.i.value);

	return FloatType(0);
}

int VariableStorage::toInt() const
{
	if (getTypeValue() == Types::ID::Integer)
		return static_cast<int>(data.i.value);
	if (getTypeValue() == Types::ID::Float)
		return static_cast<int>(data.f.value);
	else if (getTypeValue() == Types::ID::Double)
		return static_cast<int>(data.d.value);
	
	return 0;
}

snex::block VariableStorage::toBlock() const
{
	if (getTypeValue() == Types::ID::Block)
		return data.b;
	else
		return {};
}


HiseEvent VariableStorage::toEvent() const
{
	if (getType() == Types::ID::Event)
		return data.e;

	return HiseEvent();
}

size_t VariableStorage::getSizeInBytes() const noexcept
{
	return Types::Helpers::getSizeForType(getType());
}

int VariableStorage::getPointerSize() const
{
	jassert(getType() == Types::ID::Pointer);

	return data.p.size;
}

snex::VariableStorage& VariableStorage::operator=(const block& s)
{
	data.b.unused = Types::ID::Block;
	data.b.data = s.data;
	data.b.size_ = s.size();
	return *this;
}

snex::VariableStorage& VariableStorage::operator=(FloatType s)
{
	data.f.value = s;
	data.f.type = Types::ID::Float;
	
	return *this;
}

snex::VariableStorage& VariableStorage::operator=(int s)
{
	data.i.value = (int64)s;
	data.i.type = Types::ID::Integer;

	return *this;
}

snex::VariableStorage& VariableStorage::operator=(double s)
{
	data.d.value = s;
	data.d.type = Types::ID::Double;

	return *this;
}


VariableStorage::operator block() const
{
	jassert(getTypeValue() == Types::ID::Block);
	return data.b;
}

}
