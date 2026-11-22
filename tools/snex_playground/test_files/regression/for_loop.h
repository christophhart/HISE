// Run with --args:4 --backend:interpreter. Expected result: 4.
// The JIT returns 4; the interpreter currently returns 0 because WhileLoop is not registered.
// The full unit test run may stop at the corresponding JUCE assertion.
int test(int input)
{
	int value = 0;

	for(int i = 0; i < input; i++)
	{
		if(i == 1)
			continue;

		if(i == 3)
			break;

		value++;
	}
	
	return value;
}
