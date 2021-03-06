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

#ifndef _OV_DEVICE_H_
#define _OV_DEVICE_H_

#include "CameraUtil.h"
#include "DeviceAdapter.h"

#define DEFAULT_PREVIEW_FPS (15)
#define DEFAULT_PREVIEW_W   (640)
#define DEFAULT_PREVIEW_H   (480)
#define DEFAULT_PICTURE_W   (640)
#define DEFAULT_PICTURE_H   (480)
#define FORMAT_STRING_LEN 64

class OvDevice : public DeviceAdapter {
public:
    virtual status_t initSensorInfo(const CameraInfo& info);
    virtual int getCaptureMode(int width, int height);

protected:
    status_t changeSensorFormats(int *src, int len);
    status_t adjustPreviewResolutions();
    status_t setMaxPictureResolutions();
};

#endif // ifndef _OV_DEVICE_H_
