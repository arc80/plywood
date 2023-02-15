This is the development branch of Plywood, a multimedia development kit for C++.

Plywood isn't a game engine, but it provides a lot of the raw material needed to create a game engine.

Its main goals and features are:

- An alternative to the standard C++ library
- Runtime reflection and serialization
- Primitives for audio, video and graphics
- Easy to integrate third-party libraries

Note: This branch (development) has diverged significantly from main. The build system is completely different, there's a new scripting language that integrates tightly with the runtime, and a lot of the API and implementation has been bikeshed and reworked. (`HashMap`, for example, is being phased out in favor of `Map`.) The main branch will eventually be replaced with this one.

Things are very much in flux. On Windows, you can clone the repo, open a Visual Studio command prompt and run `setup.bat`. Linux and macOS support is currently broken, but could be brought back at any time.

If you think you'd be interested in contributing to this project, or you'd just like to become familiar enough to give feedback, start by browsing these files:

* [<ply-runtime.h>](https://github.com/arc80/plywood/blob/development/base/src/runtime/ply-runtime.h)
* [<ply-runtime/container.h>](https://github.com/arc80/plywood/blob/development/base/src/runtime/ply-runtime/container.h)
* [<ply-runtime/io.h>](https://github.com/arc80/plywood/blob/development/base/src/runtime/ply-runtime/io.h)
* [<ply-runtime/network.h>](https://github.com/arc80/plywood/blob/development/base/src/runtime/ply-runtime/network.h)

Feedback and good-natured criticisms welcome. Don't hesitate to ask questions, either. [Discord](https://discord.gg/WnQhuVF) is the best place for both.
