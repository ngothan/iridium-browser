// Copyright 2022 The Dawn Authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <CoreFoundation/CoreFoundation.h>
#include <CoreVideo/CVPixelBuffer.h>
#include <IOSurface/IOSurfaceRef.h>

#include <memory>
#include <utility>
#include <variant>
#include <vector>

#include "VideoViewsTests.h"
#include "dawn/common/Assert.h"
#include "dawn/common/CoreFoundationRef.h"
#include "dawn/native/MetalBackend.h"

namespace dawn {
namespace {

void AddIntegerValue(CFMutableDictionaryRef dictionary, const CFStringRef key, int32_t value) {
    CFNumberRef number(CFNumberCreate(nullptr, kCFNumberSInt32Type, &value));
    CFDictionaryAddValue(dictionary, key, number);
    CFRelease(number);
}

class PlatformTextureIOSurface : public VideoViewsTestBackend::PlatformTexture {
  public:
    PlatformTextureIOSurface(wgpu::Texture&& texture, IOSurfaceRef iosurface)
        : PlatformTexture(std::move(texture)) {
        mIOSurface = AcquireCFRef<IOSurfaceRef>(iosurface);
    }
    ~PlatformTextureIOSurface() override { mIOSurface = nullptr; }

    bool CanWrapAsWGPUTexture() override { return true; }

  private:
    CFRef<IOSurfaceRef> mIOSurface = nullptr;
};

class VideoViewsTestBackendIOSurface : public VideoViewsTestBackend {
  public:
    void OnSetUp(WGPUDevice device) override { mWGPUDevice = device; }

  private:
    OSType ToCVFormat(wgpu::TextureFormat format) {
        switch (format) {
            case wgpu::TextureFormat::R8BG8Biplanar420Unorm:
                return kCVPixelFormatType_420YpCbCr8BiPlanarVideoRange;
            case wgpu::TextureFormat::R10X6BG10X6Biplanar420Unorm:
                return kCVPixelFormatType_420YpCbCr10BiPlanarVideoRange;
            default:
                DAWN_UNREACHABLE();
                return 0;
        }
    }

    size_t GetSubSamplingFactorPerPlane(wgpu::TextureFormat format, size_t plane) {
        switch (format) {
            case wgpu::TextureFormat::R8BG8Biplanar420Unorm:
            case wgpu::TextureFormat::R10X6BG10X6Biplanar420Unorm:
                return plane == VideoViewsTestsBase::kYUVLumaPlaneIndex ? 1 : 2;
            default:
                DAWN_UNREACHABLE();
                return 0;
        }
    }

    size_t BytesPerElement(wgpu::TextureFormat format, size_t plane) {
        switch (format) {
            case wgpu::TextureFormat::R8BG8Biplanar420Unorm:
                return plane == VideoViewsTestsBase::kYUVLumaPlaneIndex ? 1 : 2;
            case wgpu::TextureFormat::R10X6BG10X6Biplanar420Unorm:
                return plane == VideoViewsTestsBase::kYUVLumaPlaneIndex ? 2 : 4;
            default:
                DAWN_UNREACHABLE();
                return 0;
        }
    }

