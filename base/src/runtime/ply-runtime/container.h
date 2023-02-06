/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#pragma once
#include <utility>

namespace ply {

//   ▄▄▄▄
//  ██  ██ ▄▄▄▄▄  ▄▄  ▄▄
//  ██▀▀██ ██  ██ ██  ██
//  ██  ██ ██  ██ ▀█▄▄██
//                 ▄▄▄█▀

struct Any {
    uptr storage = 0;

    Any() = default;

    template <typename T, typename... Args>
    Any(T*, Args&&... args) {
        T* target = (T*) &storage;
        if (sizeof(T) > sizeof(storage)) {
            storage = (uptr) Heap.alloc(sizeof(T));
            target = (T*) storage;
        }
        new (target) T{std::forward<Args>(args)...};
    }

    template <typename T>
    Any(T&& other) {
        if (sizeof(T) > sizeof(storage)) {
            storage = other.storage;
            other.storage = nullptr;
        } else {
            (T&) storage = std::move((T&) other.storage);
        }
    }

    template <typename T>
    T* get() {
        return (sizeof(T) <= sizeof(storage)) ? (T*) &storage : (T*) storage;
    }

    template <typename T>
    void destruct() {
        subst::destruct_by_member(get<T>());
        if (sizeof(T) > sizeof(storage)) {
            Heap.free((void*) storage);
        }
    }
};

//   ▄▄▄▄   ▄▄          ▄▄               ▄▄   ▄▄ ▄▄        ▄▄
//  ██  ▀▀ ▄██▄▄ ▄▄▄▄▄  ▄▄ ▄▄▄▄▄   ▄▄▄▄▄ ███▄███ ▄▄ ▄▄  ▄▄ ▄▄ ▄▄▄▄▄
//   ▀▀▀█▄  ██   ██  ▀▀ ██ ██  ██ ██  ██ ██▀█▀██ ██  ▀██▀  ██ ██  ██
//  ▀█▄▄█▀  ▀█▄▄ ██     ██ ██  ██ ▀█▄▄██ ██   ██ ██ ▄█▀▀█▄ ██ ██  ██
//                                 ▄▄▄█▀

template <typename Derived>
struct StringMixin {
    template <typename T>
    T to(const T& default_value = subst::create_default<T>()) const {
        return static_cast<const Derived*>(this)->view().to(default_value);
    }

    explicit operator bool() const {
        return (bool) static_cast<const Derived*>(this)->view();
    }

    bool is_empty() const {
        return static_cast<const Derived*>(this)->view().is_empty();
    }

    StringView sub_str(u32 start) const {
        return static_cast<const Derived*>(this)->view().sub_str(start);
    }
    StringView sub_str(u32 start, u32 num_bytes) const {
        return static_cast<const Derived*>(this)->view().sub_str(start, num_bytes);
    }

    StringView left(u32 num_bytes) const {
        return static_cast<const Derived*>(this)->view().left(num_bytes);
    }
    StringView shortened_by(u32 num_bytes) const {
        return static_cast<const Derived*>(this)->view().shortened_by(num_bytes);
    }
    StringView right(u32 num_bytes) const {
        return static_cast<const Derived*>(this)->view().right(num_bytes);
    }
    String operator+(StringView other) const;
    String operator*(u32 count) const;
    s32 find_byte(char match_byte, u32 start_pos = 0) const {
        return static_cast<const Derived*>(this)->view().find_byte(match_byte,
                                                                   start_pos);
    }
    template <typename MatchFuncOrChar>
    s32 find_byte(const MatchFuncOrChar& match_func_or_byte, u32 start_pos = 0) const {
        return static_cast<const Derived*>(this)->view().find_byte(match_func_or_byte,
                                                                   start_pos);
    }

    template <typename MatchFuncOrChar>
    s32 rfind_byte(const MatchFuncOrChar& match_func_or_byte, u32 start_pos) const {
        return static_cast<const Derived*>(this)->view().rfind_byte(match_func_or_byte,
                                                                    start_pos);
    }
    template <typename MatchFuncOrChar>
    s32 rfind_byte(const MatchFuncOrChar& match_func_or_byte) const {
        return static_cast<const Derived*>(this)->view().rfind_byte(
            match_func_or_byte, static_cast<const Derived*>(this)->num_bytes - 1);
    }

    bool starts_with(StringView arg) const {
        return static_cast<const Derived*>(this)->view().starts_with(arg);
    }
    bool ends_with(StringView arg) const {
        return static_cast<const Derived*>(this)->view().ends_with(arg);
    }

    StringView trim(bool (*match_func)(char) = is_white, bool left = true,
                    bool right = true) const {
        return static_cast<const Derived*>(this)->view().trim(match_func, left, right);
    }
    StringView ltrim(bool (*match_func)(char) = is_white) const {
        return static_cast<const Derived*>(this)->view().ltrim(match_func);
    }
    StringView rtrim(bool (*match_func)(char) = is_white) const {
        return static_cast<const Derived*>(this)->view().rtrim(match_func);
    }
    String join(ArrayView<const StringView> comps) const;
    auto split_byte(char sep) const {
        return static_cast<const Derived*>(this)->view().split_byte(sep);
    }
    String upper_asc() const;
    String lower_asc() const;
    String reversed_bytes() const;
    String filter_bytes(char (*filter_func)(char)) const;
    bool includes_null_terminator() const {
        return static_cast<const Derived*>(this)->view().includes_null_terminator();
    }
    HybridString with_null_terminator() const;
    StringView without_null_terminator() const {
        return static_cast<const Derived*>(this)->view().without_null_terminator();
    }
};

//   ▄▄▄▄   ▄▄          ▄▄
//  ██  ▀▀ ▄██▄▄ ▄▄▄▄▄  ▄▄ ▄▄▄▄▄   ▄▄▄▄▄
//   ▀▀▀█▄  ██   ██  ▀▀ ██ ██  ██ ██  ██
//  ▀█▄▄█▀  ▀█▄▄ ██     ██ ██  ██ ▀█▄▄██
//                                 ▄▄▄█▀

struct HybridString;

struct String : StringMixin<String> {
    using View = StringView;

    char* bytes = nullptr;
    u32 num_bytes = 0;

    String() = default;
    String(StringView other);
    String(const String& other) : String{other.view()} {
    }
    String(const char* s) : String{StringView{s}} {
    }
    template <typename U, typename = std::enable_if_t<std::is_same<U, char>::value>>
    String(const U& u) : String{StringView{&u, 1}} {
    }
    String(String&& other) : bytes{other.bytes}, num_bytes{other.num_bytes} {
        other.bytes = nullptr;
        other.num_bytes = 0;
    }
    String(HybridString&& other);

    ~String() {
        if (this->bytes) {
            Heap.free(this->bytes);
        }
    }

    template <typename = void>
    void operator=(StringView other) {
        char* bytes_to_free = this->bytes;
        new (this) String{other};
        if (bytes_to_free) {
            Heap.free(bytes_to_free);
        }
    }

    void operator=(const String& other) {
        this->~String();
        new (this) String{other.view()};
    }
    void operator=(String&& other) {
        this->~String();
        new (this) String{std::move(other)};
    }

    operator const StringView&() const {
        return reinterpret_cast<const StringView&>(*this);
    }
    const StringView& view() const {
        return reinterpret_cast<const StringView&>(*this);
    }

    void clear() {
        if (this->bytes) {
            Heap.free(this->bytes);
        }
        this->bytes = nullptr;
        this->num_bytes = 0;
    }

    void operator+=(StringView other) {
        *this = this->view() + other;
    }
    static String allocate(u32 num_bytes);
    void resize(u32 num_bytes);
    static String adopt(char* bytes, u32 num_bytes) {
        String str;
        str.bytes = bytes;
        str.num_bytes = num_bytes;
        return str;
    }

    char* release() {
        char* r = this->bytes;
        this->bytes = nullptr;
        this->num_bytes = 0;
        return r;
    }

    template <typename... Args>
    static String format(StringView fmt, const Args&... args);

    const char& operator[](u32 index) const {
        PLY_ASSERT(index < this->num_bytes);
        return this->bytes[index];
    }
    char& operator[](u32 index) {
        PLY_ASSERT(index < this->num_bytes);
        return this->bytes[index];
    }

    const char& back(s32 ofs = -1) const {
        PLY_ASSERT(u32(-ofs - 1) < this->num_bytes);
        return this->bytes[this->num_bytes + ofs];
    }
    char& back(s32 ofs = -1) {
        PLY_ASSERT(u32(-ofs - 1) < this->num_bytes);
        return this->bytes[this->num_bytes + ofs];
    }
};

// This makes View<String> an alias for StringView. In C++14, the only way to achieve
// this
namespace traits {
template <>
struct View<String> {
    using Type = StringView;
};
} // namespace traits

namespace impl {
template <>
struct InitListType<String> {
    using Type = StringView;
};
} // namespace impl

template <typename Derived>
String StringMixin<Derived>::operator+(StringView other) const {
    return static_cast<const Derived*>(this)->view() + other;
}

template <typename Derived>
String StringMixin<Derived>::operator*(u32 count) const {
    return static_cast<const Derived*>(this)->view() * count;
}

template <typename Derived>
String StringMixin<Derived>::join(ArrayView<const StringView> comps) const {
    return static_cast<const Derived*>(this)->view().join(comps);
}

template <typename Derived>
String StringMixin<Derived>::upper_asc() const {
    return static_cast<const Derived*>(this)->view().upper_asc();
}

template <typename Derived>
String StringMixin<Derived>::lower_asc() const {
    return static_cast<const Derived*>(this)->view().lower_asc();
}

template <typename Derived>
String StringMixin<Derived>::reversed_bytes() const {
    return static_cast<const Derived*>(this)->view().reversed_bytes();
}

template <typename Derived>
String StringMixin<Derived>::filter_bytes(char (*filter_func)(char)) const {
    return static_cast<const Derived*>(this)->view().filter_bytes(filter_func);
}

//  ▄▄  ▄▄        ▄▄            ▄▄     ▄▄  ▄▄▄▄   ▄▄          ▄▄
//  ██  ██ ▄▄  ▄▄ ██▄▄▄  ▄▄▄▄▄  ▄▄  ▄▄▄██ ██  ▀▀ ▄██▄▄ ▄▄▄▄▄  ▄▄ ▄▄▄▄▄   ▄▄▄▄▄
//  ██▀▀██ ██  ██ ██  ██ ██  ▀▀ ██ ██  ██  ▀▀▀█▄  ██   ██  ▀▀ ██ ██  ██ ██  ██
//  ██  ██ ▀█▄▄██ ██▄▄█▀ ██     ██ ▀█▄▄██ ▀█▄▄█▀  ▀█▄▄ ██     ██ ██  ██ ▀█▄▄██
//          ▄▄▄█▀                                                        ▄▄▄█▀

struct HybridString : StringMixin<HybridString> {
    char* bytes;
    u32 is_owner : 1;
    u32 num_bytes : 31;

    HybridString() : bytes{nullptr}, is_owner{0}, num_bytes{0} {
    }
    HybridString(StringView view)
        : bytes{const_cast<char*>(view.bytes)}, is_owner{0}, num_bytes{view.num_bytes} {
        PLY_ASSERT(view.num_bytes < (1u << 30));
    }

    HybridString(const String& str) : HybridString{str.view()} {
    }
    HybridString(String&& str) {
        this->is_owner = 1;
        PLY_ASSERT(str.num_bytes < (1u << 30));
        this->num_bytes = str.num_bytes;
        this->bytes = str.release();
    }

    HybridString(HybridString&& other)
        : bytes{other.bytes}, is_owner{other.is_owner}, num_bytes{other.num_bytes} {
        other.bytes = nullptr;
        other.is_owner = 0;
        other.num_bytes = 0;
    }

    HybridString(const HybridString& other);

    HybridString(const char* s)
        : bytes{const_cast<char*>(s)}, is_owner{0},
          num_bytes{(u32) std::char_traits<char>::length(s)} {
        PLY_ASSERT(s[this->num_bytes] ==
                   0); // Sanity check; num_bytes must fit in 31-bit field
    }

    ~HybridString() {
        if (this->is_owner) {
            Heap.free(this->bytes);
        }
    }

    void operator=(HybridString&& other) {
        this->~HybridString();
        new (this) HybridString(std::move(other));
    }

    void operator=(const HybridString& other) {
        this->~HybridString();
        new (this) HybridString(other);
    }

    operator StringView() const {
        return {this->bytes, this->num_bytes};
    }

    StringView view() const {
        return {this->bytes, this->num_bytes};
    }
};

template <typename Derived>
HybridString StringMixin<Derived>::with_null_terminator() const {
    return static_cast<const Derived*>(this)->view().with_null_terminator();
}

//   ▄▄▄▄
//  ██  ██ ▄▄▄▄▄  ▄▄▄▄▄   ▄▄▄▄  ▄▄  ▄▄
//  ██▀▀██ ██  ▀▀ ██  ▀▀  ▄▄▄██ ██  ██
//  ██  ██ ██     ██     ▀█▄▄██ ▀█▄▄██
//                               ▄▄▄█▀

struct BaseArray {
    void* items = nullptr;
    u32 num_items = 0;
    u32 allocated = 0;

    BaseArray() = default;

    void alloc(u32 num_items, u32 item_size);
    void realloc(u32 num_items, u32 item_size);
    void free();
    void reserve(u32 num_items, u32 item_size);
    void reserve_increment(u32 item_size);
    void truncate(u32 item_size);
};

template <typename T>
class Array {
private:
    T* items;
    u32 num_items_;
    u32 allocated;

    // T cannot be const
    PLY_STATIC_ASSERT(!std::is_const<T>::value);

    // Arrays of C-style arrays such as Array<int[2]> are not allowed.
    // One reason is that placement new doesn't work (to copy/move/construct new items).
    // Another reason is that it confuses the .natvis debug visualizer in Visual Studio.
    // Make an Array of FixedArray<> instead.
    PLY_STATIC_ASSERT(!std::is_array<T>::value);

    template <typename>
    friend class Array;

    Array(T* items, u32 num_items, u32 allocator)
        : items{items}, num_items_{num_items}, allocated{allocated} {
    }

public:
    // Constructors
    Array() : items{nullptr}, num_items_{0}, allocated{0} {
    }
    Array(const Array& other) {
        ((BaseArray*) this)->alloc(other.num_items_, (u32) sizeof(T));
        subst::unsafe_construct_array_from(this->items, other.items, other.num_items_);
    }
    Array(Array&& other)
        : items{other.items}, num_items_{other.num_items_}, allocated{other.allocated} {
        other.items = nullptr;
        other.num_items_ = 0;
        other.allocated = 0;
    }
    Array(InitList<T> init) {
        u32 init_size = safe_demote<u32>(init.size());
        ((BaseArray*) this)->alloc(init_size, (u32) sizeof(T));
        subst::construct_array_from(this->items, init.begin(), init_size);
    }
    template <typename Other, typename U = impl::ArrayViewType<Other>>
    Array(Other&& other) {
        ((BaseArray*) this)->alloc(ArrayView<U>{other}.num_items, (u32) sizeof(T));
        impl::move_or_copy_construct(this->items, std::forward<Other>(other));
    }
    ~Array() {
        PLY_STATIC_ASSERT(sizeof(Array) == sizeof(BaseArray));
        subst::destruct_array(this->items, this->num_items_);
        Heap.free(this->items);
    }

