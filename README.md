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
	* You don't really need more than one
* We can create all of the command buffers on multiple threads and then submit them all at once on the main thread
* You can assign priorities to the queues
* Queues are automatically created with the logical device
	* Device queues are implicitly destroyed when the device is destroyed

### Window surface
* Vulkan cannot directly interface with the window system
	* We use WSI (Window System Integration) extensions
		* `VK_KHR_surface`
* Window surface is required to render images

### Presentation queue
* Queue families supporting drawing commands and the ones supporting presentation may not overlap
	* there should be a distinct presentation queue

### Swap chain
* Vulkan doesn't have a default framebuffer
* It is a queue of images waiting to be rendered on the screen
* All graphics cards cannot render images directly to the screen
	* So no such functionality in Vulkan core
	* We need to enable `VK_KHR_swapchain` device extension
* Simply checking swap chain availability is not enough, we need to check if it is supported by our window surface.
* We also need to check:
	* basic surface capabilities (min/max number of images in swap chain)
	* surface formats (pixel format and color space)
	* available presentation modes
* We need to find the right settings for swap chain, settings such as 
	* Surface format (color depth)
	* Presentation mode (conditions for "swapping" images to the screen)
	* Swap extent (resolution of images in swap chain)
	* (we will have an ideal value in mind for each of these)
* Presentation mode
	* Only `VK_PRESENT_MODE_FIFO_KHR` is guaranteed to be available
	* `VK_PRESENT_MODE_IMMEDIATE_KHR`: immediately display images from the front of the queue; v-sync: off
	* `VK_PRESENT_MODE_FIFO_KHR`: the swap chain acts like a queue (writes image from the front); v-sync: on
	* `VK_PRESENT_MODE_FIFO_RELAXED_KHR`: doesnt wait for the next vertical blank; if the application is late and the queue is empty
	* `VK_PRESENT_MODE_MAILBOX_KHR`: images in the queue are replaced with new ones
* Swap extent
	* match the resolution of the window and the swap chain images

### Image views
* To view an image; access the image and which part of the image to access


## References
* [Vulkan tutorial](https://vulkan-tutorial.com/)


## Dev Screenshots
* Initial console window showing available Vulkan extensions and physical device

	<img src="img/initial.png" width=450>