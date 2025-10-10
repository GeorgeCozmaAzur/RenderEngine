#include "VulkanDescriptorSet.h"
#include "VulkanCommandBuffer.h"
#include "VulkanPipeline.h"

namespace engine
{
	namespace render
	{
		VulkanDescriptorSet::~VulkanDescriptorSet()
		{
			assert(_device);
		}

		void VulkanDescriptorSet::SetupDescriptors(VkDevice device, VkDescriptorPool pool, VkDescriptorSetLayout layout, std::vector<VkDescriptorSetLayoutBinding> layoutBindings)
		{
			if (m_vkDescriptorSet != VK_NULL_HANDLE)
				return;

			_device = device;
			_descriptorPool = pool;
			_layout = layout;
			// Allocates an empty descriptor set without actual descriptors from the pool using the set layout
			VkDescriptorSetAllocateInfo allocateInfo{};
			allocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
			allocateInfo.descriptorPool = _descriptorPool;
			allocateInfo.descriptorSetCount = 1;
			allocateInfo.pSetLayouts = &layout;
			VK_CHECK_RESULT(vkAllocateDescriptorSets(_device, &allocateInfo, &m_vkDescriptorSet));

			std::vector <VkWriteDescriptorSet> writeDescriptorSets;
			writeDescriptorSets.resize(layoutBindings.size());
			uint32_t textures_iterator = 0;
			uint32_t buffers_iterator = 0;
			for (const auto &binding : layoutBindings)
			{
				writeDescriptorSets[binding.binding].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				writeDescriptorSets[binding.binding].dstSet = m_vkDescriptorSet;
				writeDescriptorSets[binding.binding].dstBinding = binding.binding;

				writeDescriptorSets[binding.binding].descriptorType = binding.descriptorType;

				if (binding.descriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER || binding.descriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC || binding.descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER)
				{
					writeDescriptorSets[binding.binding].pBufferInfo = _BuffersDescriptors[buffers_iterator++];
				}
				else
				if (binding.descriptorType == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER || binding.descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_IMAGE || binding.descriptorType == VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT)
				{
					writeDescriptorSets[binding.binding].pImageInfo = _TexturesDescriptors[textures_iterator++];
				}

				writeDescriptorSets[binding.binding].descriptorCount = 1;
			}
			// Update the descriptor set with the actual descriptors matching shader bindings set in the layout

			// Execute the writes to update descriptors for this set
			// Note that it's also possible to gather all writes and only run updates once, even for multiple sets
			// This is possible because each VkWriteDescriptorSet also contains the destination set to be updated
			// For simplicity we will update once per set instead
			vkUpdateDescriptorSets(_device, static_cast<uint32_t>(writeDescriptorSets.size()), writeDescriptorSets.data(), 0, nullptr);
		}

		void VulkanDescriptorSet::Update(uint32_t binding, VkDescriptorType descType, VkDescriptorBufferInfo *bufferInfo, VkDescriptorImageInfo *imageInfo)
		{
			VkWriteDescriptorSet writeDescriptorSet{};
			writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			writeDescriptorSet.dstSet = m_vkDescriptorSet;
			writeDescriptorSet.descriptorType = descType;
			writeDescriptorSet.dstBinding = binding;
			writeDescriptorSet.pBufferInfo = bufferInfo;
			writeDescriptorSet.pImageInfo = imageInfo;
			writeDescriptorSet.descriptorCount = 1;

			vkUpdateDescriptorSets(_device, 1, &writeDescriptorSet, 0, NULL);
		}

		void VulkanDescriptorSet::Draw(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, uint32_t indexInDynamicUniformBuffer, VkPipelineBindPoint pipelineBindpoint)
		{
			uint32_t dynamicOffset = indexInDynamicUniformBuffer * m_dynamicAlignment;
			uint32_t dynamicOffsetCount = m_dynamicAlignment > 0 ? 1 : 0;
			vkCmdBindDescriptorSets(commandBuffer, pipelineBindpoint, pipelineLayout, 0, 1, &m_vkDescriptorSet, dynamicOffsetCount, &dynamicOffset);
		}

		void VulkanDescriptorSet::Draw(class CommandBuffer* commandBuffer, Pipeline* pipeline, uint32_t indexInDynamicUniformBuffer)
		{
			VulkanCommandBuffer* cb = dynamic_cast<VulkanCommandBuffer*>(commandBuffer);
			VulkanPipeline* p = dynamic_cast<VulkanPipeline*>(pipeline);
			Draw(cb->m_vkCommandBuffer, p->getPipelineLayout(), indexInDynamicUniformBuffer);
		}
	}
}
