//
// Created by Tonz on 24.10.2025.
//

#include "raytracedRendererRestirDI.h"

#include <iostream>

#include "imgui/imgui.h"

bool RaytracedRendererRestirDI::drawGUI() {
    bool changed{false};

    if (ImGui::CollapsingHeader("ReSTIR DI renderer")) {
        ImGui::Indent();

        ImGui::Checkbox("Tonemap",reinterpret_cast<bool*>(&tonemap_));
        ImGui::Checkbox("Accumulate",reinterpret_cast<bool*>(&pcs_.accumulate));

        changed |= ImGui::DragInt("BRDF sample count",reinterpret_cast<int*>(&pcsUnpacked_.M_brdf),0.25,0,PcsRaygen::maxSampleCount);
        changed |= ImGui::DragInt("Area sample count",reinterpret_cast<int*>(&pcsUnpacked_.M_area),0.25,0,PcsRaygen::maxSampleCount);
        changed |= ImGui::DragInt("Env sample count",reinterpret_cast<int*>(&pcsUnpacked_.M_env),0.25,0,PcsRaygen::maxSampleCount);

        changed |= ImGui::Checkbox("Spatial reuse",reinterpret_cast<bool*>(&pcsUnpacked_.doSpatialReuse));

        if (pcsUnpacked_.doSpatialReuse) {
            ImGui::Indent();
            changed |= ImGui::DragInt("Neighbor count",reinterpret_cast<int*>(&pcsUnpacked_.M_neighbor),0.25,0,31);
            changed |= ImGui::DragFloat("Search radius",&pcs_.neighborSearchRadius,0.25,1,500);

            ImGui::Unindent();
        }

        ImGui::Unindent();
    }

    return changed;
}


void RaytracedRendererRestirDI::recordInitialPassCommands(const Scene& scene, vk::raii::CommandBuffer& cmdBuf, uint32_t frameInFlightIndex, const glm::vec<2,uint32_t>& renderDims) {
    pcsUnpacked_.bufferIndices = !pcsUnpacked_.bufferIndices;
    PcsRaygen::packData(pcsUnpacked_,pcs_);

    pcs_.seed = distr_(generator_);

    cmdBuf.pushConstants(rtPipeline_.getPipelineLayout(), PcsRaygen::stageFlags,0, vk::ArrayProxy<const PcsRaygen::Data>{pcs_});
    cmdBuf.traceRaysKHR(rtPipeline_.getRaygenRegion(),rtPipeline_.getMissRegion(),rtPipeline_.getHitRegion(),{},renderDims.x,renderDims.y,1);
}

void RaytracedRendererRestirDI::recordSpatialPassCommands(const Scene& scene, vk::raii::CommandBuffer& cmdBuf, uint32_t frameInFlightIndex, const glm::vec<2,uint32_t>& renderDims) {
    pcsUnpacked_.bufferIndices = !pcsUnpacked_.bufferIndices;
    PcsRaygen::packData(pcsUnpacked_,pcs_);

    pcs_.seed = distr_(generator_);

    cmdBuf.pushConstants(rtPipeline_.getPipelineLayout(), PcsRaygen::stageFlags,0, vk::ArrayProxy<const PcsRaygen::Data>{pcs_});

    vk::StridedDeviceAddressRegionKHR raygenRegion = rtPipeline_.getRaygenRegion();
    auto spatialRaygenOffset =  static_cast<vk::StridedDeviceAddressRegionKHR>(raygenRegion.deviceAddress + raygenRegion.stride);

    cmdBuf.traceRaysKHR(spatialRaygenOffset,rtPipeline_.getMissRegion(),rtPipeline_.getHitRegion(),{},renderDims.x,renderDims.y,1);
}
void RaytracedRendererRestirDI::recordFinalShadePassCommands(const Scene& scene, vk::raii::CommandBuffer& cmdBuf, uint32_t frameInFlightIndex, const glm::vec<2,uint32_t>& renderDims) {
    pcsUnpacked_.bufferIndices = !pcsUnpacked_.bufferIndices;
    PcsRaygen::packData(pcsUnpacked_,pcs_);

    pcs_.seed = distr_(generator_);

    cmdBuf.pushConstants(rtPipeline_.getPipelineLayout(), PcsRaygen::stageFlags,0, vk::ArrayProxy<const PcsRaygen::Data>{pcs_});

    vk::StridedDeviceAddressRegionKHR raygenRegion = rtPipeline_.getRaygenRegion();
    auto spatialRaygenOffset =  static_cast<vk::StridedDeviceAddressRegionKHR>(raygenRegion.deviceAddress + raygenRegion.stride * 2);

    cmdBuf.traceRaysKHR(spatialRaygenOffset,rtPipeline_.getMissRegion(),rtPipeline_.getHitRegion(),{},renderDims.x,renderDims.y, 1);
}

