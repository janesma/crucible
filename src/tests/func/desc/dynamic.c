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

static void
create_pipeline(VkDevice device, VkPipeline *pipeline,
                VkPipelineLayout pipeline_layout)
{
    VkPipelineIaStateCreateInfo ia_create_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_IA_STATE_CREATE_INFO,
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP,
        .disableVertexReuse = false,
        .primitiveRestartEnable = false,
        .primitiveRestartIndex = 0
    };

    static const char vs_source[] = GLSL(330,
        layout(location = 0) in vec4 a_position;
        layout(set = 0, binding = 0) uniform block1 {
            vec4 color;
            vec4 offset;
        } u1;
        flat out vec4 v_color;
        void main()
        {
            gl_Position = a_position + u1.offset;
            v_color = u1.color;
        }
    );

    static const char fs_source[] = GLSL(330,
        out vec4 f_color;
        flat in vec4 v_color;
        void main()
        {
            f_color = v_color;
        }
    );

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
            .pSpecializationInfo = NULL,
        },
    };

    VkPipelineShaderStageCreateInfo fs_create_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .pNext = &vs_create_info,
        .shader = {
            .stage = VK_SHADER_STAGE_FRAGMENT,
            .shader = fs,
            .linkConstBufferCount = 0,
            .pLinkConstBufferInfo = NULL,
            .pSpecializationInfo = NULL,
        }
    };

    VkPipelineVertexInputCreateInfo vi_create_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_CREATE_INFO,
        .pNext = &fs_create_info,
        .bindingCount = 1,
        .pVertexBindingDescriptions = (VkVertexInputBindingDescription[]) {
            {
                .binding = 0,
                .strideInBytes = 16,
                .stepRate = VK_VERTEX_INPUT_STEP_RATE_VERTEX,
            },
        },
        .attributeCount = 1,
        .pVertexAttributeDescriptions = (VkVertexInputAttributeDescription[]) {
            {
                .location = 0,
                .binding = 0,
                .format = VK_FORMAT_R32G32B32A32_SFLOAT,
                .offsetInBytes = 0,
            },
        },
    };

    VkPipelineRsStateCreateInfo rs_create_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RS_STATE_CREATE_INFO,
        .pNext = &vi_create_info,
        .depthClipEnable = true,
        .rasterizerDiscardEnable = false,
        .fillMode = VK_FILL_MODE_SOLID,
        .cullMode = VK_CULL_MODE_NONE,
        .frontFace = VK_FRONT_FACE_CCW,
    };

    VkPipelineDsStateCreateInfo ds_create_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DS_STATE_CREATE_INFO,
        .pNext = &rs_create_info,
        .format = VK_FORMAT_UNDEFINED,
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

    vkCreateGraphicsPipeline(t_device,
        &(VkGraphicsPipelineCreateInfo) {
            .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
            .pNext = &cb_create_info,
            .flags = 0,
            .layout = pipeline_layout
        },
        pipeline);
    t_cleanup_push_vk_object(t_device, VK_OBJECT_TYPE_PIPELINE,
                                    *pipeline);
}

#define HEX_COLOR(v)                            \
    {                                           \
        ((v) >> 16) / 255.0,                    \
        ((v & 0xff00) >> 8) / 255.0,            \
        ((v & 0xff)) / 255.0, 1.0               \
    }

