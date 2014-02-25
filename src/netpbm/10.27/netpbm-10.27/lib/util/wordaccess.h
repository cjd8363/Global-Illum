/* These are facilities for accessing data in C programs in ways that
   exploit the way the machine defines words in order to squeeze out
   speed and CPU efficiency.

   In particular, routines in this file exploit the endianness of the
   machine and use explicit machine instructions to access C
   variables.

   A word is the amount of data that fits in a register; the amount of
   data that a single machine instruction can process.  For example,
   on IA32, a word is 32 bits because a single load or store
   instruction moves that many bits and a single add instruction
   operates on that many bits.

   To the extent that a machine is too complex to have a single size
   that fits this definition, the facilities here aren't useful.  

   These facilities revolve around two data types:  wordInt and
   wordIntBytes.  

   wordint is an unsigned integer with precision (size) of one word.
   It is just the number -- nothing is implied about how it is
   represented in memory.

   wordintBytes is an array of bytes that represent a word-sized
   unsigned integer.  x[0] is the high order 8 digits of the binary
   coding of the integer, x[1] the next highest 8 digits, etc.
   Note that it has big-endian form, regardless of what endianness the
   underlying machine uses.
*/

#ifndef WORDACCESS_H
#define WORDACCESS_H

#include "pm_config.h"

/* BYTE_ORDER is defined by pm_config.h if it knows the endianness of
   the target machine.  But it may be undefined.

   BITS_PER_WORD is defined by pm_config.h if it knows.
*/

/*----------------------------------------------------------------------------
   Define the fundamental types, wordInt and wordIntBytes.  See
   descriptions of those above.
-----------------------------------------------------------------------------*/

#ifdef BITS_PER_WORD
  #if BITS_PER_WORD == 64
    typedef unsigned long long wordint;
  #elif BITS_PER_WORD == 32
    typedef uint32_t wordint;
  #elif BITS_PER_WORD == 16
    typedef unsigned short int wordint;
  #else
    #error "Don't know how to deal with this value of BITS_PER_WORD"
  #endif
#else
  /* We can't do any fancy word-based stuff, but we can still fake it
     so the including program can use the same types and calls as on
     a system where we can do fancy word-based stuff.
  */
  typedef uint32_t wordint;
#endif  

typedef unsigned char wordintBytes[sizeof(wordint)];

#if defined(BYTE_ORDER) && BYTE_ORDER == BIG_ENDIAN

/* Big endian is easy because the machine represents the bytes in the
   exact same format as wordintBytes.  It's a simple matter of casting
   to fool the compiler.  
*/
static __inline__ wordint
bytesToWordint(wordintBytes bytes) {
    return *((wordint *)bytes);
}

wordintToBytes(wordintBytes * const bytesP,
               wordint        const wordInt) {
    
    *(wordint *)bytesP = wordInt;
}

#elif defined(BYTE_ORDER) && BYTE_ORDER == LITTLE_ENDIAN && defined(__GNUC__) && defined(i486) && defined(BITS_PER_WORD)

#if BITS_PER_WORD == 16

static __inline__ unsigned short int
bytesToWordint(wordintBytes const bytes) {
    unsigned short y;
    y = *(unsigned short*)bytes;
    __asm__ ( "xchgb %%al,%%ah" : "=%ax" (y) : "0"  (y) );
    return y;
}

static __inline__ void
wordintToBytes(wordintBytes * const bytesP,
               unsigned short const wordInt) { 
    __asm__("xchgb %%al,%%ah" : "=%ax" (wordInt) : "0"  (wordInt));
    *(unsigned short *)bytesP = wordInt;
}

#elif BITS_PER_WORD == 32

static __inline__ unsigned int
bytesToWordint(wordintBytes const bytes) {
    unsigned int y;
    y = *(unsigned int *)bytes;
    __asm__("bswap %0" : "=r" (y) : "0"  (y));
    return y;
}

static __inline__ void
wordintToBytes(wordintBytes * const bytesP,
               unsigned int   const wordInt) { 
    wordint swapped;
    swapped = wordInt;
    __asm__("bswap %0" : "=r" (swapped) : "0"  (swapped));
    *(unsigned int *)bytesP = swapped;
}

#elif BITS_PER_WORD == 64

static __inline__ unsigned long long
bytesToWordint(wordintBytes const bytes) {
    unsigned long long y;
    y = *(unsigned long long *)bytes;
    __asm__("bswap %%eax\n\t" 
            "bswap %%edx\n\t"
            "xchg %%eax, %%edx" 
            : "=A" (y) : "0"  (y) 
        );
    return y;
}

static __inline__ void
wordintToBytes(wordintBytes *     const bytesP,
               unsigned long long const wordInt) {
    __asm__("bswap %%eax\n\t" 
            "bswap %%edx\n\t"
            "xchg %%eax, %%edx" 
            : "=A" (x) : "0"  (x) 
        );
    *(unsigned long long *)bytesP = wordInt;
}

#else
  #error "Cannot handle this BITS_PER_WORD"
#endif

#else

/* It's nothing we know how to convert quickly.  So we do it the hard
   way, without assuming anything at all about the machine.
*/

static __inline__ wordint
bytesToWordint(wordintBytes const bytes) {

    wordint retval;
    unsigned int i;
    
    for (i = 1, retval = bytes[0]; i < sizeof(bytes); ++i) {
        retval = (retval << 8) + bytes[i];
    }
    return retval;
}

static __inline__ void
wordintToBytes(wordintBytes * const bytesP,
               wordint        const wordInt) {

    wordint buffer;
    int i;

    for (i = sizeof(*bytesP)-1, buffer = wordInt; i >= 0; --i) {
        (*bytesP)[i] = buffer & 0xFF;
        buffer >>= 8;
    }
}

#endif


#endif