    void operator=(const Array& other) {
        ArrayView<T> array_to_free = {this->items, this->num_items_};
        new (this) Array{other};
        subst::destruct_array(array_to_free.items, array_to_free.num_items);
        Heap.free(array_to_free.items);
    }

    void operator=(Array&& other) {
        ArrayView<T> array_to_free = {this->items, this->num_items_};
        new (this) Array{std::move(other)};
        subst::destruct_array(array_to_free.items, array_to_free.num_items);
        Heap.free(array_to_free.items);
    }
    void operator=(std::initializer_list<T> init) {
        subst::destruct_array(this->items, this->num_items_);
        u32 init_size = safe_demote<u32>(init.size());
        ((BaseArray*) this)->realloc(init_size, (u32) sizeof(T));
        subst::unsafe_construct_array_from(this->items, init.begin(), init_size);
    }
    template <typename Other, typename = impl::ArrayViewType<Other>>
    void operator=(Other&& other) {
        ArrayView<T> array_to_free = {this->items, this->num_items_};
        new (this) Array{std::forward<Other>(other)};
        subst::destruct_array(array_to_free.items, array_to_free.num_items);
        Heap.free(array_to_free.items);
    }

    T& operator[](u32 index) {
        PLY_ASSERT(index < this->num_items_);
        return this->items[index];
    }
    const T& operator[](u32 index) const {
        PLY_ASSERT(index < this->num_items_);
        return this->items[index];
    }

    T* get(u32 index = 0) {
        PLY_ASSERT(index < this->num_items_);
        return this->items + index;
    }
    const T* get(u32 index = 0) const {
        PLY_ASSERT(index < this->num_items_);
        return this->items + index;
    }

    T& back(s32 offset = -1) {
        PLY_ASSERT(offset < 0 && u32(-offset) <= this->num_items_);
        return this->items[this->num_items_ + offset];
    }
    const T& back(s32 offset = -1) const {
        PLY_ASSERT(offset < 0 && u32(-offset) <= this->num_items_);
        return this->items[this->num_items_ + offset];
    }

    T* begin() const {
        return this->items;
    }
    T* end() const {
        return this->items + this->num_items_;
    }

    explicit operator bool() const {
        return this->num_items_ > 0;
    }
    bool is_empty() const {
        return this->num_items_ == 0;
    }
    u32 num_items() const {
        return this->num_items_;
    }
    u32 size_bytes() const {
        return this->num_items_ * (u32) sizeof(T);
    }
    void clear() {
        subst::destruct_array(this->items, this->num_items_);
        Heap.free(this->items);
        this->items = nullptr;
        this->num_items_ = 0;
        this->allocated = 0;
    }

    void reserve(u32 num_items) {
        ((BaseArray*) this)->reserve(num_items, (u32) sizeof(T));
    }
    void resize(u32 num_items) {
        if (num_items < this->num_items_) {
            subst::destruct_array(this->items + num_items,
                                  this->num_items_ - num_items);
        }
        ((BaseArray*) this)->reserve(num_items, (u32) sizeof(T));
        if (num_items > this->num_items_) {
            subst::construct_array(this->items + this->num_items_,
                                   num_items - this->num_items_);
        }
        this->num_items_ = num_items;
    }

    void truncate() {
        ((BaseArray*) this)->truncate((u32) sizeof(T));
    }

    T& append(T&& item) {
        // The argument must not be a reference to an existing item in the array:
        PLY_ASSERT((&item < this->items) || (&item >= this->items + this->num_items_));
        if (this->num_items_ >= this->allocated) {
            ((BaseArray*) this)->reserve_increment((u32) sizeof(T));
        }
        T* result = new (this->items + this->num_items_) T{std::move(item)};
        this->num_items_++;
        return *result;
    }
    T& append(const T& item) {
        // The argument must not be a reference to an existing item in the array:
        PLY_ASSERT((&item < this->items) || (&item >= this->items + this->num_items_));
        if (this->num_items_ >= this->allocated) {
            ((BaseArray*) this)->reserve_increment((u32) sizeof(T));
        }
        T* result = new (this->items + this->num_items_) T{item};
        this->num_items_++;
        return *result;
    }
    template <typename... Args>
    T& append(Args&&... args) {
        if (this->num_items_ >= this->allocated) {
            ((BaseArray*) this)->reserve_increment((u32) sizeof(T));
        }
        T* result = new (this->items + this->num_items_) T{std::forward<Args>(args)...};
        this->num_items_++;
        return *result;
    }

    void extend(InitList<T> init) {
        u32 init_size = safe_demote<u32>(init.size());
        ((BaseArray*) this)->reserve(this->num_items_ + init_size, (u32) sizeof(T));
        subst::construct_array_from(this->items + this->num_items_, init.begin(),
                                    init_size);
        this->num_items_ += init_size;
    }
    template <typename Other, typename U = impl::ArrayViewType<Other>>
    void extend(Other&& other) {
        u32 num_other_items = ArrayView<U>{other}.num_items;
        ((BaseArray&) *this)
            .reserve(this->num_items_ + num_other_items, (u32) sizeof(T));
        impl::move_or_copy_construct(this->items + this->num_items_,
                                     std::forward<Other>(other));
        this->num_items_ += num_other_items;
    }
    void move_extend(ArrayView<T> other) {
        // The argument must not be a subview into the array itself:
        PLY_ASSERT((other.end() <= this->items) || (other.items >= this->end()));
        ((BaseArray&) *this)
            .reserve(this->num_items_ + other.num_items, (u32) sizeof(T));
        subst::move_construct_array(this->items + this->num_items_, other.items,
                                    other.num_items);
        this->num_items_ += other.num_items;
    }

    void pop(u32 count = 1) {
        PLY_ASSERT(count <= this->num_items_);
        resize(this->num_items_ - count);
    }
    T& insert(u32 pos, u32 count = 1) {
        PLY_ASSERT(pos <= this->num_items_);
        ((BaseArray*) this)->reserve(this->num_items_ + count, (u32) sizeof(T));
        memmove(static_cast<void*>(this->items + pos + count),
                static_cast<const void*>(this->items + pos),
                (this->num_items_ - pos) * sizeof(T)); // Underlying type is relocatable
        subst::construct_array(this->items + pos, count);
        this->num_items_ += count;
        return this->items[pos];
    }

    void erase(u32 pos, u32 count = 1) {
        PLY_ASSERT(pos + count <= this->num_items_);
        subst::destruct_array(this->items + pos, count);
        memmove(static_cast<void*>(this->items + pos),
                static_cast<const void*>(this->items + pos + count),
                (this->num_items_ - (pos + count)) *
                    sizeof(T)); // Underlying type is relocatable
        this->num_items_ -= count;
    }
    void erase_quick(u32 pos) {
        PLY_ASSERT(pos < this->num_items_);
        this->items[pos].~T();
        memcpy(static_cast<void*>(this->items + pos),
               static_cast<const void*>(this->items + (this->num_items_ - 1)),
               sizeof(T));
        this->num_items_--;
    }
    void erase_quick(u32 pos, u32 count) {
        PLY_ASSERT(pos + count <= this->num_items_);
        subst::destruct_array(this->items + pos, count);
        memmove(static_cast<void*>(this->items + pos),
                static_cast<const void*>(this->items + this->num_items_ - count),
                count * sizeof(T)); // Underlying type is relocatable
        this->num_items_ -= count;
    }

    static Array adopt(T* items, u32 num_items) {
        return {items, num_items, num_items};
    }
    T* release() {
        T* items = this->items;
        this->items = nullptr;
        this->num_items_ = 0;
        this->allocated = 0;
        return items;
    }

    template <typename Arr0, typename Arr1, typename, typename>
    friend auto operator+(Arr0&& a, Arr1&& b);

    ArrayView<T> view() {
        return {this->items, this->num_items_};
    }
    ArrayView<const T> view() const {
        return {this->items, this->num_items_};
    }
    operator ArrayView<T>() {
        return {this->items, this->num_items_};
    }
    operator ArrayView<const T>() const {
        return {this->items, this->num_items_};
    }

    StringView string_view() const {
        return {(const char*) this->items,
                safe_demote<u32>(this->num_items_ * sizeof(T))};
    }
    MutStringView mutable_string_view() const {
        return {(char*) this->items, safe_demote<u32>(this->num_items_ * sizeof(T))};
    }

    ArrayView<T> sub_view(u32 start) {
        return view().sub_view(start);
    }
    ArrayView<const T> sub_view(u32 start) const {
        return view().sub_view(start);
    }
    ArrayView<T> sub_view(u32 start, u32 num_items_) {
        return view().sub_view(start, num_items_);
    }
    ArrayView<const T> sub_view(u32 start, u32 num_items_) const {
        return view().sub_view(start, num_items_);
    }
};

namespace impl {
template <typename T>
struct InitListType<Array<T>> {
    using Type = ArrayView<const T>;
};

template <typename T>
struct ArrayTraits<Array<T>> {
    using ItemType = T;
    static constexpr bool IsOwner = true;
};
} // namespace impl

template <typename Arr0, typename Arr1, typename T0 = impl::ArrayViewType<Arr0>,
          typename T1 = impl::ArrayViewType<Arr1>>
auto operator+(Arr0&& a, Arr1&& b) {
    u32 num_items_a = ArrayView<T0>{a}.num_items;
    u32 num_items_b = ArrayView<T1>{b}.num_items;

    Array<std::remove_const_t<T0>> result;
    ((BaseArray&) result).alloc(num_items_a + num_items_b, (u32) sizeof(T0));
    impl::move_or_copy_construct(result.items, std::forward<Arr0>(a));
    impl::move_or_copy_construct(result.items + num_items_a, std::forward<Arr1>(b));
    return result;
}

//  ▄▄▄▄▄ ▄▄                   ▄▄  ▄▄▄▄
//  ██    ▄▄ ▄▄  ▄▄  ▄▄▄▄   ▄▄▄██ ██  ██ ▄▄▄▄▄  ▄▄▄▄▄   ▄▄▄▄  ▄▄  ▄▄
//  ██▀▀  ██  ▀██▀  ██▄▄██ ██  ██ ██▀▀██ ██  ▀▀ ██  ▀▀  ▄▄▄██ ██  ██
//  ██    ██ ▄█▀▀█▄ ▀█▄▄▄  ▀█▄▄██ ██  ██ ██     ██     ▀█▄▄██ ▀█▄▄██
//                                                             ▄▄▄█▀

template <typename T, u32 Size>
struct FixedArray {
#if PLY_COMPILER_MSVC
#pragma warning(push)
#pragma warning( \
    disable : 4200) // nonstandard extension used: zero-sized array in struct/union
#endif
    T items[Size];
#if PLY_COMPILER_MSVC
#pragma warning(pop)
#endif

    FixedArray() = default;

    FixedArray(InitList<T> args) {
        PLY_ASSERT(Size == args.size());
        subst::construct_array_from(this->items, args.begin(), Size);
    }

    template <typename... Args>
    FixedArray(Args&&... args) {
        PLY_STATIC_ASSERT(Size == sizeof...(Args));
        impl::InitItems<T>::init(items, std::forward<Args>(args)...);
    }

    constexpr u32 num_items() const {
        return Size;
    }

    T& operator[](u32 i) {
        PLY_ASSERT(i < Size);
        return items[i];
    }

    const T& operator[](u32 i) const {
        PLY_ASSERT(i < Size);
        return items[i];
    }

    ArrayView<T> view() {
        return {items, Size};
    }

    ArrayView<const T> view() const {
        return {items, Size};
    }

    operator ArrayView<T>() {
        return {items, Size};
    }

    operator ArrayView<const T>() const {
        return {items, Size};
    }

    MutStringView mutable_string_view() {
        return {reinterpret_cast<char*>(items), safe_demote<u32>(Size * sizeof(T))};
    }

    StringView string_view() const {
        return {reinterpret_cast<const char*>(items),
                safe_demote<u32>(Size * sizeof(T))};
    }

    T* begin() {
        return items;
    }

    T* end() {
        return items + Size;
    }

    const T* begin() const {
        return items;
    }

    const T* end() const {
        return items + Size;
    }
};

namespace impl {
template <typename T, u32 Size>
struct InitListType<FixedArray<T, Size>> {
    using Type = ArrayView<const T>;
};

template <typename T, u32 Size>
struct ArrayTraits<FixedArray<T, Size>> {
    using ItemType = T;
    static constexpr bool IsOwner = true;
};
} // namespace impl

//  ▄▄▄▄▄
//  ██    ▄▄  ▄▄ ▄▄▄▄▄   ▄▄▄▄
//  ██▀▀  ██  ██ ██  ██ ██
//  ██    ▀█▄▄██ ██  ██ ▀█▄▄▄
//

template <typename>
class Func;

template <typename Return, typename... Args>
class Func<Return(Args...)> {
private:
    typedef Return Handler(void*, Args...);

    Handler* handler = nullptr;
    void* stored_arg = nullptr;

public:
    Func() = default;

    PLY_INLINE Func(const Func& other)
        : handler{other.handler}, stored_arg{other.stored_arg} {
    }

    template <typename T>
    PLY_INLINE Func(Return (*handler)(T*, Args...), T* stored_arg)
        : handler{(Handler*) handler}, stored_arg{(void*) stored_arg} {
    }

    template <typename T>
    PLY_INLINE Func(Return (T::*handler)(Args...), T* target)
        : stored_arg{(void*) target} {
        this->handler = [this](void* target, Args... args) {
            return ((T*) target)->*(this->hidden_arg)(std::forward<Args>(args)...);
        };
    }

    // Support lambda expressions
    template <
        typename Callable,
        typename = void_t<decltype(std::declval<Callable>()(std::declval<Args>()...))>>
    Func(const Callable& callable) : stored_arg{(void*) &callable} {
        this->handler = [](void* callable, Args... args) -> Return {
            return (*(const Callable*) callable)(std::forward<Args>(args)...);
        };
    }

    PLY_INLINE void operator=(const Func& other) {
        this->handler = other.handler;
        this->stored_arg = other.stored_arg;
    }

    PLY_INLINE explicit operator bool() const {
        return this->handler != nullptr;
    }

    template <typename... CallArgs>
    PLY_INLINE Return operator()(CallArgs&&... args) const {
        if (!this->handler)
            return subst::create_default<Return>();
        PLY_PUN_SCOPE
        return this->handler(this->stored_arg, std::forward<CallArgs>(args)...);
    }
};

//   ▄▄▄▄                             ▄▄
//  ██  ██ ▄▄    ▄▄ ▄▄▄▄▄   ▄▄▄▄   ▄▄▄██
//  ██  ██ ██ ██ ██ ██  ██ ██▄▄██ ██  ██
//  ▀█▄▄█▀  ██▀▀██  ██  ██ ▀█▄▄▄  ▀█▄▄██
//

