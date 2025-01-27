/*******************************************************************\

Module: Big Integers

Author: Daniel Kroening, dkr@amazon.com

\*******************************************************************/

// Whatever is necessary to use alloca().

#ifndef CPROVER_BIG_INT_ALLOCAINC_H
#define CPROVER_BIG_INT_ALLOCAINC_H

// clang-format off
#if defined linux || defined __linux__ \
  || defined __sun                     \
  || defined UWIN                      \
  || defined osf1                      \
  || defined __MACH__                  \
  || defined __CYGWIN__                \
  || defined __EMSCRIPTEN__
// clang-format on

#  include <alloca.h>

#elif defined _MSC_VER    \
   || defined __BORLANDC__ \
   || defined __MINGW32__

# include <malloc.h>

#elif defined __vax

// In vax-alloca.mar.
extern "C" void *alloca (unsigned);

#elif defined __VMS

// DEC CXX on VMS alpha.
# include <builtins.h>
# define alloca(N) __ALLOCA(N)

#elif defined __xlC__

# pragma alloca
# include <stdlib.h>

#elif defined __FCC__

# define alloca(X) __builtin_alloca(X)

#elif defined __FreeBSD__ || defined __FreeBSD_kernel__ || defined __OpenBSD__

# include <stdlib.h>

#endif


#endif // CPROVER_BIG_INT_ALLOCAINC_H
