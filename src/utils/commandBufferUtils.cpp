#include "commandBufferUtils.h"


namespace utils {
namespace cmd {

VkCommandBuffer BeginSingleTimeCommands(VkDevice deviceVk, VkCommandPool commandPool)
{
	// transfer operations are also executed using command buffers
	// so we allocate a temporary command buffer
	VkCommandBufferAllocateInfo cmdBuffAllocInfo{};
	cmdBuffAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	cmdBuffAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	cmdBuffAllocInfo.commandPool = commandPool;
	cmdBuffAllocInfo.commandBufferCount = 1;

	VkCommandBuffer cmdBuff;
	vkAllocateCommandBuffers(deviceVk, &cmdBuffAllocInfo, &cmdBuff);

	// immediately start recording the command buffer
	VkCommandBufferBeginInfo cmdBuffBegin{};
	cmdBuffBegin.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	cmdBuffBegin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(cmdBuff, &cmdBuffBegin);

	return cmdBuff;
}

void EndSingleTimeCommands(VkDevice deviceVk, VkQueue graphicsQueue, VkCommandPool commandPool, VkCommandBuffer cmdBuff)
{
	vkEndCommandBuffer(cmdBuff);

	// submit the queue
	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &cmdBuff;

	// we dont necessarily need a transfer queue, graphics queue can handle
	// it
	vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
	// we can use a fence to wait; this can help to schedule multiple
	// transfers simultaneously
	vkQueueWaitIdle(graphicsQueue);

	vkFreeCommandBuffers(deviceVk, commandPool, 1, &cmdBuff);
}

} // namespace cmd
} // namespace utils