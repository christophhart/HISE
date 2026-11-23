// Run with --args:4 --backend:interpreter. Expected result: 4.
// A while loop body can be a single statement without braces.
int test(int input)
{
	int result = 0;
	while (result < input)
		result += 1;
	return result;
}
