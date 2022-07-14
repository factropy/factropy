// stacktrace.h (c) 2008, Timo Bingmann from http://idlebox.net/
// published under the WTFPL v2.0

#ifndef _STACKTRACE_H_
#define _STACKTRACE_H_

#include <stdio.h>
#include <stdlib.h>
#include <cxxabi.h>

#ifdef __linux__

#include <execinfo.h>

/** Print a demangled stack backtrace of the caller function to FILE* out. */
static inline void print_stacktrace(FILE *out = stderr, unsigned int max_frames = 63)
{
	fprintf(out, "stack trace:\n");

	// storage array for stack trace address data
	void* addrlist[max_frames+1];

	// retrieve current stack addresses
	int addrlen = backtrace(addrlist, sizeof(addrlist) / sizeof(void*));

	if (addrlen == 0) {
		fprintf(out, "  <empty, possibly corrupt>\n");
		return;
	}

	// resolve addresses into strings containing "filename(function+address)",
	// this array must be free()-ed
	char** symbollist = backtrace_symbols(addrlist, addrlen);

	// allocate string which will be filled with the demangled function name
	size_t funcnamesize = 256;
	char* funcname = (char*)malloc(funcnamesize);

	// iterate over the returned symbol lines. skip the first, it is the
	// address of this function.
	for (int i = 1; i < addrlen; i++) {
		char *begin_name = 0, *begin_offset = 0, *end_offset = 0;

		// find parentheses and +address offset surrounding the mangled name:
		// ./module(function+0x15c) [0x8048a6d]
		for (char *p = symbollist[i]; *p; ++p) {
		    if (*p == '(')
				begin_name = p;
		    else if (*p == '+')
				begin_offset = p;
		    else if (*p == ')' && begin_offset) {
				end_offset = p;
				break;
		    }
		}

		if (begin_name && begin_offset && end_offset && begin_name < begin_offset) {
		    *begin_name++ = '\0';
		    *begin_offset++ = '\0';
		    *end_offset = '\0';

		    // mangled name is now in [begin_name, begin_offset) and caller
		    // offset in [begin_offset, end_offset). now apply
		    // __cxa_demangle():

		    int status;
		    char* ret = abi::__cxa_demangle(begin_name,
						    funcname, &funcnamesize, &status);
		    if (status == 0) {
				funcname = ret; // use possibly realloc()-ed string
				fprintf(out, "  %s : %s+%s\n",
				symbollist[i], funcname, begin_offset);
		    }
		    else {
				// demangling failed. Output function name as a C function with
				// no arguments.
				fprintf(out, "  %s : %s()+%s\n",
				symbollist[i], begin_name, begin_offset);
		    }
		}
		else {
		    // couldn't parse the line? print the whole line.
		    fprintf(out, "  %s\n", symbollist[i]);
		}
	}

	free(funcname);
	free(symbollist);
}

#endif

#ifdef _WIN32

#include <dbghelp.h>

static inline void print_stacktrace(FILE *out = stderr, unsigned int max_frames = 63)
{
//     unsigned int   i;
//     void         * stack[ 100 ];
//     unsigned short frames;
//     SYMBOL_INFO  * symbol;
//     HANDLE         process;
//
//     process = GetCurrentProcess();
//
//     SymInitialize( process, NULL, TRUE );
//
//     frames               = CaptureStackBackTrace( 0, 100, stack, NULL );
//     symbol               = ( SYMBOL_INFO * )calloc( sizeof( SYMBOL_INFO ) + 256 * sizeof( char ), 1 );
//     symbol->MaxNameLen   = 255;
//     symbol->SizeOfStruct = sizeof( SYMBOL_INFO );
//
//     for( i = 0; i < frames && i < max_frames; i++ )
//     {
//         SymFromAddr( process, ( DWORD64 )( stack[ i ] ), 0, symbol );
//
//         fprintf(out, "%i: %s - 0x%0X\n", frames - i - 1, symbol->Name, symbol->Address );
//     }
//
//     free( symbol );
}

#endif

#endif // _STACKTRACE_H_
