<% title "HashMap Traits" %>

Creating a `HashMap` requires you to define your own Traits class. Derive your Traits class from `BaseHashMapTraits` to reduce the amount of boilerplate code. Your Traits class must define the following members:

<% member typename [strong Key] %>

_(Required)_ The type used to locate existing items in the hash map.

<% member typename [strong Item] %>

_(Required)_ The type that gets stored in the hash map itself.

<% member static u32 [strong hash](const Key&) %>

_(Optional)_ Compute a 32-bit hash value for the key. If you don't define a custom `hash` function here, the default implementation calls the key's `appendTo` member function with a `Hasher` object (an object that helps implement [MurmurHash3](https://en.wikipedia.org/wiki/MurmurHash#MurmurHash3)). 

<% member static void [strong construct](Item*, const Key&) %>

_(Optional)_ Constructs an `Item` in-place. If you don't define `construct`, the default implementation tries to pass `Key` to `Item`'s constructor (using SFINAE); if no such constructor exists, it calls `Item`'s default constructor. In all cases, if `Key` is ignored when constructing the `Item`, it's the caller's responsibility to ensure that the hash table is left in a consistent state after inserting a new item.

<% member static [em ComparandType] [strong comparand](const Item&) %>

_(Optional)_ Converts an `Item` to another type, of your choosing, that will be passed as the first argument to `equal`. If you don't define `comparand` in your traits class, it will inherit a default implementation from `BaseHashMapTraits` that returns a const reference to the `Item` itself.

<% member typename [strong Context] %>
<% member static [em ComparandType] [strong comparand](const Item&, const Context&) %>

_(Optional)_ A type to help implement `comparand`. If you don't define this type in your traits class, it will inherit an empty struct from `BaseHashMapTraits`.

<% member static bool [strong equal](const [em ComparandType]&, const Key&) %>

_(Optional)_ Returns `true` if the comparand type matches `key`. `ComparandType` can be any type of your choosing. By default, it's `Item`, but if you want to make it a different type, implement the `comparand` function in your Traits class.

<% endMembers %>
