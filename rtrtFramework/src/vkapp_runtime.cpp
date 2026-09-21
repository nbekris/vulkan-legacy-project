
#include "vkapp.h"
#include "app.h"
#include "extensions_vk.hpp"

void VkApp::drawFrame()
{
    // @@ When directed to do so (in Project 1), uncomment the
    // following 4 lines.  All the Project 1 Vulkan initialization
    // steps must be completed first.
  
     //if (acquireFrame()) {
     //    renderGui();
     //    renderFrame();
     //    submitFrame(); 
     //}
}

bool VkApp::acquireFrame()
{
    // Use a fence to wait until the command buffer has finished execution before using it again
    vkWaitForFences(m_device, 1, &m_inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);
        
    // Acquire the next image from the swap chain --> m_swapchainIndex
    VkResult result = vkAcquireNextImageKHR(m_device, m_swapchain, 1000000,//UINT64_MAX, 
                                            m_imageAvailableSemaphores[currentFrame],
                                            (VkFence)VK_NULL_HANDLE, &m_swapchainIndex);
    

    // Check if window has been resized -- or other(??) swapchain specific event
    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        recreateSizedVulkanResources();
        return false; }
    else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        throw std::runtime_error("failed to acquire swap chain image!"); }

    // Next frame successfully acquired.
    vkResetFences(m_device, 1, &m_inFlightFences[currentFrame]);
    return true;
}

void VkApp::renderFrame()
{
    
     //VkCommandBufferBeginInfo beginInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
     //beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    
     //vkBeginCommandBuffer(m_commandBuffer, &beginInfo);
    {   // Extra indent for code clarity
        // updateCameraBuffer();
        
        // Draw scene
        // if (useRaytracer) {
        //     raytrace();`
        //     denoise();
        // } else
        //     rasterize();
        
         //postProcess(); //  tone mapper and output to swapchain image.
    }   // Done recording;  Execute!
     //vkEndCommandBuffer(m_commandBuffer);
    
}

void VkApp::renderGui()
{
#ifdef GUI
    ImGui::SetCurrentContext(m_ImGuiContext);
        
    ImGui_ImplVulkan_NewFrame(); 
    ImGui_ImplGlfw_NewFrame(); 
    ImGui::NewFrame(); 
        
    if(show_gui)
        drawGUI();
        
    ImGui::Render();  // Rendering UI
    draw_data = ImGui::GetDrawData();
#endif
}
    
void VkApp::submitFrame()
{
    // Pipeline stage at which the queue submission will wait (via pWaitSemaphores)
    const VkPipelineStageFlags waitStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
     
    // The submit info structure specifies a command buffer queue submission batch
    VkSubmitInfo submitInfo{VK_STRUCTURE_TYPE_SUBMIT_INFO};
    submitInfo.pNext             = nullptr;
    submitInfo.pWaitDstStageMask = &waitStageMask; //  pipeline stages to wait for
    submitInfo.waitSemaphoreCount   = 1;  
    submitInfo.pWaitSemaphores = &m_imageAvailableSemaphores[currentFrame];  // waited upon before execution
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores    = &m_renderFinishedSemaphores[currentFrame]; // signaled when execution finishes
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &m_commandBuffer;
    vkQueueSubmit(m_queue, 1, &submitInfo, m_inFlightFences[currentFrame]);
    
    // Present frame
    VkPresentInfoKHR presentInfo{VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores    = &m_renderFinishedSemaphores[currentFrame];
    presentInfo.swapchainCount     = 1;
    presentInfo.pSwapchains        = &m_swapchain;
    presentInfo.pImageIndices      = &m_swapchainIndex;
    vkQueuePresentKHR(m_queue, &presentInfo);

    // Advance currentFrame to the next swapchain image, command buffer, and synchronization flags.
    currentFrame = (currentFrame+1) % m_imageCount;
    m_commandBuffer = m_commandBuffers[currentFrame];

    // @@ Verify success of vkQueueSubmit and vkQueuePresentKHR
    VkResult result = vkQueuePresentKHR(m_queue, &presentInfo);
    if (result != VK_SUCCESS && result != VK_ERROR_OUT_OF_DATE_KHR)
        throw std::runtime_error("failed vkQueuePresentKHR");
}


void VkApp::drawGUI()
{
    
    // @@ Once GUI is defined, you can put some gui elements on the screen
    
    // ImGui::ShowDemoWindow();  // Turn on ImGui's demonstration of all widgets.

    // Display the frame rate:
    // ImGui::Text("Rate %.3f ms/frame (%.1f FPS)",
    //             1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);

    // An example check box:
    // ImGui::Checkbox("Raytrace mode", &useRaytracer);

    // An example slider:
    // if (ImGui::SliderFloat("Exposure", &m_pcRay.exposure, 0.5f, 8.0f, "%.5f"))
    //    m_pcRay.clear = true;

}
