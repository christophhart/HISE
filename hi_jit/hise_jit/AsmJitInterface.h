/*
  ==============================================================================

    AsmJitInterface.h
    Created: 4 Apr 2017 8:32:21pm
    Author:  Christoph

  ==============================================================================
*/

#ifndef ASMJITINTERFACE_H_INCLUDED
#define ASMJITINTERFACE_H_INCLUDED



struct AsmJitHelpers
{
	typedef int(*ErrorFunction)(int, int);

	struct BaseNode
	{
		BaseNode(TypeInfo t_, const Identifier& id_);

		virtual ~BaseNode() {};


		BaseNode* clone();

		virtual bool isMemoryLocation() const = 0;

		virtual X86Gp getAsGenericRegister() const = 0;
		virtual X86Mem getAsMemoryLocation() const = 0;
		virtual X86Xmm getAsFloatingPointRegister() const = 0;

		bool isChangedGlobal() const;
		void setIsChangedGlobal();;
		bool isAnonymousNode();
		TypeInfo getType() const;;
		void setConst();
		bool isConst() const;
		Identifier getId() const;
		void setId(const Identifier& newId);

		bool canBeReused() const
		{
			return !isMemoryLocation() && id.isNull(); // TODO: add smarter ways to detect this...
		}

		bool isImmediateValue() const
		{
			return isImmediate;
		}

		template <typename T> void setIsImmediate(T value)
		{
			isImmediate = true;

			immediateData = *reinterpret_cast<double*>(&value);
		}

		template <typename T> T getImmediateValue()
		{
			if (isImmediate)
			{
				return *reinterpret_cast<T*>(&immediateData);
			}
			else
			{
				jassertfalse;
				return T();
			}
		}

		X86Gp copyToGenericRegisterIfNecessary(X86Compiler& cc);

	protected:

		TypeInfo type;
		Identifier id;

	private:

		bool isImmediate = false;

		double immediateData = 0.0;
		
		bool isConstV = false;
		bool changedGlobal = false;

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BaseNode)
	};

	template <typename T> struct TypedNode : public BaseNode
	{
		TypedNode(const TypedNode<T>& other);

		TypedNode(const X86Reg& reg_, const Identifier& id_ = Identifier());;

		TypedNode(const X86Mem& mem_, const Identifier& id_ = Identifier());;


		bool isMemoryLocation() const override;

		X86Gp getAsGenericRegister() const override;;
		X86Mem getAsMemoryLocation() const override;
		X86Xmm getAsFloatingPointRegister() const override;

		X86Reg getRegister() const { return reg; }

		void setRegister(const X86Reg& newReg) { reg = newReg; }
		void setMemoryLocation(const X86Mem& newMemory) { memory = newMemory; }


	private:

		

		X86Reg reg;
		X86Mem memory;

		bool isReg;
	};

	template <typename T> static TypedNode<T>* as(BaseNode* src)
	{
		return dynamic_cast<TypedNode<T>*>(src);
	}

