namespace hise { using namespace juce;

VarTypeChecker::VarTypes VarTypeChecker::getType(const var& value)
{
    if(value.isInt() || value.isInt64() || value.isBool())
        return VarTypes::Integer;
    if(value.isDouble())
        return VarTypes::Double;
    if(value.isString())
        return VarTypes::String;
    if(value.isBuffer())
        return VarTypes::Buffer;
    if(value.isArray())
        return VarTypes::Array;
	if(HiseJavascriptEngine::isJavascriptFunction(value))
        return VarTypes::Function;
    if(value.getDynamicObject() != nullptr)
        return VarTypes::JSON;
    if(value.isObject())
        return VarTypes::ScriptObject;
    
    return VarTypes::Undefined;
}

VarRegister::VarRegister() :
empty(var())
{
	for (int i = 0; i < NUM_VAR_REGISTERS; i++)
	{
		registerStack[i] = var();
		registerStackIds[i] = Identifier();
        
#if ENABLE_SCRIPTING_SAFE_CHECKS
        registerTypes[i] = VarTypeChecker::Undefined;
#endif
	}
}

VarRegister::VarRegister(VarRegister &other) :
empty(var())
{
	for (int i = 0; i < NUM_VAR_REGISTERS; i++)
	{
		registerStack[i] = other.registerStack[i];
		registerStackIds[i] = other.registerStackIds[i];
	}
}

VarRegister::~VarRegister()
{
	for (int i = 0; i < NUM_VAR_REGISTERS; i++)
	{
		registerStack[i] = var();
		registerStackIds[i] = Identifier();
	}
}

void VarRegister::addRegister(const Identifier &id, var newValue, VarTypeChecker::VarTypes expectedType)
{
	for (int i = 0; i < NUM_VAR_REGISTERS; i++)
	{
		if (registerStackIds[i] == id)
		{
			registerStack[i] = newValue;
            
#if ENABLE_SCRIPTING_SAFE_CHECKS
            if(!registerTypes[i])
                registerTypes[i] = expectedType;
            else
            {
                auto ok = VarTypeChecker::checkType(newValue, registerTypes[i], true);
                
                if(ok.failed())
                    throw ok.getErrorMessage();
            }
                
#endif
            
			break;
		}

		if (registerStackIds[i].isNull())
		{
            registerStackIds[i] = id;
            
#if ENABLE_SCRIPTING_SAFE_CHECKS
            registerTypes[i] = expectedType;
#endif
            
			registerStack[i] = newValue;
			break;
		}
	}
}

void VarRegister::setRegister(int registerIndex, var newValue)
{
	if (registerIndex < NUM_VAR_REGISTERS)
	{
#if ENABLE_SCRIPTING_SAFE_CHECKS
        if(registerTypes[registerIndex])
        {
            auto ok = VarTypeChecker::checkType(newValue, registerTypes[registerIndex], true);
            
            if(ok.failed())
                throw ok.getErrorMessage();
        }
#endif
        
		registerStack[registerIndex] = newValue;
	}
}

const var & VarRegister::getFromRegister(int registerIndex) const
{
	if (registerIndex < NUM_VAR_REGISTERS)
	{
		return *(registerStack + registerIndex);
	}

	return empty;
}

int VarRegister::getRegisterIndex(const Identifier &id) const
{
	for (int i = 0; i < NUM_VAR_REGISTERS; i++)
	{
		if (registerStackIds[i] == id) return i;
	}

	return -1;
}

int VarRegister::getNumUsedRegisters() const
{
	for (int i = 0; i < NUM_VAR_REGISTERS; i++)
	{
		if (registerStackIds[i].isNull()) return i;
	}

	return NUM_VAR_REGISTERS;
}

Identifier VarRegister::getRegisterId(int index) const
{
	if (index < NUM_VAR_REGISTERS)
		return registerStackIds[index];

    return {};
}

const var * VarRegister::getVarPointer(int index) const
{

	if (index < NUM_VAR_REGISTERS)
		return registerStack + index;

	return nullptr;
}

var * VarRegister::getVarPointer(int index)
{

	if (index < NUM_VAR_REGISTERS)
		return registerStack + index;

	return nullptr;
}

ReadWriteLock& VarRegister::getLock(int index)
{
	return registerLocks[index];
}

ApiClass::ApiClass(int numConstants_) :
numConstants(numConstants_)
{
	for (int i = 0; i < NUM_API_FUNCTION_SLOTS; i++)
	{
		functions0[i] = nullptr;
		functions1[i] = nullptr;
		functions2[i] = nullptr;
		functions3[i] = nullptr;
		functions4[i] = nullptr;
		functions5[i] = nullptr;
	}

	if (numConstants > 8)
	{
		constantBigStorage = Array<Constant>();
		constantBigStorage.ensureStorageAllocated(numConstants);
		
		for (int i = 0; i < numConstants; i++)
		{
			constantBigStorage.add(Constant());
		}

		constantsToUse = constantBigStorage.getRawDataPointer();
	}
	else
	{
		for (int i = 0; i < 8; i++)
		{
			constants[i] = Constant();
		}

		constantsToUse = constants;
	}

	
}

ApiClass::~ApiClass()
{
	for (int i = 0; i < NUM_API_FUNCTION_SLOTS; i++)
	{
		functions0[i] = nullptr;
		functions1[i] = nullptr;
		functions2[i] = nullptr;
		functions3[i] = nullptr;
		functions4[i] = nullptr;
		functions5[i] = nullptr;
	}

	
	for (int i = 0; i < numConstants; i++)
	{
		constantsToUse[i] = Constant();
	}

	constantsToUse = nullptr;
	constantBigStorage.clear();
}

void ApiClass::addConstant(String constantName, var value)
{
	for (int i = 0; i < numConstants; i++)
	{
		if (constantsToUse[i].id.isNull())
		{
			constantsToUse[i].id = Identifier(constantName);
			constantsToUse[i].value = value;
			return;
		}
	}
}

const var ApiClass::getConstantValue(int index) const
{
	if (index >= 0 && index < numConstants)
		return constantsToUse[index].value;

	jassertfalse;
	return var();
}

int ApiClass::getConstantIndex(const Identifier &id) const
{
	for (int i = 0; i < numConstants; i++)
	{
		if (constantsToUse[i].id == id) return i;
	}

	return -1;
}

Identifier ApiClass::getConstantName(int index) const
{
	if (index < numConstants)
		return constantsToUse[index].id;

	jassertfalse;
    return {};
}

void ApiClass::addFunction(const Identifier &id, call0 newFunction)
{
	for (int i = 0; i < NUM_API_FUNCTION_SLOTS; i++)
	{
		if (functions0[i] == nullptr)
		{
			functions0[i] = newFunction;
			id0[i] = id;
			return;
		}
	}

	jassertfalse;
}

void ApiClass::addFunction1(const Identifier &id, call1 newFunction)
{
	for (int i = 0; i < NUM_API_FUNCTION_SLOTS; i++)
	{
		if (functions1[i] == nullptr)
		{
			functions1[i] = newFunction;
			id1[i] = id;
			return;
		}
	}

	jassertfalse;
}

void ApiClass::addFunction2(const Identifier &id, call2 newFunction)
{
	for (int i = 0; i < NUM_API_FUNCTION_SLOTS; i++)
	{
		if (functions2[i] == nullptr)
		{
			functions2[i] = newFunction;
			id2[i] = id;
			return;
		}
	}

	jassertfalse;
}

void ApiClass::addFunction3(const Identifier &id, call3 newFunction)
{
	for (int i = 0; i < NUM_API_FUNCTION_SLOTS; i++)
	{
		if (functions3[i] == nullptr)
		{
			functions3[i] = newFunction;
			id3[i] = id;
			return;
		}
	}

	jassertfalse;
}

void ApiClass::addFunction4(const Identifier &id, call4 newFunction)
{
	for (int i = 0; i < NUM_API_FUNCTION_SLOTS; i++)
	{
		if (functions4[i] == nullptr)
		{
			functions4[i] = newFunction;
			id4[i] = id;
			return;
		}
	}

	jassertfalse;
}

void ApiClass::addFunction5(const Identifier &id, call5 newFunction)
{
	for (int i = 0; i < NUM_API_FUNCTION_SLOTS; i++)
	{
		if (functions5[i] == nullptr)
		{
			functions5[i] = newFunction;
			id5[i] = id;
			return;
		}
	}

	jassertfalse;
}

void ApiClass::getIndexAndNumArgsForFunction(const Identifier &id, int &index, int &numArgs) const
{
	for (int i = 0; i < NUM_API_FUNCTION_SLOTS; i++)
	{
		if (id0[i] == id)
		{
			index = i;
			numArgs = 0;
			return;
		}
		else if (id1[i] == id)
		{
			index = i;
			numArgs = 1;
			return;
		}
		else if (id2[i] == id)
		{
			index = i;
			numArgs = 2;
			return;
		}
		else if (id3[i] == id)
		{
			index = i;
			numArgs = 3;
			return;
		}
		else if (id4[i] == id)
		{
			index = i;
			numArgs = 4;
			return;
		}
		else if (id5[i] == id)
		{
			index = i;
			numArgs = 5;
			return;
		}
	}
	index = -1;
	numArgs = -1;
}

var ApiClass::callFunction(int index, var *args, int numArgs)
{
	if (index > NUM_API_FUNCTION_SLOTS)
	{
		return var();
	}

	switch (numArgs)
	{
	case 0: { auto f = functions0[index]; return f(this); }
	case 1: { auto f = functions1[index]; return f(this, args[0]); }
	case 2: { auto f = functions2[index]; return f(this, args[0], args[1]); }
	case 3: { auto f = functions3[index]; return f(this, args[0], args[1], args[2]); }
	case 4: { auto f = functions4[index]; return f(this, args[0], args[1], args[2], args[3]); }
	case 5: { auto f = functions5[index]; return f(this, args[0], args[1], args[2], args[3], args[4]); }
	}

	return var();
}

void ApiClass::getAllFunctionNames(Array<Identifier> &ids) const
{
	ids.ensureStorageAllocated(NUM_API_FUNCTION_SLOTS * 5);

	for (int i = 0; i < NUM_API_FUNCTION_SLOTS; i++)
	{
		if (!id0[i].isNull()) ids.add(id0[i]);
		if (!id1[i].isNull()) ids.add(id1[i]);
		if (!id2[i].isNull()) ids.add(id2[i]);
		if (!id3[i].isNull()) ids.add(id3[i]);
		if (!id4[i].isNull()) ids.add(id4[i]);
		if (!id5[i].isNull()) ids.add(id5[i]);
	}

	IdentifierComparator idComp;

	ids.sort(idComp);
}

void ApiClass::getAllConstants(Array<Identifier> &ids) const
{
	for (int i = 0; i < numConstants; i++)
	{
		if (!constantsToUse[i].id.isNull()) ids.add(constantsToUse[i].id);
	}
}

ApiClass::Constant::Constant(const Identifier &id_, var value_) :
id(id_),
value(value_)
{

}

ApiClass::Constant::Constant() :
id(Identifier()),
value(var())
{

}

ApiClass::Constant& ApiClass::Constant::operator=(const Constant& other)
{
	id = other.id;
	value = other.value;
	return *this;
}

} // namespace hise
