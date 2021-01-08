<% title "HashMap Traits" %>

To instantiate a `HashMap`, you must specialize it by defining a traits class. The traits class must declare a specific set of member types and static member functions, some of which are required and some of which are optional. The traits class is then passed as the sole template argument to `HashMap`.

## Examples

An example of a traits class from the WebServer. Used to map `StringView` to `StringView`. This `HashMap` doesn't own any string memory; instead, all strings are assumed to remain valid for the lifetime of the `HashMap`.

    struct ContentTypeTraits {
        using Key = StringView;
        struct Item {
            StringView extension;
            StringView mimeType;
            PLY_INLINE Item(StringView extension) : extension{extension} {
            }
        };
        PLY_INLINE static const Key& comparand(const Item& item) {
            return item.extension;
        }
    };

Another example of a traits class from the WebCooker. Used to map `CookJob` objects (allocated on the heap) to `Status` values. The address of the `CookJob` object is used as the key.

    struct CheckedTraits {
        using Key = CookJob*;
        struct Item {
            CookJob* job;
            Status status;
            PLY_INLINE Item(CookJob* job) : job{job}, status{CookInProgress} {
            }
        };
        static PLY_INLINE Key comparand(const Item& item) {
            return item.job;
        }
    };

Another example of a traits class from Plywood's JSON parser, demonstrating the use of the `Context` member type.

    struct IndexTraits {
        using Key = StringView;
        using Item = u32;
        using Context = Array<Object::Item>;
        static PLY_INLINE StringView comparand(u32 item, const Array<Object::Item>& ctx) {
            return ctx[item].key.view();
        }
    };
    
## Required Member Types

Every `HashMap` traits class must declare the following member types:

<% member typename [strong Key] %>

The type of the key used during lookup. Objects of this type are passed to `HashMap::find()` and `HashMap::insertOrFind()`.

It must be possible to hash `Key` objects; in other words, to convert any `Key` object to a 32-bit hash value in a well-distributed way.

<% member typename [strong Item] %>

The type that gets stored in the `HashMap` itself. The `HashMap` is basically a collection of `Item` objects. Given an `Item` object, it should be possible to obtain a `Key` object in some way.

<% endMembers %>

## Key Hashing

If declared in the traits class, `HashMap` will use the following static member function to hash `Key` objects:

<% member static u32 [strong hash](const Key&) %>

Compute a 32-bit hash value for the key.

<% endMembers %>

Otherwise, `HashMap` will use a `Hasher` object to hash the key. `Hasher` provides built-in support for the following types. You can extend `Hasher` to support additional types by overloading `operator<<()`.

* `u32`
* `u64`
* `float`
* Pointer to class type or `void*`. By default, the pointer value itself is hashed; the object pointed to is not examined in any way.
* `StringView`, `String` and `HybridString`. In particular, note that `char*` will be implicitly converted to `StringView`.
* `ConstBufferView`

## Key Comparison

If declared in the traits class, `HashMap` will use the following static member function to compare `Key` objects:

<% member static bool [strong equal](const Key&, const Key&) %>

Returns `true` if the given keys are equal for the purposes of this `HashMap`.

<% endMembers %>

Otherwise, `HashMap` will use `operator==()`.

## Constructing New Items

If declared in the traits class, `HashMap` will use the following static member function to construct `Item` objects:

<% member static void [strong construct](Item*, const Key&) %>

Constructs an uninitialized `Item` from the given `Key`. Must ensure that the `Item` will be converted back to the given `Key` on subsequent lookups.

<% endMembers %>

Otherwise, if `Item` exposes a constructor that accepts `const Key&`, that constructor is used to construct `Item` objects.

Otherwise, the default `Item` constructor is used. In this case, `HashMap::insertOrFind()` callers are responsible for initializing new `Item` object to a valid state.

## Converting Items to Keys Without Context

If declared in the traits class, `HashMap` will use one of the following static member functions to convert `Item` objects to `Key` objects:

<% member static Key [strong comparand](const Item&) %>

(May also return `const Key&`.) Returns a `Key` object that corresponds to the given `Item`.

<% endMembers %>

Otherwise, `HashMap` will rely on implicit conversion from `Item` to `Key`. If `Item` is a class, you can make it implicitly convertible to `Key` by implementing a user-defined conversion function `operator Key()` or `operator const Key&()`.

## Converting Items to Keys With Context

The traits class can declare an additional `Context` member type, then accept this type as the second argument to `comparand`:

<% member typename [strong Context] %>

Provides additional context for `Item`-to-`Key` conversions.

<% member static Key [strong comparand](const Item&, const Context&) %>

(May also return `const Key&`.) Returns a `Key` object that corresponds to the given `Item` and `Context`.

<% endMembers %>

If both members are declared, the second argument to `HashMap::find()` and `HashMap::insertOrFind()` will be passed to `comparand()`. Callers are reponsible for passing valid `Context` objects to those functions.

The `Context` object is intended to be a container type such as an `Array`, `FixedArray`, `std::vector` or `Pool`. In this case, `Item` can be declared as `u32` and interpreted as the index into the other container.
