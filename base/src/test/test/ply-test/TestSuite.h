/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#include <ply-runtime/Base.h>

namespace ply {
namespace test {

struct Register {
    Register(StringView name, void (*func)());
};

#define PLY_TEST_CASE(name) \
    void PLY_CAT(PLY_CAT(test_, PLY_TEST_CASE_PREFIX), __LINE__)(); \
    void (*PLY_CAT(PLY_CAT(testlink_, PLY_TEST_CASE_PREFIX), __LINE__))() = \
        &PLY_CAT(PLY_CAT(test_, PLY_TEST_CASE_PREFIX), __LINE__); \
    ::ply::test::Register PLY_CAT(PLY_CAT(auto_reg_, PLY_TEST_CASE_PREFIX), __LINE__){ \
        name, PLY_CAT(PLY_CAT(test_, PLY_TEST_CASE_PREFIX), __LINE__)}; \
    void PLY_CAT(PLY_CAT(test_, PLY_TEST_CASE_PREFIX), __LINE__)()

bool check(bool);
#define PLY_TEST_CHECK ::ply::test::check

bool run();

} // namespace test
} // namespace ply
