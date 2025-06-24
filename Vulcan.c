#include "Global.h"

#define	APP_NAME "Nx3D"

static const char *vert_shader_source = 
    "#version 450\n"
    "layout(location = 0) in vec3 pos;\n"
    "layout(location = 1) in vec3 normal;\n"
    "layout(location = 0) out vec3 frag_normal;\n"
    "void main() {\n"
    "   gl_Position = vec4(pos, 1.0);\n"
    "   frag_normal = normal;\n"
    "}\n";

static const char *frag_shader_source = 
    "#version 450\n"
    "layout(location = 0) in vec3 frag_normal;\n"
    "layout(location = 0) out vec4 out_color;\n"
    "void main() {\n"
    "   vec3 light_dir = normalize(vec3(-1.0, 0.5, 0.0));\n"
    "   float diffuse = max(dot(frag_normal, light_dir), 0.0);\n"
    "   out_color = vec4(vec3(1.0, 0.8, 0.5) * diffuse, 1.0);\n"
    "}\n";

// Function to create an instance of the Vulkan driver for RPi
void create_instance() 
{
    VkApplicationInfo app_info = 
      {
      .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
      .pApplicationName = APP_NAME,
      .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
      .pEngineName = "No Engine",
      .engineVersion = VK_MAKE_VERSION(1, 0, 0),
      .apiVersion = VK_API_VERSION_1_0,
      };
    
    VkInstanceCreateInfo create_info = 
      {
      .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
      .pApplicationInfo = &app_info,
      };
      
    VkResult res = vkCreateInstance(&create_info, NULL, &instance);
    if (res != VK_SUCCESS) 
      {
      printf("Failed to create Vulkan instance.\n");
      exit(EXIT_FAILURE);
      }
    else 
      {
      printf("  Vulkan instance successfully created.\n");
      }
	  
}

// Function to identify the physical devices available to Vulkan on RPi
// which would be an internal GPU that supports graphics, then create a
// Vulkan logic device from that info.
void create_device() 
{
    uint32_t 	device_count=0, i=0;
    uint32_t 	queue_family_count = 0;
    
    VkQueueFamilyProperties 		queue_family_properties;
    VkPhysicalDeviceMemoryProperties 	memory_properties;

    
    // get count of vulkan capable devices on this system
    vkEnumeratePhysicalDevices(instance, &device_count, NULL);
    if(device_count==0) 
      {
      printf("No Vulkan-capable devices found.\n");
      exit(EXIT_FAILURE);
      }
    printf("  %d Vulkan-capable devices found.\n",device_count);
    
    // scan thru list of devices searching for a GPU
    VkPhysicalDevice *devices = malloc(sizeof(VkPhysicalDevice) * device_count);
    vkEnumeratePhysicalDevices(instance, &device_count, devices);
    for(i=0;i<device_count;i++) 
      {
      VkPhysicalDeviceProperties props;
      vkGetPhysicalDeviceProperties(devices[i], &props);
      if(props.deviceType==VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU) 
        {
        physical_device=devices[i];
        break;
        }
      }
    free(devices);
    if(physical_device==VK_NULL_HANDLE) 
      {
      printf("Failed to find a suitable GPU.\n");
      exit(EXIT_FAILURE);
      }
    else 
      {
      printf("  Vulkan physical device = %d \n",physical_device);
      }
    
    
    // load queue properties from the selected GPU device
    vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count, NULL);
    VkQueueFamilyProperties queue_family_properties_array[queue_family_count];
    vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count, queue_family_properties_array);
    
    // scan list of GPU queues for one that supports graphics
    for(i=0;i<queue_family_count;i++) 
      {
      queue_family_properties = queue_family_properties_array[i];
      if(queue_family_properties.queueFlags & VK_QUEUE_GRAPHICS_BIT)
        {
        queue_family_index = i;
        break;
        }
      }
    if(queue_family_index==UINT32_MAX) 
      {
      printf("  Failed to find a suitable queue.\n");
      exit(EXIT_FAILURE);
      }
    else  
      {
      printf("  Vulkan queue family index = %d \n",queue_family_index);
      }

    // at this point we have enough information to create the Vulkan logic device
    VkDeviceQueueCreateInfo queue_create_info = 
      {
      .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
      .queueFamilyIndex = queue_family_index,
      .queueCount = 1,
      .pQueuePriorities = (float[]) { 1.0 },
      };
    VkPhysicalDeviceFeatures device_features = {0};
    VkDeviceCreateInfo create_info = 
      {
      .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
      .queueCreateInfoCount = 1,
      .pQueueCreateInfos = &queue_create_info,
      .pEnabledFeatures = &device_features,
      };
    VkResult res = vkCreateDevice(physical_device, &create_info, NULL, &device);
    if(res!=VK_SUCCESS) 
      {
      printf("Failed to create Vulkan logical device: %d\n", res);
      exit(EXIT_FAILURE);
      }
    else  
      {
      printf("  Vulkan logical device successfully created. \n");
      }
    
    vkGetDeviceQueue(device, queue_family_index, 0, &queue);
    
    vkGetPhysicalDeviceMemoryProperties(physical_device, &memory_properties);
    
    
}