static void
test(void)
{
    VkDescriptorSetLayout set_layout[1];
    vkCreateDescriptorSetLayout(t_device,
        &(VkDescriptorSetLayoutCreateInfo) {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
            .count = 1,
            .pBinding = (VkDescriptorSetLayoutBinding[]) {
                {
                    .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
                    .count = 1,
                    .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
                    .pImmutableSamplers = NULL,
                },
            },
        },
        &set_layout[0]);

    VkPipelineLayout pipeline_layout;
    vkCreatePipelineLayout(t_device,
        &(VkPipelineLayoutCreateInfo) {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
            .descriptorSetCount = 1,
            .pSetLayouts = set_layout,
        },
        &pipeline_layout);

    VkPipeline pipeline;
    create_pipeline(t_device, &pipeline, pipeline_layout);

    uint32_t set_count = 1;
    VkDescriptorSet set[1];
    vkAllocDescriptorSets(t_device, /*pool*/ 0,
                          VK_DESCRIPTOR_SET_USAGE_STATIC,
                          1, set_layout, set, &set_count);

    VkBuffer buffer;
    vkCreateBuffer(t_device,
        &(VkBufferCreateInfo) {
            .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            .size = 4096,
            .usage = VK_BUFFER_USAGE_GENERAL,
            .flags = 0
        },
        &buffer);
    t_cleanup_push_vk_object(t_device, VK_OBJECT_TYPE_BUFFER,
                             buffer);

    VkMemoryRequirements buffer_reqs;
    size_t buffer_reqs_size = sizeof(buffer_reqs);
    vkGetObjectInfo(t_device, VK_OBJECT_TYPE_BUFFER, buffer,
                    VK_OBJECT_INFO_TYPE_MEMORY_REQUIREMENTS,
                    &buffer_reqs_size, &buffer_reqs);

    VkDeviceMemory mem;
    vkAllocMemory(t_device,
        &(VkMemoryAllocInfo) {
            .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOC_INFO,
            .allocationSize = buffer_reqs.size,
            .memProps = VK_MEMORY_PROPERTY_HOST_DEVICE_COHERENT_BIT,
            .memPriority = VK_MEMORY_PRIORITY_NORMAL,
        },
        &mem);

    void *map;
    vkMapMemory(t_device, mem, 0, 4096, 0, &map);
    memset(map, 192, 4096);

    vkQueueBindObjectMemory(t_queue, VK_OBJECT_TYPE_BUFFER, buffer,
                            /*index*/ 0, mem, 0);

    static const float color[6][4] = {
        HEX_COLOR(0xfaff81),
        { -0.3, -0.3, 0.0, 0.0 },
        HEX_COLOR(0xffc53a),
        {  0.0,  0.0, 0.0, 0.0 },
        HEX_COLOR(0xe06d06),
        {  0.3,  0.3, 0.0, 0.0 },
    };
    memcpy(map, color, sizeof(color));
    VkBufferView buffer_view;
    vkCreateBufferView(t_device,
        &(VkBufferViewCreateInfo) {
            .sType = VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO,
            .buffer = buffer,
            .viewType = VK_BUFFER_VIEW_TYPE_RAW,
            .format = VK_FORMAT_R32G32B32A32_SFLOAT,
            .offset = 0,
            .range = sizeof(color)
        },
        &buffer_view);

    uint32_t vertex_offset = 1024;
    static const float vertex_data[] = {
        // Triangle coordinates
        -0.5, -0.5, 0.0, 1.0,
         0.5, -0.5, 0.0, 1.0,
        -0.5,  0.5, 0.0, 1.0,
         0.5,  0.5, 0.0, 1.0,
    };

    memcpy(map + vertex_offset, vertex_data, sizeof(vertex_data));

    VkDynamicVpState vp_state;
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
                    .minDepth = 0,
                    .maxDepth = 1
                },
            },
            .pScissors = (VkRect[]) {
                {{  0,  0 }, {t_width, t_height}},
            },
        },
        &vp_state);

    VkDynamicRsState rs_state;
    vkCreateDynamicRasterState(t_device,
        &(VkDynamicRsStateCreateInfo) {
            .sType = VK_STRUCTURE_TYPE_DYNAMIC_RS_STATE_CREATE_INFO,
        },
        &rs_state);

    VkDynamicCbState cb_state;
    vkCreateDynamicColorBlendState(t_device,
                                   &(VkDynamicCbStateCreateInfo) {
                                       .sType = VK_STRUCTURE_TYPE_DYNAMIC_CB_STATE_CREATE_INFO
                                   },
                                   &cb_state);

    vkUpdateDescriptors(t_device,
        set[0], 1,
        (const void * []) {
            &(VkUpdateBuffers) {
                .sType = VK_STRUCTURE_TYPE_UPDATE_BUFFERS,
                .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
                .arrayIndex = 0,
                .binding = 0,
                .count = 1,
                .pBufferViews = (VkBufferViewAttachInfo[]) {
                    {
                        .sType = VK_STRUCTURE_TYPE_BUFFER_VIEW_ATTACH_INFO,
                        .view = buffer_view,
                    },
                },
            },
        });

    VkRenderPass pass;
    vkCreateRenderPass(t_device,
        &(VkRenderPassCreateInfo) {
            .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
            .renderArea = {{0, 0}, {t_width, t_height}},
            .colorAttachmentCount = 1,
            .extent = { },
            .sampleCount = 1,
            .layers = 1,
            .pColorFormats = (VkFormat[]) { VK_FORMAT_R8G8B8A8_UNORM },
            .pColorLayouts = (VkImageLayout[]) { VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL },
            .pColorLoadOps = (VkAttachmentLoadOp[]) { VK_ATTACHMENT_LOAD_OP_CLEAR },
            .pColorStoreOps = (VkAttachmentStoreOp[]) { VK_ATTACHMENT_STORE_OP_STORE },
            .pColorLoadClearValues = (VkClearColor[]) {
                {
                    .color = { .floatColor = HEX_COLOR(0x161032) },
                    .useRawValue = false,
                },
            },
            .depthStencilFormat = VK_FORMAT_UNDEFINED,
        },
        &pass);

    vkCmdBeginRenderPass(t_cmd_buffer,
        &(VkRenderPassBegin) {
            .renderPass = pass,
            .framebuffer = t_framebuffer,
        });
    vkCmdBindVertexBuffers(t_cmd_buffer, 0, 2,
                           (VkBuffer[]) { buffer },
                           (VkDeviceSize[]) { vertex_offset });
    vkCmdBindPipeline(t_cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
    vkCmdBindDynamicStateObject(t_cmd_buffer,
                                VK_STATE_BIND_POINT_VIEWPORT, vp_state);
    vkCmdBindDynamicStateObject(t_cmd_buffer,
                                VK_STATE_BIND_POINT_RASTER, rs_state);
    vkCmdBindDynamicStateObject(t_cmd_buffer,
                                VK_STATE_BIND_POINT_COLOR_BLEND, cb_state);

    uint32_t dynamic_offsets[1] = { 0 };
    vkCmdBindDescriptorSets(t_cmd_buffer,
                            VK_PIPELINE_BIND_POINT_GRAPHICS, 0, 1,
                            &set[0], 1, dynamic_offsets);
    vkCmdDraw(t_cmd_buffer, 0, 4, 0, 1);

    dynamic_offsets[0] = 32;
    vkCmdBindDescriptorSets(t_cmd_buffer,
                            VK_PIPELINE_BIND_POINT_GRAPHICS, 0, 1,
                            &set[0], 1, dynamic_offsets);
    vkCmdDraw(t_cmd_buffer, 0, 4, 0, 1);

    dynamic_offsets[0] = 64;
    vkCmdBindDescriptorSets(t_cmd_buffer,
                            VK_PIPELINE_BIND_POINT_GRAPHICS, 0, 1,
                            &set[0], 1, dynamic_offsets);
    vkCmdDraw(t_cmd_buffer, 0, 4, 0, 1);

    vkCmdEndRenderPass(t_cmd_buffer, pass);
}

cru_define_test {
    .name = "func.desc.dynamic",
    .start = test,
};