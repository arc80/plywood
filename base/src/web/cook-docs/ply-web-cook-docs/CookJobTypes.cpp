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

void init_cook_job_types() {
    static BaseStaticPtr::PossibleValues pv;
    for (cook::CookJobType* job_type : {
             &docs::CookJobType_ExtractAPI,
             &docs::CookJobType_StyleSheetID,
             &docs::CookJobType_Page,
         }) {
        pv.enumerator_names.append(job_type->name);
        pv.ptr_values.append(job_type);
    }

    TypeDescriptor_StaticPtr* static_ptr_type =
        get_type_descriptor<StaticPtr<cook::CookJobType>>();
    PLY_ASSERT(!static_ptr_type->possible_values);
    static_ptr_type->possible_values = &pv;
}

} // namespace docs
} // namespace ply
