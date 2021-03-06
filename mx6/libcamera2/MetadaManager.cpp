/*
 * Copyright (C) 2012-2013 Freescale Semiconductor, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "MetadaManager.h"
#include "RequestManager.h"

MetadaManager::MetadaManager(SensorInfo *dev, int cameraId)
      : mCurrentRequest(NULL), mSensorInfo(dev), mCameraId(cameraId)
{
    mVpuSupportFmt[0] = HAL_PIXEL_FORMAT_YCbCr_420_SP;
    mVpuSupportFmt[1] = HAL_PIXEL_FORMAT_YCbCr_420_P;

    mPictureSupportFmt[0] = HAL_PIXEL_FORMAT_YCbCr_420_SP;
    mPictureSupportFmt[1] = HAL_PIXEL_FORMAT_YCbCr_422_I;
}

MetadaManager::~MetadaManager()
{
    if (mCurrentRequest != NULL) {
        free_camera_metadata(mCurrentRequest);
    }
}

status_t MetadaManager::getSupportedRecordingFormat(int *src, int len)
{
    if (src == NULL || len == 0) {
        return BAD_VALUE;
    }

    for (int i=0; i<MAX_VPU_SUPPORT_FORMAT && i<len; i++) {
        src[i] = mVpuSupportFmt[i];
    }
    return NO_ERROR;
}

status_t MetadaManager::getSupportedPictureFormat(int *src, int len)
{
    if (src == NULL || len == 0) {
        return BAD_VALUE;
    }

    for (int i=0; i<MAX_PICTURE_SUPPORT_FORMAT && i<len; i++) {
        src[i] = mPictureSupportFmt[i];
    }
    return NO_ERROR;
}

status_t  MetadaManager::addOrSize(camera_metadata_t *request,
        bool sizeRequest,
        size_t *entryCount,
        size_t *dataCount,
        uint32_t tag,
        const void *entryData,
        size_t entryDataCount) {
    status_t res;
    if (!sizeRequest) {
        return add_camera_metadata_entry(request, tag, entryData,
                entryDataCount);
    } else {
        int type = get_camera_metadata_tag_type(tag);
        if (type < 0 ) return BAD_VALUE;
        (*entryCount)++;
        (*dataCount) += calculate_camera_metadata_entry_data_size(type,
                entryDataCount);
        return OK;
    }
}

status_t MetadaManager::createDefaultRequest(
        int request_template,
        camera_metadata_t **request,
        bool sizeRequest) {

    size_t entryCount = 0;
    size_t dataCount = 0;
    status_t ret;

#define ADD_OR_SIZE( tag, data, count ) \
    if ( ( ret = addOrSize(*request, sizeRequest, &entryCount, &dataCount, \
            tag, data, count) ) != OK ) return ret

    static const int64_t USEC = 1000LL;
    static const int64_t MSEC = USEC * 1000LL;
    static const int64_t SEC = MSEC * 1000LL;

    /** android.request */

    static const uint8_t metadataMode = ANDROID_REQUEST_METADATA_MODE_NONE;
    ADD_OR_SIZE(ANDROID_REQUEST_METADATA_MODE, &metadataMode, 1);

    static const int32_t id = 0;
    ADD_OR_SIZE(ANDROID_REQUEST_ID, &id, 1);

    static const int32_t frameCount = 0;
    ADD_OR_SIZE(ANDROID_REQUEST_FRAME_COUNT, &frameCount, 1);

    // OUTPUT_STREAMS set by user
    entryCount += 1;
    dataCount += 5; // TODO: Should be maximum stream number

    /** android.lens */

    static const float focusDistance = 0;
    ADD_OR_SIZE(ANDROID_LENS_FOCUS_DISTANCE, &focusDistance, 1);

    static float aperture = 2.8;
    ADD_OR_SIZE(ANDROID_LENS_APERTURE, &aperture, 1);

    ADD_OR_SIZE(ANDROID_LENS_FOCAL_LENGTH, &mSensorInfo->mFocalLength, 1);

    static const float filterDensity = 0;
    ADD_OR_SIZE(ANDROID_LENS_FILTER_DENSITY, &filterDensity, 1);

    static const uint8_t opticalStabilizationMode =
            ANDROID_LENS_OPTICAL_STABILIZATION_MODE_OFF;
    ADD_OR_SIZE(ANDROID_LENS_OPTICAL_STABILIZATION_MODE,
            &opticalStabilizationMode, 1);


    /** android.sensor */


    static const int64_t frameDuration = 33333333L; // 1/30 s
    ADD_OR_SIZE(ANDROID_SENSOR_FRAME_DURATION, &frameDuration, 1);


    /** android.flash */

    static const uint8_t flashMode = ANDROID_FLASH_MODE_OFF;
    ADD_OR_SIZE(ANDROID_FLASH_MODE, &flashMode, 1);

    static const uint8_t flashPower = 10;
    ADD_OR_SIZE(ANDROID_FLASH_FIRING_POWER, &flashPower, 1);

    static const int64_t firingTime = 0;
    ADD_OR_SIZE(ANDROID_FLASH_FIRING_TIME, &firingTime, 1);

    /** Processing block modes */
    uint8_t hotPixelMode = 0;
    uint8_t demosaicMode = 0;
    uint8_t noiseMode = 0;
    uint8_t shadingMode = 0;
    uint8_t geometricMode = 0;
    uint8_t colorMode = 0;
    uint8_t tonemapMode = 0;
    uint8_t edgeMode = 0;
    uint8_t vstabMode = ANDROID_CONTROL_VIDEO_STABILIZATION_MODE_OFF;

    switch (request_template) {
      case CAMERA2_TEMPLATE_PREVIEW:
        break;
      case CAMERA2_TEMPLATE_STILL_CAPTURE:
        break;
      case CAMERA2_TEMPLATE_VIDEO_RECORD:
        vstabMode = ANDROID_CONTROL_VIDEO_STABILIZATION_MODE_ON;
        break;
      case CAMERA2_TEMPLATE_VIDEO_SNAPSHOT:
        vstabMode = ANDROID_CONTROL_VIDEO_STABILIZATION_MODE_ON;
        break;
      case CAMERA2_TEMPLATE_ZERO_SHUTTER_LAG:
        hotPixelMode = ANDROID_HOT_PIXEL_MODE_HIGH_QUALITY;
        demosaicMode = ANDROID_DEMOSAIC_MODE_HIGH_QUALITY;
        noiseMode = ANDROID_NOISE_REDUCTION_MODE_HIGH_QUALITY;
        shadingMode = ANDROID_SHADING_MODE_HIGH_QUALITY;
        geometricMode = ANDROID_GEOMETRIC_MODE_HIGH_QUALITY;
        colorMode = ANDROID_COLOR_CORRECTION_MODE_HIGH_QUALITY;
        tonemapMode = ANDROID_TONEMAP_MODE_HIGH_QUALITY;
        edgeMode = ANDROID_EDGE_MODE_HIGH_QUALITY;
        break;
      default:
        hotPixelMode = ANDROID_HOT_PIXEL_MODE_FAST;
        demosaicMode = ANDROID_DEMOSAIC_MODE_FAST;
        noiseMode = ANDROID_NOISE_REDUCTION_MODE_FAST;
        shadingMode = ANDROID_SHADING_MODE_FAST;
        geometricMode = ANDROID_GEOMETRIC_MODE_FAST;
        colorMode = ANDROID_COLOR_CORRECTION_MODE_FAST;
        tonemapMode = ANDROID_TONEMAP_MODE_FAST;
        edgeMode = ANDROID_EDGE_MODE_FAST;
        break;
    }
    ADD_OR_SIZE(ANDROID_HOT_PIXEL_MODE, &hotPixelMode, 1);
    ADD_OR_SIZE(ANDROID_DEMOSAIC_MODE, &demosaicMode, 1);
    ADD_OR_SIZE(ANDROID_NOISE_REDUCTION_MODE, &noiseMode, 1);
    ADD_OR_SIZE(ANDROID_SHADING_MODE, &shadingMode, 1);
    ADD_OR_SIZE(ANDROID_GEOMETRIC_MODE, &geometricMode, 1);
    ADD_OR_SIZE(ANDROID_COLOR_CORRECTION_MODE, &colorMode, 1);
    ADD_OR_SIZE(ANDROID_TONEMAP_MODE, &tonemapMode, 1);
    ADD_OR_SIZE(ANDROID_EDGE_MODE, &edgeMode, 1);
    ADD_OR_SIZE(ANDROID_CONTROL_VIDEO_STABILIZATION_MODE, &vstabMode, 1);

    /** android.noise */
    static const uint8_t noiseStrength = 5;
    ADD_OR_SIZE(ANDROID_NOISE_REDUCTION_STRENGTH, &noiseStrength, 1);

    /** android.color */
    static const float colorTransform[9] = {
        1.0f, 0.f, 0.f,
        0.f, 1.f, 0.f,
        0.f, 0.f, 1.f
    };
    ADD_OR_SIZE(ANDROID_COLOR_CORRECTION_TRANSFORM, colorTransform, 9);

    /** android.tonemap */
    static const float tonemapCurve[4] = {
        0.f, 0.f,
        1.f, 1.f
    };
    ADD_OR_SIZE(ANDROID_TONEMAP_CURVE_RED, tonemapCurve, 32); // sungjoong
    ADD_OR_SIZE(ANDROID_TONEMAP_CURVE_GREEN, tonemapCurve, 32);
    ADD_OR_SIZE(ANDROID_TONEMAP_CURVE_BLUE, tonemapCurve, 32);

    /** android.edge */
    static const uint8_t edgeStrength = 5;
    ADD_OR_SIZE(ANDROID_EDGE_STRENGTH, &edgeStrength, 1);

    /** android.scaler */
    int32_t cropRegion[3] = {
        0, 0, /*mSensorInfo->mMaxWidth*/
    };
    ADD_OR_SIZE(ANDROID_SCALER_CROP_REGION, cropRegion, 3);

    /** android.jpeg */
    //4.3 framework change quality type from i32 to u8
    static const uint8_t jpegQuality = 100;
    ADD_OR_SIZE(ANDROID_JPEG_QUALITY, &jpegQuality, 1);

    static const int32_t thumbnailSize[2] = {
        160, 120
    };
    ADD_OR_SIZE(ANDROID_JPEG_THUMBNAIL_SIZE, thumbnailSize, 2);

    //4.3 framework change quality type from i32 to u8
    static const uint8_t thumbnailQuality = 100;
    ADD_OR_SIZE(ANDROID_JPEG_THUMBNAIL_QUALITY, &thumbnailQuality, 1);

    static const double gpsCoordinates[3] = {
        0, 0, 0
    };
    ADD_OR_SIZE(ANDROID_JPEG_GPS_COORDINATES, gpsCoordinates, 3);

    static const uint8_t gpsProcessingMethod[32] = "None";
    ADD_OR_SIZE(ANDROID_JPEG_GPS_PROCESSING_METHOD, gpsProcessingMethod, 32);

    static const int64_t gpsTimestamp = 0;
    ADD_OR_SIZE(ANDROID_JPEG_GPS_TIMESTAMP, &gpsTimestamp, 1);

    static const int32_t jpegOrientation = 0;
    ADD_OR_SIZE(ANDROID_JPEG_ORIENTATION, &jpegOrientation, 1);

    /** android.stats */

    static const uint8_t faceDetectMode = ANDROID_STATISTICS_FACE_DETECT_MODE_FULL;
    ADD_OR_SIZE(ANDROID_STATISTICS_FACE_DETECT_MODE, &faceDetectMode, 1);

    static const uint8_t histogramMode = ANDROID_STATISTICS_HISTOGRAM_MODE_OFF;
    ADD_OR_SIZE(ANDROID_STATISTICS_HISTOGRAM_MODE, &histogramMode, 1);

    static const uint8_t sharpnessMapMode = ANDROID_STATISTICS_HISTOGRAM_MODE_OFF;
    ADD_OR_SIZE(ANDROID_STATISTICS_SHARPNESS_MAP_MODE, &sharpnessMapMode, 1);


    /** android.control */

    uint8_t controlIntent = 0;
    switch (request_template) {
      case CAMERA2_TEMPLATE_PREVIEW:
        controlIntent = ANDROID_CONTROL_CAPTURE_INTENT_PREVIEW;
        break;
      case CAMERA2_TEMPLATE_STILL_CAPTURE:
        controlIntent = ANDROID_CONTROL_CAPTURE_INTENT_STILL_CAPTURE;
        break;
      case CAMERA2_TEMPLATE_VIDEO_RECORD:
        controlIntent = ANDROID_CONTROL_CAPTURE_INTENT_VIDEO_RECORD;
        break;
      case CAMERA2_TEMPLATE_VIDEO_SNAPSHOT:
        controlIntent = ANDROID_CONTROL_CAPTURE_INTENT_VIDEO_SNAPSHOT;
        break;
      case CAMERA2_TEMPLATE_ZERO_SHUTTER_LAG:
        controlIntent = ANDROID_CONTROL_CAPTURE_INTENT_ZERO_SHUTTER_LAG;
        break;
      default:
        controlIntent = ANDROID_CONTROL_CAPTURE_INTENT_CUSTOM;
        break;
    }
    ADD_OR_SIZE(ANDROID_CONTROL_CAPTURE_INTENT, &controlIntent, 1);

    static const uint8_t controlMode = ANDROID_CONTROL_MODE_AUTO;
    ADD_OR_SIZE(ANDROID_CONTROL_MODE, &controlMode, 1);

    static const uint8_t effectMode = ANDROID_CONTROL_EFFECT_MODE_OFF;
    ADD_OR_SIZE(ANDROID_CONTROL_EFFECT_MODE, &effectMode, 1);

    static const uint8_t sceneMode = ANDROID_CONTROL_SCENE_MODE_UNSUPPORTED;
    ADD_OR_SIZE(ANDROID_CONTROL_SCENE_MODE, &sceneMode, 1);

    static const uint8_t aeMode = ANDROID_CONTROL_AE_MODE_ON;
    ADD_OR_SIZE(ANDROID_CONTROL_AE_MODE, &aeMode, 1);

    int32_t controlRegions[5] = {
        0, 0, mSensorInfo->mMaxWidth, mSensorInfo->mMaxHeight, 1000
    };
    ADD_OR_SIZE(ANDROID_CONTROL_AE_REGIONS, controlRegions, 5);

    static const int32_t aeExpCompensation = 0;
    ADD_OR_SIZE(ANDROID_CONTROL_AE_EXPOSURE_COMPENSATION, &aeExpCompensation, 1);

    static const int32_t aeTargetFpsRange[2] = {
        15, 30
    };
    ADD_OR_SIZE(ANDROID_CONTROL_AE_TARGET_FPS_RANGE, aeTargetFpsRange, 2);

    static const uint8_t aeAntibandingMode =
            ANDROID_CONTROL_AE_ANTIBANDING_MODE_AUTO;
    ADD_OR_SIZE(ANDROID_CONTROL_AE_ANTIBANDING_MODE, &aeAntibandingMode, 1);

    static const uint8_t awbMode =
            ANDROID_CONTROL_AWB_MODE_AUTO;
    ADD_OR_SIZE(ANDROID_CONTROL_AWB_MODE, &awbMode, 1);

    ADD_OR_SIZE(ANDROID_CONTROL_AWB_REGIONS, controlRegions, 5);

    uint8_t afMode = 0;
    switch (request_template) {
      case CAMERA2_TEMPLATE_PREVIEW:
        afMode = ANDROID_CONTROL_AF_MODE_CONTINUOUS_PICTURE;
        break;
      case CAMERA2_TEMPLATE_STILL_CAPTURE:
        afMode = ANDROID_CONTROL_AF_MODE_CONTINUOUS_PICTURE;
        break;
      case CAMERA2_TEMPLATE_VIDEO_RECORD:
        afMode = ANDROID_CONTROL_AF_MODE_CONTINUOUS_VIDEO;
        break;
      case CAMERA2_TEMPLATE_VIDEO_SNAPSHOT:
        afMode = ANDROID_CONTROL_AF_MODE_CONTINUOUS_VIDEO;
        break;
      case CAMERA2_TEMPLATE_ZERO_SHUTTER_LAG:
        afMode = ANDROID_CONTROL_AF_MODE_CONTINUOUS_PICTURE;
        break;
      default:
        afMode = ANDROID_CONTROL_AF_MODE_AUTO;
        break;
    }
    ADD_OR_SIZE(ANDROID_CONTROL_AF_MODE, &afMode, 1);

    ADD_OR_SIZE(ANDROID_CONTROL_AF_REGIONS, controlRegions, 5);

    if (sizeRequest) {
        ALOGV("Allocating %d entries, %d extra bytes for "
                "request template type %d",
                entryCount, dataCount, request_template);
        *request = allocate_camera_metadata(entryCount, dataCount);
        if (*request == NULL) {
            ALOGE("Unable to allocate new request template type %d "
                    "(%d entries, %d bytes extra data)", request_template,
                    entryCount, dataCount);
            return NO_MEMORY;
        }
    }
    return OK;