#define ASSERT_ASM_OK jassert(error == 0)

	static BaseNode* Parameter(X86Compiler& cc, int parameterId, TypeInfo type)
	{
		asmjit::Error error;

		

		if (HiseJITTypeHelpers::matchesType<float>(type))
		{
			auto p = cc.newXmmSs("Parameter Register");

			error = cc.setArg(parameterId, p);

			ASSERT_ASM_OK;

			return new TypedNode<float>(p);
		}
		else if (HiseJITTypeHelpers::matchesType<double>(type))
		{
			auto p = cc.newXmmSd();
			error = cc.setArg(parameterId, p);
			ASSERT_ASM_OK;

			return new TypedNode<double>(p);
		}
		else
		{
			auto p = cc.newGpd();
			error = cc.setArg(parameterId, p);

			ASSERT_ASM_OK;

			return new TypedNode<int>(p);
		}

	}

	template <typename T> static constexpr bool isFloat()
	{
		return typeid(T) == typeid(float);
	}

	template <typename T> static constexpr bool isDouble()
	{
		return typeid(T) == typeid(double);
	}

	template <typename T> static constexpr bool isInt()
	{
		return typeid(T) == typeid(int);
	}

	template <typename T> static constexpr bool isBool()
	{
		return typeid(T) == typeid(BooleanType);
	}


	template <typename T> static TypedNode<T>* Negate(X86Compiler& cc, BaseNode* operand)
	{
		if (isFloat<T>() || isDouble<T>())
		{
			if (operand->isImmediateValue())
			{
				ScopedPointer<BaseNode> ownedOperand = operand; // take ownership

				T negatedValue = (T)-1.0 * operand->getImmediateValue<T>();

				return Immediate<T>(cc, negatedValue);
			}
			else
			{
				ScopedPointer<BaseNode> ownedOperand = operand; // take ownership
				ScopedPointer<BaseNode> minusOne = Immediate<T>(cc, (T)-1.0);

				return EmitBinaryOp<Mul, T>(cc, dynamic_cast<TypedNode<T>*>(operand), as<T>(minusOne));
			}
		}
		else
		{
			if (operand->isImmediateValue())
			{
				operand->setIsImmediate<T>(operand->getImmediateValue<T>() * (T)-1);
			}
			else
			{
				asmjit::Error error;
				error = cc.neg(operand->getAsGenericRegister());
				ASSERT_ASM_OK;
			}

			return as<T>(operand);
		}
	}

	template <typename T> static TypedNode<T>* Immediate(X86Compiler& cc, T value)
	{
		asmjit::Error error;

		if (isFloat<T>())
		{
			X86Mem i = cc.newFloatConst(kConstScopeLocal, static_cast<float>(value));
			TypedNode<T>* r = new TypedNode<T>(i);
			r->setIsImmediate(value);
			return r;
			
		}
		if (isDouble<T>())
		{
			X86Mem i = cc.newDoubleConst(kConstScopeLocal, static_cast<double>(value));
			TypedNode<T>* r = new TypedNode<T>(i);
			r->setIsImmediate(value);
			return r;
		}
		else if (isInt<T>())
		{
			X86Reg i;

			//error = cc.mov(i, static_cast<uint32_t>(value));
			//ASSERT_ASM_OK;

			TypedNode<T>* r = new TypedNode<T>(i);
			r->setIsImmediate(value);
			return r;
		}
		else if (isBool<T>())
		{
			auto i = cc.newGpb();

			error = cc.mov(i, static_cast<BooleanType>(value));
			ASSERT_ASM_OK;

			TypedNode<T>* r = new TypedNode<T>(i);
			r->setIsImmediate(value);
			return r;
		}
		else
		{
			jassertfalse;
			return nullptr;
		}
	}

	template <typename T> static BaseNode* GlobalReference(X86Compiler& cc, void* data)
	{
		asmjit::Error error;

		if (isInt<T>())
		{
			X86Gp i = cc.newGpd("Global Data Register");

#if JUCE_64BIT

			X86Gp address = cc.newGpq("Global Address Register");

			error = cc.mov(address, reinterpret_cast<uint64_t>(data));
			ASSERT_ASM_OK;
			error = cc.mov(i, x86::qword_ptr(address));
			ASSERT_ASM_OK;
#else

			cc.mov(i, x86::dword_ptr(reinterpret_cast<uint64_t>(data)));
#endif

			return new AsmJitHelpers::TypedNode<int>(i);
		}
		else if (isFloat<T>())
		{
			X86Xmm i = cc.newXmmSs("Global Data Register");

#if JUCE_64BIT
			X86Gp address = cc.newGpq("Global Address Register");

			error = cc.mov(address, reinterpret_cast<uint64_t>(data));
			ASSERT_ASM_OK;
			error = cc.movss(i, x86::qword_ptr(address));

			
			ASSERT_ASM_OK;
#else
			cc.movss(i, x86::dword_ptr(reinterpret_cast<uint64_t>(data)));
#endif

			return new AsmJitHelpers::TypedNode<float>(i);
		}
		else if (isDouble<T>())
		{
			X86Xmm i = cc.newXmmSd();

#if JUCE_64BIT
			X86Gp address = cc.newGpq();

			error = cc.mov(address, reinterpret_cast<uint64_t>(data));
			ASSERT_ASM_OK;
			error = cc.movsd(i, x86::qword_ptr(address));
			ASSERT_ASM_OK;
#else
			cc.movsd(i, x86::dword_ptr(reinterpret_cast<uint64_t>(data)));
#endif

			return new AsmJitHelpers::TypedNode<double>(i);
		}
		else if (isBool<T>())
		{
			X86Gp i = cc.newGpb();

#if JUCE_64BIT

			X86Gp address = cc.newGpq();

			error = cc.mov(address, reinterpret_cast<uint64_t>(data));
			ASSERT_ASM_OK;
			error = cc.mov(i, x86::byte_ptr(address));
			ASSERT_ASM_OK;
#else

			cc.mov(i, x86::byte_ptr(reinterpret_cast<uint64_t>(data)));
#endif

			return new AsmJitHelpers::TypedNode<BooleanType>(i);

		}

		return nullptr;
	}

	static TypedNode<PointerType>* getBufferData(X86Compiler& cc, GlobalBase* g)
	{
		asmjit::Error error;

		if (g->isConst)
		{
			AddressType ptr = reinterpret_cast<AddressType>(GlobalBase::getBuffer(g)->b->buffer.getWritePointer(0));

			

#if JUCE_64BIT
			auto reg = cc.newGpq();
#else
			auto reg = cc.newGpd();
#endif

			error = cc.mov(reg, ptr);
			ASSERT_ASM_OK;

			return new TypedNode<PointerType>(reg);

		}
		else
		{
			AddressType globalAdress = reinterpret_cast<AddressType>(g);

			auto function = GlobalBase::getBufferData;

#if JUCE_64BIT
			auto gReg = cc.newGpq();
			error = cc.mov(gReg, globalAdress);
			ASSERT_ASM_OK;
#else

			auto gReg = cc.newGpd();
			error = cc.mov(gReg, globalAdress);
			ASSERT_ASM_OK;

#endif

			X86Gp fn = cc.newIntPtr("fn");
			error = cc.mov(fn, imm_ptr(function));
			ASSERT_ASM_OK;

#if JUCE_64BIT
			auto sig = FuncSignature1<AddressType, AddressType>();
#else
			auto sig = FuncSignature1<AddressType, AddressType>();
#endif

			CCFuncCall* call = cc.call(fn, sig);

			error = cc.getLastError();
			ASSERT_ASM_OK;

#if JUCE_64BIT
			X86Reg r = cc.newGpq();
#else
			X86Reg r = cc.newGpd();
#endif

			error = cc.getLastError();
			ASSERT_ASM_OK;

			error = call->setArg(0, gReg);
			
			error = call->setRet(0, r);
			
			return new TypedNode<PointerType>(r);
		}
	}

	static TypedNode<int>* getBufferDataSize(X86Compiler& cc, GlobalBase* g)
	{
		asmjit::Error error;

		if (g->isConst)
		{
			int size = GlobalBase::getBuffer(g)->b->size;

			return Immediate<int>(cc, size);
		}
		else
		{
			

			auto function = GlobalBase::getBufferDataSize;



#if JUCE_64BIT

			PointerType globalAdress = reinterpret_cast<PointerType>(g);
			auto gReg = cc.newGpq();
			error = cc.mov(gReg, globalAdress);
			ASSERT_ASM_OK;
#else

			uint32 globalAdress = reinterpret_cast<AddressType>(g);
			X86Gp gReg = cc.newIntPtr();

			error = cc.mov(gReg, imm_ptr(g));
			ASSERT_ASM_OK;

#endif

			X86Gp fn = cc.newIntPtr("fn");
			error = cc.mov(fn, imm_ptr(function));
			ASSERT_ASM_OK;

			auto sig = FuncSignature1<int, PointerType>();
			CCFuncCall* call = cc.call(fn, sig);

			X86Reg r = getRegisterForType<int>(cc);

			error = call->setArg(0, gReg);
			

			error = call->setRet(0, r);
			

			return new TypedNode<int>(r);
		}
	}

	static TypedNode<float>* BufferAccess(X86Compiler& cc, TypedNode<PointerType>* dataPointer, TypedNode<int>* offset, TypedNode<int>* bufferSize = nullptr, ErrorFunction errorFunction = nullptr)
	{
		asmjit::Error error = 0;

		X86Xmm i = cc.newXmmSs();

		X86Mem ptr;

		X86Gp address = dataPointer->getAsGenericRegister();

#if JUCE_64BIT
		if (offset->isImmediateValue())
			ptr = x86::qword_ptr(address, offset->getImmediateValue<int>() * 4);

		else
			ptr = x86::qword_ptr(address, offset->getAsGenericRegister(), 2);
#else
		if (offset->isImmediateValue())
			ptr = x86::dword_ptr(address, offset->getImmediateValue<int>() * 4);

		else
			ptr = x86::dword_ptr(address, offset->getAsGenericRegister(), 2);
#endif


		const bool useSafeFunction = bufferSize != nullptr;

		if (useSafeFunction)
		{
			auto overflow = cc.newLabel();
			auto ok = cc.newLabel();

			if (offset->isImmediateValue())
			{
				error = cc.cmp(bufferSize->getAsGenericRegister(), offset->getImmediateValue<int>());
				error = cc.jl(overflow);
			}
			else
			{
				error = cc.cmp(offset->getAsGenericRegister(), bufferSize->getAsGenericRegister());
				error = cc.jge(overflow);
			}

			error = cc.movss(i, ptr);

			cc.jmp(ok);
			cc.bind(overflow);

			ScopedPointer<TypedNode<float>> zero = Immediate(cc, 0.0f);


			cc.movss(i, zero->getAsMemoryLocation());

			


			ScopedPointer<TypedNode<int>> call = Call2<int, int, int>(cc, errorFunction, offset, bufferSize);

			cc.bind(ok);
		}
		else
		{
			error = cc.movss(i, ptr);
		}

		return new AsmJitHelpers::TypedNode<float>(i);
	}

	static void BufferAssignment(X86Compiler& cc, TypedNode<uint64_t>* dataPointer, TypedNode<int>* offset, TypedNode<float>* value, TypedNode<int>* bufferSize = nullptr, ErrorFunction errorFunction = nullptr)
	{

		asmjit::Error error;

		X86Gp address = dataPointer->getAsGenericRegister();

		const bool useSafeFunction = bufferSize != nullptr;

		X86Mem ptr;

#if JUCE_64BIT
		if (offset->isImmediateValue())
			ptr = x86::qword_ptr(address, offset->getImmediateValue<int>() * 4);
		
		else
			ptr = x86::qword_ptr(address, offset->getAsGenericRegister(), 2);
#else
		if (offset->isImmediateValue())
			ptr = x86::dword_ptr(address, offset->getImmediateValue<int>() * 4);
		else
			ptr = x86::dword_ptr(address, offset->getAsGenericRegister(), 2);
#endif
		
		if (useSafeFunction)
		{
			auto overflow = cc.newLabel();
			auto ok = cc.newLabel();

			if (offset->isImmediateValue())
			{
				error = cc.cmp(bufferSize->getAsGenericRegister(), offset->getImmediateValue<int>());
				error = cc.jl(overflow);
			}
			else
			{
				error = cc.cmp(offset->getAsGenericRegister(), bufferSize->getAsGenericRegister());
				error = cc.jge(overflow);
			}

			
			ASSERT_ASM_OK;

			
			ASSERT_ASM_OK;

			ScopedPointer<TypedNode<float>> r = createRegisterIfNecessary<float>(cc, value);
			error = cc.movss(ptr, r->getAsFloatingPointRegister());

			cc.jmp(ok);
			cc.bind(overflow);

			ScopedPointer<TypedNode<int>> call =  Call2<int, int, int>(cc, errorFunction, offset, bufferSize);

			cc.bind(ok);
		}
		else
		{
			ScopedPointer<TypedNode<float>> r = createRegisterIfNecessary<float>(cc, value);
			error = cc.movss(ptr, r->getAsFloatingPointRegister());
		}

	}

	template <typename T> static void GlobalAssignment(X86Compiler& cc, BaseNode* globalRegister, BaseNode* value)
	{
		if (value == nullptr)
		{
			jassertfalse;
			return;
		}

		globalRegister->setIsChangedGlobal();

		asmjit::Error error;

		if (isInt<T>() || isBool<T>())
		{
			error = BinaryOpInstructions::Int::store(cc, globalRegister->getAsGenericRegister(), value);
			ASSERT_ASM_OK;
		}
		else if (isFloat<T>())
		{
			error = BinaryOpInstructions::Float::store(cc, globalRegister->getAsFloatingPointRegister(), value);
			ASSERT_ASM_OK;
		}
		else if (isDouble<T>())
		{
			error = BinaryOpInstructions::Double::store(cc, globalRegister->getAsFloatingPointRegister(), value);
			ASSERT_ASM_OK;
		}
	}

	template <typename T> static void StoreGlobal(X86Compiler& cc, void* data, BaseNode* globalRegister)
	{
		asmjit::Error error;

#if JUCE_64BIT
		X86Gp address = cc.newGpq();
		error = cc.mov(address, reinterpret_cast<uint64_t>(data));
		ASSERT_ASM_OK;

		if (isInt<T>())
		{
			error = cc.mov(x86::qword_ptr(address), globalRegister->getAsGenericRegister());
			ASSERT_ASM_OK;
		}
			
		else if (isFloat<T>())
		{
			error = cc.movss(x86::qword_ptr(address), globalRegister->getAsFloatingPointRegister());
			ASSERT_ASM_OK;
		}

		else if (isDouble<T>())
		{
			error = cc.movsd(x86::qword_ptr(address), globalRegister->getAsFloatingPointRegister());
			ASSERT_ASM_OK;
		}


		else if (isBool<T>())
		{
			error = cc.mov(x86::byte_ptr(address), globalRegister->getAsGenericRegister());
			ASSERT_ASM_OK;
		}
			

#else
		if (isInt<T>())
			cc.mov(x86::dword_ptr(reinterpret_cast<uint64_t>(data)), globalRegister->getAsGenericRegister());

		else if (isBool<T>())
			cc.mov(x86::byte_ptr(reinterpret_cast<uint64_t>(data)), globalRegister->getAsGenericRegister());

		else if (isFloat<T>())
			cc.movss(x86::dword_ptr(reinterpret_cast<uint64_t>(data)), globalRegister->getAsFloatingPointRegister());

		else if (isDouble<T>())
			cc.movsd(x86::dword_ptr(reinterpret_cast<uint64_t>(data)), globalRegister->getAsFloatingPointRegister());

#endif


	}



	enum Opcodes
	{
		Add,
		Mul,
		Sub,
		Div,
		Mod,
		GT,
		LT,
		LTE,
		GTE,
		EQ,
		NEQ,
		And,
		Or,
		numOpcodes
	};

	typedef asmjit::Error(asmjit::X86Compiler::*RegisterOperation)(X86Xmm, X86Xmm);
	typedef asmjit::Error(asmjit::X86Compiler::*MemoryOperation)(X86Xmm, X86Mem);

	struct BinaryOpInstructions
	{
		struct Bool
		{
			static asmjit::Error store(X86Compiler& cc, X86Reg targetRegister, BaseNode* operand)
			{
				if (operand->isImmediateValue())
					return cc.mov(targetRegister.as<X86Gpb>(), operand->getImmediateValue<BooleanType>());
				else
					return cc.mov(targetRegister.as<X86Gpb>(), operand->getAsGenericRegister());
			}

			static asmjit::Error and_(X86Compiler& cc, X86Reg targetRegister, BaseNode* operand)
			{
				if (operand->isImmediateValue())
					return cc.and_(targetRegister.as<X86Gpb>(), operand->getImmediateValue<BooleanType>());
				else
					return cc.and_(targetRegister.as<X86Gpb>(), operand->getAsGenericRegister());
			}

			static asmjit::Error or_(X86Compiler& cc, X86Reg targetRegister, BaseNode* operand)
			{
				if (operand->isImmediateValue())
					return cc.or_(targetRegister.as<X86Gpb>(), operand->getImmediateValue<BooleanType>());
				else
					return cc.or_(targetRegister.as<X86Gpb>(), operand->getAsGenericRegister());
			}
		};

		struct Int
		{
#if JUCE_64BIT
			typedef X86Gpq IntRegisterType;
#else
			typedef X86Gpd IntRegisterType;
#endif

			static asmjit::Error store(X86Compiler& cc, X86Reg targetRegister, BaseNode* operand)
			{
				if (operand->isImmediateValue())
					return cc.mov(targetRegister.as<IntRegisterType>(), operand->getImmediateValue<int>());
				else
					return cc.mov(targetRegister.as<IntRegisterType>(), operand->getAsGenericRegister());
			}

			static asmjit::Error add(X86Compiler& cc, X86Reg targetRegister, BaseNode* operand)
			{
				if (operand->isImmediateValue())
					return cc.add(targetRegister.as<IntRegisterType>(), operand->getImmediateValue<int>());
				else
					return cc.add(targetRegister.as<IntRegisterType>(), operand->getAsGenericRegister());
			}

			static asmjit::Error sub(X86Compiler& cc, X86Reg targetRegister, BaseNode* operand)
			{
				if (operand->isImmediateValue())
					return cc.sub(targetRegister.as<IntRegisterType>(), operand->getImmediateValue<int>());
				else
					return cc.sub(targetRegister.as<IntRegisterType>(), operand->getAsGenericRegister());
			}

			static asmjit::Error mul(X86Compiler& cc, X86Reg targetRegister, BaseNode* operand)
			{
				if (operand->isImmediateValue())
					return cc.imul(targetRegister.as<IntRegisterType>(), operand->getImmediateValue<int>());
				else
					return cc.imul(targetRegister.as<IntRegisterType>(), operand->getAsGenericRegister());
			}
		};

		struct Float
		{
			static asmjit::Error store(X86Compiler& cc, X86Reg targetRegister, BaseNode* operand)
			{
				if (operand->isMemoryLocation()) return cc.movss(targetRegister.as<X86Xmm>(), operand->getAsMemoryLocation());
				else						     return cc.movss(targetRegister.as<X86Xmm>(), operand->getAsFloatingPointRegister());
			}

			static asmjit::Error add(X86Compiler& cc, X86Xmm targetRegister, BaseNode* operand)
			{
				if (operand->isMemoryLocation()) return cc.addss(targetRegister, operand->getAsMemoryLocation());
				else						     return cc.addss(targetRegister, operand->getAsFloatingPointRegister());
			}

			static asmjit::Error sub(X86Compiler& cc, X86Xmm targetRegister, BaseNode* operand)
			{
				if (operand->isMemoryLocation()) return cc.subss(targetRegister, operand->getAsMemoryLocation());
				else						     return cc.subss(targetRegister, operand->getAsFloatingPointRegister());
			}

			static asmjit::Error mul(X86Compiler& cc, X86Xmm targetRegister, BaseNode* operand)
			{
				if (operand->isMemoryLocation()) return cc.mulss(targetRegister, operand->getAsMemoryLocation());
				else						     return cc.mulss(targetRegister, operand->getAsFloatingPointRegister());
			}

			static asmjit::Error div(X86Compiler& cc, X86Xmm targetRegister, BaseNode* operand)
			{
				if (operand->isMemoryLocation()) return cc.divss(targetRegister, operand->getAsMemoryLocation());
				else						     return cc.divss(targetRegister, operand->getAsFloatingPointRegister());
			}

		};

		struct Double
		{
			static asmjit::Error store(X86Compiler& cc, X86Reg targetRegister, BaseNode* operand)
			{
				if (operand->isMemoryLocation()) return cc.movsd(targetRegister.as<X86Xmm>(), operand->getAsMemoryLocation());
				else						     return cc.movsd(targetRegister.as<X86Xmm>(), operand->getAsFloatingPointRegister());
			}

			static asmjit::Error add(X86Compiler& cc, X86Reg targetRegister, BaseNode* operand)
			{
				if (operand->isMemoryLocation()) return cc.addsd(targetRegister.as<X86Xmm>(), operand->getAsMemoryLocation());
				else						     return cc.addsd(targetRegister.as<X86Xmm>(), operand->getAsFloatingPointRegister());
			}

			static asmjit::Error sub(X86Compiler& cc, X86Reg targetRegister, BaseNode* operand)
			{
				if (operand->isMemoryLocation()) return cc.subsd(targetRegister.as<X86Xmm>(), operand->getAsMemoryLocation());
				else						     return cc.subsd(targetRegister.as<X86Xmm>(), operand->getAsFloatingPointRegister());
			}

			static asmjit::Error mul(X86Compiler& cc, X86Reg targetRegister, BaseNode* operand)
			{
				if (operand->isMemoryLocation()) return cc.mulsd(targetRegister.as<X86Xmm>(), operand->getAsMemoryLocation());
				else						     return cc.mulsd(targetRegister.as<X86Xmm>(), operand->getAsFloatingPointRegister());
			}

			static asmjit::Error div(X86Compiler& cc, X86Reg targetRegister, BaseNode* operand)
			{
				if (operand->isMemoryLocation()) return cc.divsd(targetRegister.as<X86Xmm>(), operand->getAsMemoryLocation());
				else						     return cc.divsd(targetRegister.as<X86Xmm>(), operand->getAsFloatingPointRegister());
			}
		};

		
	};

	template <typename T> static TypedNode<T>* createRegisterIfNecessary(X86Compiler& cc, TypedNode<T>* node)
	{
		asmjit::Error error;

		if (node->isMemoryLocation())
		{
			if (isDouble<T>())
			{
				X86Xmm sd = cc.newXmmSd();

				error = BinaryOpInstructions::Double::store(cc, sd, node);

				ASSERT_ASM_OK;
				return new TypedNode<T>(sd);
			}
			else if (isFloat<T>())
			{
				X86Xmm ss = cc.newXmmSs("Temp Register for binary op");
				
				error = BinaryOpInstructions::Float::store(cc, ss, node);

				ASSERT_ASM_OK;
				return new TypedNode<T>(ss);
			}
			else
			{
				jassertfalse;
				return nullptr;
			}
		}
		else
		{
			if (isInt<T>() && node->isImmediateValue())
			{
				X86Gp newReg = cc.newInt32();
				BinaryOpInstructions::Int::store(cc, newReg, node);

				return new TypedNode<T>(newReg);
			}

			return as<T>(node->clone());
		}
	}

	template <typename T, Opcodes Op> static TypedNode<BooleanType>* Compare(X86Compiler& cc, TypedNode<T>* left, TypedNode<T>* right)
	{
		X86Gp i = cc.newGpb();

		asmjit::Error error;

		if (isInt<T>())
		{
			if (left->isImmediateValue())
			{
				auto leftConst = cc.newInt32Const(kConstScopeLocal, left->getImmediateValue<int>());

				if (right->isImmediateValue())
				{
					error = cc.cmp(leftConst, right->getImmediateValue<int>());
				}
				else
				{
					error = cc.cmp(leftConst, right->getAsGenericRegister());
				}
			}
			else
			{
				if (right->isImmediateValue())
				{
					error = cc.cmp(left->getAsGenericRegister(), right->getImmediateValue<int>());
				}
				else
				{
					error = cc.cmp(left->getAsGenericRegister(), right->getAsGenericRegister());
				}
			}

			ASSERT_ASM_OK;

			switch (Op)
			{
			case AsmJitHelpers::GT:  error = cc.setg(i); break;
			case AsmJitHelpers::LT:  error = cc.setl(i); break;
			case AsmJitHelpers::LTE: error = cc.setle(i); break;
			case AsmJitHelpers::GTE: error = cc.setge(i); break;
			case AsmJitHelpers::EQ:  error = cc.sete(i); break;
			case AsmJitHelpers::NEQ: error = cc.setne(i); break;
			}
			
			ASSERT_ASM_OK;

		}
		else if (isFloat<T>() || isDouble<T>())
		{
			ScopedPointer<TypedNode<T>> left2 = createRegisterIfNecessary(cc, left);

			if (!right->isMemoryLocation())
			{
				if (isFloat<T>()) 
					error = cc.ucomiss(left2->getAsFloatingPointRegister(), right->getAsFloatingPointRegister());
				else
					error = cc.ucomisd(left2->getAsFloatingPointRegister(), right->getAsFloatingPointRegister());

				ASSERT_ASM_OK;
			}
			else
			{
				if (isFloat<T>())
					error = cc.ucomiss(left2->getAsFloatingPointRegister(), right->getAsMemoryLocation());
				else
					error = cc.ucomisd(left2->getAsFloatingPointRegister(), right->getAsMemoryLocation());

				ASSERT_ASM_OK;
			}

			switch (Op)
			{
			case AsmJitHelpers::GT:  error = cc.seta(i); break;
			case AsmJitHelpers::LT:  error = cc.setb(i); break;
			case AsmJitHelpers::LTE: error = cc.setbe(i); break;
			case AsmJitHelpers::GTE: error = cc.setae(i); break;
			case AsmJitHelpers::EQ:  error = cc.sete(i); break;
			case AsmJitHelpers::NEQ: error = cc.setne(i); break;
			}

			ASSERT_ASM_OK;
		}

		return new TypedNode<BooleanType>(i);
	}

	template <int Op, typename T> static TypedNode<T>* EmitBinaryOp(X86Compiler& cc, TypedNode<T>* left, TypedNode<T>* right)
	{
		const bool opCanBeReordered = Op == Opcodes::Add ||
									  Op == Opcodes::Mul ||
									  Op == Opcodes::EQ ||
									  Op == Opcodes::NEQ;

		TypedNode<T>* destNode;
		TypedNode<T>* opNode;

		if (opCanBeReordered && right->canBeReused()) // the parser uses RTL so it's better to reuse the right register if possible
		{
			destNode = right;
			opNode = left;
		}
		else
		{
			destNode = left;
			opNode = right;
		}

		X86Reg dest;
		
		if (destNode->canBeReused())
		{
			ScopedPointer<TypedNode<T>> temp = createRegisterIfNecessary<T>(cc, destNode);

			dest = temp->getRegister();
		}
		else
		{
			dest = getRegisterForType<T>(cc);

			if (isInt<T>()) BinaryOpInstructions::Int::store(cc, dest, destNode);
			if (isFloat<T>()) BinaryOpInstructions::Float::store(cc, dest, destNode);
			if (isDouble<T>()) BinaryOpInstructions::Double::store(cc, dest, destNode);

		}

		return EmitBinaryOpInternal<Op, T>(cc, dest, opNode);
	}

	template <int Op, typename T> static TypedNode<T>* EmitBinaryOpInternal(X86Compiler& cc, X86Reg& left, TypedNode<T>* rightNode)
	{
		asmjit::Error error = 0;

		if (isFloat<T>())
		{
			

			ASSERT_ASM_OK;

			switch (Op)
			{
			case AsmJitHelpers::Add: error = BinaryOpInstructions::Float::add(cc, left.as<X86Xmm>(), rightNode); break;
			case AsmJitHelpers::Sub: error = BinaryOpInstructions::Float::sub(cc, left.as<X86Xmm>(), rightNode); break;
			case AsmJitHelpers::Mul: error = BinaryOpInstructions::Float::mul(cc, left.as<X86Xmm>(), rightNode); break;
			case AsmJitHelpers::Div: error = BinaryOpInstructions::Float::div(cc, left.as<X86Xmm>(), rightNode); break;
				break;
			}

			ASSERT_ASM_OK;

			return new TypedNode<T>(left);
		}
		else if (isDouble<T>())
		{
			

			ASSERT_ASM_OK;

			switch (Op)
			{
			case AsmJitHelpers::Add: error = BinaryOpInstructions::Double::add(cc, left.as<X86Xmm>(), rightNode); break;
			case AsmJitHelpers::Sub: error = BinaryOpInstructions::Double::sub(cc, left.as<X86Xmm>(), rightNode); break;
			case AsmJitHelpers::Mul: error = BinaryOpInstructions::Double::mul(cc, left.as<X86Xmm>(), rightNode); break;
			case AsmJitHelpers::Div: error = BinaryOpInstructions::Double::div(cc, left.as<X86Xmm>(), rightNode); break;
				break;
			}

			ASSERT_ASM_OK;

			return new TypedNode<T>(left);
		}

		if (isInt<T>())
		{
			switch (Op)
			{
			case AsmJitHelpers::Add: error = BinaryOpInstructions::Int::add(cc, left.as<X86Gpd>(), rightNode); break;
			case AsmJitHelpers::Sub: error = BinaryOpInstructions::Int::sub(cc, left.as<X86Gpd>(), rightNode); break;
			case AsmJitHelpers::Mul: error = BinaryOpInstructions::Int::mul(cc, left.as<X86Gpd>(), rightNode); break;
			case AsmJitHelpers::Div:
			{
				X86Gp dummy = cc.newInt32("dummy");

				error = cc.xor_(dummy, dummy);
				ASSERT_ASM_OK;
				error = cc.cdq(dummy, left.as<X86Gpd>());
				ASSERT_ASM_OK;

				X86Reg right = cc.newInt32("dummy");

				BinaryOpInstructions::Int::store(cc, right, rightNode);

				error = cc.idiv(dummy, left.as<X86Gpd>(), right.as<X86Gpd>());
				ASSERT_ASM_OK;

				break;
			}

			case AsmJitHelpers::Mod:
			{
				if (rightNode->isImmediateValue())
				{
					if (isPowerOfTwo(rightNode->getImmediateValue<int>()))
					{
						int maskValue = rightNode->getImmediateValue<int>() - 1;

						cc.and_(left.as<X86Gpd>(), maskValue);

						return new TypedNode<T>(left);
					}
					else
					{
						X86Gp dummy = cc.newInt32("dummy");

						X86Reg right = cc.newInt32("right");

						BinaryOpInstructions::Int::store(cc, right, rightNode);

						error = cc.xor_(dummy, dummy);
						ASSERT_ASM_OK;
						error = cc.idiv(dummy, left.as<X86Gpd>(), right.as<X86Gpd>());
						ASSERT_ASM_OK;

						return new TypedNode<T>(dummy);
					}

					
				}
				else
				{
					X86Gp dummy = cc.newInt32("dummy");

					error = cc.xor_(dummy, dummy);
					ASSERT_ASM_OK;
					error = cc.idiv(dummy, left.as<X86Gpd>(), rightNode->getAsGenericRegister());
					ASSERT_ASM_OK;

					return new TypedNode<T>(dummy);
				}
			}

			}

			ASSERT_ASM_OK;

			return new TypedNode<T>(left);
		}

		else if (isBool<T>())
		{
			//X86Gp result = cc.newGpb();

			//cc.mov(result, left->getAsGenericRegister());

			switch (Op)
			{
			case Opcodes::Or: error = BinaryOpInstructions::Bool::or_(cc, left, rightNode); break;
			case Opcodes::And: error = BinaryOpInstructions::Bool::and_(cc, left, rightNode); break;
			}

			return new AsmJitHelpers::TypedNode<T>(left);
		}

		else
		{
			jassertfalse;
			return nullptr;
		}
	};

	template <typename T> static X86Reg getRegisterForType(X86Compiler& cc)
	{
		if (isFloat<T>())		return cc.newXmmSs("New Register");
		else if (isDouble<T>()) return cc.newXmmSd();
		else if (isInt<T>())	return cc.newGpd();
		else if (isBool<T>())	return cc.newGpb();
		return X86Reg();
	}

	static void StoreInRegister(X86Compiler& cc, BaseNode* expression, X86Reg& result)
	{
		asmjit::Error error;

		if (HiseJITTypeHelpers::matchesType<int>(expression->getType()))
		{
			error = BinaryOpInstructions::Int::store(cc, result, expression);
			ASSERT_ASM_OK;
		}
		if (HiseJITTypeHelpers::matchesType<BooleanType>(expression->getType()))
		{
			error = cc.mov(result.as<X86Gpb>(), expression->getAsGenericRegister());
			ASSERT_ASM_OK;
		}
		else if (HiseJITTypeHelpers::matchesType<double>(expression->getType()))
		{
			if(expression->isMemoryLocation())
				error = cc.movsd(result.as<X86Xmm>(), expression->getAsMemoryLocation());
			else
				error = cc.movsd(result.as<X86Xmm>(), expression->getAsFloatingPointRegister());

			ASSERT_ASM_OK;
		}
		else if (HiseJITTypeHelpers::matchesType<float>(expression->getType()))
		{
			if (expression->isMemoryLocation())
				error = cc.movss(result.as<X86Xmm>(), expression->getAsMemoryLocation());
			else
				error = cc.movss(result.as<X86Xmm>(), expression->getAsFloatingPointRegister());

			ASSERT_ASM_OK;
		}
	}

	template <typename ReturnType> static TypedNode<ReturnType>* Call0(X86Compiler& cc, void* function)
	{
		auto sig = FuncSignature0<ReturnType>();

		CCFuncCall* call = cc.call(reinterpret_cast<uint64_t>(function), sig);

		asmjit::X86Reg r = getRegisterForType<ReturnType>(cc);

		call->setRet(0, r);

		return new TypedNode<ReturnType>(r);
	}

	template <typename ReturnType, typename Param1Type> static TypedNode<ReturnType>* Call1(X86Compiler& cc, void* function, BaseNode* p1)
	{


		X86Reg i;

		if (isFloat<Param1Type>() || isDouble<Param1Type>())
		{
			if (p1->isMemoryLocation())
			{
				i = getRegisterForType<Param1Type>(cc);

				if (isFloat<Param1Type>())
					cc.movss(i.as<X86Xmm>(), p1->getAsMemoryLocation());
				else
					cc.movsd(i.as<X86Xmm>(), p1->getAsMemoryLocation());
			}

			else
				i = p1->getAsFloatingPointRegister();
		}
		else if (isInt<Param1Type>())
		{
			i = p1->copyToGenericRegisterIfNecessary(cc);
		}

		X86Reg r = getRegisterForType<ReturnType>(cc);


		X86Gp fn = cc.newIntPtr("fn");
		cc.mov(fn, imm_ptr(function));

		auto sig = FuncSignature1<ReturnType, Param1Type>();
		CCFuncCall* call = cc.call(fn, sig);

		call->setArg(0, i);
		call->setRet(0, r);

		return new TypedNode<ReturnType>(r);
	}

	template <typename ReturnType, typename Param1Type, typename Param2Type> static TypedNode<ReturnType>* Call2(X86Compiler& cc, void* function, BaseNode* p1, BaseNode* p2)
	{
		auto sig = FuncSignature2<ReturnType, Param1Type, Param2Type>();

		X86Reg a;
		X86Reg b;

		if (isFloat<Param1Type>() || isDouble<Param1Type>())
		{
			if (p1->isMemoryLocation())
			{
				a = getRegisterForType<Param1Type>(cc);

				if (isFloat<Param1Type>())
					cc.movss(a.as<X86Xmm>(), p1->getAsMemoryLocation());
				else
					cc.movsd(a.as<X86Xmm>(), p1->getAsMemoryLocation());
			}
			else
				a = p1->getAsFloatingPointRegister();
		}
		else if (isInt<Param1Type>())
			a = p1->copyToGenericRegisterIfNecessary(cc);

		if (isFloat<Param2Type>() || isDouble<Param2Type>())
		{
			if (p2->isMemoryLocation())
			{
				b = getRegisterForType<Param2Type>(cc);

				if (isFloat<Param2Type>())
					cc.movss(b.as<X86Xmm>(), p2->getAsMemoryLocation());
				else
					cc.movsd(b.as<X86Xmm>(), p2->getAsMemoryLocation());
			}
			else
				b = p1->getAsFloatingPointRegister();
		}
		else if (isInt<Param2Type>())
			b = p2->copyToGenericRegisterIfNecessary(cc);


		asmjit::X86Reg r = getRegisterForType<ReturnType>(cc);

		X86Gp fn = cc.newIntPtr("fn");
		cc.mov(fn, imm_ptr(function));

		CCFuncCall* call = cc.call(fn, sig);

		call->setArg(0, a);
		call->setArg(1, b);
		call->setRet(0, r);

		return new TypedNode<ReturnType>(r);
	}

	template <typename ReturnType, typename Param1Type, typename Param2Type, typename Param3Type> static TypedNode<ReturnType>* Call3(X86Compiler& cc, void* function, BaseNode* p1, BaseNode* p2, BaseNode* p3)
	{
		auto sig = FuncSignature3<ReturnType, Param1Type, Param2Type, Param3Type>();

		X86Reg a;
		X86Reg b;
		X86Reg c;

		if (isFloat<Param1Type>() || isDouble<Param1Type>())
		{
			if (p1->isMemoryLocation())
			{
				a = getRegisterForType<Param1Type>(cc);

				if (isFloat<Param1Type>())
					cc.movss(a.as<X86Xmm>(), p1->getAsMemoryLocation());
				else
					cc.movsd(a.as<X86Xmm>(), p1->getAsMemoryLocation());
			}
			else
				a = p1->getAsFloatingPointRegister();
		}
		else if (isInt<Param1Type>())
			a = p1->copyToGenericRegisterIfNecessary(cc);

		if (isFloat<Param2Type>() || isDouble<Param2Type>())
		{
			if (p2->isMemoryLocation())
			{
				b = getRegisterForType<Param2Type>(cc);

				if (isFloat<Param2Type>())
					cc.movss(b.as<X86Xmm>(), p2->getAsMemoryLocation());
				else
					cc.movsd(b.as<X86Xmm>(), p2->getAsMemoryLocation());
			}
			else
				b = p2->getAsFloatingPointRegister();
		}
		else if (isInt<Param2Type>())
			b = p2->copyToGenericRegisterIfNecessary(cc);

		if (isFloat<Param3Type>() || isDouble<Param3Type>())
		{
			if (p3->isMemoryLocation())
			{
				c = getRegisterForType<Param3Type>(cc);

				if (isFloat<Param3Type>())
					cc.movss(c.as<X86Xmm>(), p3->getAsMemoryLocation());
				else
					cc.movsd(c.as<X86Xmm>(), p3->getAsMemoryLocation());
			}
			else
				c = p3->getAsFloatingPointRegister();
		}
		else if (isInt<Param3Type>())
			c = p3->copyToGenericRegisterIfNecessary(cc);

		asmjit::X86Reg r = getRegisterForType<ReturnType>(cc);

		X86Gp fn = cc.newIntPtr("fn");
		cc.mov(fn, imm_ptr(function));

		CCFuncCall* call = cc.call(fn, sig);

		call->setArg(0, a);
		call->setArg(1, b);
		call->setArg(2, c);
		call->setRet(0, r);

		return new TypedNode<ReturnType>(r);
	}

	template <typename SourceType, typename TargetType> static TypedNode<TargetType>* EmitCast(X86Compiler& cc, TypedNode<SourceType>* source)
	{
		if (typeid(SourceType) == typeid(TargetType))
		{
			return dynamic_cast<TypedNode<TargetType>*>(source);
		}

		if (isFloat<SourceType>())
		{
			if (isInt<TargetType>())
			{
				X86Gp reg = cc.newGpd();

				source->isMemoryLocation() ? cc.cvtss2si(reg, source->getAsMemoryLocation()) :
					cc.cvttss2si(reg, source->getAsFloatingPointRegister());

				return new TypedNode<TargetType>(reg);
			}
			else if (isDouble<TargetType>())
			{
				X86Xmm reg = cc.newXmmSd();

				source->isMemoryLocation() ? cc.cvtss2sd(reg, source->getAsMemoryLocation()) :
					cc.cvtss2sd(reg, source->getAsFloatingPointRegister());

				return new TypedNode<TargetType>(reg);
			}
			else
			{
				jassertfalse;
				return nullptr;
			}
		}
		else if (isInt<SourceType>())
		{
			if (isFloat<TargetType>())
			{
				X86Xmm reg = cc.newXmmSs();

				cc.cvtsi2ss(reg, source->getAsGenericRegister());

				return new TypedNode<TargetType>(reg);
			}
			else if (isDouble<TargetType>())
			{
				X86Xmm reg = cc.newXmmSd();

				cc.cvtsi2sd(reg, source->getAsGenericRegister());

				return new TypedNode<TargetType>(reg);
			}
			else
			{
				jassertfalse;
				return nullptr;
			}
		}
		if (isDouble<SourceType>())
		{
			if (isInt<TargetType>())
			{
				X86Gp reg = cc.newInt32();

				source->isMemoryLocation() ? cc.cvtsd2si(reg, source->getAsMemoryLocation()) :
					cc.cvttsd2si(reg, source->getAsFloatingPointRegister());

				return new TypedNode<TargetType>(reg);
			}
			else if (isFloat<TargetType>())
			{
				X86Xmm reg = cc.newXmmSs();

				source->isMemoryLocation() ? cc.cvtsd2ss(reg, source->getAsMemoryLocation()) :
					cc.cvtsd2ss(reg, source->getAsFloatingPointRegister());

				return new TypedNode<TargetType>(reg);
			}
			else
			{
				jassertfalse;
				return nullptr;
			}
		}
		else
		{
			jassertfalse;
			return nullptr;
		}
	}

	static void invertBool(X86Compiler& cc, const TypedNode<BooleanType>* op)
	{
		cc.xor_(op->getAsGenericRegister(), 1);
	}

	static void Inc(X86Compiler& cc, const TypedNode<int>* op)
	{
		cc.inc(op->getAsGenericRegister());
	};

	static void Dec(X86Compiler& cc, const TypedNode<int>* op)
	{
		cc.dec(op->getAsGenericRegister());
	};


	template <typename T> static void Return(X86Compiler&cc, TypedNode<T>* returnNode)
	{
		ScopedPointer<const TypedNode<T>> newNode = createRegisterIfNecessary(cc, returnNode);

		if (isFloat<T>() || isDouble<T>())
		{
			auto retCpy = getRegisterForType<T>(cc);

			if (isFloat<T>()) cc.movss(retCpy.as<X86Xmm>(), newNode->getAsFloatingPointRegister());
			if (isDouble<T>()) cc.movsd(retCpy.as<X86Xmm>(), newNode->getAsFloatingPointRegister());

			cc.ret(retCpy.as<X86Xmm>());
		}
			

		else if (isInt<T>())
			cc.ret(newNode->getAsGenericRegister());
	};

};

typedef ScopedPointer<AsmJitHelpers::BaseNode> ScopedBaseNodePointer;

#include "AsmJitInterface.cpp"


#endif  // ASMJITINTERFACE_H_INCLUDED
