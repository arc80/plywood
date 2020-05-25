#include <ply-runtime/Precomp.h>

#if PLY_TARGET_APPLE

#include <ply-runtime/filesystem/Bundle.h>
#import <Foundation/Foundation.h>

namespace ply {

String getBundlePath() {
    @autoreleasepool {
        NSString* resourcePath = [[NSBundle mainBundle] resourcePath];
        return String([resourcePath fileSystemRepresentation]);
    }
}

} // namespace ply

#endif // PLY_PLATFORM_APPLE