#undef ADD_OR_SIZE
}

status_t MetadaManager::setCurrentRequest(camera_metadata_t* request)
{
    if (request == NULL) {
        return BAD_VALUE;
    }

    if (mCurrentRequest != NULL) {
        free_camera_metadata(mCurrentRequest);
    }

    mCurrentRequest = clone_camera_metadata(request);
    if (mCurrentRequest == NULL) {
        return BAD_VALUE;
    }

    return NO_ERROR;
}

status_t MetadaManager::getFrameRate(int *value)
{
    camera_metadata_entry_t streams;
    int res = find_camera_metadata_entry(mCurrentRequest,
            ANDROID_CONTROL_AE_TARGET_FPS_RANGE, &streams);
    if (res != NO_ERROR) {
        ALOGE("%s: error reading fps range tag", __FUNCTION__);
        return BAD_VALUE;
    }

    int v[2];
    for (uint32_t i = 0; i < streams.count && i < 2; i++) {
        v[i] = streams.data.i32[i];
    }

    if (v[0] > 15 && v[1] > 15) {
        *value = 30;
    }
    else {
        *value = 15;
    }
    return NO_ERROR;
}

status_t MetadaManager::getGpsCoordinates(double *pCoords, int count)
{
    camera_metadata_entry_t streams;
    int res = find_camera_metadata_entry(mCurrentRequest,
                ANDROID_JPEG_GPS_COORDINATES, &streams);
    if (res != NO_ERROR) {
        ALOGE("%s: error reading jpeg Coordinates tag", __FUNCTION__);
        return BAD_VALUE;
    }

    for (int i=0; i<(int)streams.count && i<count; i++) {
        pCoords[i] = streams.data.d[i];
    }

    return NO_ERROR;
}

