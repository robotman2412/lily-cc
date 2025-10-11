# The Lily C library
This library is the Lily C compiler's built-in C runtime library.
It defines things Lily-CC implicitly defines or assumes to exist.

## Integer routines
This library includes integer routines called by Lily-CC if some integer arithmetic is not supported by CPU hardware.

Each of the following is a separate featureset Lily-CC expects that can be emulated with functions:
- Multiply
- Divide / remainder
- Bit-shift by non-constant amounts
- Count leading / trailing zeroes
- Count ones

## Floating-point routines
This library includes floating-point routines called by Lily-CC if some integer floating-point is not supported by CPU hardware.

Each of the following is a separate featureset Lily-CC expects that can be emulated with functions:
- Add / subtract / negate
- Multiply
- Divide / remainder
- Square root
- Absolute value
