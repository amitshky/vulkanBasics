# vulkanBasics
Learning Vulkan API


## Prerequisites
* [CMake](https://cmake.org/download/)
* [Vulkan SDK](https://vulkan.lunarg.com/sdk/home) ([Installation guide](https://vulkan.lunarg.com/doc/sdk/latest/windows/getting_started.html))


## Build and Run
```
cmake -B build -S .
cmake --build build
```
* Then navigate to the output file (.exe) and run it.

OR (in VSCode)

* Start debugging (Press F5) (Currently configured for Clang with Ninja and MSVC for Windows)

OR (using bat scripts from `scripts` folder)

* Run them from the root directory of the repo. For example:
```
./scripts/config-msvc.bat
./scripts/build-msvc-rel.bat
./scripts/run-msvc-rel.bat
```


## Notes
(see [notes.md](./notes.md))



## References
* [Vulkan tutorial](https://vulkan-tutorial.com/)


## Dev Screenshots
* Initial console window showing available Vulkan extensions and physical device

	<img src="img/initial.png" width=450>

* Hello Triangle

	<img src="img/hellotriangle.png" width=450>