template <typename T>
class Owned {
private:
    template <typename>
    friend class Owned;
    T* ptr;

public:
    PLY_INLINE Owned() : ptr{nullptr} {
    }
    PLY_INLINE Owned(T* ptr) : ptr{ptr} { // FIXME: Replace with Owned<T>::adopt()
    }
    PLY_INLINE Owned(Owned&& other) : ptr{other.release()} {
    }
    template <typename Derived,
              typename std::enable_if_t<std::is_base_of<T, Derived>::value, int> = 0>
    PLY_INLINE Owned(Owned<Derived>&& other) : ptr{other.release()} {
    }
    PLY_INLINE ~Owned() {
        subst::destroy_by_member(this->ptr);
    }
    static PLY_INLINE Owned adopt(T* ptr) {
        Owned result;
        result.ptr = ptr;
        return result;
    }
    PLY_INLINE void operator=(Owned&& other) {
        PLY_ASSERT(!this->ptr || this->ptr != other.ptr);
        subst::destroy_by_member(this->ptr);
        this->ptr = other.release();
    }
    template <typename Derived,
              typename std::enable_if_t<std::is_base_of<T, Derived>::value, int> = 0>
    PLY_INLINE void operator=(Owned<Derived>&& other) {
        PLY_ASSERT(!this->ptr || this->ptr != other.ptr);
        subst::destroy_by_member(this->ptr);
        this->ptr = other.release();
    }
    PLY_INLINE void operator=(T* ptr) {
        PLY_ASSERT(!this->ptr || this->ptr != ptr);
        subst::destroy_by_member(this->ptr);
        this->ptr = ptr;
    }
    template <typename... Args>
    static PLY_INLINE Owned create(Args&&... args) {
        return Owned::adopt(new T{std::forward<Args>(args)...});
    }
    PLY_INLINE T* operator->() const {
        return this->ptr;
    }
    PLY_INLINE operator T*() const {
        return this->ptr;
    }
    PLY_INLINE T* get() const {
        return this->ptr;
    }
    PLY_INLINE T* release() {
        T* ptr = this->ptr;
        this->ptr = nullptr;
        return ptr;
    }
    PLY_INLINE void clear() {
        subst::destroy_by_member(this->ptr);
        this->ptr = nullptr;
    }
};

//  ▄▄▄▄▄           ▄▄▄
//  ██  ██  ▄▄▄▄   ██    ▄▄▄▄  ▄▄▄▄▄   ▄▄▄▄  ▄▄▄▄▄   ▄▄▄▄  ▄▄▄▄
//  ██▀▀█▄ ██▄▄██ ▀██▀▀ ██▄▄██ ██  ▀▀ ██▄▄██ ██  ██ ██    ██▄▄██
//  ██  ██ ▀█▄▄▄   ██   ▀█▄▄▄  ██     ▀█▄▄▄  ██  ██ ▀█▄▄▄ ▀█▄▄▄
//

template <typename Mixin>
class RefCounted {
private:
    Atomic<s32> m_refCount = 0;

public:
    PLY_INLINE void inc_ref() {
        s32 old_count = m_refCount.fetch_add(1, Relaxed);
        PLY_ASSERT(old_count >= 0 && old_count < UINT16_MAX);
        PLY_UNUSED(old_count);
    }
    PLY_INLINE void dec_ref() {
        s32 old_count = m_refCount.fetch_sub(1, Relaxed);
        PLY_ASSERT(old_count >= 1 && old_count < UINT16_MAX);
        if (old_count == 1) {
            static_cast<Mixin*>(this)->on_ref_count_zero();
        }
    }
    PLY_INLINE s32 get_ref_count() const {
        return m_refCount.load(Relaxed);
    }

    // Make derived classes assignable without copying the other object's refcount:
    PLY_INLINE RefCounted() {
    }
    PLY_INLINE RefCounted(const RefCounted&) {
    }
    PLY_INLINE void operator=(const RefCounted&) {
    }
};

template <typename T>
class Reference {
private:
    T* ptr;

public:
    PLY_INLINE Reference() : ptr(nullptr) {
    }
    PLY_INLINE Reference(T* ptr) : ptr(ptr) {
        if (this->ptr)
            this->ptr->inc_ref();
    }
    PLY_INLINE Reference(const Reference& ref) : ptr(ref.ptr) {
        if (this->ptr)
            this->ptr->inc_ref();
    }
    PLY_INLINE Reference(Reference&& ref) : ptr(ref.ptr) {
        ref.ptr = nullptr;
    }
    PLY_INLINE ~Reference() {
        if (this->ptr)
            this->ptr->dec_ref();
    }
    PLY_INLINE T* operator->() const {
        return this->ptr;
    }
    PLY_INLINE operator T*() const {
        return this->ptr;
    }
    PLY_INLINE T* get() const {
        return this->ptr;
    }
    PLY_INLINE void operator=(T* ptr) {
        T* old_ptr = this->ptr;
        this->ptr = ptr;
        if (this->ptr)
            this->ptr->inc_ref();
        if (old_ptr)
            old_ptr->dec_ref();
    }

    PLY_INLINE void operator=(const Reference& ref) {
        T* old_ptr = this->ptr;
        this->ptr = ref.ptr;
        if (this->ptr)
            this->ptr->inc_ref();
        if (old_ptr)
            old_ptr->dec_ref();
    }
    PLY_INLINE void operator=(Reference&& ref) {
        if (this->ptr)
            this->ptr->dec_ref();
        this->ptr = ref.ptr;
        ref.ptr = nullptr;
    }
    PLY_INLINE explicit operator bool() const {
        return this->ptr != nullptr;
    }
    PLY_INLINE T* release() {
        T* ptr = this->ptr;
        this->ptr = nullptr;
        return ptr;
    };
    PLY_INLINE void clear() {
        if (this->ptr)
            this->ptr->dec_ref();
        this->ptr = nullptr;
    }
    PLY_INLINE bool is_empty() const {
        return this->ptr == nullptr;
    }
};

//  ▄▄    ▄▄               ▄▄     ▄▄▄▄▄           ▄▄▄
//  ██ ▄▄ ██  ▄▄▄▄   ▄▄▄▄  ██  ▄▄ ██  ██  ▄▄▄▄   ██
//  ▀█▄██▄█▀ ██▄▄██  ▄▄▄██ ██▄█▀  ██▀▀█▄ ██▄▄██ ▀██▀▀
//   ██▀▀██  ▀█▄▄▄  ▀█▄▄██ ██ ▀█▄ ██  ██ ▀█▄▄▄   ██
//

template <class Mixin>
class DualRefCounted {
private:
    // high 16 bits: Strong ref count
    // low 16 bits: Weak ref count
    Atomic<u32> m_dualRefCount = 0;

public:
    void inc_ref() {
        u32 old_dual_count = m_dualRefCount.fetch_add(1, Relaxed);
        PLY_ASSERT((old_dual_count & 0xffffu) < INT16_MAX);
        PLY_UNUSED(old_dual_count);
    }

    void dec_ref() {
        u32 old_dual_count = m_dualRefCount.fetch_sub(1, Relaxed);
        PLY_ASSERT((old_dual_count & 0xffffu) > 0);
        PLY_ASSERT((old_dual_count & 0xffffu) < INT16_MAX);
        if ((old_dual_count & 0xffffu) == 1) {
            static_cast<Mixin*>(this)->on_partial_ref_count_zero();
            if (old_dual_count == 1) {
                static_cast<Mixin*>(this)->on_full_ref_count_zero();
            }
        }
    }

    void inc_weak_ref() {
        u32 old_dual_count = m_dualRefCount.fetch_add(0x10000, Relaxed);
        PLY_ASSERT((old_dual_count & 0xffffu) > 0); // Must have some strong refs
        PLY_ASSERT((old_dual_count >> 16) < INT16_MAX);
        PLY_UNUSED(old_dual_count);
    }

    void dec_weak_ref() {
        u32 old_dual_count = m_dualRefCount.fetch_sub(0x10000, Relaxed);
        PLY_ASSERT((old_dual_count >> 16) > 0);
        PLY_ASSERT((old_dual_count >> 16) < INT16_MAX);
        PLY_UNUSED(old_dual_count);
        if ((old_dual_count >> 16) == 1) {
            if (old_dual_count == 0x10000) {
                static_cast<Mixin*>(this)->on_full_ref_count_zero();
            }
        }
    }

    u32 get_ref_count() const {
        return m_dualRefCount.load(Relaxed) & 0xffffu;
    }

    u32 get_weak_ref_count() const {
        return m_dualRefCount.load(Relaxed) >> 16;
    }

    void operator=(const DualRefCounted&) = delete;
};

template <class T>
class WeakRef {
private:
    T* m_ptr;

public:
    WeakRef() : m_ptr{nullptr} {
    }

    WeakRef(T* ptr) : m_ptr{ptr} {
        if (m_ptr)
            m_ptr->inc_weak_ref();
    }

    WeakRef(const WeakRef& ref) : m_ptr{ref.m_ptr} {
        if (m_ptr)
            m_ptr->inc_weak_ref();
    }

    WeakRef(WeakRef&& ref) : m_ptr{ref.m_ptr} {
        ref.m_ptr = nullptr;
    }

    ~WeakRef() {
        if (m_ptr)
            m_ptr->dec_weak_ref();
    }

    T* operator->() const {
        return m_ptr;
    }

    operator T*() const {
        return m_ptr;
    }

    void set_from_null(T* ptr) {
        PLY_ASSERT(!m_ptr);
        PLY_ASSERT(ptr);
        m_ptr = ptr;
        ptr->inc_weak_ref();
    }

    void operator=(T* ptr) {
        T* old_ptr = m_ptr;
        m_ptr = ptr;
        if (m_ptr)
            m_ptr->inc_weak_ref();
        if (old_ptr)
            old_ptr->dec_weak_ref();
    }

    void operator=(const WeakRef& ref) {
        T* old_ptr = m_ptr;
        m_ptr = ref.m_ptr;
        if (m_ptr)
            m_ptr->inc_weak_ref();
        if (old_ptr)
            old_ptr->dec_weak_ref();
    }

    void operator=(WeakRef&& ref) {
        if (m_ptr)
            m_ptr->dec_weak_ref();
        m_ptr = ref.m_ptr;
        ref.m_ptr = nullptr;
    }
};

//  ▄▄▄▄▄▄               ▄▄▄
//    ██   ▄▄  ▄▄ ▄▄▄▄▄   ██   ▄▄▄▄
//    ██   ██  ██ ██  ██  ██  ██▄▄██
//    ██   ▀█▄▄██ ██▄▄█▀ ▄██▄ ▀█▄▄▄
//                ██

template <typename...>
struct Tuple;

template <typename A, typename B>
static constexpr bool IsSameOrConst() {
    return std::is_same<A, B>::value || std::is_same<A, const B>::value;
}

//-----------------------------------------------------
// 2-tuple
//-----------------------------------------------------
template <typename T1, typename T2>
struct Tuple<T1, T2> {
    T1 first;
    T2 second;

    Tuple() = default;
    // Note: We don't bother using SFINAE to conditionally disable constructors
    // depending on whether types are actually copy-constructible or move-constructible.
    // We also don't bother to support constructing from types other than const T& or
    // T&&. May need to make some of these conditionally explicit, though.
    Tuple(const T1& first, const T2& second) : first{first}, second{second} {
    }
    Tuple(const T1& first, T2&& second) : first{first}, second{std::move(second)} {
    }
    Tuple(T1&& first, const T2& second) : first{std::move(first)}, second{second} {
    }
    Tuple(T1&& first, T2&& second)
        : first{std::move(first)}, second{std::move(second)} {
    }
    template <typename U1, typename U2>
    Tuple(const Tuple<U1, U2>& other) : first{other.first}, second{other.second} {
    }
    template <typename U1, typename U2>
    Tuple(Tuple<U1, U2>&& other)
        : first{std::move(other.first)}, second{std::move(other.second)} {
    }
    template <typename U1, typename U2>
    void operator=(const Tuple<U1, U2>& other) {
        first = other.first;
        second = other.second;
    }
    template <typename U1, typename U2>
    void operator=(Tuple<U1, U2>&& other) {
        first = std::move(other.first);
        second = std::move(other.second);
    }
    bool operator==(const Tuple& other) const {
        return first == other.first && second == other.second;
    }
    template <
        typename U1, typename U2,
        std::enable_if_t<IsSameOrConst<T1, U1>() && IsSameOrConst<T2, U2>(), int> = 0>
    operator Tuple<U1, U2>&() {
        return reinterpret_cast<Tuple<U1, U2>&>(*this);
    }
    template <
        typename U1, typename U2,
        std::enable_if_t<IsSameOrConst<T1, U1>() && IsSameOrConst<T2, U2>(), int> = 0>
    operator const Tuple<U1, U2>&() const {
        return reinterpret_cast<const Tuple<U1, U2>&>(*this);
    }
};

//-----------------------------------------------------
// 3-tuple
//-----------------------------------------------------
template <typename T1, typename T2, typename T3>
struct Tuple<T1, T2, T3> {
    T1 first;
    T2 second;
    T3 third;

    Tuple() = default;
    Tuple(const T1& first, const T2& second, const T3& third)
        : first{first}, second{second}, third{third} {
    }
    Tuple(const T1& first, const T2& second, T3&& third)
        : first{first}, second{second}, third{std::move(third)} {
    }
    Tuple(const T1& first, T2&& second, const T3& third)
        : first{first}, second{std::move(second)}, third{third} {
    }
    Tuple(const T1& first, T2&& second, T3&& third)
        : first{first}, second{std::move(second)}, third{std::move(third)} {
    }
    Tuple(T1&& first, const T2& second, const T3& third)
        : first{std::move(first)}, second{second}, third{third} {
    }
    Tuple(T1&& first, const T2& second, T3&& third)
        : first{std::move(first)}, second{second}, third{std::move(third)} {
    }
    Tuple(T1&& first, T2&& second, const T3& third)
        : first{std::move(first)}, second{std::move(second)}, third{third} {
    }
    Tuple(T1&& first, T2&& second, T3&& third)
        : first{std::move(first)}, second{std::move(second)}, third{std::move(third)} {
    }
    template <typename U1, typename U2, typename U3>
    Tuple(const Tuple<U1, U2, U3>& other)
        : first{other.first}, second{other.second}, third{other.third} {
    }
    template <typename U1, typename U2, typename U3>
    Tuple(Tuple<U1, U2, U3>&& other)
        : first{std::move(other.first)}, second{std::move(other.second)},
          third{std::move(other.third)} {
    }
    template <typename U1, typename U2, typename U3>
    void operator=(const Tuple<U1, U2, U3>& other) {
        first = other.first;
        second = other.second;
        third = other.third;
    }
    template <typename U1, typename U2, typename U3>
    void operator=(Tuple<U1, U2, U3>&& other) {
        first = std::move(other.first);
        second = std::move(other.second);
        third = std::move(other.third);
    }
    bool operator==(const Tuple& other) const {
        return first == other.first && second == other.second && third == other.third;
    }
    template <typename U1, typename U2, typename U3,
              std::enable_if_t<IsSameOrConst<T1, U1>() && IsSameOrConst<T2, U2>() &&
                                   IsSameOrConst<T3, U3>(),
                               int> = 0>
    operator Tuple<U1, U2, U3>&() {
        return reinterpret_cast<Tuple<U1, U2, U3>&>(*this);
    }
    template <typename U1, typename U2, typename U3,
              std::enable_if_t<IsSameOrConst<T1, U1>() && IsSameOrConst<T2, U2>() &&
                                   IsSameOrConst<T3, U3>(),
                               int> = 0>
    operator const Tuple<U1, U2, U3>&() const {
        return reinterpret_cast<const Tuple<U1, U2, U3>&>(*this);
    }
};

