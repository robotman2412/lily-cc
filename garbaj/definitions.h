
#ifndef DEFINITIONS_H
#define DEFINITIONS_H

#ifndef NDEBUG
// Prompt debugger by raising SIGTRAP, if DEBUG_COMPILER is defined.
#	define DEBUGGER() raise(SIGTRAP)
#else
// Prompt debugger by raising SIGTRAP, if DEBUG_COMPILER is defined.
#	define DEBUGGER() do{}while(0)
#endif

#endif //DEFINITIONS_H
