/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#include <ply-web-cook-docs/Core.h>
#include <ply-cook/CookJob.h>

namespace ply {
namespace docs {
extern cook::CookJobType CookJobType_ExtractAPI;
extern cook::CookJobType CookJobType_StyleSheetID;
extern cook::CookJobType CookJobType_Page;

void initCookJobTypes() {
    static BaseStaticPtr::PossibleValues pv;
    for (cook::CookJobType* jobType : {
             &docs::CookJobType_ExtractAPI,
             &docs::CookJobType_StyleSheetID,
             &docs::CookJobType_Page,
         }) {
        pv.enumeratorNames.append(jobType->name);
        pv.ptrValues.append(jobType);
    }

    TypeDescriptor_StaticPtr* staticPtrType = getTypeDescriptor<StaticPtr<cook::CookJobType>>();
    PLY_ASSERT(!staticPtrType->possibleValues);
    staticPtrType->possibleValues = &pv;
}

} // namespace docs
} // namespace ply
