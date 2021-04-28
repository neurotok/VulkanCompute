#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

#include "compute.h"
#include "device.h"
#include "instance.h"
#include "memory.h"

VkBuffer InputBuffer = VK_NULL_HANDLE;
VkBuffer OutputBuffer = VK_NULL_HANDLE;
VkDeviceMemory InputBufferMemory = VK_NULL_HANDLE;
VkDeviceMemory OutputBufferMemory = VK_NULL_HANDLE;

static uint32_t FindMemoryIndexByType(uint32_t allowedTypeMask,
                                      VkMemoryPropertyFlags flags) {
  VkPhysicalDeviceMemoryProperties memProperties;
  vkGetPhysicalDeviceMemoryProperties(PhysicalDevice, &memProperties);

  uint32_t typeMask = 1;

  for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++, typeMask <<= 1) {
    if ((allowedTypeMask & typeMask) != 0) {
      if ((memProperties.memoryTypes[i].propertyFlags & flags) == flags) {
        return i;
      }
    }
  }

  printf("Failed to find memory type index\n");
  return 0;
}

VkBuffer CreateBufferAndMemory(size_t size, VkDeviceMemory *defviceMemory,
                               VkBufferUsageFlagBits usage,
                               VkMemoryPropertyFlagBits memProperties) {
  VkBuffer buffer;
  VkDeviceMemory memory;

  VkBufferCreateInfo bufferInfo = {
      .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
      .size = size,
      .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
      .usage = usage,
  };

  if (vkCreateBuffer(LogicalDevice, &bufferInfo, NULL, &buffer) != VK_SUCCESS) {
    printf("Failed to create buffer\n");
    return VK_NULL_HANDLE;
  }

  VkMemoryRequirements memoryRequirements;
  vkGetBufferMemoryRequirements(LogicalDevice, buffer, &memoryRequirements);

  VkMemoryAllocateInfo memoryAllocInfo = {
      .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
      .allocationSize = memoryRequirements.size,
      .memoryTypeIndex = FindMemoryIndexByType(
          memoryRequirements.memoryTypeBits, memProperties),
  };

  if (vkAllocateMemory(LogicalDevice, &memoryAllocInfo, NULL, &memory) !=
      VK_SUCCESS) {
    printf("Failed to allocate mamory for the buffer\n");
    vkDestroyBuffer(LogicalDevice, buffer, NULL);
    return VK_NULL_HANDLE;
  }

  if (vkBindBufferMemory(LogicalDevice, buffer, memory, 0) != VK_SUCCESS) {
    printf("Failed to bind buffer and memmory\n");

    vkDestroyBuffer(LogicalDevice, buffer, NULL);
    return VK_NULL_HANDLE;
  }

  *defviceMemory = memory;
  return buffer;
}

void CreateBuffers(size_t inputSize, size_t outputSize) {
  InputBuffer = CreateBufferAndMemory(inputSize, &InputBufferMemory,
                                      VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
                                      VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                                      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
  OutputBuffer = CreateBufferAndMemory(outputSize, &OutputBufferMemory,
                                       VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                                       VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

  VkWriteDescriptorSet writeDescriptorSet = {
      .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
      .dstSet = DescriptorSet,
      .dstBinding = 0,
      .descriptorCount = 2,
      .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
      .pBufferInfo = (VkDescriptorBufferInfo[2]){
          [0].buffer = InputBuffer,
          [0].offset = 0,
          [0].range = inputSize,
          [1].buffer = OutputBuffer,
          [1].offset = 0,
          [1].range = outputSize,
      }};

  vkUpdateDescriptorSets(LogicalDevice, 1, &writeDescriptorSet, 0, NULL);
}

static void CopyData(void *data, size_t size, bool isWrite,
                      VkDeviceMemory memory) {
  void *adress;
  if (vkMapMemory(LogicalDevice, memory, 0, size, 0, &adress) != VK_SUCCESS) {
    printf("Failed to map buffer memory\n");
    return;
  }

  isWrite ? memcpy(adress, data, size) : memcpy(data, adress, size);

  vkUnmapMemory(LogicalDevice, memory);
}

void CopyToInputBuffer(void *data, size_t size) {
  CopyData(data, size, true, InputBufferMemory);
}

void CopyFromOutputBuffer(void *data, size_t size) {
  CopyData(data, size, false, OutputBufferMemory);
}

void CopyBufferToBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, size_t size)
{

}

void CopyToLocalBuffer(void *data, size_t size, VkBuffer buffer)
{
    VkDeviceMemory tempMemory;
    VkBuffer tempBuffer = CreateBufferAndMemory(size, &tempMemory,
    VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
    VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    CopyData(data, size, 1, tempMemory);

    CopyBufferToBuffer(tempBuffer, buffer, size);

    vkDestroyBuffer(LogicalDevice, tempBuffer, NULL);
    vkFreeMemory(LogicalDevice, tempMemory, NULL);
}

void DestroyBuffers(void) {
  vkDestroyBuffer(LogicalDevice, InputBuffer, NULL);
  vkFreeMemory(LogicalDevice, InputBufferMemory, NULL);
  vkDestroyBuffer(LogicalDevice, OutputBuffer, NULL);
  vkFreeMemory(LogicalDevice, OutputBufferMemory, NULL);
}
