#pragma once
#include <VulkanTools.h>
#include "DescriptorSetLayout.h"

namespace engine
{
	namespace render
	{
		inline VkDescriptorType ToVkDescType(DescriptorType type) {
			switch (type) {
			case UNIFORM_BUFFER: return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			case IMAGE_SAMPLER: return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			case STORAGE_BUFFER: return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			case STORAGE_IMAGE: return VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
			case INPUT_ATTACHMENT: return VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
			default:return VK_DESCRIPTOR_TYPE_MAX_ENUM;
			}
		}

		inline VkShaderStageFlags ToVkShaderStage(ShaderStage stage) {
			switch (stage) {
			case VERTEX: return VK_SHADER_STAGE_VERTEX_BIT;
			case FRAGMENT: return VK_SHADER_STAGE_FRAGMENT_BIT;
			case COMPUTE: return VK_SHADER_STAGE_COMPUTE_BIT;
			default:return VK_SHADER_STAGE_FLAG_BITS_MAX_ENUM;
			}
		}

		struct VulkanDescriptorSetLayout : public DescriptorSetLayout
		{
			VkDevice _device = VK_NULL_HANDLE;

			VkDescriptorSetLayout m_descriptorSetLayout = VK_NULL_HANDLE;
			std::vector<VkDescriptorSetLayoutBinding> m_setLayoutBindings;

			void AddDescriptorSetLayoutBinding(VkDescriptorType type, VkShaderStageFlags stageFlags);

			void Create(VkDevice device, std::vector<std::pair<VkDescriptorType, VkShaderStageFlags>> layoutBindigs);
			void Create(VkDevice device);

			void Destroy();
			VulkanDescriptorSetLayout() : DescriptorSetLayout() {}
			VulkanDescriptorSetLayout(std::vector<LayoutBinding> bindings) : DescriptorSetLayout(bindings) {}
			~VulkanDescriptorSetLayout() { Destroy(); }
		};
	}
}