// Function to create command pool and command buffer for Vulkan device
void create_command_pool()
{

    // Define the command pool properties
    VkCommandPoolCreateInfo commandPoolCreateInfo = {};
    commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    commandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    commandPoolCreateInfo.queueFamilyIndex = queue_family_index;
    
    // Create the command pool
    if(vkCreateCommandPool(device, &commandPoolCreateInfo, NULL, &command_pool) != VK_SUCCESS) 
      {
      printf("Failed to create Vulkan command pool. \n");
      exit(EXIT_FAILURE);
      }
    else  
      {
      printf("  Vulkan command pool successfully created. \n");
      }
    
    // Allocate command buffers
    VkCommandBufferAllocateInfo commandBufferAllocateInfo = {};
    commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    commandBufferAllocateInfo.commandPool = command_pool;
    commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    commandBufferAllocateInfo.commandBufferCount = 1;
    
    if(vkAllocateCommandBuffers(device, &commandBufferAllocateInfo, &command_buffer) != VK_SUCCESS) 
      {
      printf("Failed to create Vulkan command buffer. \n");
      exit(EXIT_FAILURE);
      }
    else  
      {
      printf("  Vulkan command buffer successfully created. \n");
      }
    
}

/*

// Fucntion to create a render pass for Vulkan
void create_render_pass()
{

    uint32_t format_count = 0;

    // identify the surface we will be rendering to.  this is the Vulkan - GTK binder.
    if(!gtk_vulkan_create_surface(win_main, instance, &surface)) 
      {
      printf("Failed to create Vulkan surface in GTK window. \n");
      exit(EXIT_FAILURE);
      }
    else  
      {
      printf("  Vulkan surface in GTK window successfully created. \n");
      }

    // identify the color formats available on this surface of this physical device
    vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &format_count, NULL);
    VkSurfaceFormatKHR surface_formats[format_count];
    vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &format_count, surface_formats);

    vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &format_count, &surface_format);
    VkFormat color_format = surface_format.format;
    

    VkAttachmentReference color_attachment = 
	{
	0, 
	VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
	};
    

    // Specify the color attachment
    VkAttachmentDescription colorAttachment = 
	{
	.flags = 0,
	.format = color_format,						// VK_FORMAT_B8G8R8A8_UNORM 
	.samples = VK_SAMPLE_COUNT_1_BIT,
	.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
	.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
	.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
	.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
	.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
	.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
	};
    
    // Specify the subpass
    VkSubpassDescription subpass = 
	{
	.flags = 0,
	.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
	.inputAttachmentCount = 0,
	.pInputAttachments = NULL,
	.colorAttachmentCount = 1,
	.pColorAttachments = &colorAttachmentRef,
	.pResolveAttachments = NULL,
	.pDepthStencilAttachment = NULL,
	.preserveAttachmentCount = 0,
	.pPreserveAttachments = NULL,
	};
    
    // Specify the dependency
    VkSubpassDependency dependency = 
	{
	.srcSubpass = VK_SUBPASS_EXTERNAL,
	.dstSubpass = 0,
	.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
	.srcAccessMask = 0,
	.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
	.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
	};
    
    // Create the render pass
    VkRenderPassCreateInfo createInfo = 
	{
	.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
	.pNext = NULL,
	.flags = 0,
	.attachmentCount = 1,
	.pAttachments = &colorAttachment,
	.subpassCount = 1,
	.pSubpasses = &subpass,
	.dependencyCount = 1,
	.pDependencies = &dependency,
	};
	
    VkResult result = vkCreateRenderPass(device, &createInfo, NULL, &render_pass);
    if(result != VK_SUCCESS)
      {
      printf("Failed to create Vulkan render pass. \n");
      exit(EXIT_FAILURE);
      }
    else  
      {
      printf("  Vulkan render pass successfully created. \n");
      }
}



// Function to record commands into the command buffer
void record_commands()
{
    // Begin recording commands into the command buffer
    VkCommandBufferBeginInfo beginInfo = 
	{
	.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
	.pNext = NULL,
	.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT,
	.pInheritanceInfo = NULL,
	};
    vkBeginCommandBuffer(command_buffer, &beginInfo);
    
    // Begin a render pass
    VkRenderPassBeginInfo renderPassInfo = 
	{
	.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
	.pNext = NULL,
	.renderPass = render_pass,
	.framebuffer = framebuffer,
	.renderArea = { .offset = { 0, 0 }, .extent = swapchainExtent },
	.clearValueCount = 1,
	.pClearValues = &clearValue,
	};
    vkCmdBeginRenderPass(command_buffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    
    // Bind the graphics pipeline
    vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);
    
    // Bind vertex buffers
    VkDeviceSize offsets[] = { 0 };
    vkCmdBindVertexBuffers(command_buffer, 0, 1, &vertexBuffer, offsets);
    
    // Draw the geometry
    vkCmdDraw(command_buffer, numVertices, 1, 0, 0);
    
    // End the render pass
    vkCmdEndRenderPass(command_buffer);
    
    // End recording commands into the command buffer
    vkEndCommandBuffer(command_buffer);

}
*/
