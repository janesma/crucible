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

#pragma once

#include <stdbool.h>
#include <stdint.h>

#include <crucible/vk_wrapper.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct cru_format_info cru_format_info_t;

enum cru_num_type {
    CRU_NUM_TYPE_UNDEFINED = 0,
    CRU_NUM_TYPE_UNORM,
    CRU_NUM_TYPE_UINT,
    CRU_NUM_TYPE_SFLOAT,
};

struct cru_format_info {
    /// For example, "VK_FORMAT_R8_UNORM".
    const char *name;

    VkFormat format;
    enum cru_num_type num_type;
    uint8_t num_channels;
    uint8_t cpp;

    /// This is zero (VK_FORMAT_UNDEFINED) if and only if the format has no
    /// depth component.
    VkFormat depth_format;

    /// This is zero (VK_FORMAT_UNDEFINED) if and only if the format has no
    /// stencil component.
    VkFormat stencil_format;

    bool is_color:1;
    bool has_alpha:1;
};

/// \brief Lookup info for VkFormat.
///
/// If Crucible does not have info for the given format, then return NULL.
const struct cru_format_info *cru_format_get_info(VkFormat format);

#ifdef __cplusplus
}
#endif
