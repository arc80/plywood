/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <ply-runtime/container/LabelMap.h>
#include <ply-test/TestSuite.h>

namespace ply {
namespace tests {

#define PLY_TEST_CASE_PREFIX LabelMap_

//--------------------------------
// Constructors
//--------------------------------
PLY_TEST_CASE("LabelMap tests") {
    for (u32 numItems : {2, 3, 4, 5, 6, 8, 10, 20, 50, 200, 300, 1000}) {
        u32 seed = avalanche(numItems);

        // Populate a map
        LabelMap<u32> map;
        for (u32 i = 0; i < numItems; i++) {
            u32 key = avalanche(i + seed);
            u32 *value;
            PLY_TEST_CHECK(map.insertOrFind(Label{key}, &value));
            *value = i * seed;
            PLY_TEST_CHECK(!map.insertOrFind(Label{key}, &value));
        }
        PLY_TEST_CHECK(map.numItems() == numItems);

        // Find each item in map
        for (u32 i = 0; i < numItems; i++) {
            u32 key = avalanche(i + seed);
            u32 *value = map.find(Label{key});
            if (PLY_TEST_CHECK(value)) {
                PLY_TEST_CHECK(*value == i * seed);
            }
        }

        // Iterate over map
        Array<bool> found;
        found.resize(numItems);
        for (bool& f : found) {
            f = false;
        }
        for (auto item : map) {
            u32 i = deavalanche(item.key.idx) - seed;
            if (PLY_TEST_CHECK(i < numItems)) {
                PLY_TEST_CHECK(item.value == i * seed);
                PLY_TEST_CHECK(!found[i]);
                found[i] = true;
            }
        }
    }
}

} // namespace tests
} // namespace ply
