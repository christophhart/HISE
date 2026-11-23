// Run with --args:4 --backend:interpreter. Expected result: 4.
// The interpreter currently has no AnonymousBlock node for the local constructor call.
struct Box
{
	int value = 0;
	Box(int input) { value = input; }
};

int test(int input)
{
	Box local(input);
	return local.value;
}
