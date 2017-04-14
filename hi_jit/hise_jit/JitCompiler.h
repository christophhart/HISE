/*
  ==============================================================================

    JitCompiler.h
    Created: 4 Apr 2017 9:19:42pm
    Author:  Christoph

  ==============================================================================
*/

#ifndef JITCOMPILER_H_INCLUDED
#define JITCOMPILER_H_INCLUDED




class HiseJITCompiler::Pimpl
{
public:

	Pimpl(const String& codeToCompile, bool useCppMode_ = true) :
		unprocessedCode(codeToCompile),
		preprocessor(codeToCompile),
		code(preprocessor.process()),
		compiledOK(false),
		useSafeFunctions(preprocessor.shouldUseSafeBufferFunctions()),
		useCppMode(useCppMode_)
	{

	};

	~Pimpl() {}

	HiseJITScope* compileAndReturnScope()
	{

		ScopedPointer<HiseJITScope> scope = new HiseJITScope();

		GlobalParser globalParser(code, scope, useSafeFunctions, useCppMode);

		try
		{
			globalParser.parseStatementList();
		}
		catch (ParserHelpers::CodeLocation::Error e)
		{
			errorMessage = "Line " + String(getLineNumberForError(e.offsetFromStart)) + ": " + e.errorMessage;

			compiledOK = false;
			return nullptr;
		}
		catch (String e)
		{
			errorMessage = e;
			compiledOK = false;
			return nullptr;
		}

		compiledOK = true;
		errorMessage = String();
		return scope.release();

	}

	bool wasCompiledOK() const { return compiledOK; };
	String getErrorMessage() const { return errorMessage; };

	String getCode(bool getPreprocessedCode) const
	{
		return getPreprocessedCode ? code : unprocessedCode;
	};

private:

	int getLineNumberForError(int charactersFromStart)
	{
		int line = 1;

		for (int i = 0; i < jmin<int>(charactersFromStart, code.length()); i++)
		{
			if (code[i] == '\n')
			{
				line++;
			}
		}

		return line;
	}

	PreprocessorParser preprocessor;
	const String code;
	bool compiledOK;
	String errorMessage;

	const String unprocessedCode;

	bool useSafeFunctions;
	bool useCppMode;
};




#endif  // JITCOMPILER_H_INCLUDED
