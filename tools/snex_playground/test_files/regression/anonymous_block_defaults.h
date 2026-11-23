// Run with --args:4 --backend:interpreter. Expected result: 13.
// The constructor must not overwrite the default value of pad.
struct Box
{
	int pad = 9;
	int value = 0;
	Box(int input) { value = input; }
};

int test(int input)
{
	Box local(input);
	return local.pad + local.value;
}
