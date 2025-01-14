// Copyright 2021 The Dawn Authors
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

#ifndef SRC_DAWN_TESTS_END2END_VIDEOVIEWSTESTS_H_
#define SRC_DAWN_TESTS_END2END_VIDEOVIEWSTESTS_H_

#include <array>
#include <memory>
#include <vector>

#include "dawn/tests/DawnTest.h"

namespace dawn {

using Format = wgpu::TextureFormat;
DAWN_TEST_PARAM_STRUCT(Params, Format);

class VideoViewsTestBackend {
  public:
    static std::vector<BackendTestConfig> Backends();
    static std::vector<Format> Formats();
    static std::unique_ptr<VideoViewsTestBackend> Create();

    virtual ~VideoViewsTestBackend();

    virtual void OnSetUp(WGPUDevice device) = 0;
    virtual void OnTearDown() {}

    class PlatformTexture {
      public:
        PlatformTexture() = delete;
        virtual ~PlatformTexture();

        virtual bool CanWrapAsWGPUTexture() = 0;

      protected:
        explicit PlatformTexture(wgpu::Texture&& texture);

      public:
        wgpu::Texture wgpuTexture;
    };
    virtual std::unique_ptr<PlatformTexture> CreateVideoTextureForTest(wgpu::TextureFormat format,
                                                                       wgpu::TextureUsage usage,
                                                                       bool isCheckerboard,
                                                                       bool initialized) = 0;
    virtual void DestroyVideoTextureForTest(std::unique_ptr<PlatformTexture>&& platformTexture) = 0;
};

class VideoViewsTestsBase : public DawnTestWithParams<Params> {
  public:
    // The width and height in texels are 4 for all YUV formats.
    static constexpr uint32_t kYUVImageDataWidthInTexels = 4;
    static constexpr uint32_t kYUVImageDataHeightInTexels = 4;

    static constexpr size_t kYUVLumaPlaneIndex = 0;
    static constexpr size_t kYUVChromaPlaneIndex = 1;

    // RGB colors converted into YUV (per plane), for testing.
    // RGB colors are mapped to the BT.601 definition of luma.
    // https://docs.microsoft.com/en-us/windows/win32/medfound/about-yuv-video
    static constexpr std::array<dawn::utils::RGBA8, 2> kYellowYUVColor = {
        dawn::utils::RGBA8{210, 0, 0, 0xFF},    // Y
        dawn::utils::RGBA8{16, 146, 0, 0xFF}};  // UV

    static constexpr std::array<dawn::utils::RGBA8, 2> kWhiteYUVColor = {
        dawn::utils::RGBA8{235, 0, 0, 0xFF},     // Y
        dawn::utils::RGBA8{128, 128, 0, 0xFF}};  // UV

    static constexpr std::array<dawn::utils::RGBA8, 2> kBlueYUVColor = {
        dawn::utils::RGBA8{41, 0, 0, 0xFF},      // Y
        dawn::utils::RGBA8{240, 110, 0, 0xFF}};  // UV

    static constexpr std::array<dawn::utils::RGBA8, 2> kRedYUVColor = {
        dawn::utils::RGBA8{81, 0, 0, 0xFF},     // Y
        dawn::utils::RGBA8{90, 240, 0, 0xFF}};  // UV

    static constexpr dawn::utils::RGBA8 kTolerance{1, 1, 1, 0};

    template <typename T>
    static std::vector<T> GetTestTextureData(wgpu::TextureFormat format, bool isCheckerboard);
    template <typename T>
    static std::vector<T> GetTestTextureDataWithPlaneIndex(size_t planeIndex,
                                                           size_t bytesPerRow,
                                                           size_t height,
                                                           bool isCheckerboard);
    static uint32_t NumPlanes(wgpu::TextureFormat format);
    static std::array<Format, 2> PlaneFormats(Format textureFormat);

  protected:
    void SetUp() override;
    std::vector<wgpu::FeatureName> GetRequiredFeatures() override;
    bool IsMultiPlanarFormatsSupported() const;
    bool IsMultiPlanarFormatP010Supported() const;
    bool IsNorm16TextureFormatsSupported() const;
    wgpu::ShaderModule GetTestVertexShaderModule() const;
    wgpu::TextureFormat GetFormat() const;
    wgpu::TextureFormat GetPlaneFormat(int plane) const;
    bool IsFormatSupported() const;

    bool mIsMultiPlanarFormatsSupported = false;
    bool mIsMultiPlanarFormatP010Supported = false;
    bool mIsNorm16TextureFormatsSupported = false;
};

}  // namespace dawn

#endif  // SRC_DAWN_TESTS_END2END_VIDEOVIEWSTESTS_H_
