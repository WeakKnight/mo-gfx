/* -*- tab-width: 4; -*- */
/* vi: set sw=2 ts=4 expandtab: */

/*
 * ©2017-2018 Mark Callow.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <vector>

#include <vulkan/vulkan.h>

#if !defined(USE_FUNCPTRS_FOR_KHR_EXTS)
#define USE_FUNCPTRS_FOR_KHR_EXTS 0
#endif

typedef struct _SwapchainBuffers {
    VkImage image;
    VkImageView view;
} SwapchainBuffer;

class VulkanSwapchain
{
  public:
    VkFormat colorFormat;
    VkColorSpaceKHR colorSpace;

    VkSwapchainKHR swapchain = VK_NULL_HANDLE;

    uint32_t imageCount;
    std::vector<VkImage> images;
    std::vector<SwapchainBuffer> buffers;

    // Index of the detected graphics- and present-capable device queue.
    uint32_t queueIndex = UINT32_MAX;

    // Creates an OS specific surface.
    // Looks for a graphics and a present queue
    bool initSurface(struct SDL_Window* window);

    // Connect to device and get required device function pointers.
    bool connectDevice(VkDevice device);

    // Connect to instance and get required instance function pointers.
    bool connectInstance(VkInstance instance,
                         VkPhysicalDevice physicalDevice);

    // Create the swap chain and get images with given width and height
    void create(uint32_t *width, uint32_t *height,
                bool vsync = false);

    // Acquires the next image in the swap chain
    VkResult acquireNextImage(VkSemaphore presentCompleteSemaphore,
                              uint32_t *currentBuffer);

    // Present the current image to the queue
    VkResult queuePresent(VkQueue queue, uint32_t currentBuffer);

    // Present the current image to the queue
    VkResult queuePresent(VkQueue queue, uint32_t currentBuffer,
                          VkSemaphore waitSemaphore);


    // Free all Vulkan resources used by the swap chain
    void cleanup();

  private:
    VkInstance instance;
    VkDevice device;
    VkPhysicalDevice physicalDevice;
    VkSurfaceKHR surface;
#if USE_FUNCPTRS_FOR_KHR_EXTS
    PFN_vkGetPhysicalDeviceSurfaceSupportKHR
        pfnGetPhysicalDeviceSurfaceSupportKHR;
    PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR
        pfnGetPhysicalDeviceSurfaceCapabilitiesKHR;
    PFN_vkGetPhysicalDeviceSurfaceFormatsKHR
        pfnGetPhysicalDeviceSurfaceFormatsKHR;
    PFN_vkGetPhysicalDeviceSurfacePresentModesKHR
        pfnGetPhysicalDeviceSurfacePresentModesKHR;

    PFN_vkCreateSwapchainKHR pfnCreateSwapchainKHR;
    PFN_vkDestroySwapchainKHR pfnDestroySwapchainKHR;
    PFN_vkGetSwapchainImagesKHR pfnGetSwapchainImagesKHR;
    PFN_vkAcquireNextImageKHR pfnAcquireNextImageKHR;
    PFN_vkQueuePresentKHR pfnQueuePresentKHR;

#define vkGetPhysicalDeviceSurfaceSupportKHR \
            pfnGetPhysicalDeviceSurfaceSupportKHR
#define vkGetPhysicalDeviceSurfaceCapabilitiesKHR \
            pfnGetPhysicalDeviceSurfaceCapabilitiesKHR
#define vkGetPhysicalDeviceSurfaceFormatsKHR \
            pfnGetPhysicalDeviceSurfaceFormatsKHR
#define vkGetPhysicalDeviceSurfacePresentModesKHR \
            pfnGetPhysicalDeviceSurfacePresentModesKHR

#define vkCreateSwapchainKHR pfnCreateSwapchainKHR
#define vkDestroySwapchainKHR pfnDestroySwapchainKHR
#define vkGetSwapchainImagesKHR pfnGetSwapchainImagesKHR
#define vkAcquireNextImageKHR pfnAcquireNextImageKHR
#define vkQueuePresentKHR pfnQueuePresentKHR
#endif
};