status_t MetadaManager::getGpsTimeStamp(int64_t &timeStamp)
{
    camera_metadata_entry_t streams;
    int res = find_camera_metadata_entry(mCurrentRequest,
                ANDROID_JPEG_GPS_TIMESTAMP, &streams);
    if (res != NO_ERROR) {
        ALOGE("%s: error reading jpeg TimeStamp tag", __FUNCTION__);
        return BAD_VALUE;
    }

    timeStamp = streams.data.i64[0];
    return NO_ERROR;
}

status_t MetadaManager::getGpsProcessingMethod(uint8_t* src, int count)
{
    camera_metadata_entry_t streams;
    int res = find_camera_metadata_entry(mCurrentRequest,
                ANDROID_JPEG_GPS_PROCESSING_METHOD, &streams);
    if (res != NO_ERROR) {
        ALOGE("%s: error reading jpeg ProcessingMethod tag", __FUNCTION__);
        return BAD_VALUE;
    }

    int i;
    for (i=0; i<(int)streams.count && i<count-1; i++) {
        src[i] = streams.data.u8[i];
    }
    src[i] = '\0';

    return NO_ERROR;
}

status_t MetadaManager::getJpegRotation(int32_t &jpegRotation)
{
    camera_metadata_entry_t streams;
    int res = find_camera_metadata_entry(mCurrentRequest,
                ANDROID_JPEG_ORIENTATION, &streams);
    if (res != NO_ERROR) {
        ALOGE("%s: error reading jpeg Rotation tag", __FUNCTION__);
        return BAD_VALUE;
    }

    jpegRotation = streams.data.i32[0];
    return NO_ERROR;
}

