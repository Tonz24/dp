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
void RaytracedRendererRestirDI::recordFinalShadePassCommands(const Scene& scene, vk::raii::CommandBuffer& cmdBuf, uint32_t frameInFlightIndex, const glm::vec<2,uint32_t>& renderDims) {}

void RaytracedRendererRestirDI::recordTraceCommands(const Scene& scene, vk::raii::CommandBuffer& cmdBuf, uint32_t frameInFlightIndex) {

    std::cout << "////////////////////////RESTIR FRAME START///////////////////////" << std::endl;
    auto renderDims = getRenderDimensions();


    // bind the ray tracing pipeline
    cmdBuf.bindPipeline(vk::PipelineBindPoint::eRayTracingKHR,rtPipeline_.getGraphicsPipeline());
    cmdBuf.bindDescriptorSets(vk::PipelineBindPoint::eRayTracingKHR, rtPipeline_.getPipelineLayout(), 0, *getDescSetFrame(frameInFlightIndex), nullptr);

    // do initial pass
    recordInitialPassCommands(scene,cmdBuf,frameInFlightIndex,renderDims);



    pcs_.frameCtr += 1;
    if (!pcs_.accumulate) pcs_.frameCtr = 0;

    std::cout << "/////////////////////////RESTIR FRAME END////////////////////////" << std::endl;
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
