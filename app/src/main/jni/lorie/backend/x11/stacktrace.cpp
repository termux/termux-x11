#include <execinfo.h>  // for backtrace
#include <dlfcn.h>     // for dladdr
#include <cxxabi.h>    // for __cxa_demangle

#include <string>
#include <sstream>
#include <iostream>

// This function produces a stack backtrace with demangled function & method names.
void Backtrace()
{
	int skip = 1;
	void *callstack[128];
	const int nMaxFrames = sizeof(callstack) / sizeof(callstack[0]);
	char buf[1024];
	int nFrames = backtrace(callstack, nMaxFrames);
	char **symbols = backtrace_symbols(callstack, nFrames);

	std::ostringstream trace_buf;
	for (int i = skip; i < nFrames; i++) {
		Dl_info info;
		if (dladdr(callstack[i], &info)) {
			char *demangled = NULL;
			int status;
			demangled = abi::__cxa_demangle(info.dli_sname, NULL, 0, &status);
			snprintf(buf, sizeof(buf), "%d %p %s + %zd\n",
					 i, callstack[i],
					 status == 0 ? demangled : info.dli_sname,
					 (char *)callstack[i] - (char *)info.dli_saddr);
			free(demangled);
		} else {
			snprintf(buf, sizeof(buf), "%d %p\n",
					 i, callstack[i]);
		}
		trace_buf << buf;
	}
	free(symbols);
	if (nFrames == nMaxFrames)
		trace_buf << "[truncated]\n";
	std::cerr << trace_buf.str() << "\n\n\n";
	exit(1);
}