status_t MetadaManager::getJpegQuality(int32_t &quality)
{
    uint8_t u8Quality = 0;
    camera_metadata_entry_t streams;
    int res = find_camera_metadata_entry(mCurrentRequest,
                ANDROID_JPEG_QUALITY, &streams);
    if (res != NO_ERROR) {
        ALOGE("%s: error reading jpeg quality tag", __FUNCTION__);
        return BAD_VALUE;
    }

    //4.3 framework change quality type from i32 to u8
    u8Quality = streams.data.u8[0];
    quality = u8Quality;

    return NO_ERROR;
}

status_t MetadaManager::getJpegThumbQuality(int32_t &thumb)
{
    uint8_t u8Quality = 0;
    camera_metadata_entry_t streams;
    int res = find_camera_metadata_entry(mCurrentRequest,
                ANDROID_JPEG_THUMBNAIL_QUALITY, &streams);
    if (res != NO_ERROR) {
        ALOGE("%s: error reading jpeg thumbnail quality tag", __FUNCTION__);
        return BAD_VALUE;
    }

    //4.3 framework change quality type from i32 to u8
    u8Quality = streams.data.u8[0];
    thumb = u8Quality;

    return NO_ERROR;
}

status_t MetadaManager::getJpegThumbSize(int &width, int &height)
{
    camera_metadata_entry_t streams;
    int res = find_camera_metadata_entry(mCurrentRequest,
                ANDROID_JPEG_THUMBNAIL_SIZE, &streams);
    if (res != NO_ERROR) {
        ALOGE("%s: error reading jpeg thumbnail size tag", __FUNCTION__);
        return BAD_VALUE;
    }

    width = streams.data.i32[0];
    height = streams.data.i32[1];
    return NO_ERROR;
}

