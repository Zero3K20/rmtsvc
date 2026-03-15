// Fix LNK2001: unresolved external symbol __imp___iob
//
// jpeg-r-dll.lib was compiled against a pre-VS2015 CRT that declared _iob as
// __declspec(dllimport), so Jerror.obj contains a reference to __imp___iob.
// legacy_stdio_definitions.lib covers __imp__fprintf / __imp__printf etc.,
// but does NOT cover __imp___iob (the stdio stream array pointer).
//
// Resolution (Win32 / x86):
//   In 32-bit MSVC, C name "_iob" -> COFF name "__iob".
//   The old dllimport code does:
//     mov eax, dword ptr [__imp___iob]   ; load pointer stored at __imp___iob
//     ... [eax + N * sizeof(FILE)]        ; index into the array
//   so __imp___iob must be a location that *contains* the address of the FILE
//   array -- i.e. a pointer variable, not the array itself.
//
//   /ALTERNATENAME:__imp___iob=__iob redirects the unresolved __imp___iob to
//   our pointer variable __iob (== &_iob), satisfying the link.
//
//   _iob is initialised to __iob_func() (provided by legacy_stdio_definitions.lib)
//   so it always points to the CRT's live internal FILE array rather than a copy.

#include <stdio.h>

// Provided by legacy_stdio_definitions.lib (already linked via other/IPF.cpp).
extern "C" FILE * __cdecl __iob_func(void);

// _iob: pointer to the CRT's internal FILE array.
// In 32-bit MSVC: C name "_iob" -> COFF name "__iob".
extern "C" FILE *_iob = NULL;

// Map the unresolved DLL-import stub to our local pointer.
// Symbol names differ between x86 and x64, so guard appropriately.
#ifdef _M_IX86
// x86: C name "_iob" -> COFF "__iob"; import stub is "__imp___iob"
#pragma comment(linker, "/ALTERNATENAME:__imp___iob=__iob")
#endif

// Point _iob at the CRT's live FILE array (populated before any user code runs).
static int _iob_init_flag = (_iob = __iob_func(), 0);
