#include <stdlib.h>
#include <stdio.h>
#include <dlfcn.h>

int main(int argc, char **argv) {
	void *handle;

	handle = dlopen ("./recovered.so", RTLD_LAZY);
	dlclose(handle);
}
