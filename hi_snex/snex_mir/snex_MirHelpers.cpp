
MIR_type_t snex::mir::SimpleTypeParser::getMirType(bool getPointerIfRef/*=true*/) const
{
	if (isRef && getPointerIfRef)
		return MIR_T_P;
	else
	{
		if (t == Types::ID::Integer)
			return MIR_T_I64;
		if (t == Types::ID::Float)
			return MIR_T_F;
		if (t == Types::ID::Double)
			return MIR_T_D;
		if (t == Types::ID::Pointer)
			return MIR_T_P;

		return MIR_T_I8;
	}
}



bool snex::mir::TypeConverters::forEachChild(const ValueTree& v, const std::function<bool(const ValueTree&)>& f)
{
	if (f(v))
		return true;

	for (auto c : v)
	{
		if (forEachChild(c, f))
			return true;
	}

	return false;
}

snex::NamespacedIdentifier snex::mir::TypeConverters::String2NamespacedIdentifier(const String& symbol)
{
	return NamespacedIdentifier::fromString(symbol.fromFirstOccurrenceOf(" ", false, false));
}

snex::Types::ID snex::mir::TypeConverters::MirType2TypeId(MIR_type_t t)
{
	if (t == MIR_T_I64)
		return Types::ID::Integer;
	if (t == MIR_T_F)
		return Types::ID::Float;
	if (t == MIR_T_D)
		return Types::ID::Double;
	if (t == MIR_T_P)
		return Types::ID::Pointer;

	return Types::ID::Void;
}

MIR_type_t snex::mir::TypeConverters::TypeInfo2MirType(const TypeInfo& t)
{
	auto m = t.getType();

	if (m == Types::ID::Integer)
		return MIR_T_I64;
	if (m == Types::ID::Float)
		return MIR_T_F;
	if (m == Types::ID::Double)
		return MIR_T_D;
	if (m == Types::ID::Pointer)
		return MIR_T_P;

	jassertfalse;
	return MIR_T_I8;
}

snex::jit::TypeInfo snex::mir::TypeConverters::String2TypeInfo(String t)
{
	return SimpleTypeParser(t).getTypeInfo();
}

MIR_type_t snex::mir::TypeConverters::String2MirType(String symbol)
{
	return SimpleTypeParser(symbol).getMirType();
}

snex::mir::FunctionData snex::mir::TypeConverters::String2FunctionData(String s)
{
	return SimpleTypeParser(s).parseFunctionData();
}

String snex::mir::TypeConverters::NamespacedIdentifier2MangledMirVar(const NamespacedIdentifier& id)
{
	auto n = id.toString().removeCharacters("& ");

	n = n.replaceCharacters("<>,", "___");
	

	if (n.containsChar('='))
		n = n.replace("=", "_assign");
	if (n.containsChar('~'))
		n = n.replace("~", "_dest");
	if (n.contains("++"))
		n = n.replace("++", "_inc_");
	if (n.contains("--"))
		n = n.replace("--", "_dec_");
	if (n.contains("[]"))
		n = n.replace("[]", "_subscript");

	return n.replace("::", "_");
}

snex::jit::Symbol snex::mir::TypeConverters::String2Symbol(const String& symbolCode)
{
	jit::Symbol s;

	SimpleTypeParser p(symbolCode);

	s.typeInfo = p.getTypeInfo();
	s.id = p.parseNamespacedIdentifier();

	return s;
}

MIR_var snex::mir::TypeConverters::SymbolToMirVar(const jit::Symbol& s)
{
	MIR_var t;
	t.name = s.id.getIdentifier().toString().getCharPointer().getAddress();
	t.size = 0;
	t.type = TypeInfo2MirType(s.typeInfo);
	return t;
}

String snex::mir::TypeConverters::FunctionData2MirTextLabel(const NamespacedIdentifier& objectType, const FunctionData& f)
{
	String label;

	if (objectType.isValid() && objectType.toString().containsAnyOf("<>"))
	{
		label << NamespacedIdentifier2MangledMirVar(objectType);

		auto fid = NamespacedIdentifier::fromString(f.id.toString());

		for (int i = 0; i < fid.namespaces.size(); i++)
		{
			if (label.contains(fid.namespaces[i].toString()))
				fid.namespaces.remove(i--);
		}
		
        label << "_";

		label << NamespacedIdentifier2MangledMirVar(fid);
	}
	else
	{
		label << NamespacedIdentifier2MangledMirVar(NamespacedIdentifier::fromString(f.id.toString()));
	}
	
	
	
	label << "_";
	label << Types::Helpers::getCppTypeName(f.returnType.getType())[0];

	for (const auto& a : f.args)
		label << Types::Helpers::getCppTypeName(a.typeInfo.getType())[0];

	
	
	return label;
}



String snex::mir::TypeConverters::MirType2MirTextType(const MIR_type_t& t)
{
	if (t == MIR_T_I64)
		return "i64";
	if (t == MIR_T_P)
		return "i64";
	if (t == MIR_T_D)
		return "d";
	if (t == MIR_T_F)
		return "f";

	throw String("Unknown type");
}

