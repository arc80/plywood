/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#include <ply-runtime.h>
#include <ply-test/TestSuite.h>

namespace ply {
namespace tests {

#define PLY_TEST_CASE_PREFIX Array_

//--------------------------------
// Constructors
//--------------------------------
PLY_TEST_CASE("Array default constructor") {
    Array<u32> a;
    PLY_TEST_CHECK(a == ArrayView<const u32>{});
}

PLY_TEST_CASE("Array construct from braced initializer list") {
    Array<u32> a = {4, 5, 6};
    PLY_TEST_CHECK(a == ArrayView<const u32>{4, 5, 6});
}

PLY_TEST_CASE("Array copy constructor") {
    Array<u32> a = {4, 5, 6};
    Array<u32> b = a;
    PLY_TEST_CHECK(a == ArrayView<const u32>{4, 5, 6});
    PLY_TEST_CHECK(b == ArrayView<const u32>{4, 5, 6});
}

PLY_TEST_CASE("Array copy constructor") {
    Array<u32> a = {4, 5, 6};
    Array<u32> b = a;
    PLY_TEST_CHECK(a == ArrayView<const u32>{4, 5, 6});
    PLY_TEST_CHECK(b == ArrayView<const u32>{4, 5, 6});
}

PLY_TEST_CASE("Array move constructor") {
    Array<u32> a = {4, 5, 6};
    Array<u32> b = std::move(a);
    PLY_TEST_CHECK(a == ArrayView<const u32>{});
    PLY_TEST_CHECK(b == ArrayView<const u32>{4, 5, 6});
}

PLY_TEST_CASE("Array construct from any (HybridString), no move semantics") {
    Array<String> a = {"hello", "there"};
    Array<HybridString> b = a;
    PLY_TEST_CHECK(a == ArrayView<const StringView>{"hello", "there"});
    PLY_TEST_CHECK(b == ArrayView<const StringView>{"hello", "there"});
}

PLY_TEST_CASE("Array construct from any (FixedArray), no move semantics") {
    FixedArray<String, 2> a = {"hello", "there"};
    Array<String> b = a;
    PLY_TEST_CHECK(a == ArrayView<const StringView>{"hello", "there"});
    PLY_TEST_CHECK(b == ArrayView<const StringView>{"hello", "there"});
}

PLY_TEST_CASE("Array construct from any (HybridString) with move semantics") {
    Array<String> a = {"hello", "there"};
    Array<HybridString> b = std::move(a);
    PLY_TEST_CHECK(a == ArrayView<const StringView>{{}, {}});
    PLY_TEST_CHECK(b == ArrayView<const StringView>{"hello", "there"});
}

PLY_TEST_CASE("Array construct from any (FixedArray) with move semantics") {
    FixedArray<String, 2> a = {"hello", "there"};
    Array<String> b = std::move(a);
    PLY_TEST_CHECK(a == ArrayView<const StringView>{{}, {}});
    PLY_TEST_CHECK(b == ArrayView<const StringView>{"hello", "there"});
}

PLY_TEST_CASE("Array narrowing conversion") {
    Array<u32> a = {4, 5, 6};
    Array<u16> b = a;
    PLY_TEST_CHECK(b == ArrayView<const u16>{4, 5, 6});
}

PLY_TEST_CASE("Array widening conversion") {
    Array<u16> a = {4, 5, 6};
    Array<u32> b = a;
    PLY_TEST_CHECK(b == ArrayView<const u32>{4, 5, 6});
}

//--------------------------------
// Assignment Operators
//--------------------------------
PLY_TEST_CASE("Array assign from braced initializer list") {
    Array<u32> a;
    a = {4, 5, 6};
    PLY_TEST_CHECK(a == ArrayView<const u32>{4, 5, 6});
}

PLY_TEST_CASE("Array copy assignment") {
    Array<u32> a = {4, 5, 6};
    Array<u32> b;
    b = a;
    PLY_TEST_CHECK(a == ArrayView<const u32>{4, 5, 6});
    PLY_TEST_CHECK(b == ArrayView<const u32>{4, 5, 6});
}

PLY_TEST_CASE("Array move assignment") {
    Array<u32> a = {4, 5, 6};
    Array<u32> b = a;
    b = std::move(a);
    PLY_TEST_CHECK(a == ArrayView<const u32>{});
    PLY_TEST_CHECK(b == ArrayView<const u32>{4, 5, 6});
}

PLY_TEST_CASE("Array assign from any (HybridString), no move semantics") {
    Array<String> a = {"hello", "there"};
    Array<HybridString> b;
    b = a;
    PLY_TEST_CHECK(a == ArrayView<const StringView>{"hello", "there"});
    PLY_TEST_CHECK(b == ArrayView<const StringView>{"hello", "there"});
}

PLY_TEST_CASE("Array assign from any (FixedArray), no move semantics") {
    FixedArray<String, 2> a = {"hello", "there"};
    Array<String> b;
    b = a;
    PLY_TEST_CHECK(a == ArrayView<const StringView>{"hello", "there"});
    PLY_TEST_CHECK(b == ArrayView<const StringView>{"hello", "there"});
}

PLY_TEST_CASE("Array assign from any (HybridString) with move semantics") {
    Array<String> a = {"hello", "there"};
    Array<HybridString> b;
    b = std::move(a);
    PLY_TEST_CHECK(a == ArrayView<const StringView>{{}, {}});
    PLY_TEST_CHECK(b == ArrayView<const StringView>{"hello", "there"});
}

PLY_TEST_CASE("Array assign from any (FixedArray) with move semantics") {
    FixedArray<String, 2> a = {"hello", "there"};
    Array<String> b;
    b = std::move(a);
    PLY_TEST_CHECK(a == ArrayView<const StringView>{{}, {}});
    PLY_TEST_CHECK(b == ArrayView<const StringView>{"hello", "there"});
}

PLY_TEST_CASE("Array self-assignment") {
    Array<u32> a = {1, 1, 2, 3, 5, 8};
    a = a.subView(1);
    PLY_TEST_CHECK(a == ArrayView<const u32>{1, 2, 3, 5, 8});
}

//--------------------------------
// Element Access
//--------------------------------
PLY_TEST_CASE("Array subscript lookup") {
    const Array<u32> a = {4, 5, 6};
    PLY_TEST_CHECK(a[0] == 4);
    PLY_TEST_CHECK(a[1] == 5);
    PLY_TEST_CHECK(a[2] == 6);
}

PLY_TEST_CASE("Array subscript modification") {
    Array<u32> a = {4, 5, 6};
    a[1] = 7;
    PLY_TEST_CHECK(a == ArrayView<const u32>{4, 7, 6});
}

PLY_TEST_CASE("Array back lookup") {
    const Array<u32> a = {4, 5, 6};
    PLY_TEST_CHECK(a.back() == 6);
    PLY_TEST_CHECK(a.back(-2) == 5);
}

PLY_TEST_CASE("Array back modification") {
    Array<u32> a = {4, 5, 6};
    a.back() = 7;
    PLY_TEST_CHECK(a == ArrayView<const u32>{4, 5, 7});
}

PLY_TEST_CASE("Array iteration") {
    Array<u32> a = {4, 5, 6};
    u32 prev = 3;
    for (u32 i : a) {
        PLY_TEST_CHECK(i == prev + 1);
        prev = i;
    }
}

PLY_TEST_CASE("Array iteration 2") {
    Array<u32> a = {4, 5, 6};
    Array<u32> b;
    for (u32 i : a) {
        b.append(i);
    }
    PLY_TEST_CHECK(b == ArrayView<const u32>{4, 5, 6});
}

//--------------------------------
// Capacity
//--------------------------------
PLY_TEST_CASE("Array operator bool") {
    Array<u32> a;
    PLY_TEST_CHECK(!(bool) a);
    a = {4, 5, 6};
    PLY_TEST_CHECK((bool) a);
}

PLY_TEST_CASE("Array isEmpty") {
    Array<u32> a;
    PLY_TEST_CHECK(a.isEmpty());
    a = {4, 5, 6};
    PLY_TEST_CHECK(!a.isEmpty());
}

PLY_TEST_CASE("Array numItems") {
    Array<u32> a;
    PLY_TEST_CHECK(a.numItems() == 0);
    a = {4, 5, 6};
    PLY_TEST_CHECK(a.numItems() == 3);
}

PLY_TEST_CASE("Array sizeBytes") {
    Array<u32> a;
    PLY_TEST_CHECK(a.sizeBytes() == 0);
    a = {4, 5, 6};
    PLY_TEST_CHECK(a.sizeBytes() == 12);
}

//--------------------------------
// Modifers
//--------------------------------
PLY_TEST_CASE("Array clear") {
    Array<u32> a = {4, 5, 6};
    a.clear();
    PLY_TEST_CHECK(a == ArrayView<const u32>{});
}

// FIXME: Add reserve() test?
// Ideally it would measure the number of allocations performed under the hood.

PLY_TEST_CASE("Array resize") {
    Array<u32> a;
    a.resize(3);
    PLY_TEST_CHECK(a.numItems() == 3);
}

PLY_TEST_CASE("Array resize 2") {
    Array<String> a;
    a.resize(3);
    PLY_TEST_CHECK(a == ArrayView<const StringView>{{}, {}, {}});
}

PLY_TEST_CASE("Array append, no move semantics") {
    String s0 = "hello";
    String s1 = "there";
    Array<String> a;
    a.append(s0);
    a.append(s1);
    PLY_TEST_CHECK(s0 == "hello");
    PLY_TEST_CHECK(s1 == "there");
    PLY_TEST_CHECK(a == ArrayView<const StringView>{"hello", "there"});
}

PLY_TEST_CASE("Array append with move semantics") {
    String s0 = "hello";
    String s1 = "there";
    Array<String> a;
    a.append(std::move(s0));
    a.append(std::move(s1));
    PLY_TEST_CHECK(s0 == "");
    PLY_TEST_CHECK(s1 == "");
    PLY_TEST_CHECK(a == ArrayView<const StringView>{"hello", "there"});
}

PLY_TEST_CASE("Array append any (HybridString), no move semantics") {
    String s = "hello";
    Array<HybridString> a;
    a.append(s);
    PLY_TEST_CHECK(s == "hello");
    PLY_TEST_CHECK(a == ArrayView<const StringView>{"hello"});
}

PLY_TEST_CASE("Array append any (HybridString) with move semantics") {
    String s = "hello";
    Array<HybridString> a;
    a.append(std::move(s));
    PLY_TEST_CHECK(s == "");
    PLY_TEST_CHECK(a == ArrayView<const StringView>{"hello"});
}

PLY_TEST_CASE("Array extend from braced initializer list") {
    Array<String> a;
    a.extend({"hello", "there"});
    PLY_TEST_CHECK(a == ArrayView<const StringView>{"hello", "there"});
}

PLY_TEST_CASE("Array extend from any (same type), no move semantics") {
    Array<String> a = {"hello", "there"};
    Array<String> b;
    b.extend(a);
    PLY_TEST_CHECK(a == ArrayView<const StringView>{"hello", "there"});
    PLY_TEST_CHECK(b == ArrayView<const StringView>{"hello", "there"});
}

PLY_TEST_CASE("Array extend from any (same type) with move semantics") {
    Array<String> a = {"hello", "there"};
    Array<String> b;
    b.extend(std::move(a));
    PLY_TEST_CHECK(a == ArrayView<const StringView>{{}, {}});
    PLY_TEST_CHECK(b == ArrayView<const StringView>{"hello", "there"});
}

PLY_TEST_CASE("Array extend from any (FixedArray), no move semantics") {
    FixedArray<String, 2> a = {"hello", "there"};
    Array<String> b;
    b.extend(a);
    PLY_TEST_CHECK(a == ArrayView<const StringView>{"hello", "there"});
    PLY_TEST_CHECK(b == ArrayView<const StringView>{"hello", "there"});
}

PLY_TEST_CASE("Array extend from any (FixedArray) with move semantics") {
    FixedArray<String, 2> a = {"hello", "there"};
    Array<String> b;
    b.extend(std::move(a));
    PLY_TEST_CHECK(a == ArrayView<const StringView>{{}, {}});
    PLY_TEST_CHECK(b == ArrayView<const StringView>{"hello", "there"});
}

PLY_TEST_CASE("Array extend from any (HybridString), no move semantics") {
    Array<String> a = {"hello", "there"};
    Array<HybridString> b;
    b.extend(a);
    PLY_TEST_CHECK(a == ArrayView<const StringView>{"hello", "there"});
    PLY_TEST_CHECK(b == ArrayView<const StringView>{"hello", "there"});
}

PLY_TEST_CASE("Array extend from any (HybridString) with move semantics") {
    Array<String> a = {"hello", "there"};
    Array<HybridString> b;
    b.extend(std::move(a));
    PLY_TEST_CHECK(a == ArrayView<const StringView>{{}, {}});
    PLY_TEST_CHECK(b == ArrayView<const StringView>{"hello", "there"});
}

PLY_TEST_CASE("Array moveExtend") {
    Array<String> a = {"hello", "there"};
    Array<String> b = {"my", "friend"};
    a.moveExtend(b);
    PLY_TEST_CHECK(a == ArrayView<const StringView>{"hello", "there", "my", "friend"});
    PLY_TEST_CHECK(b == ArrayView<const StringView>{{}, {}});
}

PLY_TEST_CASE("Array pop") {
    Array<u32> a = {4, 5, 6};
    a.pop();
    PLY_TEST_CHECK(a == ArrayView<const u32>{4, 5});
    a.pop(2);
    PLY_TEST_CHECK(a == ArrayView<const u32>{});
}

PLY_TEST_CASE("Array insert") {
    Array<u32> a = {4, 5, 6};
    a.insert(2) = 7;
    PLY_TEST_CHECK(a == ArrayView<const u32>{4, 5, 7, 6});
}

PLY_TEST_CASE("Array insert 2") {
    Array<String> a = {"hello", "there"};
    a.insert(1, 2);
    PLY_TEST_CHECK(a == ArrayView<const StringView>{"hello", {}, {}, "there"});
}

PLY_TEST_CASE("Array erase") {
    Array<u32> a = {4, 5, 6};
    a.erase(0);
    PLY_TEST_CHECK(a == ArrayView<const u32>{5, 6});

    Array<u32> b = {4, 5, 6, 7};
    b.erase(1, 2);
    PLY_TEST_CHECK(b == ArrayView<const u32>{4, 7});
}

PLY_TEST_CASE("Array eraseQuick") {
    Array<u32> a = {4, 5, 6};
    a.eraseQuick(0);
    PLY_TEST_CHECK(a == ArrayView<const u32>{6, 5});

    Array<u32> b = {4, 5, 6, 7, 8, 9, 10};
    b.eraseQuick(1, 2);
    PLY_TEST_CHECK(b == ArrayView<const u32>{4, 9, 10, 7, 8});
}

PLY_TEST_CASE("Array concatentate without move") {
    Array<String> a = {"hello", "there"};
    Array<String> b = a + ArrayView<const StringView>{"my", "friend"};
    PLY_TEST_CHECK(a == ArrayView<const StringView>{{"hello"}, {"there"}});
    PLY_TEST_CHECK(b == ArrayView<const StringView>{"hello", "there", "my", "friend"});
}

PLY_TEST_CASE("Array concatenate with move") {
    Array<String> a = {"hello", "there"};
    Array<String> b = std::move(a) + ArrayView<const StringView>{"my", "friend"};
    PLY_TEST_CHECK(a == ArrayView<const StringView>{{}, {}});
    PLY_TEST_CHECK(b == ArrayView<const StringView>{"hello", "there", "my", "friend"});
}

} // namespace tests
} // namespace ply
