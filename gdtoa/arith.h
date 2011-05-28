#if defined (__i386__)
#	define IEEE_8087
#	define Arith_Kind_ASL 1
#elif defined(__x86_64__)
#	define IEEE_8087
#	define Arith_Kind_ASL 1
#	define Long int
#	define Intcast (int)(long)
#	define Double_Align
#	define X64_bit_pointers
#else
#	error Unknown platform in gdtoa/arith.h!
#endif
