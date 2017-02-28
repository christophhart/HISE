Has common interfaces to allow changing underlying implementation. These are things that aren't specific to NativeJIT where we don't want to take an external dependency.

For example, there's an IAllocator interface that allows you to swap in an arbitrary allocator fairly easily.