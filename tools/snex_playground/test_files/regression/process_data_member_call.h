// Reproducer for the interpreter assertion in FunctionCall::resolveDataPointers().
// Use ProcessTestCase from hi_snex/unit_test/snex_jit_UnitTests.cpp (NumChannels = 2).
// The standalone --run CLI does not register ProcessData, so it cannot run this file directly.
// Expected: compilation succeeds and test returns 1.
int test(ProcessData<1>& data)
{
	auto fd = data.toFrameData();

	float sum = 0.2f;

	while(fd.next())
	{
		sum += fd[0];
	}

	return (int)sum;
}
