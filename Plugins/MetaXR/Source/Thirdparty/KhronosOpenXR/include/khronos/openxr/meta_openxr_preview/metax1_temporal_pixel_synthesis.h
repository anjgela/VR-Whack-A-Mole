#ifndef METAX1_TEMPORAL_PIXEL_SYNTHESIS_H_
#define METAX1_TEMPORAL_PIXEL_SYNTHESIS_H_ 1

/**********************
This file is @generated from the OpenXR XML API registry.
Language    :   C99
Copyright   :   (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.
***********************/

#include <openxr/openxr.h>

#ifdef __cplusplus
extern "C" {
#endif


#ifndef XR_METAX1_temporal_pixel_synthesis

// XR_METAX1_temporal_pixel_synthesis is a preprocessor guard. Do not pass it to API calls.
#define XR_METAX1_temporal_pixel_synthesis 1
#define XR_METAX1_temporal_pixel_synthesis_SPEC_VERSION 1
#define XR_METAX1_TEMPORAL_PIXEL_SYNTHESIS_EXTENSION_NAME "XR_METAX1_temporal_pixel_synthesis"
static const XrStructureType XR_TYPE_PIXEL_SYNTHESIS_INFO_METAX1 = (XrStructureType) 1000668000;
static const XrStructureType XR_TYPE_SYSTEM_PIXEL_SYNTHESIS_PROPERTIES_METAX1 = (XrStructureType) 1000668001;
static const XrStructureType XR_TYPE_VIEW_PIXEL_SYNTHESIS_CONFIGURATION_VIEW_METAX1 = (XrStructureType) 1000668002;

typedef enum XrPixelSynthesisQualityLevelMETAX1 {
    // Disable pixel synthesis processing
    XR_PIXEL_SYNTHESIS_QUALITY_LEVEL_DISABLED_METAX1 = 0,
    // Low quality pixel synthesis for best performance
    XR_PIXEL_SYNTHESIS_QUALITY_LEVEL_LOW_METAX1 = 1,
    // Medium quality pixel synthesis for balanced performance and quality
    XR_PIXEL_SYNTHESIS_QUALITY_LEVEL_MEDIUM_METAX1 = 2,
    // High quality pixel synthesis for best quality
    XR_PIXEL_SYNTHESIS_QUALITY_LEVEL_HIGH_METAX1 = 3,
    // Dynamic quality pixel synthesis based on available performance overhead
    XR_PIXEL_SYNTHESIS_QUALITY_LEVEL_DYNAMIC_METAX1 = 4,
    XR_PIXEL_SYNTHESIS_QUALITY_LEVEL_METAX1_MAX_ENUM = 0x7FFFFFFF
} XrPixelSynthesisQualityLevelMETAX1;

typedef enum XrPixelSynthesisFoveaRadiusMETAX1 {
    // Small radius fovea region for best performance
    XR_PIXEL_SYNTHESIS_FOVEA_RADIUS_SMALL_METAX1 = 1,
    // Medium radius fovea region for balanced performance and quality
    XR_PIXEL_SYNTHESIS_FOVEA_RADIUS_MEDIUM_METAX1 = 2,
    // Large radius fovea region for best quality
    XR_PIXEL_SYNTHESIS_FOVEA_RADIUS_LARGE_METAX1 = 3,
    // Dynamic fovea radius based on available performance overhead
    XR_PIXEL_SYNTHESIS_FOVEA_RADIUS_DYNAMIC_METAX1 = 4,
    XR_PIXEL_SYNTHESIS_FOVEA_RADIUS_METAX1_MAX_ENUM = 0x7FFFFFFF
} XrPixelSynthesisFoveaRadiusMETAX1;
typedef XrFlags64 XrPixelSynthesisFlagsMETAX1;

// Flag bits for XrPixelSynthesisFlagsMETAX1
// Use depth when computing pixel synthesis for increased quality (slower)
static const XrPixelSynthesisFlagsMETAX1 XR_PIXEL_SYNTHESIS_USE_DEPTH_BIT_METAX1 = 0x00000001;
// Use stencil to mask portions of the screen where pixel synthesis should not be applied
static const XrPixelSynthesisFlagsMETAX1 XR_PIXEL_SYNTHESIS_USE_STENCIL_BIT_METAX1 = 0x00000002;
// Depth data is stored in the blue channel of the depth buffer instead of the red channel
static const XrPixelSynthesisFlagsMETAX1 XR_PIXEL_SYNTHESIS_DEPTH_IN_BLUE_BIT_METAX1 = 0x00000004;
// Whether pixel synthesis should use eye tracked fovea position
static const XrPixelSynthesisFlagsMETAX1 XR_PIXEL_SYNTHESIS_USE_EYE_TRACKED_FOVEATION_BIT_METAX1 = 0x00000008;
// Whether pixel synthesis should enable upscaling. The quality of upscaling can be specified using the upscalingQuality property
static const XrPixelSynthesisFlagsMETAX1 XR_PIXEL_SYNTHESIS_ENABLE_UPSCALING_BIT_METAX1 = 0x00000010;

typedef XrFlags64 XrPixelSynthesisSupportedFeatureFlagsMETAX1;

// Flag bits for XrPixelSynthesisSupportedFeatureFlagsMETAX1
// Whether this runtime supports the use of depth when computing pixel synthesis
static const XrPixelSynthesisSupportedFeatureFlagsMETAX1 XR_PIXEL_SYNTHESIS_SUPPORTED_FEATURE_DEPTH_BIT_METAX1 = 0x00000001;
// Whether this runtime supports the use of stencil masks when computing pixel synthesis
static const XrPixelSynthesisSupportedFeatureFlagsMETAX1 XR_PIXEL_SYNTHESIS_SUPPORTED_FEATURE_STENCIL_BIT_METAX1 = 0x00000002;
// Whether this runtime supports reading depth from the blue channel of the depth buffer
static const XrPixelSynthesisSupportedFeatureFlagsMETAX1 XR_PIXEL_SYNTHESIS_SUPPORTED_FEATURE_DEPTH_IN_BLUE_BIT_METAX1 = 0x00000004;
// Whether this runtime supports eye tracked foveation for pixel synthesis
static const XrPixelSynthesisSupportedFeatureFlagsMETAX1 XR_PIXEL_SYNTHESIS_SUPPORTED_FEATURE_EYE_TRACKED_FOVEATION_BIT_METAX1 = 0x00000008;
// Whether this runtime supports upscaling when computing pixel synthesis
static const XrPixelSynthesisSupportedFeatureFlagsMETAX1 XR_PIXEL_SYNTHESIS_SUPPORTED_FEATURE_UPSCALING_BIT_METAX1 = 0x00000010;
// Whether this runtime supports the use of dynamic quality levels
static const XrPixelSynthesisSupportedFeatureFlagsMETAX1 XR_PIXEL_SYNTHESIS_SUPPORTED_FEATURE_DYNAMIC_QUALITY_BIT_METAX1 = 0x00000020;

typedef struct XrPixelSynthesisInfoMETAX1 {
    XrStructureType                       type;
    const void* XR_MAY_ALIAS              next;
    XrPixelSynthesisFlagsMETAX1           layerFlags;
    XrSwapchainSubImage                   motionVectorSubImage;
    XrFovf                                motionVectorFov;
    XrVector2f                            motionVectorScale;
    XrVector2f                            motionVectorOffset;
    XrPosef                               appSpaceDeltaPose;
    XrSwapchainSubImage                   depthSubImage;
    float                                 minDepth;
    float                                 maxDepth;
    float                                 nearZ;
    float                                 farZ;
    XrSwapchainSubImage                   stencilSubImage;
    uint32_t                              stencilBitMask;
    uint32_t                              stencilValue;
    XrPixelSynthesisQualityLevelMETAX1    foveaQuality;
    XrPixelSynthesisQualityLevelMETAX1    peripheryQuality;
    XrPixelSynthesisFoveaRadiusMETAX1     foveaRadius;
    XrPixelSynthesisQualityLevelMETAX1    upscalingQuality;
    XrBool32                              resetAggregations;
} XrPixelSynthesisInfoMETAX1;

typedef struct XrSystemPixelSynthesisPropertiesMETAX1 {
    XrStructureType                                type;
    void* XR_MAY_ALIAS                             next;
    XrBool32                                       supportsPixelSynthesis;
    XrPixelSynthesisSupportedFeatureFlagsMETAX1    supportedFeatureFlags;
} XrSystemPixelSynthesisPropertiesMETAX1;

typedef struct XrViewPixelSynthesisConfigurationViewMETAX1 {
    XrStructureType       type;
    void* XR_MAY_ALIAS    next;
    XrBool32              recommendedPixelSynthesis;
    uint32_t              recommendedMotionVectorImageRectWidth;
    uint32_t              recommendedMotionVectorImageRectHeight;
} XrViewPixelSynthesisConfigurationViewMETAX1;

#endif /* XR_METAX1_temporal_pixel_synthesis */

#ifdef __cplusplus
}
#endif

#endif