status_t MetadaManager::generateFrameRequest(camera_metadata_t * frame)
{
    if (mCurrentRequest == NULL || frame == NULL) {
        FLOGE("%s invalid param", __FUNCTION__);
        return BAD_VALUE;
    }

    camera_metadata_entry_t streams;
    int res;

    res = find_camera_metadata_entry(mCurrentRequest,
            ANDROID_REQUEST_ID, &streams);
    if (res != NO_ERROR) {
        FLOGE("%s: error reading output stream tag", __FUNCTION__);
        return BAD_VALUE;
    }

    int requestId = streams.data.i32[0];
    res = add_camera_metadata_entry(frame, ANDROID_REQUEST_ID, &requestId, 1);
    if (res != NO_ERROR) {
        FLOGE("%s: error add ANDROID_REQUEST_ID tag", __FUNCTION__);
        return BAD_VALUE;
    }

    static const int32_t frameCount = 0;
    res = add_camera_metadata_entry(frame, ANDROID_REQUEST_FRAME_COUNT,
                          &frameCount, 1);
    if (res != NO_ERROR) {
        FLOGE("%s: error add ANDROID_REQUEST_FRAME_COUNT tag", __FUNCTION__);
        return BAD_VALUE;
    }

    nsecs_t timeStamp = systemTime();
    res = add_camera_metadata_entry(frame, ANDROID_SENSOR_TIMESTAMP,
                         &timeStamp, 1);
    if (res != NO_ERROR) {
        FLOGE("%s: error add ANDROID_SENSOR_TIMESTAMP tag", __FUNCTION__);
        return BAD_VALUE;
    }

    return 0;
}

status_t MetadaManager::getRequestType(int *reqType)
{
    if (mCurrentRequest == NULL) {
        FLOGE("mCurrentRequest is invalid");
        return BAD_VALUE;
    }

    int requestType;
    camera_metadata_entry_t streams;
    int res;

    res = find_camera_metadata_entry(mCurrentRequest,
            ANDROID_REQUEST_ID, &streams);
    if (res != NO_ERROR) {
        FLOGE("%s: error reading output stream tag", __FUNCTION__);
        return BAD_VALUE;
    }

    int requestId = streams.data.i32[0];
    if (requestId >= PreviewRequestIdStart && requestId < PreviewRequestIdEnd) {
        requestType = REQUEST_TYPE_PREVIEW;
        FLOG_RUNTIME("%s request type preview", __FUNCTION__);
    }
    else if (requestId >= RecordingRequestIdStart && requestId < RecordingRequestIdEnd) {
        requestType = REQUEST_TYPE_RECORD;
        FLOG_RUNTIME("%s request type record", __FUNCTION__);
    }
    else if (requestId >= CaptureRequestIdStart && requestId < CaptureRequestIdEnd) {
        requestType = REQUEST_TYPE_CAPTURE;
        FLOG_RUNTIME("%s request type capture", __FUNCTION__);
    }
    else {
        FLOGE("%s invalid request type id:%d", __FUNCTION__, requestId);
        return BAD_VALUE;
    }

    *reqType = requestType;
    return NO_ERROR;
}