void RaytracedRendererRestirDI::recordTraceCommands(const Scene& scene, vk::raii::CommandBuffer& cmdBuf, uint32_t frameInFlightIndex) {

    auto renderDims = getRenderDimensions();

    // bind the ray tracing pipeline
    cmdBuf.bindPipeline(vk::PipelineBindPoint::eRayTracingKHR,rtPipeline_.getGraphicsPipeline());
    cmdBuf.bindDescriptorSets(vk::PipelineBindPoint::eRayTracingKHR, rtPipeline_.getPipelineLayout(), 0, *getDescSetFrame(frameInFlightIndex), nullptr);


    vk::MemoryBarrier2 barrierToInitial{
        .srcStageMask =  vk::PipelineStageFlagBits2::eRayTracingShaderKHR,
        .srcAccessMask = vk::AccessFlagBits2::eShaderStorageWrite | vk::AccessFlagBits2::eShaderStorageRead,
        .dstStageMask =  vk::PipelineStageFlagBits2::eRayTracingShaderKHR,
        .dstAccessMask = vk::AccessFlagBits2::eShaderStorageWrite | vk::AccessFlagBits2::eShaderStorageRead,
    };
    vk::DependencyInfo depToInitial{
        .memoryBarrierCount = 1,
        .pMemoryBarriers = &barrierToInitial
    };
    cmdBuf.pipelineBarrier2(depToInitial);

    // do initial pass
    recordInitialPassCommands(scene,cmdBuf,frameInFlightIndex,renderDims);

    // do spatial pass if checked
    if (pcsUnpacked_.doSpatialReuse) {
        vk::MemoryBarrier2 barrierInitialToSpatial{
            .srcStageMask =  vk::PipelineStageFlagBits2::eRayTracingShaderKHR,
            .srcAccessMask = vk::AccessFlagBits2::eShaderStorageWrite | vk::AccessFlagBits2::eShaderStorageRead,
            .dstStageMask =  vk::PipelineStageFlagBits2::eRayTracingShaderKHR,
            .dstAccessMask = vk::AccessFlagBits2::eShaderStorageWrite | vk::AccessFlagBits2::eShaderStorageRead,
        };
        vk::DependencyInfo depInitialToSpatial{
            .memoryBarrierCount = 1,
            .pMemoryBarriers = &barrierInitialToSpatial
        };
        cmdBuf.pipelineBarrier2(depInitialToSpatial);
        recordSpatialPassCommands(scene,cmdBuf,frameInFlightIndex,renderDims);
    }


    // do final shading pass
    vk::MemoryBarrier2 barrierSpatialToFinal{
        .srcStageMask =  vk::PipelineStageFlagBits2::eRayTracingShaderKHR,
        .srcAccessMask = vk::AccessFlagBits2::eShaderStorageWrite | vk::AccessFlagBits2::eShaderStorageRead,
        .dstStageMask =  vk::PipelineStageFlagBits2::eRayTracingShaderKHR,
        .dstAccessMask = vk::AccessFlagBits2::eShaderStorageWrite | vk::AccessFlagBits2::eShaderStorageRead,
    };
    vk::DependencyInfo depSpatialToFinal{
        .memoryBarrierCount = 1,
        .pMemoryBarriers = &barrierSpatialToFinal
    };
    cmdBuf.pipelineBarrier2(depSpatialToFinal);
    recordFinalShadePassCommands(scene,cmdBuf,frameInFlightIndex,renderDims);

    vk::MemoryBarrier2 barrierAfterFinal{
        .srcStageMask =  vk::PipelineStageFlagBits2::eRayTracingShaderKHR,
        .srcAccessMask = vk::AccessFlagBits2::eShaderStorageWrite | vk::AccessFlagBits2::eShaderStorageRead,
        .dstStageMask =  vk::PipelineStageFlagBits2::eRayTracingShaderKHR,
        .dstAccessMask = vk::AccessFlagBits2::eShaderStorageWrite | vk::AccessFlagBits2::eShaderStorageRead,
    };
    vk::DependencyInfo depFinal{
        .memoryBarrierCount = 1,
        .pMemoryBarriers = &barrierAfterFinal
    };
    cmdBuf.pipelineBarrier2(depFinal);

    pcs_.frameCtr += 1;
    if (!pcs_.accumulate) pcs_.frameCtr = 0;
}


void RaytracedRendererRestirDI::initReservoirBuffers() {
    for (int i = 0; i < reservoirSSBOs_.size(); ++i) {
        VkUtils::destroyBufferVMA(std::move(reservoirSSBOs_[i]));

        auto renderDims = getRenderDimensions();

        vk::DeviceSize bufferSize = sizeof(Reservoir) * renderDims.x * renderDims.y;
        reservoirSSBOs_[i] = VkUtils::createBufferVMA(bufferSize,vk::BufferUsageFlagBits::eStorageBuffer);
    }
    Renderer::setReservoirSSBOs(reservoirSSBOs_);
}
