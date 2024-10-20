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
	for(auto& flist: functions)
	{
		for(auto& f: flist)
			f = nullptr;
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
	for(auto& flist: functions)
	{
		for(auto& f: flist)
			f = nullptr;
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
	addFunctionT<0>(id, reinterpret_cast<void*>(newFunction));
}

void ApiClass::addFunction1(const Identifier &id, call1 newFunction)
{
	addFunctionT<1>(id, reinterpret_cast<void*>(newFunction));
}

void ApiClass::addFunction2(const Identifier &id, call2 newFunction)
{
	addFunctionT<2>(id, reinterpret_cast<void*>(newFunction));
}

void ApiClass::addFunction3(const Identifier &id, call3 newFunction)
{
	addFunctionT<3>(id, reinterpret_cast<void*>(newFunction));
}

void ApiClass::addFunction4(const Identifier &id, call4 newFunction)
{
	addFunctionT<4>(id, reinterpret_cast<void*>(newFunction));
}

void ApiClass::addFunction5(const Identifier &id, call5 newFunction)
{
	addFunctionT<5>(id, reinterpret_cast<void*>(newFunction));
}

bool ApiClass::getIndexAndNumArgsForFunction(const Identifier &id, int &index, int &numArgs) const
{
	for (int i = 0; i < NUM_API_FUNCTION_SLOTS; i++)
	{
		int thisIndex = 0;

		for(auto& idList: ids)
		{
			if(idList[i] == id)
			{
				index = i;
				numArgs = thisIndex;
				return true;
			}

			thisIndex++;
		}
	}

	index = -1;
	numArgs = -1;
	return false;
}

var ApiClass::callFunction(int index, var *args, int numArgs)
{
	if (index > NUM_API_FUNCTION_SLOTS)
	{
		return var();
	}

	switch (numArgs)
	{
	case 0: { auto f = reinterpret_cast<call0>(functions[numArgs][index]); return f(this); }
	case 1: { auto f = reinterpret_cast<call1>(functions[numArgs][index]); return f(this, args[0]); }
	case 2: { auto f = reinterpret_cast<call2>(functions[numArgs][index]); return f(this, args[0], args[1]); }
	case 3: { auto f = reinterpret_cast<call3>(functions[numArgs][index]); return f(this, args[0], args[1], args[2]); }
	case 4: { auto f = reinterpret_cast<call4>(functions[numArgs][index]); return f(this, args[0], args[1], args[2], args[3]); }
	case 5: { auto f = reinterpret_cast<call5>(functions[numArgs][index]); return f(this, args[0], args[1], args[2], args[3], args[4]); }
	}

	return var();
}

void ApiClass::getAllFunctionNames(Array<Identifier> &listToFill) const
{
	listToFill.ensureStorageAllocated(NUM_API_FUNCTION_SLOTS * NumMaxArguments);

	for (int i = 0; i < NUM_API_FUNCTION_SLOTS; i++)
	{
		for(auto& idList: ids)
		{
			if(idList[i].isNull())
				listToFill.add(idList[i]);
		}
	}

	IdentifierComparator idComp;

	listToFill.sort(idComp);
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
