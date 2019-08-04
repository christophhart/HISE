/*
==============================================================================

NewFunkyParser.h
Created: 31 Jan 2019 4:53:19pm
Author:  Christoph

==============================================================================
*/


namespace snex {
namespace jit
{
using namespace juce;



void Operations::Function::process(BaseCompiler* compiler, BaseScope* scope)
{
	Statement::process(compiler, scope);

	COMPILER_PASS(BaseCompiler::FunctionParsing)
	{
		functionScope = new FunctionScope(scope);
		functionScope->data = data;
		functionScope->parameters.addArray(parameters);
		functionScope->parentFunction = dynamic_cast<ReferenceCountedObject*>(this);

		dynamic_cast<ClassScope*>(scope)->addFunction(new FunctionData(data));

		

		for (int i = 0; i < parameters.size(); i++)
		{
			auto initValue = VariableStorage(data.args[i], 0);
			auto initType = initValue.getType();
			jassert(initType == data.args[i]);

			functionScope->allocate(parameters[i], initValue);
		}

		try
		{
			NewFunctionParser p(compiler, *this);

			statements = p.parseStatementList();

			compiler->executePass(BaseCompiler::ResolvingSymbols, functionScope, statements);
			compiler->executePass(BaseCompiler::TypeCheck, functionScope, statements);

			functionScope->parentFunction = nullptr;

			compiler->setCurrentPass(BaseCompiler::FunctionParsing);
		}
		catch (ParserHelpers::CodeLocation::Error& e)
		{
			statements = nullptr;
			functionScope = nullptr;

			throw e;
		}

		

		
	}

	COMPILER_PASS(BaseCompiler::FunctionCompilation)
	{
		ScopedPointer<asmjit::StringLogger> l = new asmjit::StringLogger();

		auto runtime = getRuntime(compiler);
		auto jitClassScope = findClassScope(scope);

		ScopedPointer<asmjit::CodeHolder> code = new asmjit::CodeHolder();
		code->setLogger(l);
		code->setErrorHandler(this);
		code->init(runtime->getCodeInfo());

		//code->setErrorHandler(this);

		ScopedPointer<asmjit::X86Compiler> cc = new asmjit::X86Compiler(code);

		FuncSignatureX sig;

		AsmCodeGenerator::fillSignature(data, sig, false);
		auto func = cc->addFunc(sig);

		dynamic_cast<ClassCompiler*>(compiler)->setFunctionCompiler(cc);

		compiler->registerPool.clear();

		compiler->executePass(BaseCompiler::RegisterAllocation, functionScope, statements);
		compiler->executePass(BaseCompiler::CodeGeneration, functionScope, statements);

		cc->endFunc();
		cc->finalize();
		cc = nullptr;

		runtime->add(&data.function, code);

		auto fClass = dynamic_cast<FunctionClass*>(scope);

		bool success = fClass->injectFunctionPointer(data);

		jassert(success);

		auto& as = dynamic_cast<ClassCompiler*>(compiler)->assembly;

		as << "; function " << data.getSignature() << "\n";
		as <<  l->getString();

		code->setLogger(nullptr);
		l = nullptr;
		code = nullptr;

		compiler->setCurrentPass(BaseCompiler::FunctionCompilation);
	}
}


}
}