status_t MetadaManager::getFocalLength(float &focalLength)
{
    focalLength = mSensorInfo->mFocalLength;
    return NO_ERROR;
}

status_t MetadaManager::getRequestStreams(camera_metadata_entry_t *reqStreams)
{
    if (reqStreams == NULL) {
        FLOGE("%s invalid param", __FUNCTION__);
        return BAD_VALUE;
    }

    return find_camera_metadata_entry(mCurrentRequest,
                ANDROID_REQUEST_OUTPUT_STREAMS, reqStreams);
}

status_t MetadaManager::createStaticInfo(camera_metadata_t **info, bool sizeRequest)
{
    size_t entryCount = 0;
    size_t dataCount = 0;
    status_t ret;

    fAssert(mSensorInfo != NULL);

#define ADD_OR_SIZE( tag, data, count ) \
    if ( ( ret = addOrSize(*info, sizeRequest, &entryCount, &dataCount, \
            tag, data, count) ) != OK ) return ret

    // android.lens
    static float minFocusDistance = 0;
    ADD_OR_SIZE(ANDROID_LENS_INFO_MINIMUM_FOCUS_DISTANCE,
            &minFocusDistance, 1);
    ADD_OR_SIZE(ANDROID_LENS_INFO_HYPERFOCAL_DISTANCE,
            &minFocusDistance, 1);

    ADD_OR_SIZE(ANDROID_LENS_INFO_AVAILABLE_FOCAL_LENGTHS,
            &mSensorInfo->mFocalLength, 1);

    static float aperture = 2.8;
    ADD_OR_SIZE(ANDROID_LENS_INFO_AVAILABLE_APERTURES,
            &aperture, 1);

    static const float filterDensity = 0;
    ADD_OR_SIZE(ANDROID_LENS_INFO_AVAILABLE_FILTER_DENSITIES,
            &filterDensity, 1);
    static const uint8_t availableOpticalStabilization =
            ANDROID_LENS_OPTICAL_STABILIZATION_MODE_OFF;
    ADD_OR_SIZE(ANDROID_LENS_INFO_AVAILABLE_OPTICAL_STABILIZATION,
            &availableOpticalStabilization, 1);

    static const int32_t lensShadingMapSize[] = {1, 1};
    ADD_OR_SIZE(ANDROID_LENS_INFO_SHADING_MAP_SIZE, lensShadingMapSize,
            sizeof(lensShadingMapSize)/sizeof(int32_t));

    static const float lensShadingMap[3 * 1 * 1 ] =
            { 1.f, 1.f, 1.f };
    ADD_OR_SIZE(ANDROID_LENS_INFO_SHADING_MAP, lensShadingMap,
            sizeof(lensShadingMap)/sizeof(float));

    int32_t lensFacing = mCameraId ?
            ANDROID_LENS_FACING_FRONT : ANDROID_LENS_FACING_BACK;
    ADD_OR_SIZE(ANDROID_LENS_FACING, &lensFacing, 1);
#if 0
    nsecs_t kExposureTimeRange[2] =
            {1000L, 30000000000L};
    // android.sensor
    ADD_OR_SIZE(ANDROID_SENSOR_EXPOSURE_TIME_RANGE,
            kExposureTimeRange, 2);

    ADD_OR_SIZE(ANDROID_SENSOR_MAX_FRAME_DURATION,
            &mSensorInfo->mMaxFrameDuration, 1);

    uint32_t kAvailableSensitivities[5] =
             {100, 200, 400, 800, 1600};
    ADD_OR_SIZE(ANDROID_SENSOR_AVAILABLE_SENSITIVITIES,
            kAvailableSensitivities,
            sizeof(kAvailableSensitivities)
            /sizeof(uint32_t));

    uint8_t kColorFilterArrangement = ANDROID_SENSOR_RGGB;
    ADD_OR_SIZE(ANDROID_SENSOR_COLOR_FILTER_ARRANGEMENT,
            &kColorFilterArrangement, 1);
#endif
    static const float sensorPhysicalSize[2] =
        {mSensorInfo->mPhysicalWidth, mSensorInfo->mPhysicalHeight
    }; // mm
    ADD_OR_SIZE(ANDROID_SENSOR_INFO_PHYSICAL_SIZE,
            sensorPhysicalSize, 2);

    int32_t pixelArraySize[2] = {
        mSensorInfo->mMaxWidth, mSensorInfo->mMaxHeight
    };
    ADD_OR_SIZE(ANDROID_SENSOR_INFO_PIXEL_ARRAY_SIZE, pixelArraySize, 2);
    ADD_OR_SIZE(ANDROID_SENSOR_INFO_ACTIVE_ARRAY_SIZE, pixelArraySize,2);

#if 0
    uint32_t kMaxRawValue = 4000;
    ADD_OR_SIZE(ANDROID_SENSOR_WHITE_LEVEL,
            &kMaxRawValue, 1);

    uint32_t kBlackLevel = 1000;
    static const int32_t blackLevelPattern[4] = {
            kBlackLevel, kBlackLevel,
            kBlackLevel, kBlackLevel
    };
    ADD_OR_SIZE(ANDROID_SENSOR_BLACK_LEVEL_PATTERN,
            blackLevelPattern, sizeof(blackLevelPattern)/sizeof(int32_t));
#endif
    //TODO: sensor color calibration fields

    // android.flash
    uint8_t flashAvailable = 0;
    ADD_OR_SIZE(ANDROID_FLASH_INFO_AVAILABLE, &flashAvailable, 1);

    static const int64_t flashChargeDuration = 0;
    ADD_OR_SIZE(ANDROID_FLASH_INFO_CHARGE_DURATION, &flashChargeDuration, 1);

    // android.tonemap

    static const int32_t tonemapCurvePoints = 128;
    ADD_OR_SIZE(ANDROID_TONEMAP_MAX_CURVE_POINTS, &tonemapCurvePoints, 1);

    // android.scaler

    ADD_OR_SIZE(ANDROID_SCALER_AVAILABLE_FORMATS,
            mSensorInfo->mAvailableFormats,
            mSensorInfo->mAvailableFormatCount);
#if 0
    const uint32_t kAvailableFormats[3] = {
        HAL_PIXEL_FORMAT_RAW_SENSOR,
        HAL_PIXEL_FORMAT_BLOB,
        HAL_PIXEL_FORMAT_YCrCb_420_SP
    };
    ADD_OR_SIZE(ANDROID_SCALER_AVAILABLE_FORMATS,
            kAvailableFormats,
            sizeof(kAvailableFormats)/sizeof(uint32_t));
#endif
    int32_t availableRawSizes[2] = {
        mSensorInfo->mMaxWidth, mSensorInfo->mMaxHeight
    };
    ADD_OR_SIZE(ANDROID_SCALER_AVAILABLE_RAW_SIZES,
            availableRawSizes, 2);

    ADD_OR_SIZE(ANDROID_SCALER_AVAILABLE_RAW_MIN_DURATIONS,
            &mSensorInfo->mMinFrameDuration, 1);


    ADD_OR_SIZE(ANDROID_SCALER_AVAILABLE_PROCESSED_SIZES,
        mSensorInfo->mPreviewResolutions,
        mSensorInfo->mPreviewResolutionCount);
    ADD_OR_SIZE(ANDROID_SCALER_AVAILABLE_JPEG_SIZES,
        mSensorInfo->mPictureResolutions,
        mSensorInfo->mPictureResolutionCount);

    ADD_OR_SIZE(ANDROID_SCALER_AVAILABLE_PROCESSED_MIN_DURATIONS,
            &mSensorInfo->mMinFrameDuration,
            sizeof(mSensorInfo->mMinFrameDuration)/sizeof(uint64_t));

    ADD_OR_SIZE(ANDROID_SCALER_AVAILABLE_JPEG_MIN_DURATIONS,
            &mSensorInfo->mMinFrameDuration,
            sizeof(mSensorInfo->mMinFrameDuration)/sizeof(uint64_t));

    static const float maxZoom = 4;
    ADD_OR_SIZE(ANDROID_SCALER_AVAILABLE_MAX_DIGITAL_ZOOM, &maxZoom, 1);

    // android.jpeg

    static const int32_t jpegThumbnailSizes[] = {
            96, 96,
            160, 120,
            0, 0
    };

    ADD_OR_SIZE(ANDROID_JPEG_AVAILABLE_THUMBNAIL_SIZES,
            jpegThumbnailSizes, sizeof(jpegThumbnailSizes)/sizeof(int32_t));

    static const int32_t jpegMaxSize = 8 * 1024 * 1024;
    ADD_OR_SIZE(ANDROID_JPEG_MAX_SIZE, &jpegMaxSize, 1);

    // android.stats

    static const uint8_t availableFaceDetectModes[] = {
            ANDROID_STATISTICS_FACE_DETECT_MODE_OFF
    };
    ADD_OR_SIZE(ANDROID_STATISTICS_INFO_AVAILABLE_FACE_DETECT_MODES,
            availableFaceDetectModes,
            sizeof(availableFaceDetectModes));

    static const int32_t maxFaceCount = 0;
    ADD_OR_SIZE(ANDROID_STATISTICS_INFO_MAX_FACE_COUNT,
            &maxFaceCount, 1);

    static const int32_t histogramSize = 64;
    ADD_OR_SIZE(ANDROID_STATISTICS_INFO_HISTOGRAM_BUCKET_COUNT,
            &histogramSize, 1);

    static const int32_t maxHistogramCount = 1000;
    ADD_OR_SIZE(ANDROID_STATISTICS_INFO_MAX_HISTOGRAM_COUNT,
            &maxHistogramCount, 1);

    static const int32_t sharpnessMapSize[2] = {64, 64};
    ADD_OR_SIZE(ANDROID_STATISTICS_INFO_SHARPNESS_MAP_SIZE,
            sharpnessMapSize, sizeof(sharpnessMapSize)/sizeof(int32_t));

    static const int32_t maxSharpnessMapValue = 1000;
    ADD_OR_SIZE(ANDROID_STATISTICS_INFO_MAX_SHARPNESS_MAP_VALUE,
            &maxSharpnessMapValue, 1);

    // android.control

    static const uint8_t availableSceneModes[] = {
            ANDROID_CONTROL_SCENE_MODE_PORTRAIT,
            ANDROID_CONTROL_SCENE_MODE_LANDSCAPE
    };
    ADD_OR_SIZE(ANDROID_CONTROL_AVAILABLE_SCENE_MODES,
            availableSceneModes, sizeof(availableSceneModes));

    static const uint8_t availableEffects[] = {
            ANDROID_CONTROL_EFFECT_MODE_OFF
    };
    ADD_OR_SIZE(ANDROID_CONTROL_AVAILABLE_EFFECTS,
            availableEffects, sizeof(availableEffects));

    int32_t max3aRegions = 0;
    ADD_OR_SIZE(ANDROID_CONTROL_MAX_REGIONS,
            &max3aRegions, 1);

    static const uint8_t availableAeModes[] = {
            ANDROID_CONTROL_AE_MODE_OFF,
            ANDROID_CONTROL_AE_MODE_ON
    };
    ADD_OR_SIZE(ANDROID_CONTROL_AE_AVAILABLE_MODES,
            availableAeModes, sizeof(availableAeModes));

    static const camera_metadata_rational exposureCompensationStep = {
            1, 1
    };
    ADD_OR_SIZE(ANDROID_CONTROL_AE_COMPENSATION_STEP,
            &exposureCompensationStep, 1);

    int32_t exposureCompensationRange[] = {-3, 3};
    ADD_OR_SIZE(ANDROID_CONTROL_AE_COMPENSATION_RANGE,
            exposureCompensationRange,
            sizeof(exposureCompensationRange)/sizeof(int32_t));

    ADD_OR_SIZE(ANDROID_CONTROL_AE_AVAILABLE_TARGET_FPS_RANGES,
            mSensorInfo->mTargetFpsRange,
            sizeof(mSensorInfo->mTargetFpsRange)/sizeof(int32_t));

    static const uint8_t availableAntibandingModes[] = {
            ANDROID_CONTROL_AE_ANTIBANDING_MODE_OFF,
            ANDROID_CONTROL_AE_ANTIBANDING_MODE_AUTO
    };
    ADD_OR_SIZE(ANDROID_CONTROL_AE_AVAILABLE_ANTIBANDING_MODES,
            availableAntibandingModes, sizeof(availableAntibandingModes));

    static const uint8_t availableAwbModes[] = {
            ANDROID_CONTROL_AWB_MODE_OFF,
            ANDROID_CONTROL_AWB_MODE_AUTO
    };
    ADD_OR_SIZE(ANDROID_CONTROL_AWB_AVAILABLE_MODES,
            availableAwbModes, sizeof(availableAwbModes));

    static const uint8_t availableAfModes[] = {
            ANDROID_CONTROL_AF_MODE_OFF
    };
    ADD_OR_SIZE(ANDROID_CONTROL_AF_AVAILABLE_MODES,
                availableAfModes, sizeof(availableAfModes));

    static const uint8_t availableVstabModes[] = {
            ANDROID_CONTROL_VIDEO_STABILIZATION_MODE_OFF
    };
    ADD_OR_SIZE(ANDROID_CONTROL_AVAILABLE_VIDEO_STABILIZATION_MODES,
            availableVstabModes, sizeof(availableVstabModes));

    static const uint8_t quirkTriggerAuto = 1;
    ADD_OR_SIZE(ANDROID_QUIRKS_TRIGGER_AF_WITH_AUTO,
            &quirkTriggerAuto, 1);

    static const uint8_t quirkUseZslFormat = 1;
    ADD_OR_SIZE(ANDROID_QUIRKS_USE_ZSL_FORMAT,
            &quirkUseZslFormat, 1);

	//ANDROID_QUIRKS_METERING_CROP_REGION will influence face dectect and FOV.
	//Face dectect is not supported.
	//If quirk is set, FOV will calculated by PreviewAspect, VideoAspect,
	//arrayAspect(sensorAspect), stillAspect(pictureAspect) in the framework.
	//If quirk not set, FOV will calculated by arrayAspect, stillAspect. It's just the camera work mode.
	//So we not set ANDROID_QUIRKS_METERING_CROP_REGION to 1.
#if 0
    static const uint8_t quirkMeteringCropRegion = 1;
    ADD_OR_SIZE(ANDROID_QUIRKS_METERING_CROP_REGION,
            &quirkMeteringCropRegion, 1);
#endif


#undef ADD_OR_SIZE
    /** Allocate metadata if sizing */
    if (sizeRequest) {
        ALOGV("Allocating %d entries, %d extra bytes for "
                "static camera info",
                entryCount, dataCount);
        *info = allocate_camera_metadata(entryCount, dataCount);
        if (*info == NULL) {
            ALOGE("Unable to allocate camera static info"
                    "(%d entries, %d bytes extra data)",
                    entryCount, dataCount);
            return NO_MEMORY;
        }
    }
    return OK;
}

