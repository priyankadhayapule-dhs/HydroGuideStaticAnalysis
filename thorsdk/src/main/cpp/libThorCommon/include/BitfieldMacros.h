//
// Copyright 2017 EchoNous, Inc.
//
#pragma once

#include <cstdint>

// Convenient macros for bit manipulations intended for use in low level functions that have
// to set up various hardware components. Macros defined here are based on the ideas borrowed from
// http://www.coranac.com/documents/working-with-bits-and-bitfields/
//
// Assumptions:
// 1) Associated with each bit field are two macro defintions -
//    one with a _BIT suffix indicating the position of LSB of the field
//    and another one with a _LEN suffix indicating its length.
// 2) Macros starting with BIT_ prefix set up 32-bit variables and
// 3) Macros starting with BIT64_ prefix set up 64-bit variables

#define BIT(n)					(1 << (n))
#define BIT64(n)                ((uint64_t)1 << (n))

#define BIT_SET(y, mask)		(y |=  (mask))
#define BIT_CLEAR(y, mask)		(y &= ~(mask))
#define BIT_FLIP(y, mask)		(y ^=  (mask))

//! Create a bitmask of length \a len.
#define BIT_MASK(len)			(BIT(len)-1)
#define BIT64_MASK(len)         (BIT64(len)-1)

//! Create a bitfield mask of length \a starting at bit \a start.
#define BF_MASK(bit, len)     ( BIT_MASK(len)<<(bit) )
#define BF64_MASK(bit, len)   ( (uint64_t)BIT64_MASK(len)<<(bit))

//! Prepare a bitmask for insertion or combining.
#define BF_PREP(x, bit, len)  ( ((x)&BIT_MASK(len)) << (bit) )
#define BF64_PREP(x, bit, len) \
    ( (uint64_t)((x)&BIT64_MASK(len)) << (bit) )

//! Extract a bitfield of length \a len starting at \a bit from \a y.
#define BF_GET(y, bit, len)   ( ((y)>>(bit)) & BIT_MASK(len) )
#define BF64_GET(y, bit, len)   ( ((uint64_t)(y)>>(bit)) & BIT64_MASK(len) )
            
//! Insert a new bitfield value \a x into \a y.
#define BF_SET(y, x, bit, len)    \
    ( y = ((y) &~ BF_MASK(bit, len)) | BF_PREP(x, bit, len) )

#define BF64_SET(y, x, bit, len)    \
    ( y = ((y) &~ BF64_MASK(bit, len)) | BF64_PREP(x, bit, len) )

// Macros for named bit field operations

//! Massage \a x for use in bitfield \a name.
#define BFN_PREP(x, name)      ( ((x)<<name##_BIT) & BF_MASK(name##_BIT, name##_LEN))
#define BFN64_PREP(x, name)    ( ((uint64_t)(x)<<name##_BIT) & BF64_MASK(name##_BIT, name##_LEN))

//! Get the value of bitfield \a name from \a y. Equivalent to (var=) y.name
#define BFN_GET(y, name)       ( ((y) & BF_MASK(name##_BIT, name##_LEN))>>name##_BIT )
#define BFN64_GET(y, name)     ( (uint64_t)((y) & BF64_MASK(name##_BIT, name##_LEN))>>name##_BIT )

//! Set bitfield \a name from \a y to \a x: y.name= x.
#define BFN_SET(y, x, name)    ( y = ((y)&~BF_MASK(name##_BIT, name##_LEN)) | BFN_PREP(x&BIT_MASK(name##_LEN),name) )
#define BFN64_SET(y, x, name)  ( y = ((y)&~BF64_MASK(name##_BIT, name##_LEN)) | BFN64_PREP(x&BIT64_MASK(name##_LEN),name) )


