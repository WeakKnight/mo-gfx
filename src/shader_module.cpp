#include "shader_module.h"
#include <spdlog/spdlog.h>
#include "string_utils.h"
#include "device.h"

ShaderModule::ShaderModule(std::shared_ptr<Device> device, const std::string& path, shaderc_shader_kind kind)
{
	m_device = device;

    auto content = StringUtils::ReadFile(path);
    auto byteCodes = CompileFile(path, kind, content, true);

	VkShaderModuleCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = byteCodes.size() * sizeof(uint32_t);
	createInfo.pCode = byteCodes.data();

    auto result = vkCreateShaderModule(m_device->GetApiHandle(), &createInfo, nullptr, &m_apiHandle);
    assert(result == VK_SUCCESS);
}

ShaderModule::~ShaderModule()
{
    if (m_apiHandle != VK_NULL_HANDLE)
    {
        vkDestroyShaderModule(m_device->GetApiHandle(), m_apiHandle, nullptr);
    }
}

std::vector<uint32_t> ShaderModule::CompileFile(const std::string& sourceName,
    shaderc_shader_kind kind,
    const std::string& source,
    bool optimize) const
{
    shaderc::Compiler compiler;
    shaderc::CompileOptions options;

    // Like -DMY_DEFINE=1
    options.AddMacroDefinition("MY_DEFINE", "1");
    if (optimize)
        options.SetOptimizationLevel(shaderc_optimization_level_size);

    shaderc::SpvCompilationResult module =
        compiler.CompileGlslToSpv(source, kind, sourceName.c_str(), options);

    if (module.GetCompilationStatus() != shaderc_compilation_status_success)
    {
        spdlog::error(module.GetErrorMessage());
        return std::vector<uint32_t>();
    }

    return { module.cbegin(), module.cend() };
}