//  ▄▄  ▄▄                               ▄▄
//  ███ ██ ▄▄  ▄▄ ▄▄▄▄▄▄▄   ▄▄▄▄  ▄▄▄▄▄  ▄▄  ▄▄▄▄
//  ██▀███ ██  ██ ██ ██ ██ ██▄▄██ ██  ▀▀ ██ ██
//  ██  ██ ▀█▄▄██ ██ ██ ██ ▀█▄▄▄  ██     ██ ▀█▄▄▄
//

struct Numeric {
    enum Type { U64, S64, Double };
    Type type;
    union {
        u64 rep_u64;
        s64 rep_s64;
        double rep_double;
    };

    Numeric() : type(U64), rep_u64(0) {
    }

    void operator=(bool v) {
        type = U64;
        rep_u64 = v;
    }
    void operator=(u8 v) {
        type = U64;
        rep_u64 = v;
    }
    void operator=(u16 v) {
        type = U64;
        rep_u64 = v;
    }
    void operator=(u32 v) {
        type = U64;
        rep_u64 = v;
    }
    void operator=(u64 v) {
        type = U64;
        rep_u64 = v;
    }
    void operator=(s8 v) {
        type = S64;
        rep_s64 = v;
    }
    void operator=(s16 v) {
        type = S64;
        rep_s64 = v;
    }
    void operator=(s32 v) {
        type = S64;
        rep_s64 = v;
    }
    void operator=(s64 v) {
        type = S64;
        rep_s64 = v;
    }
    void operator=(float v) {
        type = Double;
        rep_double = v;
    }
    void operator=(double v) {
        type = Double;
        rep_double = v;
    }

    template <typename T>
    T cast(bool& precise) {
        T result;
        switch (type) {
            case U64:
                result = (T) rep_u64;
                precise = ((u64) result == rep_u64);
                break;
            case S64:
                result = (T) rep_s64;
                precise = ((s64) result == rep_s64);
                break;
            case Double:
                result = (T) rep_double;
                precise = ((double) result == rep_double);
                break;
        }
        return result;
    }

    template <typename T>
    T cast() {
        bool dummy;
        return cast<T>(dummy);
    }
};

//   ▄▄▄▄          ▄▄   ▄▄▄▄         ▄▄▄▄
//  ██  ▀▀  ▄▄▄▄  ▄██▄▄  ██  ▄▄▄▄▄  ██  ▀▀  ▄▄▄▄  ▄▄▄▄  ▄▄▄▄▄   ▄▄▄▄
//   ▀▀▀█▄ ██▄▄██  ██    ██  ██  ██  ▀▀▀█▄ ██    ██  ██ ██  ██ ██▄▄██
//  ▀█▄▄█▀ ▀█▄▄▄   ▀█▄▄ ▄██▄ ██  ██ ▀█▄▄█▀ ▀█▄▄▄ ▀█▄▄█▀ ██▄▄█▀ ▀█▄▄▄
//                                                      ██

template <typename T, typename V>
struct SetInScope {
    T& target;                            // The variable to set/reset
    std::remove_reference_t<T> old_value; // Backup of original value
    const V& new_value_ref; // Extends the lifetime of temporary values in the case of
                            // eg. SetInScope<StringView, String>

    template <typename U>
    SetInScope(T& target, U&& new_value)
        : target{target}, old_value{std::move(target)}, new_value_ref{new_value} {
        target = std::forward<U>(new_value);
    }
    ~SetInScope() {
        this->target = std::move(this->old_value);
    }
};

#define PLY_SET_IN_SCOPE(target, value) \
    SetInScope<decltype(target), decltype(value)> PLY_UNIQUE_VARIABLE(set_in_scope) { \
        target, value \
    }

//------------------------------------------------------------
// OnScopeExit
//------------------------------------------------------------
template <typename Callback>
struct OnScopeExit {
    Callback cb;
    PLY_INLINE ~OnScopeExit() {
        cb();
    }
};
template <typename Callback>
PLY_INLINE OnScopeExit<Callback> set_on_scope_exit(Callback&& cb) {
    return {std::forward<Callback>(cb)};
}
#define PLY_ON_SCOPE_EXIT(cb) \
    auto PLY_UNIQUE_VARIABLE(on_scope_exit) = set_on_scope_exit([&] cb)

//   ▄▄▄▄           ▄▄  ▄▄         ▄▄
//  ██  ▀▀ ▄▄    ▄▄ ▄▄ ▄██▄▄  ▄▄▄▄ ██▄▄▄
//   ▀▀▀█▄ ██ ██ ██ ██  ██   ██    ██  ██
//  ▀█▄▄█▀  ██▀▀██  ██  ▀█▄▄ ▀█▄▄▄ ██  ██
//

struct SwitchType {
    void (*construct)(void*);
    void (*destruct)(void*);
    void (*move)(void*, void*);
    void (*copy)(void*, const void*);

    template <typename T>
    static SwitchType get() {
        return {
            // construct
            [](void* ptr) { new (ptr) T{}; },
            // destruct
            [](void* ptr) { subst::destruct_by_member((T*) ptr); },
            // move
            [](void* dst, void* src) { subst::unsafe_move<T>((T*) dst, (T*) src); },
            // copy
            [](void* dst, const void* src) {
                subst::unsafe_copy<T>((T*) dst, (const T*) src);
            },
        };
    };
};

template <typename Container, typename State, typename Container::ID RequiredID>
struct SwitchWrapper {
    Container& ctr;

    PLY_INLINE SwitchWrapper(Container& ctr) : ctr(ctr) {
    }
    PLY_INLINE explicit operator bool() const {
        return ctr.id == RequiredID;
    }
    template <typename... Args>
    PLY_INLINE SwitchWrapper& switch_to(Args&&... args) {
        Container::id_to_type[ureg(ctr.id)].destruct(&ctr.storage);
        ctr.id = RequiredID;
        new (&ctr.storage) State{std::forward<Args>(args)...};
        return *this;
    }
    PLY_INLINE const State* operator->() const {
        PLY_ASSERT(ctr.id == RequiredID);
        return (State*) &ctr.storage;
    }
    PLY_INLINE State* operator->() {
        PLY_ASSERT(ctr.id == RequiredID);
        return (State*) &ctr.storage;
    }
    PLY_INLINE const State* get() const {
        PLY_ASSERT(ctr.id == RequiredID);
        return (State*) &ctr.storage;
    }
    PLY_INLINE State* get() {
        PLY_ASSERT(ctr.id == RequiredID);
        return (State*) &ctr.storage;
    }
    PLY_INLINE void operator=(const State& state) {
        PLY_ASSERT(ctr.id == RequiredID);
        *(State*) &ctr.storage = state;
    }
    PLY_INLINE void operator=(State&& state) {
        PLY_ASSERT(ctr.id == RequiredID);
        *(State*) &ctr.storage = std::move(state);
    }
};

#define PLY_STATE(name)

#define SWITCH_FOOTER(ctr_name, default_state) \
    using T = ctr_name; \
    ID id; \
    Storage_ storage; \
    static SwitchType id_to_type[]; \
    template <typename, typename = void> \
    struct TypeToID; \
    PLY_INLINE ctr_name() : id{ID::default_state} { new (&storage) default_state{}; } \
    PLY_INLINE ~ctr_name() { id_to_type[ply::ureg(id)].destruct(&storage); } \
    PLY_INLINE ctr_name(const ctr_name& other) : id{other.id} { \
        id_to_type[ply::ureg(id)].construct(&storage); \
        id_to_type[ply::ureg(id)].copy(&storage, &other.storage); \
    } \
    PLY_INLINE ctr_name(ctr_name&& other) : id{other.id} { \
        id_to_type[ply::ureg(id)].construct(&storage); \
        id_to_type[ply::ureg(id)].move(&storage, &other.storage); \
    } \
    template <typename S, typename = ply::void_t<decltype(TypeToID<S>::value)>> \
    PLY_INLINE ctr_name(S&& other) : id{TypeToID<S>::value} { \
        id_to_type[ply::ureg(id)].construct(&storage); \
        id_to_type[ply::ureg(id)].move(&storage, &other); \
    } \
    PLY_INLINE void switch_to(ID new_id) { \
        id_to_type[ply::ureg(id)].destruct(&storage); \
        id = new_id; \
        id_to_type[ply::ureg(id)].construct(&storage); \
    } \
    PLY_INLINE void operator=(const ctr_name& other) { \
        switch_to(other.id); \
        id_to_type[ply::ureg(id)].copy(&storage, &other.storage); \
    } \
    PLY_INLINE void operator=(ctr_name&& other) { \
        switch_to(other.id); \
        id_to_type[ply::ureg(id)].move(&storage, &other.storage); \
    }

// We use partial specialization on TypeToID<> because of
// http://open-std.org/JTC1/SC22/WG21/docs/cwg_defects.html#727
// https://stackoverflow.com/q/49707184/3043469
#define SWITCH_ACCESSOR(state, func) \
    template <typename dummy> \
    struct TypeToID<state, dummy> { \
        static constexpr ID value = ID::state; \
    }; \
    PLY_INLINE ply::SwitchWrapper<T, state, T::ID::state> func() { return {*this}; } \
    PLY_INLINE const ply::SwitchWrapper<const T, state, T::ID::state> func() const { \
        return {*this}; \
    }

#define SWITCH_TABLE_BEGIN(ctr_name) ply::SwitchType ctr_name::id_to_type[] = {

#define SWITCH_TABLE_STATE(ctr_name, state) ply::SwitchType::get<ctr_name::state>(),

#define SWITCH_TABLE_END(ctr_name) \
    } \
    ; \
    PLY_STATIC_ASSERT(PLY_STATIC_ARRAY_SIZE(ctr_name::id_to_type) == \
                      (ply::ureg) ctr_name::ID::Count); \
    PLY_STATIC_ASSERT((ply::ureg) ctr_name::ID::Count > 0);

//  ▄▄▄▄▄                ▄▄▄
//  ██  ██  ▄▄▄▄   ▄▄▄▄   ██
//  ██▀▀▀  ██  ██ ██  ██  ██
//  ██     ▀█▄▄█▀ ▀█▄▄█▀ ▄██▄
//

#if PLY_WITH_ASSERTS
#define PLY_WITH_POOL_DEBUG_CHECKS 1
#endif

template <typename T, typename Index>
class PoolIndex;
template <typename T>
class PoolIterator;
template <typename T>
class PoolPtr;

//-----------------------------------------------------------------------
// BasePool
//-----------------------------------------------------------------------
class BasePool {
public:
    void* m_items = nullptr;
    u32 m_size = 0;
    u32 m_allocated = 0;
    mutable u32 m_firstFree = u32(-1);
    mutable u32 m_sortedFreeList : 1;
    u32 m_freeListSize : 31;
#if PLY_WITH_POOL_DEBUG_CHECKS
    mutable u16 m_numReaders = 0;
#endif

    BasePool();
    void base_reserve(u32 new_size, u32 item_size);
    u32 base_alloc(u32 item_size);
    void base_free(u32 index, u32 item_size);
    PLY_DLL_ENTRY void base_sort_free_list(u32 stride) const;
    void base_clear();
};

//-----------------------------------------------------------------------
// Pool<T>
//-----------------------------------------------------------------------
template <typename T_>
class Pool : public BasePool {
public:
    typedef T_ T;
    PLY_STATIC_ASSERT(sizeof(T) >= sizeof(u32));

    Pool();
    void clear();
    ~Pool();
    template <typename Index, typename... Args>
    PoolIndex<T, Index> new_item(Args&&... args);
    template <typename Index>
    void del_item(PoolIndex<T, Index> index);

    template <typename Index>
    PoolPtr<T> get(PoolIndex<T, Index>);
    template <typename Index>
    PoolPtr<const T> get(PoolIndex<T, Index>) const;
    u32 index_of(const T* item) const;

    PoolIterator<T> begin();
    PoolIterator<T> end();
    PoolIterator<const T> begin() const;
    PoolIterator<const T> end() const;
};

//-----------------------------------------------------------------------
// PoolIndex<T, Index>
//-----------------------------------------------------------------------
template <typename T_, typename Index_>
class PoolIndex {
public:
    typedef T_ T;
    typedef Index_ Index;
    PLY_STATIC_ASSERT((Index) -1 > 0); // Should be an unsigned type
    static constexpr Index InvalidIndex = (Index) -1;

    Index idx = InvalidIndex;

    PoolIndex() = default;
    PoolIndex(u32 idx);
    PoolIndex(const PoolIndex<const T, Index>& other);
    // Don't allow implicit cast to integer type (and no implicit bool cast either!)
    bool is_valid() const;
    bool operator==(const PoolIndex& other) const;
    bool operator!=(const PoolIndex& other) const;
};

//-----------------------------------------------------------------------
// OwnPoolHandle<T, Index>
//-----------------------------------------------------------------------
template <typename Traits>
class OwnPoolHandle : public PoolIndex<typename Traits::T, typename Traits::Index> {
public:
    OwnPoolHandle() = default;
    OwnPoolHandle(u32 idx);
    OwnPoolHandle(OwnPoolHandle&& other);
    ~OwnPoolHandle();

    void assign(PoolIndex<typename Traits::T, typename Traits::Index> other);
    void clear();
    PoolPtr<typename Traits::T> get() const;
};

//---------------------------------------------------------
// PoolIterator
//---------------------------------------------------------
template <typename T>
class PoolIterator {
private:
    const Pool<T>* m_pool;
    u32 m_nextFree;
    u32 m_index;

public:
    PoolIterator(const Pool<T>* pool, u32 next_free, u32 index);
    ~PoolIterator();
    PoolIterator(const PoolIterator& other);
    void operator=(const PoolIterator&) = delete;
    template <typename Index>
    PoolIndex<T, Index> get_index() const;
    PoolPtr<T> operator*() const;
    PoolIterator& operator++();
    bool is_valid() const;
    bool operator==(const PoolIterator& other) const;
    bool operator!=(const PoolIterator& other) const;
};

//---------------------------------------------------------
// PoolPtr
//---------------------------------------------------------
template <typename T>
class PoolPtr {
public:
#if PLY_WITH_POOL_DEBUG_CHECKS
    const Pool<T>* pool;
#endif
    T* ptr;

    PoolPtr();
    PoolPtr(const Pool<T>* pool, T* ptr);
    PoolPtr(const PoolPtr<T>& other);
    ~PoolPtr();
    void operator=(const PoolPtr<T>& other);
    bool operator==(const PoolPtr<T>& other) const;
    operator T*() const = delete; // Disallow cast to pointer because we don't want
                                  // callers to accidentally store the pointer
    T* get() const;
    explicit operator bool() const;
    T* operator->() const;
    T* release();
    // Allow implicit conversion to PoolPtr<const T>&
    operator const PoolPtr<const T>&() const;
};

//------------------------------------------------------------------------------------
// PoolIndex<T, Index> inline functions
//------------------------------------------------------------------------------------
template <typename T, typename Index>
PoolIndex<T, Index>::PoolIndex(u32 idx) : idx{safe_demote<Index>(idx)} {
}

