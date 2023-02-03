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
        subst::destructByMember(get<T>());
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
    T to(const T& defaultValue = subst::createDefault<T>()) const {
        return static_cast<const Derived*>(this)->view().to(defaultValue);
    }

    explicit operator bool() const {
        return (bool) static_cast<const Derived*>(this)->view();
    }

    bool isEmpty() const {
        return static_cast<const Derived*>(this)->view().isEmpty();
    }

    StringView subStr(u32 start) const {
        return static_cast<const Derived*>(this)->view().subStr(start);
    }
    StringView subStr(u32 start, u32 numBytes) const {
        return static_cast<const Derived*>(this)->view().subStr(start, numBytes);
    }

    StringView left(u32 numBytes) const {
        return static_cast<const Derived*>(this)->view().left(numBytes);
    }
    StringView shortenedBy(u32 numBytes) const {
        return static_cast<const Derived*>(this)->view().shortenedBy(numBytes);
    }
    StringView right(u32 numBytes) const {
        return static_cast<const Derived*>(this)->view().right(numBytes);
    }
    String operator+(StringView other) const;
    String operator*(u32 count) const;
    s32 findByte(char matchByte, u32 startPos = 0) const {
        return static_cast<const Derived*>(this)->view().findByte(matchByte, startPos);
    }
    template <typename MatchFuncOrChar>
    s32 findByte(const MatchFuncOrChar& matchFuncOrByte, u32 startPos = 0) const {
        return static_cast<const Derived*>(this)->view().findByte(matchFuncOrByte,
                                                                  startPos);
    }

    template <typename MatchFuncOrChar>
    s32 rfindByte(const MatchFuncOrChar& matchFuncOrByte, u32 startPos) const {
        return static_cast<const Derived*>(this)->view().rfindByte(matchFuncOrByte,
                                                                   startPos);
    }
    template <typename MatchFuncOrChar>
    s32 rfindByte(const MatchFuncOrChar& matchFuncOrByte) const {
        return static_cast<const Derived*>(this)->view().rfindByte(
            matchFuncOrByte, static_cast<const Derived*>(this)->numBytes - 1);
    }

    bool startsWith(StringView arg) const {
        return static_cast<const Derived*>(this)->view().startsWith(arg);
    }
    bool endsWith(StringView arg) const {
        return static_cast<const Derived*>(this)->view().endsWith(arg);
    }

    StringView trim(bool (*matchFunc)(char) = isWhite, bool left = true,
                    bool right = true) const {
        return static_cast<const Derived*>(this)->view().trim(matchFunc, left, right);
    }
    StringView ltrim(bool (*matchFunc)(char) = isWhite) const {
        return static_cast<const Derived*>(this)->view().ltrim(matchFunc);
    }
    StringView rtrim(bool (*matchFunc)(char) = isWhite) const {
        return static_cast<const Derived*>(this)->view().rtrim(matchFunc);
    }
    String join(ArrayView<const StringView> comps) const;
    auto splitByte(char sep) const {
        return static_cast<const Derived*>(this)->view().splitByte(sep);
    }
    String upperAsc() const;
    String lowerAsc() const;
    String reversedBytes() const;
    String filterBytes(char (*filterFunc)(char)) const;
    bool includesNullTerminator() const {
        return static_cast<const Derived*>(this)->view().includesNullTerminator();
    }
    HybridString withNullTerminator() const;
    StringView withoutNullTerminator() const {
        return static_cast<const Derived*>(this)->view().withoutNullTerminator();
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
    u32 numBytes = 0;

    String() = default;
    String(StringView other);
    String(const String& other) : String{other.view()} {
    }
    String(const char* s) : String{StringView{s}} {
    }
    template <typename U, typename = std::enable_if_t<std::is_same<U, char>::value>>
    String(const U& u) : String{StringView{&u, 1}} {
    }
    String(String&& other) : bytes{other.bytes}, numBytes{other.numBytes} {
        other.bytes = nullptr;
        other.numBytes = 0;
    }
    String(HybridString&& other);

    ~String() {
        if (this->bytes) {
            Heap.free(this->bytes);
        }
    }

    template <typename = void>
    void operator=(StringView other) {
        char* bytesToFree = this->bytes;
        new (this) String{other};
        if (bytesToFree) {
            Heap.free(bytesToFree);
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
        this->numBytes = 0;
    }

    void operator+=(StringView other) {
        *this = this->view() + other;
    }
    static String allocate(u32 numBytes);
    void resize(u32 numBytes);
    static String adopt(char* bytes, u32 numBytes) {
        String str;
        str.bytes = bytes;
        str.numBytes = numBytes;
        return str;
    }

    char* release() {
        char* r = this->bytes;
        this->bytes = nullptr;
        this->numBytes = 0;
        return r;
    }

    template <typename... Args>
    static String format(StringView fmt, const Args&... args);

    const char& operator[](u32 index) const {
        PLY_ASSERT(index < this->numBytes);
        return this->bytes[index];
    }
    char& operator[](u32 index) {
        PLY_ASSERT(index < this->numBytes);
        return this->bytes[index];
    }

    const char& back(s32 ofs = -1) const {
        PLY_ASSERT(u32(-ofs - 1) < this->numBytes);
        return this->bytes[this->numBytes + ofs];
    }
    char& back(s32 ofs = -1) {
        PLY_ASSERT(u32(-ofs - 1) < this->numBytes);
        return this->bytes[this->numBytes + ofs];
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
String StringMixin<Derived>::upperAsc() const {
    return static_cast<const Derived*>(this)->view().upperAsc();
}

template <typename Derived>
String StringMixin<Derived>::lowerAsc() const {
    return static_cast<const Derived*>(this)->view().lowerAsc();
}

template <typename Derived>
String StringMixin<Derived>::reversedBytes() const {
    return static_cast<const Derived*>(this)->view().reversedBytes();
}

template <typename Derived>
String StringMixin<Derived>::filterBytes(char (*filterFunc)(char)) const {
    return static_cast<const Derived*>(this)->view().filterBytes(filterFunc);
}

//  ▄▄  ▄▄        ▄▄            ▄▄     ▄▄  ▄▄▄▄   ▄▄          ▄▄
//  ██  ██ ▄▄  ▄▄ ██▄▄▄  ▄▄▄▄▄  ▄▄  ▄▄▄██ ██  ▀▀ ▄██▄▄ ▄▄▄▄▄  ▄▄ ▄▄▄▄▄   ▄▄▄▄▄
//  ██▀▀██ ██  ██ ██  ██ ██  ▀▀ ██ ██  ██  ▀▀▀█▄  ██   ██  ▀▀ ██ ██  ██ ██  ██
//  ██  ██ ▀█▄▄██ ██▄▄█▀ ██     ██ ▀█▄▄██ ▀█▄▄█▀  ▀█▄▄ ██     ██ ██  ██ ▀█▄▄██
//          ▄▄▄█▀                                                        ▄▄▄█▀

struct HybridString : StringMixin<HybridString> {
    char* bytes;
    u32 isOwner : 1;
    u32 numBytes : 31;

    HybridString() : bytes{nullptr}, isOwner{0}, numBytes{0} {
    }
    HybridString(StringView view)
        : bytes{const_cast<char*>(view.bytes)}, isOwner{0}, numBytes{view.numBytes} {
        PLY_ASSERT(view.numBytes < (1u << 30));
    }

    HybridString(const String& str) : HybridString{str.view()} {
    }
    HybridString(String&& str) {
        this->isOwner = 1;
        PLY_ASSERT(str.numBytes < (1u << 30));
        this->numBytes = str.numBytes;
        this->bytes = str.release();
    }

    HybridString(HybridString&& other)
        : bytes{other.bytes}, isOwner{other.isOwner}, numBytes{other.numBytes} {
        other.bytes = nullptr;
        other.isOwner = 0;
        other.numBytes = 0;
    }

    HybridString(const HybridString& other);

    HybridString(const char* s)
        : bytes{const_cast<char*>(s)}, isOwner{0},
          numBytes{(u32) std::char_traits<char>::length(s)} {
        PLY_ASSERT(s[this->numBytes] ==
                   0); // Sanity check; numBytes must fit in 31-bit field
    }

    ~HybridString() {
        if (this->isOwner) {
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
        return {this->bytes, this->numBytes};
    }

    StringView view() const {
        return {this->bytes, this->numBytes};
    }
};

template <typename Derived>
HybridString StringMixin<Derived>::withNullTerminator() const {
    return static_cast<const Derived*>(this)->view().withNullTerminator();
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

    void alloc(u32 numItems, u32 itemSize);
    void realloc(u32 numItems, u32 itemSize);
    void free();
    void reserve(u32 numItems, u32 itemSize);
    void reserveIncrement(u32 itemSize);
    void truncate(u32 itemSize);
};

template <typename T>
class Array {
private:
    T* items;
    u32 numItems_;
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

    Array(T* items, u32 numItems, u32 allocator)
        : items{items}, numItems_{numItems}, allocated{allocated} {
    }

public:
    // Constructors
    Array() : items{nullptr}, numItems_{0}, allocated{0} {
    }
    Array(const Array& other) {
        ((BaseArray*) this)->alloc(other.numItems_, (u32) sizeof(T));
        subst::unsafeConstructArrayFrom(this->items, other.items, other.numItems_);
    }
    Array(Array&& other)
        : items{other.items}, numItems_{other.numItems_}, allocated{other.allocated} {
        other.items = nullptr;
        other.numItems_ = 0;
        other.allocated = 0;
    }
    Array(InitList<T> init) {
        u32 initSize = safeDemote<u32>(init.size());
        ((BaseArray*) this)->alloc(initSize, (u32) sizeof(T));
        subst::constructArrayFrom(this->items, init.begin(), initSize);
    }
    template <typename Other, typename U = impl::ArrayViewType<Other>>
    Array(Other&& other) {
        ((BaseArray*) this)->alloc(ArrayView<U>{other}.numItems, (u32) sizeof(T));
        impl::moveOrCopyConstruct(this->items, std::forward<Other>(other));
    }
    ~Array() {
        PLY_STATIC_ASSERT(sizeof(Array) == sizeof(BaseArray));
        subst::destructArray(this->items, this->numItems_);
        Heap.free(this->items);
    }

    void operator=(const Array& other) {
        ArrayView<T> arrayToFree = {this->items, this->numItems_};
        new (this) Array{other};
        subst::destructArray(arrayToFree.items, arrayToFree.numItems);
        Heap.free(arrayToFree.items);
    }

    void operator=(Array&& other) {
        ArrayView<T> arrayToFree = {this->items, this->numItems_};
        new (this) Array{std::move(other)};
        subst::destructArray(arrayToFree.items, arrayToFree.numItems);
        Heap.free(arrayToFree.items);
    }
    void operator=(std::initializer_list<T> init) {
        subst::destructArray(this->items, this->numItems_);
        u32 initSize = safeDemote<u32>(init.size());
        ((BaseArray*) this)->realloc(initSize, (u32) sizeof(T));
        subst::unsafeConstructArrayFrom(this->items, init.begin(), initSize);
    }
    template <typename Other, typename = impl::ArrayViewType<Other>>
    void operator=(Other&& other) {
        ArrayView<T> arrayToFree = {this->items, this->numItems_};
        new (this) Array{std::forward<Other>(other)};
        subst::destructArray(arrayToFree.items, arrayToFree.numItems);
        Heap.free(arrayToFree.items);
    }

    T& operator[](u32 index) {
        PLY_ASSERT(index < this->numItems_);
        return this->items[index];
    }
    const T& operator[](u32 index) const {
        PLY_ASSERT(index < this->numItems_);
        return this->items[index];
    }

    T* get(u32 index = 0) {
        PLY_ASSERT(index < this->numItems_);
        return this->items + index;
    }
    const T* get(u32 index = 0) const {
        PLY_ASSERT(index < this->numItems_);
        return this->items + index;
    }

    T& back(s32 offset = -1) {
        PLY_ASSERT(offset < 0 && u32(-offset) <= this->numItems_);
        return this->items[this->numItems_ + offset];
    }
    const T& back(s32 offset = -1) const {
        PLY_ASSERT(offset < 0 && u32(-offset) <= this->numItems_);
        return this->items[this->numItems_ + offset];
    }

    T* begin() const {
        return this->items;
    }
    T* end() const {
        return this->items + this->numItems_;
    }

    explicit operator bool() const {
        return this->numItems_ > 0;
    }
    bool isEmpty() const {
        return this->numItems_ == 0;
    }
    u32 numItems() const {
        return this->numItems_;
    }
    u32 sizeBytes() const {
        return this->numItems_ * (u32) sizeof(T);
    }
    void clear() {
        subst::destructArray(this->items, this->numItems_);
        Heap.free(this->items);
        this->items = nullptr;
        this->numItems_ = 0;
        this->allocated = 0;
    }

    void reserve(u32 numItems) {
        ((BaseArray*) this)->reserve(numItems, (u32) sizeof(T));
    }
    void resize(u32 numItems) {
        if (numItems < this->numItems_) {
            subst::destructArray(this->items + numItems, this->numItems_ - numItems);
        }
        ((BaseArray*) this)->reserve(numItems, (u32) sizeof(T));
        if (numItems > this->numItems_) {
            subst::constructArray(this->items + this->numItems_,
                                  numItems - this->numItems_);
        }
        this->numItems_ = numItems;
    }

    void truncate() {
        ((BaseArray*) this)->truncate((u32) sizeof(T));
    }

    T& append(T&& item) {
        // The argument must not be a reference to an existing item in the array:
        PLY_ASSERT((&item < this->items) || (&item >= this->items + this->numItems_));
        if (this->numItems_ >= this->allocated) {
            ((BaseArray*) this)->reserveIncrement((u32) sizeof(T));
        }
        T* result = new (this->items + this->numItems_) T{std::move(item)};
        this->numItems_++;
        return *result;
    }
    T& append(const T& item) {
        // The argument must not be a reference to an existing item in the array:
        PLY_ASSERT((&item < this->items) || (&item >= this->items + this->numItems_));
        if (this->numItems_ >= this->allocated) {
            ((BaseArray*) this)->reserveIncrement((u32) sizeof(T));
        }
        T* result = new (this->items + this->numItems_) T{item};
        this->numItems_++;
        return *result;
    }
    template <typename... Args>
    T& append(Args&&... args) {
        if (this->numItems_ >= this->allocated) {
            ((BaseArray*) this)->reserveIncrement((u32) sizeof(T));
        }
        T* result = new (this->items + this->numItems_) T{std::forward<Args>(args)...};
        this->numItems_++;
        return *result;
    }

    void extend(InitList<T> init) {
        u32 initSize = safeDemote<u32>(init.size());
        ((BaseArray*) this)->reserve(this->numItems_ + initSize, (u32) sizeof(T));
        subst::constructArrayFrom(this->items + this->numItems_, init.begin(),
                                  initSize);
        this->numItems_ += initSize;
    }
    template <typename Other, typename U = impl::ArrayViewType<Other>>
    void extend(Other&& other) {
        u32 numOtherItems = ArrayView<U>{other}.numItems;
        ((BaseArray&) *this).reserve(this->numItems_ + numOtherItems, (u32) sizeof(T));
        impl::moveOrCopyConstruct(this->items + this->numItems_,
                                  std::forward<Other>(other));
        this->numItems_ += numOtherItems;
    }
    void moveExtend(ArrayView<T> other) {
        // The argument must not be a subview into the array itself:
        PLY_ASSERT((other.end() <= this->items) || (other.items >= this->end()));
        ((BaseArray&) *this).reserve(this->numItems_ + other.numItems, (u32) sizeof(T));
        subst::moveConstructArray(this->items + this->numItems_, other.items,
                                  other.numItems);
        this->numItems_ += other.numItems;
    }

    void pop(u32 count = 1) {
        PLY_ASSERT(count <= this->numItems_);
        resize(this->numItems_ - count);
    }
    T& insert(u32 pos, u32 count = 1) {
        PLY_ASSERT(pos <= this->numItems_);
        ((BaseArray*) this)->reserve(this->numItems_ + count, (u32) sizeof(T));
        memmove(static_cast<void*>(this->items + pos + count),
                static_cast<const void*>(this->items + pos),
                (this->numItems_ - pos) * sizeof(T)); // Underlying type is relocatable
        subst::constructArray(this->items + pos, count);
        this->numItems_ += count;
        return this->items[pos];
    }

    void erase(u32 pos, u32 count = 1) {
        PLY_ASSERT(pos + count <= this->numItems_);
        subst::destructArray(this->items + pos, count);
        memmove(static_cast<void*>(this->items + pos),
                static_cast<const void*>(this->items + pos + count),
                (this->numItems_ - (pos + count)) *
                    sizeof(T)); // Underlying type is relocatable
        this->numItems_ -= count;
    }
    void eraseQuick(u32 pos) {
        PLY_ASSERT(pos < this->numItems_);
        this->items[pos].~T();
        memcpy(static_cast<void*>(this->items + pos),
               static_cast<const void*>(this->items + (this->numItems_ - 1)),
               sizeof(T));
        this->numItems_--;
    }
    void eraseQuick(u32 pos, u32 count) {
        PLY_ASSERT(pos + count <= this->numItems_);
        subst::destructArray(this->items + pos, count);
        memmove(static_cast<void*>(this->items + pos),
                static_cast<const void*>(this->items + this->numItems_ - count),
                count * sizeof(T)); // Underlying type is relocatable
        this->numItems_ -= count;
    }

    static Array adopt(T* items, u32 numItems) {
        return {items, numItems, numItems};
    }
    T* release() {
        T* items = this->items;
        this->items = nullptr;
        this->numItems_ = 0;
        this->allocated = 0;
        return items;
    }

    template <typename Arr0, typename Arr1, typename, typename>
    friend auto operator+(Arr0&& a, Arr1&& b);

    ArrayView<T> view() {
        return {this->items, this->numItems_};
    }
    ArrayView<const T> view() const {
        return {this->items, this->numItems_};
    }
    operator ArrayView<T>() {
        return {this->items, this->numItems_};
    }
    operator ArrayView<const T>() const {
        return {this->items, this->numItems_};
    }

    StringView stringView() const {
        return {(const char*) this->items,
                safeDemote<u32>(this->numItems_ * sizeof(T))};
    }
    MutStringView mutableStringView() const {
        return {(char*) this->items, safeDemote<u32>(this->numItems_ * sizeof(T))};
    }

    ArrayView<T> subView(u32 start) {
        return view().subView(start);
    }
    ArrayView<const T> subView(u32 start) const {
        return view().subView(start);
    }
    ArrayView<T> subView(u32 start, u32 numItems_) {
        return view().subView(start, numItems_);
    }
    ArrayView<const T> subView(u32 start, u32 numItems_) const {
        return view().subView(start, numItems_);
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
    u32 numItemsA = ArrayView<T0>{a}.numItems;
    u32 numItemsB = ArrayView<T1>{b}.numItems;

    Array<std::remove_const_t<T0>> result;
    ((BaseArray&) result).alloc(numItemsA + numItemsB, (u32) sizeof(T0));
    impl::moveOrCopyConstruct(result.items, std::forward<Arr0>(a));
    impl::moveOrCopyConstruct(result.items + numItemsA, std::forward<Arr1>(b));
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
        subst::constructArrayFrom(this->items, args.begin(), Size);
    }

    template <typename... Args>
    FixedArray(Args&&... args) {
        PLY_STATIC_ASSERT(Size == sizeof...(Args));
        impl::InitItems<T>::init(items, std::forward<Args>(args)...);
    }

    constexpr u32 numItems() const {
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

    MutStringView mutableStringView() {
        return {reinterpret_cast<char*>(items), safeDemote<u32>(Size * sizeof(T))};
    }

    StringView stringView() const {
        return {reinterpret_cast<const char*>(items),
                safeDemote<u32>(Size * sizeof(T))};
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
    void* storedArg = nullptr;

public:
    Func() = default;

    PLY_INLINE Func(const Func& other)
        : handler{other.handler}, storedArg{other.storedArg} {
    }

    template <typename T>
    PLY_INLINE Func(Return (*handler)(T*, Args...), T* storedArg)
        : handler{(Handler*) handler}, storedArg{(void*) storedArg} {
    }

    template <typename T>
    PLY_INLINE Func(Return (T::*handler)(Args...), T* target)
        : storedArg{(void*) target} {
        this->handler = [this](void* target, Args... args) {
            return ((T*) target)->*(this->hiddenArg)(std::forward<Args>(args)...);
        };
    }

    // Support lambda expressions
    template <
        typename Callable,
        typename = void_t<decltype(std::declval<Callable>()(std::declval<Args>()...))>>
    Func(const Callable& callable) : storedArg{(void*) &callable} {
        this->handler = [](void* callable, Args... args) -> Return {
            return (*(const Callable*) callable)(std::forward<Args>(args)...);
        };
    }

    PLY_INLINE void operator=(const Func& other) {
        this->handler = other.handler;
        this->storedArg = other.storedArg;
    }

    PLY_INLINE explicit operator bool() const {
        return this->handler != nullptr;
    }

    template <typename... CallArgs>
    PLY_INLINE Return operator()(CallArgs&&... args) const {
        if (!this->handler)
            return subst::createDefault<Return>();
        PLY_PUN_SCOPE
        return this->handler(this->storedArg, std::forward<CallArgs>(args)...);
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
        subst::destroyByMember(this->ptr);
    }
    static PLY_INLINE Owned adopt(T* ptr) {
        Owned result;
        result.ptr = ptr;
        return result;
    }
    PLY_INLINE void operator=(Owned&& other) {
        PLY_ASSERT(!this->ptr || this->ptr != other.ptr);
        subst::destroyByMember(this->ptr);
        this->ptr = other.release();
    }
    template <typename Derived,
              typename std::enable_if_t<std::is_base_of<T, Derived>::value, int> = 0>
    PLY_INLINE void operator=(Owned<Derived>&& other) {
        PLY_ASSERT(!this->ptr || this->ptr != other.ptr);
        subst::destroyByMember(this->ptr);
        this->ptr = other.release();
    }
    PLY_INLINE void operator=(T* ptr) {
        PLY_ASSERT(!this->ptr || this->ptr != ptr);
        subst::destroyByMember(this->ptr);
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
        subst::destroyByMember(this->ptr);
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
    PLY_INLINE void incRef() {
        s32 oldCount = m_refCount.fetchAdd(1, Relaxed);
        PLY_ASSERT(oldCount >= 0 && oldCount < UINT16_MAX);
        PLY_UNUSED(oldCount);
    }
    PLY_INLINE void decRef() {
        s32 oldCount = m_refCount.fetchSub(1, Relaxed);
        PLY_ASSERT(oldCount >= 1 && oldCount < UINT16_MAX);
        if (oldCount == 1) {
            static_cast<Mixin*>(this)->onRefCountZero();
        }
    }
    PLY_INLINE s32 getRefCount() const {
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
            this->ptr->incRef();
    }
    PLY_INLINE Reference(const Reference& ref) : ptr(ref.ptr) {
        if (this->ptr)
            this->ptr->incRef();
    }
    PLY_INLINE Reference(Reference&& ref) : ptr(ref.ptr) {
        ref.ptr = nullptr;
    }
    PLY_INLINE ~Reference() {
        if (this->ptr)
            this->ptr->decRef();
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
        T* oldPtr = this->ptr;
        this->ptr = ptr;
        if (this->ptr)
            this->ptr->incRef();
        if (oldPtr)
            oldPtr->decRef();
    }

    PLY_INLINE void operator=(const Reference& ref) {
        T* oldPtr = this->ptr;
        this->ptr = ref.ptr;
        if (this->ptr)
            this->ptr->incRef();
        if (oldPtr)
            oldPtr->decRef();
    }
    PLY_INLINE void operator=(Reference&& ref) {
        if (this->ptr)
            this->ptr->decRef();
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
            this->ptr->decRef();
        this->ptr = nullptr;
    }
    PLY_INLINE bool isEmpty() const {
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
    void incRef() {
        u32 oldDualCount = m_dualRefCount.fetchAdd(1, Relaxed);
        PLY_ASSERT((oldDualCount & 0xffffu) < INT16_MAX);
        PLY_UNUSED(oldDualCount);
    }

    void decRef() {
        u32 oldDualCount = m_dualRefCount.fetchSub(1, Relaxed);
        PLY_ASSERT((oldDualCount & 0xffffu) > 0);
        PLY_ASSERT((oldDualCount & 0xffffu) < INT16_MAX);
        if ((oldDualCount & 0xffffu) == 1) {
            static_cast<Mixin*>(this)->onPartialRefCountZero();
            if (oldDualCount == 1) {
                static_cast<Mixin*>(this)->onFullRefCountZero();
            }
        }
    }

    void incWeakRef() {
        u32 oldDualCount = m_dualRefCount.fetchAdd(0x10000, Relaxed);
        PLY_ASSERT((oldDualCount & 0xffffu) > 0); // Must have some strong refs
        PLY_ASSERT((oldDualCount >> 16) < INT16_MAX);
        PLY_UNUSED(oldDualCount);
    }

    void decWeakRef() {
        u32 oldDualCount = m_dualRefCount.fetchSub(0x10000, Relaxed);
        PLY_ASSERT((oldDualCount >> 16) > 0);
        PLY_ASSERT((oldDualCount >> 16) < INT16_MAX);
        PLY_UNUSED(oldDualCount);
        if ((oldDualCount >> 16) == 1) {
            if (oldDualCount == 0x10000) {
                static_cast<Mixin*>(this)->onFullRefCountZero();
            }
        }
    }

    u32 getRefCount() const {
        return m_dualRefCount.load(Relaxed) & 0xffffu;
    }

    u32 getWeakRefCount() const {
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
            m_ptr->incWeakRef();
    }

    WeakRef(const WeakRef& ref) : m_ptr{ref.m_ptr} {
        if (m_ptr)
            m_ptr->incWeakRef();
    }

    WeakRef(WeakRef&& ref) : m_ptr{ref.m_ptr} {
        ref.m_ptr = nullptr;
    }

    ~WeakRef() {
        if (m_ptr)
            m_ptr->decWeakRef();
    }

    T* operator->() const {
        return m_ptr;
    }

    operator T*() const {
        return m_ptr;
    }

    void setFromNull(T* ptr) {
        PLY_ASSERT(!m_ptr);
        PLY_ASSERT(ptr);
        m_ptr = ptr;
        ptr->incWeakRef();
    }

    void operator=(T* ptr) {
        T* oldPtr = m_ptr;
        m_ptr = ptr;
        if (m_ptr)
            m_ptr->incWeakRef();
        if (oldPtr)
            oldPtr->decWeakRef();
    }

    void operator=(const WeakRef& ref) {
        T* oldPtr = m_ptr;
        m_ptr = ref.m_ptr;
        if (m_ptr)
            m_ptr->incWeakRef();
        if (oldPtr)
            oldPtr->decWeakRef();
    }

    void operator=(WeakRef&& ref) {
        if (m_ptr)
            m_ptr->decWeakRef();
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
        u64 repU64;
        s64 repS64;
        double repDouble;
    };

    Numeric() : type(U64), repU64(0) {
    }

    void operator=(bool v) {
        type = U64;
        repU64 = v;
    }
    void operator=(u8 v) {
        type = U64;
        repU64 = v;
    }
    void operator=(u16 v) {
        type = U64;
        repU64 = v;
    }
    void operator=(u32 v) {
        type = U64;
        repU64 = v;
    }
    void operator=(u64 v) {
        type = U64;
        repU64 = v;
    }
    void operator=(s8 v) {
        type = S64;
        repS64 = v;
    }
    void operator=(s16 v) {
        type = S64;
        repS64 = v;
    }
    void operator=(s32 v) {
        type = S64;
        repS64 = v;
    }
    void operator=(s64 v) {
        type = S64;
        repS64 = v;
    }
    void operator=(float v) {
        type = Double;
        repDouble = v;
    }
    void operator=(double v) {
        type = Double;
        repDouble = v;
    }

    template <typename T>
    T cast(bool& precise) {
        T result;
        switch (type) {
            case U64:
                result = (T) repU64;
                precise = ((u64) result == repU64);
                break;
            case S64:
                result = (T) repS64;
                precise = ((s64) result == repS64);
                break;
            case Double:
                result = (T) repDouble;
                precise = ((double) result == repDouble);
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
    T& target;                           // The variable to set/reset
    std::remove_reference_t<T> oldValue; // Backup of original value
    const V& newValueRef; // Extends the lifetime of temporary values in the case of eg.
                          // SetInScope<StringView, String>

    template <typename U>
    SetInScope(T& target, U&& newValue)
        : target{target}, oldValue{std::move(target)}, newValueRef{newValue} {
        target = std::forward<U>(newValue);
    }
    ~SetInScope() {
        this->target = std::move(this->oldValue);
    }
};

#define PLY_SET_IN_SCOPE(target, value) \
    SetInScope<decltype(target), decltype(value)> PLY_UNIQUE_VARIABLE(setInScope) { \
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
PLY_INLINE OnScopeExit<Callback> setOnScopeExit(Callback&& cb) {
    return {std::forward<Callback>(cb)};
}
#define PLY_ON_SCOPE_EXIT(cb) \
    auto PLY_UNIQUE_VARIABLE(onScopeExit) = setOnScopeExit([&] cb)

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
            [](void* ptr) { subst::destructByMember((T*) ptr); },
            // move
            [](void* dst, void* src) { subst::unsafeMove<T>((T*) dst, (T*) src); },
            // copy
            [](void* dst, const void* src) {
                subst::unsafeCopy<T>((T*) dst, (const T*) src);
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
    PLY_INLINE SwitchWrapper& switchTo(Args&&... args) {
        Container::idToType[ureg(ctr.id)].destruct(&ctr.storage);
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

#define SWITCH_FOOTER(ctrName, defaultState) \
    using T = ctrName; \
    ID id; \
    Storage_ storage; \
    static SwitchType idToType[]; \
    template <typename, typename = void> \
    struct TypeToID; \
    PLY_INLINE ctrName() : id{ID::defaultState} { new (&storage) defaultState{}; } \
    PLY_INLINE ~ctrName() { idToType[ply::ureg(id)].destruct(&storage); } \
    PLY_INLINE ctrName(const ctrName& other) : id{other.id} { \
        idToType[ply::ureg(id)].construct(&storage); \
        idToType[ply::ureg(id)].copy(&storage, &other.storage); \
    } \
    PLY_INLINE ctrName(ctrName&& other) : id{other.id} { \
        idToType[ply::ureg(id)].construct(&storage); \
        idToType[ply::ureg(id)].move(&storage, &other.storage); \
    } \
    template <typename S, typename = ply::void_t<decltype(TypeToID<S>::value)>> \
    PLY_INLINE ctrName(S&& other) : id{TypeToID<S>::value} { \
        idToType[ply::ureg(id)].construct(&storage); \
        idToType[ply::ureg(id)].move(&storage, &other); \
    } \
    PLY_INLINE void switchTo(ID newID) { \
        idToType[ply::ureg(id)].destruct(&storage); \
        id = newID; \
        idToType[ply::ureg(id)].construct(&storage); \
    } \
    PLY_INLINE void operator=(const ctrName& other) { \
        switchTo(other.id); \
        idToType[ply::ureg(id)].copy(&storage, &other.storage); \
    } \
    PLY_INLINE void operator=(ctrName&& other) { \
        switchTo(other.id); \
        idToType[ply::ureg(id)].move(&storage, &other.storage); \
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

#define SWITCH_TABLE_BEGIN(ctrName) ply::SwitchType ctrName::idToType[] = {

#define SWITCH_TABLE_STATE(ctrName, state) ply::SwitchType::get<ctrName::state>(),

#define SWITCH_TABLE_END(ctrName) \
    } \
    ; \
    PLY_STATIC_ASSERT(PLY_STATIC_ARRAY_SIZE(ctrName::idToType) == \
                      (ply::ureg) ctrName::ID::Count); \
    PLY_STATIC_ASSERT((ply::ureg) ctrName::ID::Count > 0);

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
    void baseReserve(u32 newSize, u32 itemSize);
    u32 baseAlloc(u32 itemSize);
    void baseFree(u32 index, u32 itemSize);
    PLY_DLL_ENTRY void baseSortFreeList(u32 stride) const;
    void baseClear();
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
    PoolIndex<T, Index> newItem(Args&&... args);
    template <typename Index>
    void delItem(PoolIndex<T, Index> index);

    template <typename Index>
    PoolPtr<T> get(PoolIndex<T, Index>);
    template <typename Index>
    PoolPtr<const T> get(PoolIndex<T, Index>) const;
    u32 indexOf(const T* item) const;

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
    bool isValid() const;
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
    PoolIterator(const Pool<T>* pool, u32 nextFree, u32 index);
    ~PoolIterator();
    PoolIterator(const PoolIterator& other);
    void operator=(const PoolIterator&) = delete;
    template <typename Index>
    PoolIndex<T, Index> getIndex() const;
    PoolPtr<T> operator*() const;
    PoolIterator& operator++();
    bool isValid() const;
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
PoolIndex<T, Index>::PoolIndex(u32 idx) : idx{safeDemote<Index>(idx)} {
}

template <typename T, typename Index>
PoolIndex<T, Index>::PoolIndex(const PoolIndex<const T, Index>& other)
    : idx{other.idx} {
}

template <typename T, typename Index>
bool PoolIndex<T, Index>::isValid() const {
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

inline void BasePool::baseReserve(u32 newSize, u32 itemSize) {
    m_allocated = (u32) roundUpPowerOf2(max<u32>(
        newSize, 8)); // FIXME: Generalize to other resize strategies when needed
    PLY_ASSERT(m_allocated != 0); // Overflow check
    m_items = Heap.realloc(
        m_items,
        itemSize * m_allocated); // FIXME: Generalize to other heaps when needed
}

inline u32 BasePool::baseAlloc(u32 itemSize) {
    if (m_firstFree != u32(-1)) {
        u32 nextFree = *(u32*) PLY_PTR_OFFSET(m_items, m_firstFree * itemSize);
        u32 index = m_firstFree;
        m_firstFree = nextFree;
        m_freeListSize--;
        PLY_ASSERT((m_freeListSize == 0) == (m_firstFree == u32(-1)));
        return index;
    }
    if (m_size >= m_allocated) {
        baseReserve(m_size + 1, itemSize);
    }
    return m_size++;
}

inline void BasePool::baseFree(u32 index, u32 itemSize) {
    PLY_ASSERT(index < m_size);
    *(u32*) PLY_PTR_OFFSET(m_items, index * itemSize) = m_firstFree;
    m_firstFree = index;
    m_sortedFreeList = 0;
    PLY_ASSERT(m_freeListSize < (1u << 31) - 1);
    m_freeListSize++;
}

inline void BasePool::baseClear() {
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
        baseSortFreeList(sizeof(T));
        T* item = static_cast<T*>(m_items);
        T* end = item + m_size;
        u32 skipIndex = m_firstFree;
        for (;;) {
            T* skip =
                static_cast<T*>(m_items) + (skipIndex == u32(-1) ? m_size : skipIndex);
            while (item < skip) {
                item->~T();
                item++;
            }
            if (item == end)
                break;
            skipIndex = *reinterpret_cast<u32*>(skip);
            item++;
        }
    }
    baseClear();
}

template <typename T>
Pool<T>::~Pool() {
    clear();
}

template <typename T>
template <typename Index, typename... Args>
PoolIndex<T, Index> Pool<T>::newItem(Args&&... args) {
#if PLY_WITH_POOL_DEBUG_CHECKS
    // There must not be any Iterators or Ptrs when modifying the pool!
    PLY_ASSERT(m_numReaders == 0);
#endif
    u32 index = baseAlloc(sizeof(T));
    new (static_cast<T*>(m_items) + index) T{std::forward<Args>(args)...};
    return index;
}

template <typename T>
template <typename Index>
void Pool<T>::delItem(PoolIndex<T, Index> index) {
#if PLY_WITH_POOL_DEBUG_CHECKS
    // There must not be any Iterators or Ptrs when modifying the pool!
    PLY_ASSERT(m_numReaders == 0);
#endif
    static_cast<T*>(m_items)[index.idx].~T();
    baseFree(index.idx, sizeof(T));
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
u32 Pool<T>::indexOf(const T* item) const {
    u32 index = safeDemote<u32>(item - static_cast<const T*>(m_items));
    PLY_ASSERT(index < m_size);
    return index;
}

//! Return iterator suitable for range-for.
//! The pool must not be modified during iteration.
template <typename T>
PoolIterator<T> Pool<T>::begin() {
    baseSortFreeList(sizeof(T));
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
    baseSortFreeList(sizeof(T));
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
PoolIterator<T>::PoolIterator(const Pool<T>* pool, u32 nextFree, u32 index)
    : m_pool(pool), m_nextFree(nextFree), m_index(index) {
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
PoolIndex<T, Index> PoolIterator<T>::getIndex() const {
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
bool PoolIterator<T>::isValid() const {
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
        Traits::getPool().delItem(*this);
    }
}

template <typename Traits>
void OwnPoolHandle<Traits>::assign(
    PoolIndex<typename Traits::T, typename Traits::Index> other) {
    if (this->idx != this->InvalidIndex) {
        Traits::getPool().delItem(*this);
    }
    this->idx = other.idx;
}

template <typename Traits>
void OwnPoolHandle<Traits>::clear() {
    if (this->idx != this->InvalidIndex) {
        Traits::getPool().delItem(*this);
    }
    this->idx = this->InvalidIndex;
}

template <typename Traits>
PoolPtr<typename Traits::T> OwnPoolHandle<Traits>::get() const {
    return Traits::getPool().get(*this);
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
    uptr numReservedBytes = 0;
    uptr numCommittedBytes = 0;

    static constexpr uptr DefaultNumReservedBytes = 1024 * 1024 * 1024;

    BaseBigPool(uptr numReservedBytes = DefaultNumReservedBytes);
    ~BaseBigPool();
    void commitPages(uptr newTotalBytes);
};

template <typename T = char>
class BigPool : protected BaseBigPool {
private:
    uptr numItems_ = 0;

public:
    PLY_INLINE BigPool(uptr numReservedBytes = DefaultNumReservedBytes)
        : BaseBigPool{numReservedBytes} {
    }
    PLY_INLINE uptr numItems() const {
        return this->numItems_;
    }
    PLY_INLINE const T& operator[](uptr idx) const {
        PLY_ASSERT(idx < this->numItems_);
        return ((const T*) this->base)[idx];
    }
    PLY_INLINE T& operator[](uptr idx) {
        PLY_ASSERT(idx < this->numItems_);
        return ((T*) this->base)[idx];
    }
    PLY_INLINE const T* end() const {
        return ((const T*) this->base) + this->numItems_;
    }
    PLY_INLINE T* get(uptr idx) const {
        PLY_ASSERT(idx < this->numItems_);
        return ((T*) this->base) + idx;
    }
    PLY_INLINE T& back(sptr ofs = -1) const {
        PLY_ASSERT(ofs < 0 && uptr(-ofs) <= this->numItems_);
        return ((T*) this->base)[this->numItems_ + ofs];
    }
    PLY_INLINE T* beginWrite(uptr maxNumItems = 1) {
        uptr newTotalBytes = sizeof(T) * (this->numItems_ + maxNumItems);
        if (newTotalBytes > this->numCommittedBytes) {
            this->commitPages(newTotalBytes);
        }
        return ((T*) this->base) + this->numItems_;
    }
    PLY_INLINE void endWrite(uptr numItems = 1) {
        this->numItems_ += numItems;
        PLY_ASSERT(sizeof(T) * this->numItems_ <= this->numCommittedBytes);
    }
    PLY_INLINE T* alloc(uptr numItems = 1) {
        T* result = beginWrite(numItems);
        endWrite(numItems);
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
        u64 fileOffset = 0; // Total number of bytes used in all preceding blocks
        Reference<Footer> nextBlock;
        Footer* prevBlock = nullptr;
        char* bytes = nullptr;
        u32 startOffset = 0;
        u32 numBytesUsed = 0; // Number of bytes in this block that contain valid data
        u32 blockSize = 0;    // Total number of bytes allocated for this block
        mutable s32 refCount = 0;

        PLY_DLL_ENTRY void onRefCountZero();
        PLY_INLINE void incRef() {
            this->refCount++;
        }
        PLY_INLINE void decRef() {
            PLY_ASSERT(this->refCount > 0);
            if (--this->refCount == 0) {
                this->onRefCountZero();
            }
        }
        PLY_INLINE char* start() const {
            return this->bytes + this->startOffset;
        }
        PLY_INLINE char* unused() const {
            return this->bytes + this->numBytesUsed;
        }
        PLY_INLINE char* end() const {
            return this->bytes + this->blockSize;
        }
        PLY_INLINE u32 offsetOf(char* byte) {
            uptr ofs = byte - this->bytes;
            PLY_ASSERT(ofs <= this->numBytesUsed);
            return (u32) ofs;
        }
        PLY_INLINE StringView viewUsedBytes() const {
            return {this->start(), this->numBytesUsed - this->startOffset};
        }
        PLY_INLINE MutStringView viewUnusedBytes() {
            return {this->unused(), this->blockSize - this->numBytesUsed};
        }
        // Returns a WeakRef to next block, and the next byte after the last byte in
        // this block, taking the difference in fileOffsets into account since it's
        // possible for adjacent blocks to overlap.
        PLY_DLL_ENTRY WeakRef weakRefToNext() const;
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
            PLY_ASSERT(!block || (uptr(byte - block->bytes) <= block->blockSize));
        }
        PLY_INLINE WeakRef(Footer* block, u32 offset)
            : block{block}, byte{block->bytes + offset} {
            PLY_ASSERT(offset <= block->blockSize);
        }
        PLY_INLINE void operator=(const WeakRef& other) {
            this->block = other.block;
            this->byte = other.byte;
        }
        PLY_INLINE bool operator!=(const WeakRef& other) const {
            return this->block != other.block || this->byte != other.byte;
        }
        PLY_INLINE WeakRef normalized() const {
            if (this->block->nextBlock && (this->byte == this->block->unused())) {
                return {this->block->nextBlock, this->block->start()};
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
            PLY_ASSERT(!block || (uptr(byte - block->bytes) <= block->blockSize));
        }
        PLY_INLINE Ref(Reference<Footer>&& block, char* byte)
            : block{std::move(block)}, byte{byte} {
            PLY_ASSERT(!block ||
                       (uptr(byte - this->block->bytes) <= this->block->blockSize));
        }
        PLY_INLINE Ref(const WeakRef& weakRef)
            : block{weakRef.block}, byte{weakRef.byte} {
        }
        PLY_INLINE operator WeakRef() const {
            return {this->block, this->byte};
        }
        PLY_INLINE void operator=(const WeakRef& weakRef) {
            this->block = weakRef.block;
            this->byte = weakRef.byte;
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
        WeakRef curPos;
        WeakRef endPos;

        RangeForIterator(const WeakRef& startPos, const WeakRef& endPos = {})
            : curPos{startPos}, endPos{endPos} {
        }
        PLY_INLINE RangeForIterator& begin() {
            return *this;
        }
        PLY_INLINE RangeForIterator& end() {
            return *this;
        }
        PLY_INLINE void operator++() {
            this->curPos = this->curPos.block->weakRefToNext();
        }
        PLY_INLINE bool operator!=(const RangeForIterator&) const {
            return this->curPos != this->endPos;
        }
        PLY_INLINE StringView operator*() const {
            return StringView::fromRange(this->curPos.byte,
                                         (this->curPos.block == this->endPos.block)
                                             ? this->endPos.byte
                                             : this->curPos.block->unused());
        }
    };

    //--------------------------------------
    // Static member functions
    //--------------------------------------
    static PLY_DLL_ENTRY Reference<Footer> createBlock(u32 numBytes = DefaultBlockSize);
    static PLY_DLL_ENTRY Reference<Footer> createOverlayBlock(const WeakRef& pos,
                                                              u32 numBytes);
    static PLY_DLL_ENTRY Footer* appendBlock(Footer* block,
                                             u32 numBytes = DefaultBlockSize);
    static PLY_DLL_ENTRY void appendBlockWithRecycle(Reference<Footer>& block,
                                                     u32 numBytes = DefaultBlockSize);
    static PLY_INLINE RangeForIterator iterateOverViews(const WeakRef& start,
                                                        const WeakRef& end = {}) {
        return {start, end};
    }
    static PLY_DLL_ENTRY u32 jumpToNextBlock(WeakRef* weakRef);
    static PLY_DLL_ENTRY u32 jumpToPrevBlock(WeakRef* weakRef);

    static PLY_DLL_ENTRY String toString(Ref&& start, const WeakRef& end = {});

    //--------------------------------------
    // BlockList object
    //--------------------------------------
    Reference<Footer> head;
    Footer* tail = nullptr;

    PLY_DLL_ENTRY BlockList();
    PLY_DLL_ENTRY ~BlockList();
    PLY_DLL_ENTRY char* appendBytes(u32 numBytes);
    PLY_DLL_ENTRY void popLastBytes(u32 numBytes);
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
PLY_DLL_ENTRY void destructSequence(Reference<BlockList::Footer>* headRef,
                                    void (*destructViewAs)(StringView));
PLY_DLL_ENTRY void beginWriteInternal(BlockList::Footer** tail, u32 numBytes);
PLY_DLL_ENTRY void popTail(BlockList::Footer** tail, u32 numBytes,
                           void (*destructViewAs)(StringView));
PLY_DLL_ENTRY void truncate(BlockList::Footer** tail, const BlockList::WeakRef& to);
PLY_DLL_ENTRY u32 getTotalNumBytes(BlockList::Footer* head);
PLY_DLL_ENTRY char* read(BlockList::WeakRef* weakRef, u32 itemSize);
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
    PLY_INLINE ArrayView<T> beginRead() {
        sptr numBytesAvailable = this->impl.block->unused() - this->impl.byte;
        if (numBytesAvailable == 0) {
            numBytesAvailable = BlockList::jumpToNextBlock(&impl);
        } else {
            // numBytesAvailable should always be a multiple of sizeof(T).
            PLY_ASSERT(numBytesAvailable >= sizeof(T));
        }
        return ArrayView<T>::from(StringView{this->impl.byte, numBytesAvailable});
    }
    PLY_INLINE void endRead(u32 numItems) {
        PLY_ASSERT(this->impl.block->unused() - this->impl.byte >=
                   sizeof(T) * numItems);
        this->impl.byte += sizeof(T) * numItems;
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
        sptr numBytesAvailable = this->impl.block->unused() - this->impl.byte;
        // It is illegal to call operator++ at the end of the sequence.
        PLY_ASSERT(numBytesAvailable >= sizeof(T));
        this->impl.byte += sizeof(T);
        numBytesAvailable -= sizeof(T);
        if (numBytesAvailable == 0) {
            numBytesAvailable = BlockList::jumpToNextBlock(&impl);
            // We might now be at the end of the sequence.
        } else {
            // numBytesAvailable should always be a multiple of sizeof(T).
            PLY_ASSERT(numBytesAvailable >= sizeof(T));
        }
    }
    PLY_INLINE void operator--() {
        sptr numBytesPreceding = this->impl.byte - this->impl.block->start();
        if (numBytesPreceding == 0) {
            numBytesPreceding = BlockList::jumpToPrevBlock(&impl);
        }
        // It is illegal to call operator-- at the start of the sequence.
        PLY_ASSERT(numBytesPreceding >= sizeof(T));
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
    Reference<BlockList::Footer> headBlock;
    BlockList::Footer* tailBlock = nullptr;

public:
    PLY_INLINE Sequence() : headBlock{BlockList::createBlock()}, tailBlock{headBlock} {
    }

    PLY_INLINE Sequence(Sequence&& other)
        : headBlock{std::move(other.headBlock)}, tailBlock{other.tailBlock} {
    }

    PLY_INLINE ~Sequence() {
        impl::destructSequence(&headBlock, subst::destructViewAs<T>);
    }

    PLY_INLINE void operator=(Sequence&& other) {
        impl::destructSequence(&headBlock, subst::destructViewAs<T>);
        new (this) Sequence{std::move(other)};
    }

    PLY_INLINE T& head() {
        // It is illegal to call head() on an empty sequence.
        PLY_ASSERT(this->headBlock->viewUsedBytes().numBytes >= sizeof(T));
        return *(T*) this->headBlock->start();
    }
    PLY_INLINE T& tail() {
        // It is illegal to call tail() on an empty sequence.
        PLY_ASSERT(this->tailBlock->viewUsedBytes().numBytes >= sizeof(T));
        return ((T*) this->tailBlock->unused())[-1];
    }

    PLY_INLINE bool isEmpty() const {
        // Only an empty sequence can have an empty head block.
        return this->headBlock->viewUsedBytes().isEmpty();
    }
    PLY_INLINE u32 numItems() const {
        // Fast division by integer constant.
        return impl::getTotalNumBytes(this->headBlock) / sizeof(T);
    }

    PLY_INLINE ArrayView<T> beginWriteViewNoConstruct() {
        if (this->tailBlock->viewUnusedBytes().numBytes < sizeof(T)) {
            impl::beginWriteInternal(&this->tailBlock, sizeof(T));
        }
        return ArrayView<T>::from(this->tailBlock->viewUnusedBytes());
    }

    PLY_INLINE T* beginWriteNoConstruct() {
        if (this->tailBlock->viewUnusedBytes().numBytes < sizeof(T)) {
            impl::beginWriteInternal(&this->tailBlock, sizeof(T));
        }
        return (T*) this->tailBlock->unused();
    }

    PLY_INLINE void endWrite(u32 numItems = 1) {
        PLY_ASSERT(sizeof(T) * numItems <= this->tailBlock->viewUnusedBytes().numBytes);
        this->tailBlock->numBytesUsed += sizeof(T) * numItems;
    }

    PLY_INLINE T& append(const T& item) {
        T* result = beginWriteNoConstruct();
        new (result) T{item};
        endWrite();
        return *result;
    }
    PLY_INLINE T& append(T&& item) {
        T* result = beginWriteNoConstruct();
        new (result) T{std::move(item)};
        endWrite();
        return *result;
    }
    template <typename... Args>
    PLY_INLINE T& append(Args&&... args) {
        T* result = beginWriteNoConstruct();
        new (result) T{std::forward<Args>(args)...};
        endWrite();
        return *result;
    }

    PLY_INLINE void popTail(u32 numItems = 1) {
        impl::popTail(&this->tailBlock, numItems * (u32) sizeof(T),
                      subst::destructViewAs<T>);
    }
    PLY_INLINE void truncate(const WeakSequenceRef<T>& to) {
        impl::truncate(&this->tailBlock, to.impl);
    }

    PLY_INLINE void clear() {
        *this = Sequence{};
    }

    PLY_INLINE Array<T> moveToArray() {
        char* startByte = this->headBlock->start();
        String str = BlockList::toString({std::move(this->headBlock), startByte});
        u32 numItems = str.numBytes / sizeof(T); // Divide by constant is fast
        return Array<T>::adopt((T*) str.release(), numItems);
    }

    WeakSequenceRef<T> begin() {
        return BlockList::WeakRef{this->headBlock, this->headBlock->start()};
    }
    WeakSequenceRef<T> end() {
        return BlockList::WeakRef{this->tailBlock, this->tailBlock->unused()};
    }
    WeakSequenceRef<const T> begin() const {
        return BlockList::WeakRef{this->headBlock, this->headBlock->start()};
    }
    WeakSequenceRef<const T> end() const {
        return BlockList::WeakRef{this->tailBlock, this->tailBlock->unused()};
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
    // Flags passed to insertOrFind():
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
        u8* prevLink;
        void* itemSlot;
    };

    struct Callbacks {
        u32 itemSize = 0;
        bool isTriviallyDestructible = false;
        bool requiresContext = false;
        void (*construct)(void* item, const void* key) = nullptr;
        void (*destruct)(void* item) = nullptr;
        void (*moveConstruct)(void* dstItem, void* srcItem) = nullptr;
        void (*moveAssign)(void* dstItem, void* srcItem) = nullptr;
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

        // moveConstruct
        static PLY_NO_INLINE void moveConstruct(void* dstItem, void* srcItem) {
            new (dstItem) Item{std::move(*(Item*) srcItem)};
        }

        // moveAssign
        static PLY_NO_INLINE void moveAssign(void* dstItem, void* srcItem) {
            *(Item*) dstItem = std::move(*(Item*) srcItem);
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
                &moveConstruct,
                &moveAssign,
                &hash,
                &match,
            };
            return &ins;
        };
    };

    // Note: Must be kept binary compatible with HashMap<>::CellGroup
    struct CellGroup {
        u8 nextDelta[4]; // EmptySlot means the slot is unused
        u8 firstDelta[4];
        u32 hashes[4];
    };

    // Note: Must be kept binary compatible with HashMap<>
    CellGroup* m_cellGroups;
    u32 m_sizeMask;
    u32 m_population;

    PLY_DLL_ENTRY HashMap(const Callbacks* cb, u32 initialSize);
    PLY_DLL_ENTRY HashMap(HashMap&& other);
    PLY_DLL_ENTRY void moveAssign(const Callbacks* cb, HashMap&& other);
    PLY_DLL_ENTRY void clear(const Callbacks* cb);
    static PLY_DLL_ENTRY CellGroup* createTable(const Callbacks* cb,
                                                u32 size = InitialSize);
    static PLY_DLL_ENTRY void destroyTable(const Callbacks* cb, CellGroup* cellGroups,
                                           u32 size);
    PLY_DLL_ENTRY void migrateToNewTable(const Callbacks* cb);
    PLY_DLL_ENTRY FindResult findNext(FindInfo* info, const Callbacks* cb,
                                      const void* key, const void* context) const;
    PLY_DLL_ENTRY FindResult insertOrFind(FindInfo* info, const Callbacks* cb,
                                          const void* key, const void* context,
                                          u32 flags);
    PLY_DLL_ENTRY void* insertForMigration(u32 itemSize, u32 hash);
    PLY_DLL_ENTRY void erase(FindInfo* info, const Callbacks* cb, u8*& linkToAdjust);

    // Must be kept binary compatible with HashMap<>::Cursor:
    struct Cursor {
        HashMap* m_map;
        FindInfo m_findInfo;
        FindResult m_findResult;

        PLY_DLL_ENTRY void constructFindWithInsert(const Callbacks* cb, HashMap* map,
                                                   const void* key, const void* context,
                                                   u32 flags);
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
        PLY_ASSERT(cursor->m_findInfo.itemSlot);
        return cursor->m_findInfo.itemSlot;
    }

    PLY_INLINE const Item* operator->() const {
        const Cursor* cursor = static_cast<const Cursor*>(this);
        PLY_ASSERT(cursor->m_findInfo.itemSlot);
        return cursor->m_findInfo.itemSlot;
    }
};

template <class Cursor, typename Item>
class CursorMixin<Cursor, Item, true> {
public:
    PLY_INLINE Item operator->() const {
        const Cursor* cursor = static_cast<const Cursor*>(this);
        PLY_ASSERT(*cursor->m_findInfo.itemSlot);
        return *cursor->m_findInfo.itemSlot;
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
        u8 nextDelta[4]; // EmptySlot means the slot is unused
        u8 firstDelta[4];
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
        u8* prevLink;
        Item* itemSlot; // null means not found
    };

public:
    PLY_INLINE HashMap(u32 initialSize = 8) {
        new (this) impl::HashMap(Callbacks::instance(), initialSize);
    }

    PLY_INLINE HashMap(HashMap&& other) {
        new (this) impl::HashMap{std::move((impl::HashMap&) other)};
        other.m_cellGroups = nullptr;
    }

    PLY_INLINE ~HashMap() {
        if (m_cellGroups) {
            impl::HashMap::destroyTable(Callbacks::instance(),
                                        (impl::HashMap::CellGroup*) m_cellGroups,
                                        m_sizeMask + 1);
        }
    }

    PLY_INLINE void operator=(HashMap&& other) {
        reinterpret_cast<impl::HashMap*>(this)->moveAssign(
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
            m_findResult = reinterpret_cast<impl::HashMap*>(m_map)->insertOrFind(
                (impl::HashMap::FindInfo*) &m_findInfo, Callbacks::instance(), &key,
                context, impl::HashMap::AllowFind);
        }
        // Find with insert
        PLY_INLINE Cursor(HashMap* map, const Key& key, const Context* context,
                          u32 flags) {
            reinterpret_cast<impl::HashMap::Cursor*>(this)->constructFindWithInsert(
                Callbacks::instance(), (impl::HashMap*) map, &key, context, flags);
        }

    public:
        PLY_INLINE void operator=(const Cursor& other) {
            m_map = other.m_map;
            m_findInfo = other.m_findInfo;
            m_findResult = other.m_findResult;
        }
        PLY_INLINE bool isValid() const {
            return m_findResult != impl::HashMap::FindResult::NotFound;
        }
        PLY_INLINE bool wasFound() const {
            return m_findResult == impl::HashMap::FindResult::Found;
        }
        PLY_INLINE void next(const Key& key, const Context& context = {}) {
            m_findResult = reinterpret_cast<const impl::HashMap*>(m_map)->findNext(
                (impl::HashMap::FindInfo*) &m_findInfo, Callbacks::instance(), &key,
                &context);
        }
        PLY_INLINE Item& operator*() {
            PLY_ASSERT(m_findInfo.itemSlot);
            return *m_findInfo.itemSlot;
        }
        PLY_INLINE const Item& operator*() const {
            PLY_ASSERT(m_findInfo.itemSlot);
            return *m_findInfo.itemSlot;
        }
        PLY_INLINE void erase() {
            u8* unusedLink = nullptr;
            reinterpret_cast<impl::HashMap*>(m_map)->erase(
                (impl::HashMap::FindInfo*) &m_findInfo, Callbacks::instance(),
                unusedLink);
            m_findResult = impl::HashMap::FindResult::NotFound;
        }
        PLY_INLINE void eraseAndAdvance(const Key& key, const Context& context = {}) {
            FindInfo infoToErase = m_findInfo;
            m_findResult = reinterpret_cast<const impl::HashMap&>(m_map).findNext(
                (impl::HashMap::FindInfo*) &m_findInfo, Callbacks::instance(), &key,
                &context);
            reinterpret_cast<impl::HashMap*>(m_map)->erase(
                (impl::HashMap::FindInfo*) &infoToErase, Callbacks::instance(),
                m_findInfo.prevLink);
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
                               ->insertOrFind((impl::HashMap::FindInfo*) &m_findInfo,
                                              Callbacks::instance(), &key, context,
                                              impl::HashMap::AllowFind);
        }

    public:
        PLY_INLINE void operator=(const ConstCursor& other) {
            m_map = other.m_map;
            m_findInfo = other.m_findInfo;
            m_findResult = other.m_findResult;
        }
        PLY_INLINE bool isValid() const {
            return m_findResult != impl::HashMap::FindResult::NotFound;
        }
        PLY_INLINE bool wasFound() const {
            return m_findResult == impl::HashMap::FindResult::Found;
        }
        PLY_INLINE void next(const Key& key, const Context& context = {}) {
            m_findResult = reinterpret_cast<const impl::HashMap&>(m_map).findNext(
                (impl::HashMap::FindInfo*) &m_findInfo, Callbacks::instance(), &key,
                &context);
        }
        PLY_INLINE Item& operator*() {
            PLY_ASSERT(m_findInfo.itemSlot);
            return *m_findInfo.itemSlot;
        }
        PLY_INLINE const Item& operator*() const {
            PLY_ASSERT(m_findInfo.itemSlot);
            return *m_findInfo.itemSlot;
        }
    };

    PLY_INLINE bool isEmpty() const {
        return m_population == 0;
    }

    PLY_INLINE u32 numItems() const {
        return m_population;
    }

    PLY_INLINE Cursor insertOrFind(const Key& key, const Context* context = nullptr) {
        return {this, key, context,
                impl::HashMap::AllowFind | impl::HashMap::AllowInsert};
    }

    // insertMulti is experimental and should not be used. Will likely delete.
    // FIXME: Delete this function and its associated support code, including
    // Cursor::next()
    PLY_INLINE Cursor insertMulti(const Key& key, const Context* context = nullptr) {
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

        bool isValid() const {
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
                if (group->nextDelta[m_idx & 3] != impl::HashMap::EmptySlot)
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
            PLY_ASSERT(group->nextDelta[m_idx & 3] != impl::HashMap::EmptySlot);
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

        bool isValid() const {
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
                if (group->nextDelta[m_idx & 3] != impl::HashMap::EmptySlot)
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
            PLY_ASSERT(group->nextDelta[m_idx & 3] != impl::HashMap::EmptySlot);
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

//  ▄▄           ▄▄            ▄▄▄  ▄▄   ▄▄
//  ██     ▄▄▄▄  ██▄▄▄   ▄▄▄▄   ██  ███▄███  ▄▄▄▄  ▄▄▄▄▄
//  ██     ▄▄▄██ ██  ██ ██▄▄██  ██  ██▀█▀██  ▄▄▄██ ██  ██
//  ██▄▄▄ ▀█▄▄██ ██▄▄█▀ ▀█▄▄▄  ▄██▄ ██   ██ ▀█▄▄██ ██▄▄█▀
//                                                 ██

namespace impl {

struct BaseLabelMap {
    struct Cell {
        Label keys[4];
    };

    Cell* cells = nullptr;
    u32 population = 0;
    u32 capacity = 0;

    enum Operation { Find, Insert, Erase, Repopulate };

    struct TypeInfo {
        u32 valueSize;
        void (*construct)(void* obj);
        void (*destruct)(void* obj);
        void (*memcpy)(void* dst, void* src);
    };

    template <typename T>
    static TypeInfo typeInfo;
};

template <typename T>
BaseLabelMap::TypeInfo BaseLabelMap::typeInfo = {
    u32{sizeof(T)},
    [](void* obj) { new (obj) T; },
    [](void* obj) { ((T*) obj)->~T(); },
    [](void* dst, void* src) { memcpy(dst, src, sizeof(T)); },
};

void construct(BaseLabelMap* map, const BaseLabelMap::TypeInfo* typeInfo);
void destruct(BaseLabelMap* map, const BaseLabelMap::TypeInfo* typeInfo);
bool operate(BaseLabelMap* map, BaseLabelMap::Operation op, Label key,
             const BaseLabelMap::TypeInfo* typeInfo, void** value);

} // namespace impl

template <typename T>
struct LabelMapIterator;

template <typename T>
class LabelMap {
private:
    struct Cell {
        Label keys[4];
        T values[4];
    };

    Cell* cells;
    u32 population;
    u32 capacity; // Always a power of 2

    using Base = impl::BaseLabelMap;
    friend struct LabelMapIterator<T>;

public:
    PLY_INLINE LabelMap() {
        PLY_PUN_SCOPE
        construct((Base*) this, &Base::typeInfo<T>);
    }

    PLY_INLINE ~LabelMap() {
        PLY_PUN_SCOPE
        destruct((Base*) this, &Base::typeInfo<T>);
    }

    PLY_INLINE u32 numItems() const {
        return this->population;
    }

    PLY_INLINE T* find(Label key) {
        PLY_PUN_SCOPE
        T* value;
        operate((Base*) this, Base::Find, key, &Base::typeInfo<T>, (void**) &value);
        return value;
    }

    PLY_INLINE const T* find(Label key) const {
        PLY_PUN_SCOPE
        const T* value;
        operate((Base*) this, Base::Find, key, &Base::typeInfo<T>, (void**) &value);
        return value;
    }

    PLY_INLINE bool insertOrFind(Label key, T** value) {
        PLY_PUN_SCOPE
        return operate((Base*) this, Base::Insert, key, &Base::typeInfo<T>,
                       (void**) value);
    }

    PLY_INLINE T* insert(Label key) {
        PLY_PUN_SCOPE
        T* value;
        operate((Base*) this, Base::Insert, key, &Base::typeInfo<T>, (void**) &value);
        return value;
    }

    struct Item {
        Label key;
        T& value;
    };
    LabelMapIterator<T> begin();
    LabelMapIterator<T> end();
    LabelMapIterator<const T> begin() const;
    LabelMapIterator<const T> end() const;
};

template <typename T>
struct LabelMapIterator {
    using Map = LabelMap<T>;
    using Cell = typename Map::Cell;
    using Item = typename Map::Item;

    Map* map;
    u32 index;

    PLY_INLINE void operator++() {
        u32 mask = this->map->capacity - 1;
        while (true) {
            this->index++;
            PLY_ASSERT(this->index <= this->map->capacity);
            if (this->index >= this->map->capacity)
                break;
            Cell* cell = this->map->cells + ((this->index & mask) >> 2);
            if (cell->keys[this->index & 3])
                break;
        }
    }

    PLY_INLINE bool operator!=(const LabelMapIterator& other) const {
        PLY_ASSERT(this->map == other.map);
        return this->index != other.index;
    }

    PLY_INLINE Item operator*() const {
        u32 mask = this->map->capacity - 1;
        PLY_ASSERT(this->index < this->map->capacity);
        Cell* cell = this->map->cells + ((this->index & mask) >> 2);
        u32 cellIdx = (this->index & 3);
        PLY_ASSERT(cell->keys[cellIdx]);
        return {cell->keys[cellIdx], cell->values[cellIdx]};
    }
};

template <typename T>
PLY_INLINE LabelMapIterator<T> LabelMap<T>::begin() {
    LabelMapIterator<T> iter{this, (u32) -1};
    ++iter;
    return iter;
}

template <typename T>
PLY_INLINE LabelMapIterator<T> LabelMap<T>::end() {
    return {this, this->capacity};
}

template <typename T>
PLY_INLINE LabelMapIterator<const T> LabelMap<T>::begin() const {
    LabelMapIterator<const T> iter{this, (u32) -1};
    ++iter;
    return iter;
}

template <typename T>
PLY_INLINE LabelMapIterator<const T> LabelMap<T>::end() const {
    return {this, this->capacity};
}

//  ▄▄   ▄▄
//  ███▄███  ▄▄▄▄  ▄▄▄▄▄
//  ██▀█▀██  ▄▄▄██ ██  ██
//  ██   ██ ▀█▄▄██ ██▄▄█▀
//                 ██

// Key can be u32, u64, String, StringView, Label or T*.
template <typename Key, typename Value>
struct Map;

struct BaseMap {
    Array<s32> indices;
    BaseArray items;
};

enum MapOperation {
    M_Find,
    M_Insert,
    M_Reindex,
};

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

// map_operate() is defined out-of-line (not inline) and explicitly instantiated for
// u32, u64, String and StringView keys only.
template <typename Key>
void* map_operate(BaseMap* map, MapOperation op, View<Key> key, const MapTypeInfo* ti,
                  bool* was_found = nullptr);

// KeyTraits
template <typename T>
struct KeyTraits {
    using Rep = T;
    static T view(T key) {
        return key;
    }
};
template <>
struct KeyTraits<String> {
    using Rep = String;
    static StringView view(const String& key) {
        return key;
    }
};
template <>
struct KeyTraits<Label> {
    using Rep = u32;
    static u32 view(Label key) {
        return key.idx;
    }
};
template <typename T>
struct KeyTraits<T*> {
    using Rep = uptr;
    static uptr view(T* key) {
        return (uptr) key.idx;
    }
};

template <typename Key, typename Value>
struct Map {
    struct Item {
        Key key;
        Value value;
    };

    Array<s32> indices;
    Array<Item> items;

    using Rep = typename KeyTraits<Key>::Rep;

    Value* find(Key key) {
        PLY_PUN_SCOPE
        return (Value*) map_operate<Rep>((BaseMap*) this, M_Find,
                                         KeyTraits<Key>::view(key),
                                         &map_type_info<Key, Value>);
    }
    const Value* find(Key key) const {
        PLY_PUN_SCOPE
        return (Value*) map_operate<Rep>((BaseMap*) this, M_Find,
                                         KeyTraits<Key>::view(key),
                                         &map_type_info<Key, Value>);
    }
    Value* insert_or_find(Key key, bool* was_found = nullptr) {
        PLY_PUN_SCOPE
        return (Value*) map_operate<Rep>((BaseMap*) this, M_Insert,
                                         KeyTraits<Key>::view(key),
                                         &map_type_info<Key, Value>, was_found);
    }
    template <typename T>
    void assign(Label key, T&& arg) {
        PLY_PUN_SCOPE
        void* value =
            map_operate<Rep>((BaseMap*) this, M_Insert, KeyTraits<Key>::view(key),
                             &map_type_info<Key, Value>);
        *((Value*) value) = std::forward<T>(arg);
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
    static PLY_INLINE u32 decodeValue(const char*& ptr) {
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

    static PLY_INLINE u32 getEncLen(u32 value) {
        u32 encLen = 0;
        do {
            encLen++;
            value >>= 7;
        } while (value > 0);
        return encLen;
    }

    static PLY_NO_INLINE void encodeValue(char*& ptr, u32 value) {
        u32 shift = (getEncLen(value) - 1) * 7;
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
        static bool match(u32 id, const StringView& key, const BigPool<>& bigPool) {
            const char* ptr = bigPool.get(id);
            u32 numBytes = impl::LabelEncoder::decodeValue(ptr);
            return StringView{ptr, numBytes} == key;
        }
    };

    PLY_DEFINE_RACE_DETECTOR(raceDetector)
    BigPool<> bigPool;
    HashMap<Traits> strToIndex;

public:
    LabelStorage();

    Label insert(StringView view);
    Label find(StringView view) const;
    StringView view(Label label) const;
};

extern LabelStorage g_labelStorage;

} // namespace ply