    std::unique_ptr<VideoViewsTestBackend::PlatformTexture> CreateVideoTextureForTest(
        wgpu::TextureFormat format,
        wgpu::TextureUsage usage,
        bool isCheckerboard,
        bool initialized) override {
        CFMutableDictionaryRef dict(CFDictionaryCreateMutable(kCFAllocatorDefault, 0,
                                                              &kCFTypeDictionaryKeyCallBacks,
                                                              &kCFTypeDictionaryValueCallBacks));
        AddIntegerValue(dict, kIOSurfaceWidth, VideoViewsTestsBase::kYUVImageDataWidthInTexels);
        AddIntegerValue(dict, kIOSurfaceHeight, VideoViewsTestsBase::kYUVImageDataHeightInTexels);
        AddIntegerValue(dict, kIOSurfacePixelFormat, ToCVFormat(format));

        size_t num_planes = VideoViewsTestsBase::NumPlanes(format);

        CFMutableArrayRef planes(
            CFArrayCreateMutable(kCFAllocatorDefault, num_planes, &kCFTypeArrayCallBacks));
        size_t total_bytes_alloc = 0;
        for (size_t plane = 0; plane < num_planes; ++plane) {
            const size_t factor = GetSubSamplingFactorPerPlane(format, plane);
            const size_t plane_width = VideoViewsTestsBase::kYUVImageDataWidthInTexels / factor;
            const size_t plane_height = VideoViewsTestsBase::kYUVImageDataHeightInTexels / factor;
            const size_t plane_bytes_per_element = BytesPerElement(format, plane);
            const size_t plane_bytes_per_row = IOSurfaceAlignProperty(
                kIOSurfacePlaneBytesPerRow, plane_width * plane_bytes_per_element);
            const size_t plane_bytes_alloc =
                IOSurfaceAlignProperty(kIOSurfacePlaneSize, plane_height * plane_bytes_per_row);
            const size_t plane_offset =
                IOSurfaceAlignProperty(kIOSurfacePlaneOffset, total_bytes_alloc);

            CFMutableDictionaryRef plane_info(
                CFDictionaryCreateMutable(kCFAllocatorDefault, 0, &kCFTypeDictionaryKeyCallBacks,
                                          &kCFTypeDictionaryValueCallBacks));

            AddIntegerValue(plane_info, kIOSurfacePlaneWidth, plane_width);
            AddIntegerValue(plane_info, kIOSurfacePlaneHeight, plane_height);
            AddIntegerValue(plane_info, kIOSurfacePlaneBytesPerElement, plane_bytes_per_element);
            AddIntegerValue(plane_info, kIOSurfacePlaneBytesPerRow, plane_bytes_per_row);
            AddIntegerValue(plane_info, kIOSurfacePlaneSize, plane_bytes_alloc);
            AddIntegerValue(plane_info, kIOSurfacePlaneOffset, plane_offset);
            CFArrayAppendValue(planes, plane_info);
            CFRelease(plane_info);
            total_bytes_alloc = plane_offset + plane_bytes_alloc;
        }
        CFDictionaryAddValue(dict, kIOSurfacePlaneInfo, planes);
        CFRelease(planes);

        total_bytes_alloc = IOSurfaceAlignProperty(kIOSurfaceAllocSize, total_bytes_alloc);
        AddIntegerValue(dict, kIOSurfaceAllocSize, total_bytes_alloc);

        IOSurfaceRef surface = IOSurfaceCreate(dict);
        CFRelease(dict);

        if (initialized) {
            IOSurfaceLock(surface, 0, nullptr);
            for (size_t plane = 0; plane < num_planes; ++plane) {
                void* pointer = IOSurfaceGetBaseAddressOfPlane(surface, plane);
                if (format == wgpu::TextureFormat::R10X6BG10X6Biplanar420Unorm) {
                    std::vector<uint16_t> data =
                        VideoViewsTestsBase::GetTestTextureDataWithPlaneIndex<uint16_t>(
                            plane, IOSurfaceGetBytesPerRowOfPlane(surface, plane) / 2,
                            IOSurfaceGetHeightOfPlane(surface, plane), isCheckerboard);
                    memcpy(pointer, data.data(), data.size() * 2);
                } else {
                    std::vector<uint8_t> data =
                        VideoViewsTestsBase::GetTestTextureDataWithPlaneIndex<uint8_t>(
                            plane, IOSurfaceGetBytesPerRowOfPlane(surface, plane),
                            IOSurfaceGetHeightOfPlane(surface, plane), isCheckerboard);
                    memcpy(pointer, data.data(), data.size());
                }
            }
            IOSurfaceUnlock(surface, 0, nullptr);
        }

        wgpu::TextureDescriptor textureDesc;
        textureDesc.format = format;
        textureDesc.dimension = wgpu::TextureDimension::e2D;
        textureDesc.usage = usage;
        textureDesc.size = {VideoViewsTestsBase::kYUVImageDataWidthInTexels,
                            VideoViewsTestsBase::kYUVImageDataHeightInTexels, 1};

        wgpu::DawnTextureInternalUsageDescriptor internalDesc;
        internalDesc.internalUsage = wgpu::TextureUsage::CopySrc;
        textureDesc.nextInChain = &internalDesc;

        native::metal::ExternalImageDescriptorIOSurface descriptor = {};
        descriptor.cTextureDescriptor =
            reinterpret_cast<const WGPUTextureDescriptor*>(&textureDesc);
        descriptor.isInitialized = initialized;
        descriptor.ioSurface = surface;

        return std::make_unique<PlatformTextureIOSurface>(
            wgpu::Texture::Acquire(native::metal::WrapIOSurface(mWGPUDevice, &descriptor)),
            surface);
    }

    void DestroyVideoTextureForTest(
        std::unique_ptr<VideoViewsTestBackend::PlatformTexture>&& platformTexture) override {}

    WGPUDevice mWGPUDevice = nullptr;
};

}  // anonymous namespace

// static
std::vector<BackendTestConfig> VideoViewsTestBackend::Backends() {
    return {MetalBackend()};
}

// static
std::vector<Format> VideoViewsTestBackend::Formats() {
    return {wgpu::TextureFormat::R8BG8Biplanar420Unorm,
            wgpu::TextureFormat::R10X6BG10X6Biplanar420Unorm};
}

// static
std::unique_ptr<VideoViewsTestBackend> VideoViewsTestBackend::Create() {
    return std::make_unique<VideoViewsTestBackendIOSurface>();
}

}  // namespace dawn