template <typename T, typename Index>
PoolIndex<T, Index>::PoolIndex(const PoolIndex<const T, Index>& other)
    : idx{other.idx} {
}

template <typename T, typename Index>
bool PoolIndex<T, Index>::is_valid() const {
    return idx != InvalidIndex;
}

template <typename T, typename Index>
bool PoolIndex<T, Index>::operator==(const PoolIndex& other) const {
    return idx == other.idx;
}

template <typename T, typename Index>
bool PoolIndex<T, Index>::operator!=(const PoolIndex& other) const {
    return idx != other.idx;
}

//------------------------------------------------------------------------------------
// PoolPtr<T> inline functions
//------------------------------------------------------------------------------------
template <typename T>
PoolPtr<T>::PoolPtr()
    :
#if PLY_WITH_POOL_DEBUG_CHECKS
      pool{nullptr},
#endif
      ptr{nullptr} {
}

template <typename T>
PoolPtr<T>::PoolPtr(const Pool<T>* pool, T* ptr)
    :
#if PLY_WITH_POOL_DEBUG_CHECKS
      pool{pool},
#endif
      ptr{ptr} {
#if PLY_WITH_POOL_DEBUG_CHECKS
    pool->m_numReaders++;
#endif
}

template <typename T>
PoolPtr<T>::PoolPtr(const PoolPtr<T>& other)
    :
#if PLY_WITH_POOL_DEBUG_CHECKS
      pool{(const Pool<T>*) other.pool},
#endif
      ptr{other.ptr} {
#if PLY_WITH_POOL_DEBUG_CHECKS
    if (pool) {
        pool->m_numReaders++;
    }
#endif
}

template <typename T>
PoolPtr<T>::~PoolPtr() {
#if PLY_WITH_POOL_DEBUG_CHECKS
    PLY_ASSERT((bool) pool == (bool) ptr); // Either both nullptr, or neither nullptr
    if (pool) {
        PLY_ASSERT(pool->m_numReaders > 0);
        pool->m_numReaders--;
    }
#endif
}

template <typename T>
bool PoolPtr<T>::operator==(const PoolPtr<T>& other) const {
    PLY_ASSERT(ptr != other.ptr || pool == other.pool);
    return ptr == other.ptr;
}

template <typename T>
void PoolPtr<T>::operator=(const PoolPtr<T>& other) {
#if PLY_WITH_POOL_DEBUG_CHECKS
    if (pool) {
        pool->m_numReaders--;
    }
    pool = (const Pool<T>*) other.pool;
    if (pool) {
        pool->m_numReaders++;
    }
#endif
    ptr = other.ptr;
}

template <typename T>
T* PoolPtr<T>::get() const {
    return ptr;
}

template <typename T>
PoolPtr<T>::operator bool() const {
    return ptr != nullptr;
}

template <typename T>
T* PoolPtr<T>::operator->() const {
    return ptr;
}

template <typename T>
T* PoolPtr<T>::release() {
#if PLY_WITH_POOL_DEBUG_CHECKS
    PLY_ASSERT((bool) pool == (bool) ptr); // Either both nullptr, or neither nullptr
    if (pool) {
        PLY_ASSERT(pool->m_numReaders > 0);
        pool->m_numReaders--;
    }
    pool = nullptr;
#endif
    T* r = ptr;
    ptr = nullptr;
    return r;
}

// Allow implicit conversion to PoolPtr<const T>&
template <typename T>
PoolPtr<T>::operator const PoolPtr<const T>&() const {
    return *(const PoolPtr<const T>*) this;
}

//------------------------------------------------------------------------------------
// BasePool inline functions
//------------------------------------------------------------------------------------
inline BasePool::BasePool() {
    m_sortedFreeList = 1;
    m_freeListSize = 0;
}

inline void BasePool::base_reserve(u32 new_size, u32 item_size) {
    m_allocated = (u32) round_up_power_of2(max<u32>(
        new_size, 8)); // FIXME: Generalize to other resize strategies when needed
    PLY_ASSERT(m_allocated != 0); // Overflow check
    m_items = Heap.realloc(
        m_items,
        item_size * m_allocated); // FIXME: Generalize to other heaps when needed
}

inline u32 BasePool::base_alloc(u32 item_size) {
    if (m_firstFree != u32(-1)) {
        u32 next_free = *(u32*) PLY_PTR_OFFSET(m_items, m_firstFree * item_size);
        u32 index = m_firstFree;
        m_firstFree = next_free;
        m_freeListSize--;
        PLY_ASSERT((m_freeListSize == 0) == (m_firstFree == u32(-1)));
        return index;
    }
    if (m_size >= m_allocated) {
        base_reserve(m_size + 1, item_size);
    }
    return m_size++;
}

inline void BasePool::base_free(u32 index, u32 item_size) {
    PLY_ASSERT(index < m_size);
    *(u32*) PLY_PTR_OFFSET(m_items, index * item_size) = m_firstFree;
    m_firstFree = index;
    m_sortedFreeList = 0;
    PLY_ASSERT(m_freeListSize < (1u << 31) - 1);
    m_freeListSize++;
}

inline void BasePool::base_clear() {
    Heap.free(m_items);
    m_items = nullptr;
    m_size = 0;
    m_allocated = 0;
    m_firstFree = -1;
    m_sortedFreeList = 1;
    m_freeListSize = 0;
}

//------------------------------------------------------------------------------------
// Pool<T> inline functions
//------------------------------------------------------------------------------------
template <typename T>
Pool<T>::Pool() {
}

template <typename T>
void Pool<T>::clear() {
    if (!std::is_trivially_destructible<T>::value) {
        base_sort_free_list(sizeof(T));
        T* item = static_cast<T*>(m_items);
        T* end = item + m_size;
        u32 skip_index = m_firstFree;
        for (;;) {
            T* skip = static_cast<T*>(m_items) +
                      (skip_index == u32(-1) ? m_size : skip_index);
            while (item < skip) {
                item->~T();
                item++;
            }
            if (item == end)
                break;
            skip_index = *reinterpret_cast<u32*>(skip);
            item++;
        }
    }
    base_clear();
}

template <typename T>
Pool<T>::~Pool() {
    clear();
}

template <typename T>
template <typename Index, typename... Args>
PoolIndex<T, Index> Pool<T>::new_item(Args&&... args) {
#if PLY_WITH_POOL_DEBUG_CHECKS
    // There must not be any Iterators or Ptrs when modifying the pool!
    PLY_ASSERT(m_numReaders == 0);
#endif
    u32 index = base_alloc(sizeof(T));
    new (static_cast<T*>(m_items) + index) T{std::forward<Args>(args)...};
    return index;
}

template <typename T>
template <typename Index>
void Pool<T>::del_item(PoolIndex<T, Index> index) {
#if PLY_WITH_POOL_DEBUG_CHECKS
    // There must not be any Iterators or Ptrs when modifying the pool!
    PLY_ASSERT(m_numReaders == 0);
#endif
    static_cast<T*>(m_items)[index.idx].~T();
    base_free(index.idx, sizeof(T));
}

template <typename T>
template <typename Index>
PoolPtr<T> Pool<T>::get(PoolIndex<T, Index> index) {
    PLY_ASSERT(index.idx < m_size);
    return {this, static_cast<T*>(m_items) + index.idx};
}

template <typename T>
template <typename Index>
PoolPtr<const T> Pool<T>::get(PoolIndex<T, Index> index) const {
    PLY_ASSERT(index.idx < m_size);
    return {(Pool<const T>*) this, static_cast<const T*>(m_items) + index.idx};
}

template <typename T>
u32 Pool<T>::index_of(const T* item) const {
    u32 index = safe_demote<u32>(item - static_cast<const T*>(m_items));
    PLY_ASSERT(index < m_size);
    return index;
}

//! Return iterator suitable for range-for.
//! The pool must not be modified during iteration.
template <typename T>
PoolIterator<T> Pool<T>::begin() {
    base_sort_free_list(sizeof(T));
    PoolIterator<T> iter{this, m_firstFree, u32(-1)};
    ++iter;
    return iter;
}

//! Return iterator suitable for range-for.
//! The pool must not be modified during iteration.
template <typename T>
PoolIterator<T> Pool<T>::end() {
    return {this, u32(-1), m_size};
}

//! Return iterator suitable for range-for.
//! The pool must not be modified during iteration.
template <typename T>
PoolIterator<const T> Pool<T>::begin() const {
    base_sort_free_list(sizeof(T));
    PoolIterator<const T> iter{(Pool<const T>*) this, m_firstFree, u32(-1)};
    ++iter;
    return iter;
}

//! Return iterator suitable for range-for.
//! The pool must not be modified during iteration.
template <typename T>
PoolIterator<const T> Pool<T>::end() const {
    return {this, u32(-1), m_size};
}

//------------------------------------------------------------------------------------
// PoolIterator<T> inline functions
//------------------------------------------------------------------------------------
template <typename T>
PoolIterator<T>::PoolIterator(const Pool<T>* pool, u32 next_free, u32 index)
    : m_pool(pool), m_nextFree(next_free), m_index(index) {
#if PLY_WITH_POOL_DEBUG_CHECKS
    m_pool->m_numReaders++;
#endif
}

template <typename T>
PoolIterator<T>::~PoolIterator() {
#if PLY_WITH_POOL_DEBUG_CHECKS
    PLY_ASSERT(m_pool->m_numReaders > 0);
    m_pool->m_numReaders--;
#endif
}

template <typename T>
PoolIterator<T>::PoolIterator(const PoolIterator& other) : m_pool(other.m_pool) {
#if PLY_WITH_POOL_DEBUG_CHECKS
    m_pool->m_numReaders++;
#endif
    m_nextFree = other.m_nextFree;
    m_index = other.m_index;
}

template <typename T>
template <typename Index>
PoolIndex<T, Index> PoolIterator<T>::get_index() const {
    return {m_index};
}

template <typename T>
PoolPtr<T> PoolIterator<T>::operator*() const {
    PLY_ASSERT(m_index < m_pool->m_size);
    return {m_pool, static_cast<T*>(m_pool->m_items) + m_index};
}

template <typename T>
PoolIterator<T>& PoolIterator<T>::operator++() {
    m_index++;
    while (m_index < m_pool->m_size) {
        PLY_ASSERT(m_index <= m_nextFree);
        if (m_index < m_nextFree)
            break;
        m_nextFree =
            *reinterpret_cast<const u32*>(static_cast<T*>(m_pool->m_items) + m_index);
        m_index++;
    }
    return *this;
}

template <typename T>
bool PoolIterator<T>::is_valid() const {
    return m_index < m_pool->m_size;
}

template <typename T>
bool PoolIterator<T>::operator==(const PoolIterator<T>& other) const {
    PLY_ASSERT(m_pool == other.m_pool);
    return m_index == other.m_index;
}

template <typename T>
bool PoolIterator<T>::operator!=(const PoolIterator<T>& other) const {
    PLY_ASSERT(m_pool == other.m_pool);
    return m_index != other.m_index;
}

//------------------------------------------------------------------------------------
// OwnPoolHandle<T, Index> inline functions
//------------------------------------------------------------------------------------
template <typename Traits>
OwnPoolHandle<Traits>::OwnPoolHandle(u32 idx)
    : PoolIndex<typename Traits::T, typename Traits::Index>{idx} {
}

template <typename Traits>
OwnPoolHandle<Traits>::OwnPoolHandle(OwnPoolHandle<Traits>&& other)
    : PoolIndex<typename Traits::T, typename Traits::Index>{other.idx} {
    other.idx = this->InvalidIndex;
}

template <typename Traits>
OwnPoolHandle<Traits>::~OwnPoolHandle() {
    if (this->idx != this->InvalidIndex) {
        Traits::get_pool().del_item(*this);
    }
}

template <typename Traits>
void OwnPoolHandle<Traits>::assign(
    PoolIndex<typename Traits::T, typename Traits::Index> other) {
    if (this->idx != this->InvalidIndex) {
        Traits::get_pool().del_item(*this);
    }
    this->idx = other.idx;
}

template <typename Traits>
void OwnPoolHandle<Traits>::clear() {
    if (this->idx != this->InvalidIndex) {
        Traits::get_pool().del_item(*this);
    }
    this->idx = this->InvalidIndex;
}

template <typename Traits>
PoolPtr<typename Traits::T> OwnPoolHandle<Traits>::get() const {
    return Traits::get_pool().get(*this);
}

//  ▄▄▄▄▄  ▄▄        ▄▄▄▄▄                ▄▄▄
//  ██  ██ ▄▄  ▄▄▄▄▄ ██  ██  ▄▄▄▄   ▄▄▄▄   ██
//  ██▀▀█▄ ██ ██  ██ ██▀▀▀  ██  ██ ██  ██  ██
//  ██▄▄█▀ ██ ▀█▄▄██ ██     ▀█▄▄█▀ ▀█▄▄█▀ ▄██▄
//             ▄▄▄█▀

// FIXME: Implement a 32-bit friendly BigPool, especially for biscuit::Tokenizer
PLY_STATIC_ASSERT(PLY_PTR_SIZE == 8);

struct BaseBigPool {
    char* base = nullptr;
    uptr num_reserved_bytes = 0;
    uptr num_committed_bytes = 0;

    static constexpr uptr DefaultNumReservedBytes = 1024 * 1024 * 1024;

    BaseBigPool(uptr num_reserved_bytes = DefaultNumReservedBytes);
    ~BaseBigPool();
    void commit_pages(uptr new_total_bytes);
};

template <typename T = char>
class BigPool : protected BaseBigPool {
private:
    uptr num_items_ = 0;

public:
    PLY_INLINE BigPool(uptr num_reserved_bytes = DefaultNumReservedBytes)
        : BaseBigPool{num_reserved_bytes} {
    }
    PLY_INLINE uptr num_items() const {
        return this->num_items_;
    }
    PLY_INLINE const T& operator[](uptr idx) const {
        PLY_ASSERT(idx < this->num_items_);
        return ((const T*) this->base)[idx];
    }
    PLY_INLINE T& operator[](uptr idx) {
        PLY_ASSERT(idx < this->num_items_);
        return ((T*) this->base)[idx];
    }
    PLY_INLINE const T* end() const {
        return ((const T*) this->base) + this->num_items_;
    }
    PLY_INLINE T* get(uptr idx) const {
        PLY_ASSERT(idx < this->num_items_);
        return ((T*) this->base) + idx;
    }
    PLY_INLINE T& back(sptr ofs = -1) const {
        PLY_ASSERT(ofs < 0 && uptr(-ofs) <= this->num_items_);
        return ((T*) this->base)[this->num_items_ + ofs];
    }
    PLY_INLINE T* begin_write(uptr max_num_items = 1) {
        uptr new_total_bytes = sizeof(T) * (this->num_items_ + max_num_items);
        if (new_total_bytes > this->num_committed_bytes) {
            this->commit_pages(new_total_bytes);
        }
        return ((T*) this->base) + this->num_items_;
    }
    PLY_INLINE void end_write(uptr num_items = 1) {
        this->num_items_ += num_items;
        PLY_ASSERT(sizeof(T) * this->num_items_ <= this->num_committed_bytes);
    }
    PLY_INLINE T* alloc(uptr num_items = 1) {
        T* result = begin_write(num_items);
        end_write(num_items);
        return result;
    }
    PLY_INLINE T& append(T&& item) {
        return *new (this->alloc()) T{std::move(item)};
    }
    PLY_INLINE T& append(const T& item) {
        return *new (this->alloc()) T{item};
    }
    template <typename... Args>
    PLY_INLINE T& append(Args&&... args) {
        return *new (this->alloc()) T{std::forward<Args>(args)...};
    }
};

