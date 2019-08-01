namespace hnode {
namespace jit {
using namespace juce;
using namespace asmjit;


bool AsmJitHelpers::BaseNode::isChangedGlobal() const { return changedGlobal; }
void AsmJitHelpers::BaseNode::setIsChangedGlobal() { changedGlobal = true; }
bool AsmJitHelpers::BaseNode::isAnonymousNode() { return !id.isNull(); }
void AsmJitHelpers::BaseNode::setConst() { isConstV = true; }
bool AsmJitHelpers::BaseNode::isConst() const { return isConstV; }
Identifier AsmJitHelpers::BaseNode::getId() const { return id; }
void AsmJitHelpers::BaseNode::setId(const Identifier& newId) { id = newId; }

bool AsmJitHelpers::BaseNode::canBeReused() const
{
	return !isMemoryLocation() && id.isNull(); // TODO: add smarter ways to detect this...
}

template <typename T> AsmJitHelpers::TypedNode<T>::TypedNode(const TypedNode<T>& other) :
	BaseNode(other.id),
	reg(other.reg),
	isReg(other.isReg),
	memory(other.memory)
{}

template <typename T> AsmJitHelpers::TypedNode<T>::TypedNode(const X86Reg& reg_, const Identifier& id_ /*= Identifier()*/) :
	BaseNode(id_),
	reg(reg_),
	isReg(true)
{}

template <typename T> AsmJitHelpers::TypedNode<T>::TypedNode(const X86Mem& mem_, const Identifier& id_ /*= Identifier()*/) :
	BaseNode(id_),
	memory(mem_),
	isReg(false)
{}




template <typename T> bool AsmJitHelpers::TypedNode<T>::isMemoryLocation() const { return !isReg; }
template <typename T> X86Gp AsmJitHelpers::TypedNode<T>::getAsGenericRegister() const
{
	jassert(!isImmediateValue());
	return reg.as<X86Gpd>();
}


X86Gp AsmJitHelpers::BaseNode::copyToGenericRegisterIfNecessary(X86Compiler& cc)
{
	if (isImmediateValue())
	{
		auto newReg = cc.newInt32();
		BinaryOpInstructions::Int::store(cc, newReg, this);

		return newReg;
	}
	else
	{
		return getAsGenericRegister();
	}
}


template <typename T> X86Mem AsmJitHelpers::TypedNode<T>::getAsMemoryLocation() const
{
	return memory;
}
template <typename T> X86Xmm AsmJitHelpers::TypedNode<T>::getAsFloatingPointRegister() const
{
	jassert(!isImmediateValue());
	return reg.as<X86Xmm>();
}

} // end namespace jit
} // end namespace hnode