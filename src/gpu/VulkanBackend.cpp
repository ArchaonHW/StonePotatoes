#include "VulkanBackend.h"
#include "VulkanContext.h"

#ifdef SP_VULKAN

#include <vulkan/vulkan.h>

#include <algorithm>
#include <chrono>
#include <cstring>
#include <fstream>
#include <limits>
#include <utility>

namespace StonePotatoes {
namespace GPU {

namespace {

std::string VkErr(VkResult r) {
    return "VkResult=" + std::to_string(static_cast<int>(r));
}

} // namespace

struct VulkanBackend::Impl {
    std::shared_ptr<VulkanContext> context;
    // Non-owning aliases from context, kept local to minimize changes to the
    // compute resource implementation. VulkanContext owns these handles.
    VkPhysicalDevice phys = VK_NULL_HANDLE;
    VkDevice device = VK_NULL_HANDLE;
    uint32_t queueFamily = 0;
    VkQueue queue = VK_NULL_HANDLE;
    VkCommandPool cmdPool = VK_NULL_HANDLE;
    VkDescriptorPool descPool = VK_NULL_HANDLE;
    VkQueryPool queryPool = VK_NULL_HANDLE;
    float timestampPeriodNs = 0.0f;   // 0 → timestamp queries unsupported
    std::string lastError;

    struct Buffer {
        VkBuffer buf = VK_NULL_HANDLE;
        VkDeviceMemory mem = VK_NULL_HANDLE;
        uint64_t size = 0;
    };
    struct Kernel {
        VkShaderModule module = VK_NULL_HANDLE;
        VkDescriptorSetLayout dsl = VK_NULL_HANDLE;
        VkPipelineLayout layout = VK_NULL_HANDLE;
        VkPipeline pipeline = VK_NULL_HANDLE;
        uint32_t bufferCount = 0;
        uint32_t pcBytes = 0;
    };
    std::vector<Buffer> buffers;
    std::vector<Kernel> kernels;
    VkFence fence = VK_NULL_HANDLE;   // sync dispatch is serialized — one is enough
    VkCommandBuffer cb = VK_NULL_HANDLE;

    bool Fail(std::string msg) {
        lastError = std::move(msg);
        return false;
    }