//  ▄▄▄▄▄  ▄▄▄               ▄▄     ▄▄    ▄▄         ▄▄
//  ██  ██  ██   ▄▄▄▄   ▄▄▄▄ ██  ▄▄ ██    ▄▄  ▄▄▄▄  ▄██▄▄
//  ██▀▀█▄  ██  ██  ██ ██    ██▄█▀  ██    ██ ▀█▄▄▄   ██
//  ██▄▄█▀ ▄██▄ ▀█▄▄█▀ ▀█▄▄▄ ██ ▀█▄ ██▄▄▄ ██  ▄▄▄█▀  ▀█▄▄
//

struct OutStream;

struct BlockList {
    static const u32 DefaultBlockSize = 2048;

    struct WeakRef;

    //--------------------------------------
    // Blocks are allocated on the heap, and the block footer is located contiguously in
    // memory immediately following the block data.
    //--------------------------------------
    struct Footer {
        u64 file_offset = 0; // Total number of bytes used in all preceding blocks
        Reference<Footer> next_block;
        Footer* prev_block = nullptr;
        char* bytes = nullptr;
        u32 start_offset = 0;
        u32 num_bytes_used = 0; // Number of bytes in this block that contain valid data
        u32 block_size = 0;     // Total number of bytes allocated for this block
        mutable s32 ref_count = 0;

        PLY_DLL_ENTRY void on_ref_count_zero();
        PLY_INLINE void inc_ref() {
            this->ref_count++;
        }
        PLY_INLINE void dec_ref() {
            PLY_ASSERT(this->ref_count > 0);
            if (--this->ref_count == 0) {
                this->on_ref_count_zero();
            }
        }
        PLY_INLINE char* start() const {
            return this->bytes + this->start_offset;
        }
        PLY_INLINE char* unused() const {
            return this->bytes + this->num_bytes_used;
        }
        PLY_INLINE char* end() const {
            return this->bytes + this->block_size;
        }
        PLY_INLINE u32 offset_of(char* byte) {
            uptr ofs = byte - this->bytes;
            PLY_ASSERT(ofs <= this->num_bytes_used);
            return (u32) ofs;
        }
        PLY_INLINE StringView view_used_bytes() const {
            return {this->start(), this->num_bytes_used - this->start_offset};
        }
        PLY_INLINE MutStringView view_unused_bytes() {
            return {this->unused(), this->block_size - this->num_bytes_used};
        }
        // Returns a WeakRef to next block, and the next byte after the last byte in
        // this block, taking the difference in file_offsets into account since it's
        // possible for adjacent blocks to overlap.
        PLY_DLL_ENTRY WeakRef weak_ref_to_next() const;
    };

    //--------------------------------------
    // WeakRef
    //--------------------------------------
    struct WeakRef {
        Footer* block = nullptr;
        char* byte = nullptr;

        PLY_INLINE WeakRef() {
        }
        PLY_INLINE WeakRef(const WeakRef& other)
            : block{other.block}, byte{other.byte} {
        }
        PLY_INLINE WeakRef(Footer* block, char* byte) : block{block}, byte{byte} {
            PLY_ASSERT(!block || (uptr(byte - block->bytes) <= block->block_size));
        }
        PLY_INLINE WeakRef(Footer* block, u32 offset)
            : block{block}, byte{block->bytes + offset} {
            PLY_ASSERT(offset <= block->block_size);
        }
        PLY_INLINE void operator=(const WeakRef& other) {
            this->block = other.block;
            this->byte = other.byte;
        }
        PLY_INLINE bool operator!=(const WeakRef& other) const {
            return this->block != other.block || this->byte != other.byte;
        }
        PLY_INLINE WeakRef normalized() const {
            if (this->block->next_block && (this->byte == this->block->unused())) {
                return {this->block->next_block, this->block->start()};
            }
            return *this;
        }
    };

    //--------------------------------------
    // Ref
    //--------------------------------------
    struct Ref {
        Reference<Footer> block;
        char* byte = nullptr;

        PLY_INLINE Ref() {
        }
        PLY_INLINE Ref(const Ref& other) : block{other.block}, byte{other.byte} {
        }
        PLY_INLINE Ref(Ref&& other) : block{std::move(other.block)}, byte{other.byte} {
        }
        PLY_INLINE Ref(Footer* block) : block{block}, byte{block->bytes} {
        }
        PLY_INLINE Ref(Footer* block, char* byte) : block{block}, byte{byte} {
            PLY_ASSERT(!block || (uptr(byte - block->bytes) <= block->block_size));
        }
        PLY_INLINE Ref(Reference<Footer>&& block, char* byte)
            : block{std::move(block)}, byte{byte} {
            PLY_ASSERT(!block ||
                       (uptr(byte - this->block->bytes) <= this->block->block_size));
        }
        PLY_INLINE Ref(const WeakRef& weak_ref)
            : block{weak_ref.block}, byte{weak_ref.byte} {
        }
        PLY_INLINE operator WeakRef() const {
            return {this->block, this->byte};
        }
        PLY_INLINE void operator=(const WeakRef& weak_ref) {
            this->block = weak_ref.block;
            this->byte = weak_ref.byte;
        }
        PLY_INLINE void operator=(Ref&& other) {
            this->block = std::move(other.block);
            this->byte = other.byte;
        }
    };

    //--------------------------------------
    // RangeForIterator
    //--------------------------------------
    struct RangeForIterator {
        WeakRef cur_pos;
        WeakRef end_pos;

        RangeForIterator(const WeakRef& start_pos, const WeakRef& end_pos = {})
            : cur_pos{start_pos}, end_pos{end_pos} {
        }
        PLY_INLINE RangeForIterator& begin() {
            return *this;
        }
        PLY_INLINE RangeForIterator& end() {
            return *this;
        }
        PLY_INLINE void operator++() {
            this->cur_pos = this->cur_pos.block->weak_ref_to_next();
        }
        PLY_INLINE bool operator!=(const RangeForIterator&) const {
            return this->cur_pos != this->end_pos;
        }
        PLY_INLINE StringView operator*() const {
            return StringView::from_range(this->cur_pos.byte,
                                          (this->cur_pos.block == this->end_pos.block)
                                              ? this->end_pos.byte
                                              : this->cur_pos.block->unused());
        }
    };

    //--------------------------------------
    // Static member functions
    //--------------------------------------
    static PLY_DLL_ENTRY Reference<Footer>
    create_block(u32 num_bytes = DefaultBlockSize);
    static PLY_DLL_ENTRY Reference<Footer> create_overlay_block(const WeakRef& pos,
                                                                u32 num_bytes);
    static PLY_DLL_ENTRY Footer* append_block(Footer* block,
                                              u32 num_bytes = DefaultBlockSize);
    static PLY_DLL_ENTRY void
    append_block_with_recycle(Reference<Footer>& block,
                              u32 num_bytes = DefaultBlockSize);
    static PLY_INLINE RangeForIterator iterate_over_views(const WeakRef& start,
                                                          const WeakRef& end = {}) {
        return {start, end};
    }
    static PLY_DLL_ENTRY u32 jump_to_next_block(WeakRef* weak_ref);
    static PLY_DLL_ENTRY u32 jump_to_prev_block(WeakRef* weak_ref);

    static PLY_DLL_ENTRY String to_string(Ref&& start, const WeakRef& end = {});

    //--------------------------------------
    // BlockList object
    //--------------------------------------
    Reference<Footer> head;
    Footer* tail = nullptr;

    PLY_DLL_ENTRY BlockList();
    PLY_DLL_ENTRY ~BlockList();
    PLY_DLL_ENTRY char* append_bytes(u32 num_bytes);
    PLY_DLL_ENTRY void pop_last_bytes(u32 num_bytes);
    PLY_INLINE WeakRef end() const {
        return {tail, tail->unused()};
    }
};

//   ▄▄▄▄
//  ██  ▀▀  ▄▄▄▄   ▄▄▄▄▄ ▄▄  ▄▄  ▄▄▄▄  ▄▄▄▄▄   ▄▄▄▄  ▄▄▄▄
//   ▀▀▀█▄ ██▄▄██ ██  ██ ██  ██ ██▄▄██ ██  ██ ██    ██▄▄██
//  ▀█▄▄█▀ ▀█▄▄▄  ▀█▄▄██ ▀█▄▄██ ▀█▄▄▄  ██  ██ ▀█▄▄▄ ▀█▄▄▄
//                    ██

namespace impl {
PLY_DLL_ENTRY void destruct_sequence(Reference<BlockList::Footer>* head_ref,
                                     void (*destruct_view_as)(StringView));
PLY_DLL_ENTRY void begin_write_internal(BlockList::Footer** tail, u32 num_bytes);
PLY_DLL_ENTRY void pop_tail(BlockList::Footer** tail, u32 num_bytes,
                            void (*destruct_view_as)(StringView));
PLY_DLL_ENTRY void truncate(BlockList::Footer** tail, const BlockList::WeakRef& to);
PLY_DLL_ENTRY u32 get_total_num_bytes(BlockList::Footer* head);
PLY_DLL_ENTRY char* read(BlockList::WeakRef* weak_ref, u32 item_size);
} // namespace impl

template <typename T>
class Sequence;

//-----------------------------------------------------------
// WeakSequenceRef
//-----------------------------------------------------------
template <typename T>
class WeakSequenceRef {
private:
    BlockList::WeakRef impl;
    template <typename>
    friend class Sequence;

    PLY_INLINE WeakSequenceRef(const BlockList::WeakRef& impl) : impl{impl} {
    }

public:
    PLY_INLINE WeakSequenceRef() = default;
    PLY_INLINE ArrayView<T> begin_read() {
        sptr num_bytes_available = this->impl.block->unused() - this->impl.byte;
        if (num_bytes_available == 0) {
            num_bytes_available = BlockList::jump_to_next_block(&impl);
        } else {
            // num_bytes_available should always be a multiple of sizeof(T).
            PLY_ASSERT(num_bytes_available >= sizeof(T));
        }
        return ArrayView<T>::from(StringView{this->impl.byte, num_bytes_available});
    }
    PLY_INLINE void end_read(u32 num_items) {
        PLY_ASSERT(this->impl.block->unused() - this->impl.byte >=
                   sizeof(T) * num_items);
        this->impl.byte += sizeof(T) * num_items;
    }
    PLY_INLINE WeakSequenceRef normalized() const {
        return this->impl.normalized();
    }

    // Range for support.
    PLY_INLINE T& operator*() const {
        // It is illegal to call operator* at the end of the sequence.
        PLY_ASSERT(this->impl.block->unused() - this->impl.byte >= sizeof(T));
        return *(T*) this->impl.byte;
    }
    PLY_INLINE T* operator->() const {
        // It is illegal to call operator-> at the end of the sequence.
        PLY_ASSERT(this->impl.block->unused() - this->impl.byte >= sizeof(T));
        return (T*) this->impl.byte;
    }
    PLY_INLINE void operator++() {
        sptr num_bytes_available = this->impl.block->unused() - this->impl.byte;
        // It is illegal to call operator++ at the end of the sequence.
        PLY_ASSERT(num_bytes_available >= sizeof(T));
        this->impl.byte += sizeof(T);
        num_bytes_available -= sizeof(T);
        if (num_bytes_available == 0) {
            num_bytes_available = BlockList::jump_to_next_block(&impl);
            // We might now be at the end of the sequence.
        } else {
            // num_bytes_available should always be a multiple of sizeof(T).
            PLY_ASSERT(num_bytes_available >= sizeof(T));
        }
    }
    PLY_INLINE void operator--() {
        sptr num_bytes_preceding = this->impl.byte - this->impl.block->start();
        if (num_bytes_preceding == 0) {
            num_bytes_preceding = BlockList::jump_to_prev_block(&impl);
        }
        // It is illegal to call operator-- at the start of the sequence.
        PLY_ASSERT(num_bytes_preceding >= sizeof(T));
        this->impl.byte -= sizeof(T);
    }
    PLY_INLINE bool operator!=(const WeakSequenceRef& other) const {
        return this->impl.byte != other.impl.byte;
    }

    // The reference returned here remains valid as long as item continues to exist in
    // the underlying sequence.
    PLY_INLINE T& read() {
        return *(T*) impl::read(&impl, sizeof(T));
    }
    PLY_INLINE void* byte() const {
        return this->impl.byte;
    }
};

template <typename T>
class Sequence {
private:
    Reference<BlockList::Footer> head_block;
    BlockList::Footer* tail_block = nullptr;

public:
    PLY_INLINE Sequence()
        : head_block{BlockList::create_block()}, tail_block{head_block} {
    }

    PLY_INLINE Sequence(Sequence&& other)
        : head_block{std::move(other.head_block)}, tail_block{other.tail_block} {
    }

    PLY_INLINE ~Sequence() {
        impl::destruct_sequence(&head_block, subst::destruct_view_as<T>);
    }

    PLY_INLINE void operator=(Sequence&& other) {
        impl::destruct_sequence(&head_block, subst::destruct_view_as<T>);
        new (this) Sequence{std::move(other)};
    }

    PLY_INLINE T& head() {
        // It is illegal to call head() on an empty sequence.
        PLY_ASSERT(this->head_block->view_used_bytes().num_bytes >= sizeof(T));
        return *(T*) this->head_block->start();
    }
    PLY_INLINE T& tail() {
        // It is illegal to call tail() on an empty sequence.
        PLY_ASSERT(this->tail_block->view_used_bytes().num_bytes >= sizeof(T));
        return ((T*) this->tail_block->unused())[-1];
    }

    PLY_INLINE bool is_empty() const {
        // Only an empty sequence can have an empty head block.
        return this->head_block->view_used_bytes().is_empty();
    }
    PLY_INLINE u32 num_items() const {
        // Fast division by integer constant.
        return impl::get_total_num_bytes(this->head_block) / sizeof(T);
    }

    PLY_INLINE ArrayView<T> begin_write_view_no_construct() {
        if (this->tail_block->view_unused_bytes().num_bytes < sizeof(T)) {
            impl::begin_write_internal(&this->tail_block, sizeof(T));
        }
        return ArrayView<T>::from(this->tail_block->view_unused_bytes());
    }

    PLY_INLINE T* begin_write_no_construct() {
        if (this->tail_block->view_unused_bytes().num_bytes < sizeof(T)) {
            impl::begin_write_internal(&this->tail_block, sizeof(T));
        }
        return (T*) this->tail_block->unused();
    }

    PLY_INLINE void end_write(u32 num_items = 1) {
        PLY_ASSERT(sizeof(T) * num_items <=
                   this->tail_block->view_unused_bytes().num_bytes);
        this->tail_block->num_bytes_used += sizeof(T) * num_items;
    }

    PLY_INLINE T& append(const T& item) {
        T* result = begin_write_no_construct();
        new (result) T{item};
        end_write();
        return *result;
    }
    PLY_INLINE T& append(T&& item) {
        T* result = begin_write_no_construct();
        new (result) T{std::move(item)};
        end_write();
        return *result;
    }
    template <typename... Args>
    PLY_INLINE T& append(Args&&... args) {
        T* result = begin_write_no_construct();
        new (result) T{std::forward<Args>(args)...};
        end_write();
        return *result;
    }

