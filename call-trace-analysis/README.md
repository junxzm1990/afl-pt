# Call Trace Analysis

This analysis takes as inputs the corpus from fuzzer and re-executes each test case and collected the call chains. A call chain is defined as — When the execution reaches a leaf node on the program’s call graph, the sequence of functions on the stack is deemed as a callchain. The length of a call chain represents a "locally maximal" execution depth.
