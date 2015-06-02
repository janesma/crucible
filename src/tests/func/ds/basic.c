// Copyright 2015 Intel Corporation
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice (including the next
// paragraph) shall be included in all copies or substantial portions of the
// Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.

#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdlib.h>
#include <string.h>

#include <poll.h>
#include <libpng16/png.h>

#include <crucible/cru.h>

static inline uint32_t
align_u32(uint32_t value, uint32_t alignment)
{
   return (value + alignment - 1) & ~(alignment - 1);
}

static VkPipeline
test_create_solid_color_pipeline(void)
{
    VkPipelineIaStateCreateInfo ia_create_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_IA_STATE_CREATE_INFO,
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP,
        .disableVertexReuse = false,
        .primitiveRestartEnable = false,
        .primitiveRestartIndex = 0
    };

    static const char vs_source[] =
        GLSL(330,
             layout(location = 0) in vec4 a_position;
             layout(location = 1) in vec4 a_color;
             out vec4 v_color;
             void main()
             {
                 gl_Position = a_position;
                 v_color = a_color;
             });

    static const char fs_source[] =
        GLSL(330,
             out vec4 f_color;
             in vec4 v_color;
             void main()
             {
                 f_color = v_color;
             });

    VkShader vs;
    vkCreateShader(t_device,
                   &(VkShaderCreateInfo) {
                       .sType = VK_STRUCTURE_TYPE_SHADER_CREATE_INFO,
                           .codeSize = sizeof(vs_source),
                           .pCode = vs_source,
                           .flags = 0,
                           },
                   &vs);

    VkShader fs;
    vkCreateShader(t_device,
                   &(VkShaderCreateInfo) {
                       .sType = VK_STRUCTURE_TYPE_SHADER_CREATE_INFO,
                           .codeSize = sizeof(fs_source),
                           .pCode = fs_source,
                           .flags = 0,
                           },
                   &fs);

    VkPipelineShaderStageCreateInfo vs_create_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .pNext = &ia_create_info,
        .shader = {
            .stage = VK_SHADER_STAGE_VERTEX,
            .shader = vs,
            .linkConstBufferCount = 0,
            .pLinkConstBufferInfo = NULL,
            .pSpecializationInfo = NULL
        }
    };

    VkPipelineShaderStageCreateInfo fs_create_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .pNext = &vs_create_info,
        .shader = {
            .stage = VK_SHADER_STAGE_FRAGMENT,
            .shader = fs,
            .linkConstBufferCount = 0,
            .pLinkConstBufferInfo = NULL,
            .pSpecializationInfo = NULL
        }
    };

    VkPipelineVertexInputCreateInfo vi_create_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_CREATE_INFO,
        .pNext = &fs_create_info,
        .bindingCount = 2,
        .pVertexBindingDescriptions = (VkVertexInputBindingDescription[]) {
            {
                .binding = 0,
                .strideInBytes = 16,
                .stepRate = VK_VERTEX_INPUT_STEP_RATE_VERTEX
            },
            {
                .binding = 1,
                .strideInBytes = 16,
                .stepRate = VK_VERTEX_INPUT_STEP_RATE_INSTANCE
            }
        },
        .attributeCount = 2,
        .pVertexAttributeDescriptions = (VkVertexInputAttributeDescription[]) {
            {
                .location = 0,
                .binding = 0,
                .format = VK_FORMAT_R32G32B32A32_SFLOAT,
                .offsetInBytes = 0
            },
            {
                .location = 1,
                .binding = 1,
                .format = VK_FORMAT_R32G32B32A32_SFLOAT,
                .offsetInBytes = 0
            }
        }
    };

    VkPipelineRsStateCreateInfo rs_create_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RS_STATE_CREATE_INFO,
        .pNext = &vi_create_info,

        .depthClipEnable = true,
        .rasterizerDiscardEnable = false,
        .fillMode = VK_FILL_MODE_SOLID,
        .cullMode = VK_CULL_MODE_NONE,
        .frontFace = VK_FRONT_FACE_CCW
    };

    VkPipelineDsStateCreateInfo ds_create_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DS_STATE_CREATE_INFO,
        .pNext = &rs_create_info,
        .format = VK_FORMAT_D24_UNORM,
        .depthTestEnable = true,
        .depthWriteEnable = true,
        .depthCompareOp = VK_COMPARE_OP_GREATER
    };

    VkPipelineCbStateCreateInfo cb_create_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_CB_STATE_CREATE_INFO,
        .pNext = &ds_create_info,
        .attachmentCount = 1,
        .pAttachments = (VkPipelineCbAttachmentState []) {
            { .channelWriteMask = VK_CHANNEL_A_BIT |
              VK_CHANNEL_R_BIT | VK_CHANNEL_G_BIT | VK_CHANNEL_B_BIT },
        }
    };

    VkPipeline pipeline;
    vkCreateGraphicsPipeline(t_device,
                             &(VkGraphicsPipelineCreateInfo) {
                                 .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
                                 .pNext = &cb_create_info,
                                 .flags = 0,
                                 .layout = VK_NULL_HANDLE
                             },
                             &pipeline);

    vkDestroyObject(t_device, VK_OBJECT_TYPE_SHADER, fs);
    vkDestroyObject(t_device, VK_OBJECT_TYPE_SHADER, vs);

    return pipeline;
}