    PLY_INLINE void pop_tail(u32 num_items = 1) {
        impl::pop_tail(&this->tail_block, num_items * (u32) sizeof(T),
                       subst::destruct_view_as<T>);
    }
    PLY_INLINE void truncate(const WeakSequenceRef<T>& to) {
        impl::truncate(&this->tail_block, to.impl);
    }

    PLY_INLINE void clear() {
        *this = Sequence{};
    }

    PLY_INLINE Array<T> move_to_array() {
        char* start_byte = this->head_block->start();
        String str = BlockList::to_string({std::move(this->head_block), start_byte});
        u32 num_items = str.num_bytes / sizeof(T); // Divide by constant is fast
        return Array<T>::adopt((T*) str.release(), num_items);
    }

    WeakSequenceRef<T> begin() {
        return BlockList::WeakRef{this->head_block, this->head_block->start()};
    }
    WeakSequenceRef<T> end() {
        return BlockList::WeakRef{this->tail_block, this->tail_block->unused()};
    }
    WeakSequenceRef<const T> begin() const {
        return BlockList::WeakRef{this->head_block, this->head_block->start()};
    }
    WeakSequenceRef<const T> end() const {
        return BlockList::WeakRef{this->tail_block, this->tail_block->unused()};
    }
};

//  ▄▄  ▄▄               ▄▄
//  ██  ██  ▄▄▄▄   ▄▄▄▄  ██▄▄▄
//  ██▀▀██  ▄▄▄██ ▀█▄▄▄  ██  ██
//  ██  ██ ▀█▄▄██  ▄▄▄█▀ ██  ██
//

// Adapted from https://github.com/aappleby/smhasher
// Specifically
// https://raw.githubusercontent.com/aappleby/smhasher/master/src/PMurHash.c
struct Hasher {
    u32 accum = 0;

    static PLY_INLINE u32 finalize(u32 h) {
        h ^= h >> 16;
        h *= 0x85ebca6bu;
        h ^= h >> 13;
        h *= 0xc2b2ae35u;
        h ^= h >> 16;
        return h;
    }

    PLY_DLL_ENTRY u32 result() const;

    template <typename T>
    static PLY_INLINE u32 hash(const T& obj) {
        Hasher h;
        h << obj;
        return h.result();
    }

    // Special case hash functions
    static PLY_INLINE u32 hash(u64 obj) {
        return (u32) avalanche(obj);
    }
    static PLY_INLINE u32 hash(u32 obj) {
        return avalanche(obj);
    }
    static PLY_INLINE u32 hash(s64 obj) {
        return (u32) avalanche((u64) obj);
    }
    static PLY_INLINE u32 hash(s32 obj) {
        return avalanche((u32) obj);
    }
};

//------------------------------------------
// Built-in hash support
//------------------------------------------
PLY_DLL_ENTRY Hasher& operator<<(Hasher& hasher, u32 value);

PLY_INLINE Hasher& operator<<(Hasher& hasher, float value) {
#if PLY_COMPILER_GCC
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
#endif
    hasher << (value == 0 ? 0u : *reinterpret_cast<u32*>(&value));
#if PLY_COMPILER_GCC
#pragma GCC diagnostic pop
#endif
    return hasher;
}

PLY_INLINE Hasher& operator<<(Hasher& hasher, u64 value) {
    hasher << u32(value);
    hasher << u32(value >> 32);
    return hasher;
}

// For pointer to class or void*, use pointer value only.
// (Note: char* is not accepted here.)
template <
    typename T,
    std::enable_if_t<std::is_class<T>::value || std::is_same<T, void>::value, int> = 0>
PLY_INLINE Hasher& operator<<(Hasher& hasher, const T* value) {
    hasher << uptr(value);
    return hasher;
};

PLY_DLL_ENTRY Hasher& operator<<(Hasher& hasher, StringView buf);

//  ▄▄           ▄▄            ▄▄▄
//  ██     ▄▄▄▄  ██▄▄▄   ▄▄▄▄   ██
//  ██     ▄▄▄██ ██  ██ ██▄▄██  ██
//  ██▄▄▄ ▀█▄▄██ ██▄▄█▀ ▀█▄▄▄  ▄██▄
//

struct Label {
    u32 idx = 0;

    PLY_INLINE Label() {
    }
    explicit PLY_INLINE Label(u32 idx) : idx{idx} {
    }
    explicit PLY_INLINE operator bool() const {
        return idx != 0;
    }
    PLY_INLINE bool operator==(const Label& other) const {
        return this->idx == other.idx;
    }
    PLY_INLINE bool operator!=(const Label& other) const {
        return this->idx != other.idx;
    }
};

PLY_INLINE Hasher& operator<<(Hasher& hasher, const Label& label) {
    hasher << label.idx;
    return hasher;
}

//  ▄▄  ▄▄               ▄▄     ▄▄   ▄▄
//  ██  ██  ▄▄▄▄   ▄▄▄▄  ██▄▄▄  ███▄███  ▄▄▄▄  ▄▄▄▄▄
//  ██▀▀██  ▄▄▄██ ▀█▄▄▄  ██  ██ ██▀█▀██  ▄▄▄██ ██  ██
//  ██  ██ ▀█▄▄██  ▄▄▄█▀ ██  ██ ██   ██ ▀█▄▄██ ██▄▄█▀
//                                             ██

namespace impl {
struct HashMap {
    static constexpr u32 InitialSize = 4;
    static constexpr u32 LinearSearchLimit = 128;
    static constexpr u8 EmptySlot = 0xff;
    // Flags passed to insert_or_find():
    static constexpr u32 AllowFind = 1;
    static constexpr u32 AllowInsert = 2;

    PLY_STATIC_ASSERT(LinearSearchLimit > 0 &&
                      LinearSearchLimit < 256); // Must fit in CellGroup::deltas

    enum class FindResult {
        NotFound,
        Found,
        InsertedNew,
        Overflow,
    };

    // Must be kept binary compatible with HashMap<>::FindInfo
    struct FindInfo {
        // FIXME: Merge FindResult and return these from internal functions
        u32 idx;
        u8* prev_link;
        void* item_slot;
    };

    struct Callbacks {
        u32 item_size = 0;
        bool is_trivially_destructible = false;
        bool requires_context = false;
        void (*construct)(void* item, const void* key) = nullptr;
        void (*destruct)(void* item) = nullptr;
        void (*move_construct)(void* dst_item, void* src_item) = nullptr;
        void (*move_assign)(void* dst_item, void* src_item) = nullptr;
        u32 (*hash)(const void* key) = nullptr;
        bool (*match)(const void* item, const void* key, const void* context) = nullptr;
    };

    PLY_MAKE_WELL_FORMEDNESS_CHECK_1(HasConstruct, &T0::construct);
    PLY_MAKE_WELL_FORMEDNESS_CHECK_1(HasHash, &T0::hash);
    PLY_MAKE_WELL_FORMEDNESS_CHECK_1(HasContext, (typename T0::Context*) nullptr);

    template <class, class = void>
    struct Context {
        using Type = Empty;
    };
    template <class Traits>
    struct Context<Traits, void_t<typename Traits::Context>> {
        using Type = typename Traits::Context;
    };

    template <class Traits>
    struct CallbackMaker {
        using Key = typename Traits::Key;
        using Item = typename Traits::Item;

        // construct
        template <typename U = Traits, std::enable_if_t<HasConstruct<U>, int> = 0>
        static PLY_NO_INLINE void construct(void* item, const void* key) {
            Traits::construct((Item*) item, *(const Key*) key);
        }
        template <typename U = Traits,
                  std::enable_if_t<!HasConstruct<U> &&
                                       std::is_constructible<typename U::Item,
                                                             typename U::Key>::value,
                                   int> = 0>
        static PLY_NO_INLINE void construct(void* item, const void* key) {
            new (item) Item{*(const Key*) key};
        }
        template <typename U = Traits,
                  std::enable_if_t<!HasConstruct<U> &&
                                       !std::is_constructible<typename U::Item,
                                                              typename U::Key>::value,
                                   int> = 0>
        static PLY_NO_INLINE void construct(void* item, const void* key) {
            new (item) Item{};
        }

        // destruct
        static PLY_NO_INLINE void destruct(void* item) {
            ((Item*) item)->~Item();
        }

        // move_construct
        static PLY_NO_INLINE void move_construct(void* dst_item, void* src_item) {
            new (dst_item) Item{std::move(*(Item*) src_item)};
        }

        // move_assign
        static PLY_NO_INLINE void move_assign(void* dst_item, void* src_item) {
            *(Item*) dst_item = std::move(*(Item*) src_item);
        }

        // hash
        template <typename U = Traits, std::enable_if_t<HasHash<U>, int> = 0>
        static PLY_NO_INLINE u32 hash(const void* key) {
            return Traits::hash(*(const Key*) key);
        }
        template <typename U = Traits, std::enable_if_t<!HasHash<U>, int> = 0>
        static PLY_NO_INLINE u32 hash(const void* key) {
            return Hasher::hash(*(const Key*) key);
        }

        // match (item and key)
        template <typename U = Traits, std::enable_if_t<!HasContext<U>, int> = 0>
        static PLY_NO_INLINE bool match(const void* item, const void* key,
                                        const void*) {
            return Traits::match(*(const Item*) item, *(const Key*) key);
        }
        template <typename U = Traits, std::enable_if_t<HasContext<U>, int> = 0>
        static PLY_NO_INLINE bool match(const void* item, const void* key,
                                        const void* context) {
            return Traits::match(*(const Item*) item, *(const Key*) key,
                                 *(const typename U::Context*) context);
        }

        static PLY_INLINE const Callbacks* instance() {
            static Callbacks ins = {
                sizeof(Item),
                std::is_trivially_destructible<Item>::value,
                HasContext<Traits>,
                &construct,
                &destruct,
                &move_construct,
                &move_assign,
                &hash,
                &match,
            };
            return &ins;
        };
    };

    // Note: Must be kept binary compatible with HashMap<>::CellGroup
    struct CellGroup {
        u8 next_delta[4]; // EmptySlot means the slot is unused
        u8 first_delta[4];
        u32 hashes[4];
    };

    // Note: Must be kept binary compatible with HashMap<>
    CellGroup* m_cellGroups;
    u32 m_sizeMask;
    u32 m_population;

    PLY_DLL_ENTRY HashMap(const Callbacks* cb, u32 initial_size);
    PLY_DLL_ENTRY HashMap(HashMap&& other);
    PLY_DLL_ENTRY void move_assign(const Callbacks* cb, HashMap&& other);
    PLY_DLL_ENTRY void clear(const Callbacks* cb);
    static PLY_DLL_ENTRY CellGroup* create_table(const Callbacks* cb,
                                                 u32 size = InitialSize);
    static PLY_DLL_ENTRY void destroy_table(const Callbacks* cb, CellGroup* cell_groups,
                                            u32 size);
    PLY_DLL_ENTRY void migrate_to_new_table(const Callbacks* cb);
    PLY_DLL_ENTRY FindResult find_next(FindInfo* info, const Callbacks* cb,
                                       const void* key, const void* context) const;
    PLY_DLL_ENTRY FindResult insert_or_find(FindInfo* info, const Callbacks* cb,
                                            const void* key, const void* context,
                                            u32 flags);
    PLY_DLL_ENTRY void* insert_for_migration(u32 item_size, u32 hash);
    PLY_DLL_ENTRY void erase(FindInfo* info, const Callbacks* cb, u8*& link_to_adjust);

    // Must be kept binary compatible with HashMap<>::Cursor:
    struct Cursor {
        HashMap* m_map;
        FindInfo m_findInfo;
        FindResult m_findResult;

        PLY_DLL_ENTRY void construct_find_with_insert(const Callbacks* cb, HashMap* map,
                                                      const void* key,
                                                      const void* context, u32 flags);
    };
};
} // namespace impl

//------------------------------------------------------------------
// CursorMixin is some template magic that lets us use different return values for
// Cursor::operator->, depending on whether Item is a pointer or not.
//
// CursorMixin *could* be eliminated by eliminating operator-> from Cursor, or
// forbidding operator-> for some types of Item, or by eliminating both operator-> and
// operator* and just exposing an explicit get() function instead.
//------------------------------------------------------------------
template <class Cursor, typename Item, bool = std::is_pointer<Item>::value>
class CursorMixin {
public:
    PLY_INLINE Item* operator->() {
        Cursor* cursor = static_cast<Cursor*>(this);
        PLY_ASSERT(cursor->m_findInfo.item_slot);
        return cursor->m_findInfo.item_slot;
    }

    PLY_INLINE const Item* operator->() const {
        const Cursor* cursor = static_cast<const Cursor*>(this);
        PLY_ASSERT(cursor->m_findInfo.item_slot);
        return cursor->m_findInfo.item_slot;
    }
};

template <class Cursor, typename Item>
class CursorMixin<Cursor, Item, true> {
public:
    PLY_INLINE Item operator->() const {
        const Cursor* cursor = static_cast<const Cursor*>(this);
        PLY_ASSERT(*cursor->m_findInfo.item_slot);
        return *cursor->m_findInfo.item_slot;
    }
};

template <class Traits>
class HashMap {
private:
    using Key = typename Traits::Key;
    using Item = typename Traits::Item;
    using Context = typename impl::HashMap::Context<Traits>::Type;
    using Callbacks = impl::HashMap::CallbackMaker<Traits>;

    // Note: Must be kept binary compatible with impl::HashMap::CellGroup
    struct CellGroup {
        u8 next_delta[4]; // EmptySlot means the slot is unused
        u8 first_delta[4];
        u32 hashes[4];
        Item items[4];
    };

    // Note: Must be kept binary compatible with impl::HashMap
    CellGroup* m_cellGroups;
    u32 m_sizeMask;
    u32 m_population;

    // Must be kept binary-compatible with impl::HashMap::FindInfo:
    struct FindInfo {
        u32 idx;
        u8* prev_link;
        Item* item_slot; // null means not found
    };

public:
    PLY_INLINE HashMap(u32 initial_size = 8) {
        new (this) impl::HashMap(Callbacks::instance(), initial_size);
    }

    PLY_INLINE HashMap(HashMap&& other) {
        new (this) impl::HashMap{std::move((impl::HashMap&) other)};
        other.m_cellGroups = nullptr;
    }

    PLY_INLINE ~HashMap() {
        if (m_cellGroups) {
            impl::HashMap::destroy_table(Callbacks::instance(),
                                         (impl::HashMap::CellGroup*) m_cellGroups,
                                         m_sizeMask + 1);
        }
    }

    PLY_INLINE void operator=(HashMap&& other) {
        reinterpret_cast<impl::HashMap*>(this)->move_assign(
            Callbacks::instance(), std::move(reinterpret_cast<impl::HashMap&>(other)));
        other.m_cellGroups = nullptr;
    }

    PLY_INLINE void clear() {
        reinterpret_cast<impl::HashMap*>(this)->clear(Callbacks::instance());
    }

