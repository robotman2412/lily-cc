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

## Example trait syntax

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

## Example generics syntax

```
// A plain struct without any generics.
struct Foo {
    a: u32,
    b: usize,
}

// A struct that directly embeds a generic-typed member.
struct Bar<T> {
    a: T,
}

// Associated type for Bar<u32>
let baz: Bar::<u32>;
```

# Concept B

This alternate syntax concept has unambiguous end-of-expression and is highly structured in how `()`, `[]` and `{}` are used; `()` is for expressions, `[]` is for generics and `{}` is for structs and initializers.
In this language, the line between expressions and statements is much thinner; things like `if`, `while` and `for` are all expressions.

## Example generics syntax

```
struct Bar[T] {
    a: T
}
```

## Example statement syntax

```
// The basic form of if statements
if expr expr
// And else statements
else expr

// Recommended form for best readability:
if condition (
    body
) else (
    else_body
)

// Variables declared with `let` again, followed by the type:
let array: *(u32) // Fat pointer to slice of u32

// A traditional function call syntax like expr(exprs) would be ambiguous
expr.()
// Which would be in line with traditional field references:
struct.field
// Array indexing works with the same syntax as functions:
array.(index)
```

# Concept C

Another alternate concept to reduce ambiguity in the invocation of generics.
Same generics syntax as concept B, but different disambiguation of expressions.

## Example statement syntax

```
// C-like if statements return.
if (condition) {
} else {
}

// A different syntax for slices and arrays again:
let array: *u32(); // If the size were known it would be in the parentheses
// For completeness, function types:
let funcptr: *fn(u32) -> u32;
```