    void Shutdown() {
        if (!context && device == VK_NULL_HANDLE) return;
        if (device) vkDeviceWaitIdle(device);
        for (auto& k : kernels) {
            if (k.pipeline) vkDestroyPipeline(device, k.pipeline, nullptr);
            if (k.layout) vkDestroyPipelineLayout(device, k.layout, nullptr);
            if (k.dsl) vkDestroyDescriptorSetLayout(device, k.dsl, nullptr);
            if (k.module) vkDestroyShaderModule(device, k.module, nullptr);
        }
        kernels.clear();
        for (auto& b : buffers) {
            if (b.buf) vkDestroyBuffer(device, b.buf, nullptr);
            if (b.mem) vkFreeMemory(device, b.mem, nullptr);
        }
        buffers.clear();
        if (fence) vkDestroyFence(device, fence, nullptr);
        fence = VK_NULL_HANDLE;
        cb = VK_NULL_HANDLE;  // freed with the command pool | 隨命令池釋放
        if (queryPool) vkDestroyQueryPool(device, queryPool, nullptr);
        if (descPool) vkDestroyDescriptorPool(device, descPool, nullptr);
        if (cmdPool) vkDestroyCommandPool(device, cmdPool, nullptr);
        // The shared context owns the device and instance. Keep our reference
        // so this backend can be initialized again against the same context;
        // other graphics consumers may also retain their own references.
        phys = VK_NULL_HANDLE;
        device = VK_NULL_HANDLE;
        queueFamily = 0;
        queue = VK_NULL_HANDLE;
        cmdPool = VK_NULL_HANDLE;
        descPool = VK_NULL_HANDLE;
        queryPool = VK_NULL_HANDLE;
        timestampPeriodNs = 0.0f;
        fence = VK_NULL_HANDLE;
        cb = VK_NULL_HANDLE;
        lastError.clear();
    }
};

namespace {

// Create an instance and fill physical-device infos; instance destroyed by
// caller. 建立 instance 並回填實體裝置資訊；instance 由呼叫端銷毀。
bool ProbeDevices(std::vector<VkPhysicalDevice>& out,
                  std::vector<DeviceCaps>& caps, std::string* err) {
    VkApplicationInfo app{VK_STRUCTURE_TYPE_APPLICATION_INFO};
    app.pApplicationName = "StonePotatoes";
    app.apiVersion = VK_API_VERSION_1_1;
    VkInstanceCreateInfo ici{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
    ici.pApplicationInfo = &app;
    VkInstance inst = VK_NULL_HANDLE;
    if (vkCreateInstance(&ici, nullptr, &inst) != VK_SUCCESS) {
        if (err) *err = "vkCreateInstance failed (no loader?)";
        return false;
    }
    uint32_t n = 0;
    vkEnumeratePhysicalDevices(inst, &n, nullptr);
    out.resize(n);
    if (n) vkEnumeratePhysicalDevices(inst, &n, out.data());
    for (auto pd : out) {
        VkPhysicalDeviceProperties props;
        VkPhysicalDeviceFeatures feats;
        VkPhysicalDeviceMemoryProperties mem;
        vkGetPhysicalDeviceProperties(pd, &props);
        vkGetPhysicalDeviceFeatures(pd, &feats);
        vkGetPhysicalDeviceMemoryProperties(pd, &mem);
        uint32_t qn = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(pd, &qn, nullptr);
        std::vector<VkQueueFamilyProperties> qprops(qn);
        vkGetPhysicalDeviceQueueFamilyProperties(pd, &qn, qprops.data());
        bool compute = false;
        for (const auto& q : qprops)
            if (q.queueFlags & VK_QUEUE_COMPUTE_BIT) compute = true;
        if (!compute) continue;

        DeviceCaps c;
        c.type = props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU
                     ? DeviceType::DiscreteGPU
                     : DeviceType::IntegratedGPU;
        c.name = props.deviceName;
        c.computeUnits = props.limits.maxComputeWorkGroupCount[0] ==
                                 std::numeric_limits<uint32_t>::max()
                             ? 0
                             : 1;  // unitless here; SM count is vendor-specific
        c.memoryBytes = 0;
        for (uint32_t i = 0; i < mem.memoryHeapCount; ++i)
            if (mem.memoryHeaps[i].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT)
                c.memoryBytes += mem.memoryHeaps[i].size;
        if (!c.memoryBytes)
            for (uint32_t i = 0; i < mem.memoryHeapCount; ++i)
                c.memoryBytes += mem.memoryHeaps[i].size;
        c.supportsFloat64 = feats.shaderFloat64 == VK_TRUE;
        c.supportsAtomics = true;
        caps.push_back(std::move(c));
    }
    vkDestroyInstance(inst, nullptr);
    return !caps.empty();
}

} // namespace

bool VulkanBackend::IsAvailable() {
    std::vector<VkPhysicalDevice> devs;
    std::vector<DeviceCaps> caps;
    return ProbeDevices(devs, caps, nullptr);
}

std::vector<DeviceCaps> VulkanBackend::EnumerateGPUs() {
    std::vector<VkPhysicalDevice> devs;
    std::vector<DeviceCaps> caps;
    ProbeDevices(devs, caps, nullptr);
    return caps;
}

VulkanBackend::VulkanBackend(std::shared_ptr<VulkanContext> context)
    : m_Impl(std::make_unique<Impl>()) {
    m_Impl->context = std::move(context);
}
VulkanBackend::~VulkanBackend() { Shutdown(); }
bool VulkanBackend::IsInitialized() const {
    return m_Impl->device != VK_NULL_HANDLE && m_Impl->context &&
           m_Impl->context->IsInitialized();
}
std::shared_ptr<VulkanContext> VulkanBackend::GetContext() const {
    return m_Impl->context;
}
const DeviceCaps& VulkanBackend::GetCaps() const {
    static const DeviceCaps empty;
    return m_Impl->context ? m_Impl->context->GetCaps() : empty;
}
const std::string& VulkanBackend::LastError() const { return m_Impl->lastError; }
void VulkanBackend::Shutdown() { m_Impl->Shutdown(); }

bool VulkanBackend::Initialize(uint32_t deviceIndex) {
    Impl& im = *m_Impl;
    if (im.device) return true;
    if (!im.context) im.context = std::make_shared<VulkanContext>();
    if (!im.context->Initialize(deviceIndex))
        return im.Fail(im.context->LastError());
    im.phys = im.context->GetPhysicalDevice();
    im.device = im.context->GetDevice();
    im.queueFamily = im.context->GetComputeQueueFamily();
    im.queue = im.context->GetComputeQueue();

    VkCommandPoolCreateInfo cpci{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
    cpci.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    cpci.queueFamilyIndex = im.queueFamily;
    if (vkCreateCommandPool(im.device, &cpci, nullptr, &im.cmdPool) != VK_SUCCESS)
        return im.Fail("vkCreateCommandPool failed");

    VkDescriptorPoolSize sz{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 64};
    VkDescriptorPoolCreateInfo dpci{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
    dpci.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    dpci.maxSets = 32;
    dpci.poolSizeCount = 1;
    dpci.pPoolSizes = &sz;
    if (vkCreateDescriptorPool(im.device, &dpci, nullptr, &im.descPool) !=
        VK_SUCCESS)
        return im.Fail("vkCreateDescriptorPool failed");

    VkFenceCreateInfo fci{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
    if (vkCreateFence(im.device, &fci, nullptr, &im.fence) != VK_SUCCESS)
        return im.Fail("vkCreateFence failed");
    VkCommandBufferAllocateInfo cbai{
        VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    cbai.commandPool = im.cmdPool;
    cbai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cbai.commandBufferCount = 1;
    if (vkAllocateCommandBuffers(im.device, &cbai, &im.cb) != VK_SUCCESS)
        return im.Fail("vkAllocateCommandBuffers failed");

    // GPU timestamps for honest kernel timing (vendor-supported only).
    // GPU 時間戳以誠實量測核心耗時（僅供應商支援時）。
    VkPhysicalDeviceProperties props;
    vkGetPhysicalDeviceProperties(im.phys, &props);
    if (props.limits.timestampComputeAndGraphics &&
        im.context->GetTimestampValidBits() > 0) {
        im.timestampPeriodNs = props.limits.timestampPeriod;
        VkQueryPoolCreateInfo qpi{VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO};
        qpi.queryType = VK_QUERY_TYPE_TIMESTAMP;
        qpi.queryCount = 2;
        if (vkCreateQueryPool(im.device, &qpi, nullptr, &im.queryPool) !=
            VK_SUCCESS)
            im.timestampPeriodNs = 0.0f;
    }

    return true;
}

// --- Buffers | 緩衝 ---

VulkanBackend::BufferHandle VulkanBackend::CreateBuffer(uint64_t bytes) {
    Impl& im = *m_Impl;
    BufferHandle h;
    if (!im.device || bytes == 0) return h;

    VkBufferCreateInfo bci{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    bci.size = bytes;
    bci.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
                VK_BUFFER_USAGE_TRANSFER_SRC_BIT |
                VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    bci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    VkBuffer buf = VK_NULL_HANDLE;
    if (vkCreateBuffer(im.device, &bci, nullptr, &buf) != VK_SUCCESS)
        return h;

    VkMemoryRequirements req;
    vkGetBufferMemoryRequirements(im.device, buf, &req);
    VkPhysicalDeviceMemoryProperties mem;
    vkGetPhysicalDeviceMemoryProperties(im.phys, &mem);
    const VkFlags want = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                         VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    uint32_t type = UINT32_MAX;
    for (uint32_t i = 0; i < mem.memoryTypeCount; ++i)
        if ((req.memoryTypeBits & (1u << i)) &&
            (mem.memoryTypes[i].propertyFlags & want) == want) {
            type = i;
            break;
        }
    if (type == UINT32_MAX) {
        vkDestroyBuffer(im.device, buf, nullptr);
        im.Fail("no host-visible coherent memory type");
        return h;
    }
    VkMemoryAllocateInfo mai{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
    mai.allocationSize = req.size;
    mai.memoryTypeIndex = type;
    VkDeviceMemory dm = VK_NULL_HANDLE;
    if (vkAllocateMemory(im.device, &mai, nullptr, &dm) != VK_SUCCESS) {
        vkDestroyBuffer(im.device, buf, nullptr);
        return h;
    }
    vkBindBufferMemory(im.device, buf, dm, 0);

    // Reuse a free slot or append. 重用空槽或追加。
    for (size_t i = 0; i < im.buffers.size(); ++i)
        if (!im.buffers[i].buf) {
            im.buffers[i] = {buf, dm, bytes};
            h.id = i;
            return h;
        }
    im.buffers.push_back({buf, dm, bytes});
    h.id = im.buffers.size() - 1;
    return h;
}

void VulkanBackend::DestroyBuffer(BufferHandle h) {
    Impl& im = *m_Impl;
    if (!im.device || h.id >= im.buffers.size()) return;
    auto& b = im.buffers[h.id];
    vkDeviceWaitIdle(im.device);
    if (b.buf) vkDestroyBuffer(im.device, b.buf, nullptr);
    if (b.mem) vkFreeMemory(im.device, b.mem, nullptr);
    b = Impl::Buffer{};
}

bool VulkanBackend::WriteBuffer(BufferHandle h, const void* data,
                                uint64_t bytes, uint64_t offset) {
    Impl& im = *m_Impl;
    if (!im.device || h.id >= im.buffers.size()) return false;
    const auto& b = im.buffers[h.id];
    if (!b.buf || offset + bytes > b.size) return false;
    void* p = nullptr;
    if (vkMapMemory(im.device, b.mem, offset, bytes, 0, &p) != VK_SUCCESS)
        return false;
    std::memcpy(p, data, bytes);
    vkUnmapMemory(im.device, b.mem);
    return true;
}

bool VulkanBackend::ReadBuffer(BufferHandle h, void* out, uint64_t bytes,
                               uint64_t offset) const {
    const Impl& im = *m_Impl;
    if (!im.device || h.id >= im.buffers.size()) return false;
    const auto& b = im.buffers[h.id];
    if (!b.buf || offset + bytes > b.size) return false;
    void* p = nullptr;
    if (vkMapMemory(im.device, b.mem, offset, bytes, 0, &p) != VK_SUCCESS)
        return false;
    std::memcpy(out, p, bytes);
    vkUnmapMemory(im.device, b.mem);
    return true;
}

// --- Kernels | 核心 ---

VulkanBackend::KernelHandle VulkanBackend::CreateKernel(
    const uint32_t* spirv, size_t spirvBytes, uint32_t bufferCount,
    uint32_t pushConstantBytes) {
    Impl& im = *m_Impl;
    KernelHandle h;
    if (!im.device || !spirv || spirvBytes == 0 || bufferCount == 0 ||
        bufferCount > 8 || pushConstantBytes > 128)
        return h;

    Impl::Kernel k;
    k.bufferCount = bufferCount;
    k.pcBytes = pushConstantBytes;

    VkShaderModuleCreateInfo smi{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
    smi.codeSize = spirvBytes;
    smi.pCode = spirv;
    if (vkCreateShaderModule(im.device, &smi, nullptr, &k.module) !=
        VK_SUCCESS) {
        im.Fail("vkCreateShaderModule failed (invalid SPIR-V?)");
        return h;
    }

    std::vector<VkDescriptorSetLayoutBinding> binds(bufferCount);
    for (uint32_t i = 0; i < bufferCount; ++i) {
        binds[i].binding = i;
        binds[i].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        binds[i].descriptorCount = 1;
        binds[i].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    }
    VkDescriptorSetLayoutCreateInfo dli{
        VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
    dli.bindingCount = bufferCount;
    dli.pBindings = binds.data();
    if (vkCreateDescriptorSetLayout(im.device, &dli, nullptr, &k.dsl) !=
        VK_SUCCESS) {
        vkDestroyShaderModule(im.device, k.module, nullptr);
        return h;
    }

    VkPushConstantRange pcr{VK_SHADER_STAGE_COMPUTE_BIT, 0,
                            pushConstantBytes};
    VkPipelineLayoutCreateInfo pli{
        VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
    pli.setLayoutCount = 1;
    pli.pSetLayouts = &k.dsl;
    pli.pushConstantRangeCount = pushConstantBytes ? 1 : 0;
    pli.pPushConstantRanges = pushConstantBytes ? &pcr : nullptr;
    if (vkCreatePipelineLayout(im.device, &pli, nullptr, &k.layout) !=
        VK_SUCCESS) {
        vkDestroyDescriptorSetLayout(im.device, k.dsl, nullptr);
        vkDestroyShaderModule(im.device, k.module, nullptr);
        return h;
    }

    VkComputePipelineCreateInfo cpi{
        VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO};
    cpi.stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    cpi.stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    cpi.stage.module = k.module;
    cpi.stage.pName = "main";
    cpi.layout = k.layout;
    if (vkCreateComputePipelines(im.device, VK_NULL_HANDLE, 1, &cpi, nullptr,
                               &k.pipeline) != VK_SUCCESS) {
        vkDestroyPipelineLayout(im.device, k.layout, nullptr);
        vkDestroyDescriptorSetLayout(im.device, k.dsl, nullptr);
        vkDestroyShaderModule(im.device, k.module, nullptr);
        im.Fail("vkCreateComputePipelines failed");
        return h;
    }

    for (size_t i = 0; i < im.kernels.size(); ++i)
        if (!im.kernels[i].pipeline) {
            im.kernels[i] = k;
            h.id = i;
            return h;
        }
    im.kernels.push_back(k);
    h.id = im.kernels.size() - 1;
    return h;
}

VulkanBackend::KernelHandle VulkanBackend::CreateKernelFromFile(
    const std::string& spvPath, uint32_t bufferCount,
    uint32_t pushConstantBytes) {
    std::ifstream f(spvPath, std::ios::binary | std::ios::ate);
    if (!f) {
        m_Impl->Fail("cannot open SPIR-V: " + spvPath);
        return {};
    }
    const std::streamoff n = f.tellg();
    f.seekg(0);
    std::vector<uint32_t> words(static_cast<size_t>(n) / 4);
    f.read(reinterpret_cast<char*>(words.data()), n);
    if (!f || n % 4 != 0) {
        m_Impl->Fail("bad SPIR-V file: " + spvPath);
        return {};
    }
    return CreateKernel(words.data(), static_cast<size_t>(n), bufferCount,
                        pushConstantBytes);
}

void VulkanBackend::DestroyKernel(KernelHandle h) {
    Impl& im = *m_Impl;
    if (!im.device || h.id >= im.kernels.size()) return;
    auto& k = im.kernels[h.id];
    vkDeviceWaitIdle(im.device);
    if (k.pipeline) vkDestroyPipeline(im.device, k.pipeline, nullptr);
    if (k.layout) vkDestroyPipelineLayout(im.device, k.layout, nullptr);
    if (k.dsl) vkDestroyDescriptorSetLayout(im.device, k.dsl, nullptr);
    if (k.module) vkDestroyShaderModule(im.device, k.module, nullptr);
    k = Impl::Kernel{};
}

// --- Dispatch | 派發 ---

VulkanBackend::DispatchResult VulkanBackend::Dispatch(
    KernelHandle kernel, const std::vector<BufferHandle>& buffers,
    uint32_t groupsX, uint32_t groupsY, uint32_t groupsZ,
    const void* pushConstants, uint32_t pushConstantBytes) {
    Impl& im = *m_Impl;
    DispatchResult r;
    if (!im.device || kernel.id >= im.kernels.size()) return r;
    const auto& k = im.kernels[kernel.id];
    if (!k.pipeline || buffers.size() != k.bufferCount) return r;
    for (const auto& bh : buffers)
        if (bh.id >= im.buffers.size() || !im.buffers[bh.id].buf) return r;
    if (pushConstantBytes > k.pcBytes) return r;

    VkDescriptorSetAllocateInfo dai{
        VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
    dai.descriptorPool = im.descPool;
    dai.descriptorSetCount = 1;
    dai.pSetLayouts = &k.dsl;
    VkDescriptorSet ds = VK_NULL_HANDLE;
    if (vkAllocateDescriptorSets(im.device, &dai, &ds) != VK_SUCCESS) return r;

    std::vector<VkDescriptorBufferInfo> bis(buffers.size());
    std::vector<VkWriteDescriptorSet> writes(buffers.size());
    for (size_t i = 0; i < buffers.size(); ++i) {
        bis[i] = {im.buffers[buffers[i].id].buf, 0,
                  im.buffers[buffers[i].id].size};
        writes[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[i].dstSet = ds;
        writes[i].dstBinding = static_cast<uint32_t>(i);
        writes[i].descriptorCount = 1;
        writes[i].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        writes[i].pBufferInfo = &bis[i];
    }
    vkUpdateDescriptorSets(im.device, static_cast<uint32_t>(writes.size()),
                           writes.data(), 0, nullptr);

    VkCommandBuffer cb = im.cb;
    vkResetCommandBuffer(cb, 0);
    VkCommandBufferBeginInfo bi{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    bi.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(cb, &bi);
    const bool useTs = im.timestampPeriodNs > 0.0f;
    if (useTs) {
        vkCmdResetQueryPool(cb, im.queryPool, 0, 2);
        vkCmdWriteTimestamp(cb, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                            im.queryPool, 0);
    }
    vkCmdBindPipeline(cb, VK_PIPELINE_BIND_POINT_COMPUTE, k.pipeline);
    vkCmdBindDescriptorSets(cb, VK_PIPELINE_BIND_POINT_COMPUTE, k.layout, 0, 1,
                            &ds, 0, nullptr);
    if (pushConstantBytes)
        vkCmdPushConstants(cb, k.layout, VK_SHADER_STAGE_COMPUTE_BIT, 0,
                           pushConstantBytes, pushConstants);
    vkCmdDispatch(cb, groupsX, groupsY, groupsZ);
    if (useTs)
        vkCmdWriteTimestamp(cb, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                            im.queryPool, 1);
    vkEndCommandBuffer(cb);

    VkSubmitInfo si{VK_STRUCTURE_TYPE_SUBMIT_INFO};
    si.commandBufferCount = 1;
    si.pCommandBuffers = &cb;
    vkResetFences(im.device, 1, &im.fence);
    const auto t0 = std::chrono::steady_clock::now();
    const VkResult sr = vkQueueSubmit(im.queue, 1, &si, im.fence);
    if (sr == VK_SUCCESS) {
        vkWaitForFences(im.device, 1, &im.fence, VK_TRUE, ~0ull);
        r.ok = true;
    }
    r.wallMs = std::chrono::duration<double, std::milli>(
                   std::chrono::steady_clock::now() - t0)
                   .count();

    if (r.ok && useTs) {
        uint64_t ts[2] = {0, 0};
        vkGetQueryPoolResults(im.device, im.queryPool, 0, 2, sizeof(ts), ts,
                              sizeof(uint64_t),
                              VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WAIT_BIT);
        r.gpuMs = static_cast<double>(ts[1] - ts[0]) *
                  static_cast<double>(im.timestampPeriodNs) / 1e6;
    } else {
        r.gpuMs = r.wallMs;
    }

    vkFreeDescriptorSets(im.device, im.descPool, 1, &ds);
    return r;
}

} // namespace GPU
} // namespace StonePotatoes

#else // !SP_VULKAN — stubs returning failure | 無 Vulkan：回傳失敗的存根

#include <vector>

namespace StonePotatoes {
namespace GPU {

struct VulkanBackend::Impl {};  // empty when built without Vulkan | 無 Vulkan 時為空

bool VulkanBackend::IsAvailable() { return false; }
std::vector<DeviceCaps> VulkanBackend::EnumerateGPUs() { return {}; }
VulkanBackend::VulkanBackend(std::shared_ptr<VulkanContext>)
    : m_Impl(std::make_unique<Impl>()) {}
VulkanBackend::~VulkanBackend() = default;
bool VulkanBackend::Initialize(uint32_t) { return false; }
void VulkanBackend::Shutdown() {}
bool VulkanBackend::IsInitialized() const { return false; }
std::shared_ptr<VulkanContext> VulkanBackend::GetContext() const { return {}; }
const DeviceCaps& VulkanBackend::GetCaps() const {
    static DeviceCaps c;
    return c;
}
const std::string& VulkanBackend::LastError() const {
    static std::string s = "built without SP_VULKAN";
    return s;
}
VulkanBackend::BufferHandle VulkanBackend::CreateBuffer(uint64_t) { return {}; }
void VulkanBackend::DestroyBuffer(BufferHandle) {}
bool VulkanBackend::WriteBuffer(BufferHandle, const void*, uint64_t, uint64_t) {
    return false;
}
bool VulkanBackend::ReadBuffer(BufferHandle, void*, uint64_t, uint64_t) const {
    return false;
}
VulkanBackend::KernelHandle VulkanBackend::CreateKernel(
    const uint32_t*, size_t, uint32_t, uint32_t) {
    return {};
}
VulkanBackend::KernelHandle VulkanBackend::CreateKernelFromFile(
    const std::string&, uint32_t, uint32_t) {
    return {};
}
void VulkanBackend::DestroyKernel(KernelHandle) {}
VulkanBackend::DispatchResult VulkanBackend::Dispatch(
    KernelHandle, const std::vector<BufferHandle>&, uint32_t, uint32_t,
    uint32_t, const void*, uint32_t) {
    return {};
}

} // namespace GPU
} // namespace StonePotatoes

#endif // SP_VULKAN