static void
test(void)
{
    VkPipeline pipeline;
    VkImage ds;
    VkBuffer vertex_buffer;
    VkDeviceMemory mem;
    void *vertex_map;
    VkDynamicVpState vp_state;
    VkDynamicRsState rs_state;
    VkDynamicDsState ds_state;
    VkDynamicCbState cb_state;
    VkDepthStencilView ds_view;
    VkFramebuffer framebuffer;
    VkRenderPass pass;

    vkCreateBuffer(t_device,
                   &(VkBufferCreateInfo) {
                       .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
                       .size = 4096,
                       .usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                       .flags = 0
                   },
                   &vertex_buffer);

    vkCreateImage(t_device,
                  &(VkImageCreateInfo) {
                      .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
                      .imageType = VK_IMAGE_TYPE_2D,
                      .format = VK_FORMAT_D24_UNORM,
                      .extent = { .width = t_width, .height = t_height, .depth = 1 },
                      .mipLevels = 1,
                      .arraySize = 1,
                      .samples = 1,
                      .tiling = VK_IMAGE_TILING_OPTIMAL,
                      .usage = VK_IMAGE_USAGE_DEPTH_STENCIL_BIT,
                      .flags = 0,
                  },
                  &ds);

    VkMemoryRequirements vb_requirements;
    size_t size = sizeof(vb_requirements);
    vkGetObjectInfo(t_device, VK_OBJECT_TYPE_BUFFER, vertex_buffer,
                    VK_OBJECT_INFO_TYPE_MEMORY_REQUIREMENTS,
                    &size, &vb_requirements);

    VkMemoryRequirements ds_requirements;
    size = sizeof(ds_requirements);
    vkGetObjectInfo(t_device, VK_OBJECT_TYPE_IMAGE, ds,
                    VK_OBJECT_INFO_TYPE_MEMORY_REQUIREMENTS,
                    &size, &ds_requirements);

    size_t mem_size =
        align_u32(vb_requirements.size, 4096) + 
        align_u32(ds_requirements.size, 4096);

    printf("mem size %ld\n", mem_size);

    vkAllocMemory(t_device,
                  &(VkMemoryAllocInfo) {
                      .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOC_INFO,
                      .allocationSize = mem_size,
                      .memProps = VK_MEMORY_PROPERTY_HOST_DEVICE_COHERENT_BIT,
                      .memPriority = VK_MEMORY_PRIORITY_NORMAL
                  },
                  &mem);

    vkMapMemory(t_device, mem, 0, mem_size, 0, &vertex_map);
    memset(vertex_map, 0, mem_size);

    uint32_t offset = 0;
    vkQueueBindObjectMemory(t_queue, VK_OBJECT_TYPE_BUFFER,
                            vertex_buffer, 0, mem, offset);
    offset = align_u32(offset + vb_requirements.size, 4096);

    vkQueueBindObjectMemory(t_queue, VK_OBJECT_TYPE_IMAGE,
                            ds, 0, mem, offset);

    vkCreateDynamicViewportState(t_device,
        &(VkDynamicVpStateCreateInfo) {
            .sType = VK_STRUCTURE_TYPE_DYNAMIC_VP_STATE_CREATE_INFO,
            .viewportAndScissorCount = 1,
            .pViewports = (VkViewport[]) {
                {
                    .originX = 0,
                    .originY = 0,
                    .width = t_width,
                    .height = t_height,
                    .minDepth = -1,
                    .maxDepth = 1
                },
            },
            .pScissors = (VkRect[]) {
                { {  0,  0 }, { t_width, t_height } },
            }
        },
        &vp_state);

    vkCreateDynamicRasterState(t_device,
                               &(VkDynamicRsStateCreateInfo) {
                                   .sType = VK_STRUCTURE_TYPE_DYNAMIC_RS_STATE_CREATE_INFO,
                               },
                               &rs_state);

    vkCreateDynamicDepthStencilState(t_device,
                                     &(VkDynamicDsStateCreateInfo) {
                                         .sType = VK_STRUCTURE_TYPE_DYNAMIC_DS_STATE_CREATE_INFO,
                                     },
                                     &ds_state);

    vkCreateDynamicColorBlendState(t_device,
                                   &(VkDynamicCbStateCreateInfo) {
                                       .sType = VK_STRUCTURE_TYPE_DYNAMIC_CB_STATE_CREATE_INFO
                                   },
                                   &cb_state);

    vkCreateDepthStencilView(t_device,
                             &(VkDepthStencilViewCreateInfo) {
                                 .sType = VK_STRUCTURE_TYPE_DEPTH_STENCIL_VIEW_CREATE_INFO,
                                 .image = ds,
                                 .mipLevel = 0,
                                 .baseArraySlice = 0,
                                 .arraySize = 1,
                                 .msaaResolveImage = 0,
                                 .msaaResolveSubResource = { 0, }
                             },
                             &ds_view);
    vkCreateFramebuffer(t_device,
                        &(VkFramebufferCreateInfo) {
                            .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
                                .colorAttachmentCount = 1,
                                .pColorAttachments = (VkColorAttachmentBindInfo[]) {
                                {
                                    .view = t_image_color_view,
                                    .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
                                }
                            },
                            .pDepthStencilAttachment = &(VkDepthStencilBindInfo) {
                                .view = ds_view,
                                .layout = 0
                            },
                            .sampleCount = 1,
                            .width = t_width,
                            .height = t_height,
                            .layers = 1
                        },
                        &framebuffer);

    vkCreateRenderPass(t_device,
                       &(VkRenderPassCreateInfo) {
                           .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
                           .renderArea = { { 0, 0 }, { t_width, t_height } },
                           .colorAttachmentCount = 1,
                           .extent = { },
                           .sampleCount = 1,
                           .layers = 1,
                           .pColorFormats = (VkFormat[]) { VK_FORMAT_R8G8B8A8_UNORM },
                           .pColorLayouts = (VkImageLayout[]) { VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL },
                           .pColorLoadOps = (VkAttachmentLoadOp[]) { VK_ATTACHMENT_LOAD_OP_CLEAR },
                           .pColorStoreOps = (VkAttachmentStoreOp[]) { VK_ATTACHMENT_STORE_OP_STORE },
                           .pColorLoadClearValues = (VkClearColor[]) {
                               { .color = { .floatColor = { 0.2, 0.2, 0.2, 1.0 } }, .useRawValue = false }
                           },
                           .depthStencilFormat = VK_FORMAT_D24_UNORM,
                           .depthStencilLayout = 0,
                           .depthLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
                           .depthLoadClearValue = 0.5,
                           .depthStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
                       },
                       &pass);

    pipeline = test_create_solid_color_pipeline();

    static const float vertex_data[] = {
        /* First triangle coordinates */
        -0.7, -0.5, 0.5, 1.0,
        0.3, -0.5, 0.5, 1.0,
        -0.2,  0.5, 0.5, 1.0,

        /* Second triangle coordinates */
        -0.3, -0.3, 0.4, 1.0,
        0.7, -0.3, 0.4, 1.0,
        0.2,  0.7, 0.8, 1.0,

        /* First triangle color */
        1.0,  1.0, 0.2, 1.0,

        /* Second triangle color */
        0.2,  0.2, 1.0, 1.0,
    };
    memcpy(vertex_map, vertex_data, sizeof(vertex_data));

    vkCmdBeginRenderPass(t_cmd_buffer,
                         &(VkRenderPassBegin) {
                             .renderPass = pass,
                             .framebuffer = framebuffer
                         });

    vkCmdBindVertexBuffers(t_cmd_buffer, 0, 2,
                           (VkBuffer[]) { vertex_buffer, vertex_buffer },
                           (VkDeviceSize[]) { 0, 6 * 4 * sizeof(float) });

   vkCmdBindPipeline(t_cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

   vkCmdBindDynamicStateObject(t_cmd_buffer,
                               VK_STATE_BIND_POINT_VIEWPORT, vp_state);
   vkCmdBindDynamicStateObject(t_cmd_buffer,
                               VK_STATE_BIND_POINT_RASTER, rs_state);
   vkCmdBindDynamicStateObject(t_cmd_buffer,
                               VK_STATE_BIND_POINT_DEPTH_STENCIL, ds_state);
   vkCmdBindDynamicStateObject(t_cmd_buffer,
                               VK_STATE_BIND_POINT_COLOR_BLEND, cb_state);

   vkCmdDraw(t_cmd_buffer, 0, 3, 0, 1);
   vkCmdDraw(t_cmd_buffer, 3, 3, 1, 1);

   vkCmdEndRenderPass(t_cmd_buffer, pass);
}

cru_define_test {
    .name = "func.ds.basic",
    .start = test
};