String snex::mir::TypeConverters::Symbol2MirTextSymbol(const jit::Symbol& s)
{
	String t;

	auto tn = TypeInfo2MirTextType(s.typeInfo);

	if (s.typeInfo.isRef() && tn != "p")
		tn = "i64";

	t << tn << ":" << NamespacedIdentifier2MangledMirVar(s.id);
	return t;
}

String snex::mir::TypeConverters::TypeInfo2MirTextType(const TypeInfo& t, bool refToPtr)
{
	if (refToPtr && t.isRef())
		return "i64";

	if (t.getType() == Types::ID::Integer)
		return "i64";
	if (t.getType() == Types::ID::Double)
		return "d";
	if (t.getType() == Types::ID::Float)
		return "f";
	if (t.getType() == Types::ID::Pointer ||
		t.getType() == Types::ID::Block)
		return "p";

	throw String("Unknown type");
}

String snex::mir::TypeConverters::MirTypeAndToken2InstructionText(MIR_type_t type, const String& token)
{
	static StringPairArray intOps, fltOps, dblOps;

	intOps.set(JitTokens::assign_, "MOV");
	intOps.set(JitTokens::plus, "ADD");
	intOps.set(JitTokens::minus, "SUB");
	intOps.set(JitTokens::times, "MUL");
	intOps.set(JitTokens::divide, "DIV");
	intOps.set(JitTokens::modulo, "MOD");
	intOps.set(JitTokens::greaterThan, "GTS");
	intOps.set(JitTokens::greaterThanOrEqual, "GES");
	intOps.set(JitTokens::lessThan, "LTS");
	intOps.set(JitTokens::lessThanOrEqual, "LES");
	intOps.set(JitTokens::equals, "EQ");
	intOps.set(JitTokens::notEquals, "NE");
    intOps.set(JitTokens::logicalAnd, "AND");
    intOps.set(JitTokens::logicalOr, "OR");

	fltOps.set(JitTokens::assign_, "FMOV");
	fltOps.set(JitTokens::plus, "FADD");
	fltOps.set(JitTokens::minus, "FSUB");
	fltOps.set(JitTokens::times, "FMUL");
	fltOps.set(JitTokens::divide, "FDIV");
	fltOps.set(JitTokens::greaterThan, "FGT");
	fltOps.set(JitTokens::greaterThanOrEqual, "FGE");
	fltOps.set(JitTokens::lessThan, "FLT");
	fltOps.set(JitTokens::lessThanOrEqual, "FLE");
	fltOps.set(JitTokens::equals, "FEQ");
	fltOps.set(JitTokens::notEquals, "FNE");

	dblOps.set(JitTokens::assign_, "DMOV");
	dblOps.set(JitTokens::plus, "DADD");
	dblOps.set(JitTokens::minus, "DSUB");
	dblOps.set(JitTokens::times, "DMUL");
	dblOps.set(JitTokens::divide, "DDIV");
	dblOps.set(JitTokens::greaterThan, "DGT");
	dblOps.set(JitTokens::greaterThanOrEqual, "DGE");
	dblOps.set(JitTokens::lessThan, "DLT");
	dblOps.set(JitTokens::lessThanOrEqual, "DLE");
	dblOps.set(JitTokens::equals, "DEQ");
	dblOps.set(JitTokens::notEquals, "DNE");

	if (type == MIR_T_I64 ||
        type == MIR_T_P)	return intOps.getValue(token, "").toLowerCase();
	if (type == MIR_T_F)	return fltOps.getValue(token, "").toLowerCase();
	if (type == MIR_T_D)	return dblOps.getValue(token, "").toLowerCase();

	jassertfalse;
	return "";
}

String snex::mir::TypeConverters::TemplateString2MangledLabel(const String& templateArgs)
{
	return templateArgs.removeCharacters(" <>,");
}

String snex::mir::TypeConverters::StringOperand2ReturnBlock(const String& op, int returnBlockSize)
{
	return "rblk:" + String(returnBlockSize) + "(" + op + ")";
}

String snex::mir::TypeConverters::TypeAndReturnBlockToReturnType(const TypeInfo& rt, int returnBlockSize/*=-1*/)
{
	if (returnBlockSize != -1)
		return StringOperand2ReturnBlock("return_block", returnBlockSize);
	else if (rt.isValid())
		return TypeInfo2MirTextType(rt, true);
	else
		return {};
}

String snex::mir::TypeConverters::VectorOp2Signature(const ValueTree& v)
{
	const String vf = "pointer& Math::{FUNCTION}(pointer& Param0, pointer& Param1)";
	const String sf = "pointer& Math::{FUNCTION}(pointer& Param0, float Param1)";

	auto isScalar = (bool)v[InstructionPropertyIds::Scalar];

	auto prototypeToUse = isScalar ? sf : vf;

	auto opType = v[InstructionPropertyIds::OpType].toString()[0];

	String op = "v";

	switch (opType)
	{
	case '*': op << "mul"; break;
	case '+': op << "add"; break;
	case '-': op << "sub"; break;
	case '/': op << "div"; break;
	case '=': op << "mov"; break;
	}

	if (isScalar)
		op << "s";

	return prototypeToUse.replace("{FUNCTION}", op);
}
