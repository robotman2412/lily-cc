
# Unnamed Language 2
The goal: A language good for embedded that gets out of your way when needed.

The problem: C is exact but has quirks and is overly verbose, C++ needs less verbosity but turns the quirks up to 11 and Rust needs too many unsafe keywords for kernels.
In addition, all three are statically typed language with little to no dynamic typing features.

The solution: This language?

## Language features I like
- Traits on structs
    - Encourages a composition model
    - Fat pointers for dynamic dispatch
- Dynamic typing
    - Notable traits and subclasses discoverable from abstract pointer
    - Doesn't necessarily imply thin or fat pointers
    - Ideal for generalizing driver systems over many device archetypes
- Class-style inheritance
    - No diamond dependencies allowed (looking at you, C++)
    - Isn't inherently dynamic typing inof itself
- Fat enums / tagged unions
    - Unlike Rust, allow reinterpretation through an explicitly "unsafe operation"

## New features I may add
- Traits on modules
    - Designed to implement abstractions around CPU architecture
- Trait-associated values
    - Constant value stored in the v-table
- Plain-old-data as a first-class feature
    - Transmuting any two plain-old-data of the same size must not require "unsafety"
- First-class traits for primitive types
    - E.g. traits like Primitive, Integer, Numeric

## Brief
The idea is a language to support static and dynamic typing, with memory safety better than C (but borrow checkers are pain, so not as far as Rust).
The language itself should be built on simple axioms, from which larger concepts are constructed.

# Concept A
A language with a `trait` system.
Traits can be marked as `dynamic`, for dynamic typing support.
Dynamic traits are generated greedily, and dynamic objects can tell you not only whether a dynamic trait is implemented but all dynamic traits that are implement.
Unlike Rust, the `dynamic` keyword doesn't mean dynamic dispatch, it means dynamic typing.
Instead, dynamic dispatch simply uses the trait's type without a `dynamic` prefix.

## Example syntax
```
dynamic trait Thing {
    fn weight(*self) -> u32;
}

trait Cat: Thing {
    fn meow(*self);
}

trait Train: Thing {
    fn honk(*self);
}

// This type means a const pointer to any dynamic object.
let something: *dynamic = ...;
// The `is` operator checks whether the given dynamic trait is implemented.
if (something is Thing) {
    // The `as` operator for dynamic objects panics if it doesn't match.
    // It gets a pointer that can be used with a matching dynamic trait.
    printf("It's a thing that weighs {}\n", (something as *Thing).weight());
}
if (something is Cat) {
    // Since `Cat` is inherently `Thing`, you can call `Thing` functions on it.
    (something as *Cat).meow();
}
// Alternative concise syntax?
if (something is Train as thing) {
    // The type of thing is then `*Train`.
    thing.honk();
}
```
