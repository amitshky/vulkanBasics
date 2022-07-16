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
### General overview
* Create an instance of the Vulkan API (`VkInstance`)
* Query for Vulkan supported hardware `VkPhysicalDevice`
* Create a logical device `VkDevice` to specify which features you want to use (`VkPhysicalDeviceFeatures`)
* Vulkan commands are executed asynchronously by submitting them to `VkQueue`
* There are different queue families, each of which supports a specific set of operations
* We need two more components to render window surface - `VkSurfaceKHR` and `VkSwapchainKHR`
* Drawing an image requires `VkImageView` and `VkFramebuffer`
* Render pass determines how the contents of the image should be treated
* Before Vulkan operations are submitted to the queue, they need to be recorded to `VkCommandBuffer`
* The commands are allocated from a `VkCommandPool`

### Validation layers
* Similar to extensions, validation layers need to be enabled by specifying their name
* All of the useful Standard validation is bundled into `VK_LAYER_KHRONOS_validation`
* Validation layers log debug messages into standard output by default
	* This can be changed by providing explicit callback
* Message callbacks filters logs (you can configure to show certail logs)

### Physical device
* We need to select a physical device that supports the features we need
* Multiple physical devices can be selected and run simultaneously

### Queue families
* Every Vulkan operation requries commands to be submitted to a queue
* Each family of queues only allows a subset of commands


### Logical device and queues
* Specify which queues to create after querying queue families
* Currently available drivers will only allow to create a small number of queues for each queue family
	* you don't really need more than one
* We can create all of the command buffers on multiple threads and then submit them all at once on the main thread
* you can assign priorities to the queues
* Queues are automatically created with the logical device
	* Device queues are implicitly destroyed when the device is destroyed


## References
* [Vulkan tutorial](https://vulkan-tutorial.com/)


## Dev Screenshots
* Initial console window showing available Vulkan extensions and physical device

	<img src="img/initial.png" width=450>