    class Cursor : public CursorMixin<Cursor, Item> {
    private:
        PLY_STATIC_ASSERT(sizeof(CellGroup) ==
                          sizeof(impl::HashMap::CellGroup) + sizeof(Item) * 4);

        friend class HashMap;
        template <class, typename, bool>
        friend class CursorMixin;

        // Must be kept binary-compatible with impl::HashMap::Cursor:
        HashMap* m_map;
        FindInfo m_findInfo;
        impl::HashMap::FindResult m_findResult;

        // Find without insert
        PLY_INLINE Cursor(HashMap* map, const Key& key, const Context* context)
            : m_map{map} {
            m_findResult = reinterpret_cast<impl::HashMap*>(m_map)->insert_or_find(
                (impl::HashMap::FindInfo*) &m_findInfo, Callbacks::instance(), &key,
                context, impl::HashMap::AllowFind);
        }
        // Find with insert
        PLY_INLINE Cursor(HashMap* map, const Key& key, const Context* context,
                          u32 flags) {
            reinterpret_cast<impl::HashMap::Cursor*>(this)->construct_find_with_insert(
                Callbacks::instance(), (impl::HashMap*) map, &key, context, flags);
        }

    public:
        PLY_INLINE void operator=(const Cursor& other) {
            m_map = other.m_map;
            m_findInfo = other.m_findInfo;
            m_findResult = other.m_findResult;
        }
        PLY_INLINE bool is_valid() const {
            return m_findResult != impl::HashMap::FindResult::NotFound;
        }
        PLY_INLINE bool was_found() const {
            return m_findResult == impl::HashMap::FindResult::Found;
        }
        PLY_INLINE void next(const Key& key, const Context& context = {}) {
            m_findResult = reinterpret_cast<const impl::HashMap*>(m_map)->find_next(
                (impl::HashMap::FindInfo*) &m_findInfo, Callbacks::instance(), &key,
                &context);
        }
        PLY_INLINE Item& operator*() {
            PLY_ASSERT(m_findInfo.item_slot);
            return *m_findInfo.item_slot;
        }
        PLY_INLINE const Item& operator*() const {
            PLY_ASSERT(m_findInfo.item_slot);
            return *m_findInfo.item_slot;
        }
        PLY_INLINE void erase() {
            u8* unused_link = nullptr;
            reinterpret_cast<impl::HashMap*>(m_map)->erase(
                (impl::HashMap::FindInfo*) &m_findInfo, Callbacks::instance(),
                unused_link);
            m_findResult = impl::HashMap::FindResult::NotFound;
        }
        PLY_INLINE void erase_and_advance(const Key& key, const Context& context = {}) {
            FindInfo info_to_erase = m_findInfo;
            m_findResult = reinterpret_cast<const impl::HashMap&>(m_map).find_next(
                (impl::HashMap::FindInfo*) &m_findInfo, Callbacks::instance(), &key,
                &context);
            reinterpret_cast<impl::HashMap*>(m_map)->erase(
                (impl::HashMap::FindInfo*) &info_to_erase, Callbacks::instance(),
                m_findInfo.prev_link);
        }
    };

    class ConstCursor : public CursorMixin<ConstCursor, Item> {
    private:
        PLY_STATIC_ASSERT(sizeof(CellGroup) ==
                          sizeof(impl::HashMap::CellGroup) + sizeof(Item) * 4);

        friend class HashMap;
        template <class, typename, bool>
        friend class CursorMixin;

        const HashMap* m_map;
        FindInfo m_findInfo;
        impl::HashMap::FindResult m_findResult;

        // Find without insert
        PLY_INLINE ConstCursor(const HashMap* map, const Key& key,
                               const Context* context)
            : m_map{map} {
            m_findResult = ((impl::HashMap*) m_map)
                               ->insert_or_find((impl::HashMap::FindInfo*) &m_findInfo,
                                                Callbacks::instance(), &key, context,
                                                impl::HashMap::AllowFind);
        }

    public:
        PLY_INLINE void operator=(const ConstCursor& other) {
            m_map = other.m_map;
            m_findInfo = other.m_findInfo;
            m_findResult = other.m_findResult;
        }
        PLY_INLINE bool is_valid() const {
            return m_findResult != impl::HashMap::FindResult::NotFound;
        }
        PLY_INLINE bool was_found() const {
            return m_findResult == impl::HashMap::FindResult::Found;
        }
        PLY_INLINE void next(const Key& key, const Context& context = {}) {
            m_findResult = reinterpret_cast<const impl::HashMap&>(m_map).find_next(
                (impl::HashMap::FindInfo*) &m_findInfo, Callbacks::instance(), &key,
                &context);
        }
        PLY_INLINE Item& operator*() {
            PLY_ASSERT(m_findInfo.item_slot);
            return *m_findInfo.item_slot;
        }
        PLY_INLINE const Item& operator*() const {
            PLY_ASSERT(m_findInfo.item_slot);
            return *m_findInfo.item_slot;
        }
    };

    PLY_INLINE bool is_empty() const {
        return m_population == 0;
    }

    PLY_INLINE u32 num_items() const {
        return m_population;
    }

    PLY_INLINE Cursor insert_or_find(const Key& key, const Context* context = nullptr) {
        return {this, key, context,
                impl::HashMap::AllowFind | impl::HashMap::AllowInsert};
    }

    // insert_multi is experimental and should not be used. Will likely delete.
    // FIXME: Delete this function and its associated support code, including
    // Cursor::next()
    PLY_INLINE Cursor insert_multi(const Key& key, const Context* context = nullptr) {
        return {this, key, context, impl::HashMap::AllowInsert};
    }

    PLY_INLINE Cursor find(const Key& key, const Context* context = nullptr) {
        return {this, key, context};
    }
    PLY_INLINE ConstCursor find(const Key& key,
                                const Context* context = nullptr) const {
        return {this, key, context};
    }

    //------------------------------------------------------------------
    // Iterator
    //------------------------------------------------------------------
    class Iterator {
    private:
        friend class HashMap;
        HashMap& m_map;
        u32 m_idx;

    public:
        Iterator(HashMap& map, u32 idx) : m_map{map}, m_idx{idx} {
        }

        bool is_valid() const {
            return m_idx <= m_map.m_sizeMask;
        }

        void next() {
            for (;;) {
                m_idx++;
                PLY_ASSERT(m_idx <= m_map.m_sizeMask + 1);
                if (m_idx > m_map.m_sizeMask)
                    break;
                CellGroup* group =
                    m_map.m_cellGroups + ((m_idx & m_map.m_sizeMask) >> 2);
                if (group->next_delta[m_idx & 3] != impl::HashMap::EmptySlot)
                    break;
            }
        }

        bool operator!=(const Iterator& other) const {
            PLY_ASSERT(&m_map == &other.m_map);
            return m_idx != other.m_idx;
        }

        void operator++() {
            next();
        }

        Item& operator*() const {
            PLY_ASSERT(m_idx <= m_map.m_sizeMask);
            CellGroup* group = m_map.m_cellGroups + ((m_idx & m_map.m_sizeMask) >> 2);
            PLY_ASSERT(group->next_delta[m_idx & 3] != impl::HashMap::EmptySlot);
            return group->items[m_idx & 3];
        }

        Item* operator->() const {
            return &(**this);
        }
    };

    //------------------------------------------------------------------
    // ConstIterator
    //------------------------------------------------------------------
    class ConstIterator {
    private:
        friend class HashMap;
        const HashMap& m_map;
        u32 m_idx;

    public:
        ConstIterator(const HashMap& map, u32 idx) : m_map{map}, m_idx{idx} {
        }

        bool is_valid() const {
            return m_idx <= m_map.m_sizeMask;
        }

        void next() {
            for (;;) {
                m_idx++;
                PLY_ASSERT(m_idx <= m_map.m_sizeMask + 1);
                if (m_idx > m_map.m_sizeMask)
                    break;
                CellGroup* group =
                    m_map.m_cellGroups + ((m_idx & m_map.m_sizeMask) >> 2);
                if (group->next_delta[m_idx & 3] != impl::HashMap::EmptySlot)
                    break;
            }
        }

        bool operator!=(const ConstIterator& other) const {
            PLY_ASSERT(&m_map == &other.m_map);
            return m_idx != other.m_idx;
        }

        void operator++() {
            next();
        }

        const Item& operator*() const {
            PLY_ASSERT(m_idx <= m_map.m_sizeMask);
            CellGroup* group = m_map.m_cellGroups + ((m_idx & m_map.m_sizeMask) >> 2);
            PLY_ASSERT(group->next_delta[m_idx & 3] != impl::HashMap::EmptySlot);
            return group->items[m_idx & 3];
        }

        const Item* operator->() const {
            return &(**this);
        }
    };

    Iterator begin() {
        if (this->m_sizeMask == 0) {
            return {*this, m_sizeMask + 1};
        }
        Iterator iter{*this, (u32) -1};
        iter.next();
        return iter;
    }
    ConstIterator begin() const {
        if (this->m_sizeMask == 0) {
            return {*this, m_sizeMask + 1};
        }
        ConstIterator iter{*this, (u32) -1};
        iter.next();
        return iter;
    }
    Iterator end() {
        return {*this, m_sizeMask + 1};
    }
    ConstIterator end() const {
        return {*this, m_sizeMask + 1};
    }
};

//  ▄▄   ▄▄
//  ███▄███  ▄▄▄▄  ▄▄▄▄▄
//  ██▀█▀██  ▄▄▄██ ██  ██
//  ██   ██ ▀█▄▄██ ██▄▄█▀
//                 ██

// Key can be u32, u64, String, StringView, Label or T*.

struct BaseMap {
    Array<s32> indices;
    BaseArray items;
};

enum MapOperation { M_Find, M_Insert };

struct MapTypeInfo {
    u32 item_size;
    u32 value_offset;
    void (*construct)(void* value);
    void (*destruct)(void* value);
};

template <typename Key, typename Value>
const MapTypeInfo map_type_info = {
    u32{sizeof(typename Map<Key, Value>::Item)},
    u32{(uptr) & ((typename Map<Key, Value>::Item*) 0)->value},
    [](void* value) { new (value) Value; },
    [](void* value) { ((Value*) value)->~Value(); },
};

// map_operate() is defined out-of-line and explicitly instantiated for u32, u64,
// String and StringView.
template <typename Prim>
void* map_operate(BaseMap* map, MapOperation op, View<Prim> key, const MapTypeInfo* ti,
                  bool* was_found = nullptr);

// MapHelper<> helps adapt Map<> to different key types.
template <typename K>
struct MapHelper {  // Base template for u32, u64 and StringView.
    using Prim = K; // The internal type used by map_operate().
    static K prim(K key) {
        return key;
    }
};
template <>
struct MapHelper<String> { // Specialization for String.
    using Prim = String;
    static StringView prim(StringView key) {
        return key;
    }
};
template <>
struct MapHelper<Label> { // Specialization for Label.
    using Prim = u32;
    static u32 prim(Label key) {
        return key.idx;
    }
};
template <typename T>
struct MapHelper<T*> { // Specialization for T*.
    using Prim = uptr;
    static uptr prim(T* key) {
        return (uptr) key;
    }
};

// ┏━━━━━━━┓
// ┃  Map  ┃
// ┗━━━━━━━┛
template <typename Key, typename Value>
struct Map {
    struct Item {
        Key key;
        Value value;
    };

    Array<s32> indices;
    Array<Item> items;

    using Prim = typename MapHelper<Key>::Prim;

    Value* find(View<Key> key) {
        PLY_PUN_SCOPE
        return (Value*) map_operate<Prim>((BaseMap*) this, M_Find,
                                          MapHelper<Key>::prim(key),
                                          &map_type_info<Key, Value>);
    }
    const Value* find(View<Key> key) const {
        PLY_PUN_SCOPE
        return (Value*) map_operate<Prim>((BaseMap*) this, M_Find,
                                          MapHelper<Key>::prim(key),
                                          &map_type_info<Key, Value>);
    }
    Value* insert_or_find(View<Key> key, bool* was_found = nullptr) {
        PLY_PUN_SCOPE
        return (Value*) map_operate<Prim>((BaseMap*) this, M_Insert,
                                          MapHelper<Key>::prim(key),
                                          &map_type_info<Key, Value>, was_found);
    }
    template <typename T>
    void assign(View<Key> key, T&& arg) {
        PLY_PUN_SCOPE
        void* value =
            map_operate<Prim>((BaseMap*) this, M_Insert, MapHelper<Key>::prim(key),
                              &map_type_info<Key, Value>);
        *((Value*) value) = std::forward<T>(arg);
    }
    u32 num_items() const {
        return this->items.num_items();
    }

    // Range-for support.
    Item* begin() {
        return this->items.begin();
    }
    Item* end() {
        return this->items.end();
    }
    const Item* begin() const {
        return this->items.begin();
    }
    const Item* end() const {
        return this->items.end();
    }
};

//  ▄▄           ▄▄            ▄▄▄   ▄▄▄▄   ▄▄
//  ██     ▄▄▄▄  ██▄▄▄   ▄▄▄▄   ██  ██  ▀▀ ▄██▄▄  ▄▄▄▄  ▄▄▄▄▄   ▄▄▄▄   ▄▄▄▄▄  ▄▄▄▄
//  ██     ▄▄▄██ ██  ██ ██▄▄██  ██   ▀▀▀█▄  ██   ██  ██ ██  ▀▀  ▄▄▄██ ██  ██ ██▄▄██
//  ██▄▄▄ ▀█▄▄██ ██▄▄█▀ ▀█▄▄▄  ▄██▄ ▀█▄▄█▀  ▀█▄▄ ▀█▄▄█▀ ██     ▀█▄▄██ ▀█▄▄██ ▀█▄▄▄
//                                                                     ▄▄▄█▀

namespace impl {

struct LabelEncoder {
    static PLY_INLINE u32 decode_value(const char*& ptr) {
        u32 value = 0;
        for (;;) {
            u8 c = *ptr;
            ptr++;
            value += (c & 127);
            if ((c >> 7) == 0)
                break;
            value <<= 7;
        }
        return value;
    }

    static PLY_INLINE u32 get_enc_len(u32 value) {
        u32 enc_len = 0;
        do {
            enc_len++;
            value >>= 7;
        } while (value > 0);
        return enc_len;
    }

    static PLY_NO_INLINE void encode_value(char*& ptr, u32 value) {
        u32 shift = (get_enc_len(value) - 1) * 7;
        for (;;) {
            *ptr = u8(((value >> shift) & 127) | ((shift != 0) << 7));
            ptr++;
            if (shift == 0)
                break;
            shift -= 7;
        }
    }
};

} // namespace impl

struct LabelStorage {
private:
    struct Traits {
        using Key = StringView;
        using Item = u32;
        using Context = BigPool<>;
        static bool match(u32 id, const StringView& key, const BigPool<>& big_pool) {
            const char* ptr = big_pool.get(id);
            u32 num_bytes = impl::LabelEncoder::decode_value(ptr);
            return StringView{ptr, num_bytes} == key;
        }
    };

    PLY_DEFINE_RACE_DETECTOR(race_detector)
    BigPool<> big_pool;
    HashMap<Traits> str_to_index;

public:
    LabelStorage();

    Label insert(StringView view);
    Label find(StringView view) const;
    StringView view(Label label) const;
};

extern LabelStorage g_labelStorage;

} // namespace ply
