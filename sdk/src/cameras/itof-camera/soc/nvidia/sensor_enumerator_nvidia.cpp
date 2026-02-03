/*
 * BSD 3-Clause License
 *
 * Copyright (c) 2019, Analog Devices, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "connections/target/target_sensor_enumerator.h"
#include "target_definitions.h"

#include <dirent.h>
#ifdef USE_GLOG
#include <glog/logging.h>
#else
#include <aditof/log.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <sys/stat.h>
#include <unistd.h>

using namespace aditof;

namespace local {

aditof::Status findDevicePathsAtMedia(const std::string &media,
                                      std::vector<std::string> &dev_paths,
                                      std::vector<std::string> &subdev_paths,
                                      std::vector<std::string> &device_names) {
    using namespace aditof;
    using namespace std;

    char *buf;
    int size = 0;

    /* Run media-ctl to get the video processing pipes */
    char cmd[64];
    sprintf(cmd, "media-ctl -d %s --print-dot", media.c_str());
    FILE *fp = popen(cmd, "r");
    if (!fp) {
        LOG(WARNING) << "Error running media-ctl";
        return Status::GENERIC_ERROR;
    }

    /* Read the media-ctl output stream */
    buf = (char *)malloc(128 * 1024);
    while (!feof(fp)) {
        fread(&buf[size], 1, 1, fp);
        size++;
    }
    pclose(fp);
    buf[size] = '\0';

    /* Search command media-ctl for device/subdevice names */
    string str(buf);
    free(buf);

    size_t pos = 0;
    while ((pos = str.find("vi-output, adsd3500", pos)) != string::npos) {
        size_t start = str.find("/dev/video", pos);
        if (start != string::npos) {
            string dev_path = str.substr(start, strlen("/dev/videoX"));
            dev_paths.push_back(dev_path);
        }
        pos += strlen("vi-output, adsd3500");
    }

    // pos = 0;
    // while ((pos = str.find("adsd3500", pos)) != string::npos) {
    //     size_t start = str.find("/dev/v4l-subdev", pos);
    //     if (start != string::npos) {
    //         string subdev_path = str.substr(start, strlen("/dev/v4l-subdevX"));
    //         subdev_paths.push_back(subdev_path);
    //         device_names.push_back("adsd3500");
    //         LOG(INFO) << "subdev " << subdev_path << "part of adsd3500 test";
    //     }
    //     pos += strlen("adsd3500");
    // }

    pos = 0;
    while ((pos = str.find("/dev/v4l-subdev", pos)) != std::string::npos) {
        // Look 60 characters before the found position for "adsd3500"
        size_t contextStart = (pos >= 60 ? pos - 60 : 0);
        std::string context = str.substr(contextStart, pos - contextStart);
        if (context.find("adsd3500") != std::string::npos) {
            std::string subdev_path = str.substr(pos, strlen("/dev/v4l-subdevX"));
            LOG(INFO) << "subdev " << subdev_path << "part of adsd3500 test";
            subdev_paths.push_back(subdev_path);
            device_names.push_back("adsd3500");
        }
    pos += strlen("/dev/v4l-subdevX");
    }

    return Status::OK;
}

}; // namespace local

Status TargetSensorEnumerator::searchSensors() {
    Status status = Status::OK;

    LOG(INFO) << "Looking for sensors on the target - Loek";

    // Find all media device paths
    std::vector<std::string> mediaPaths;
    const std::string mediaDirPath("/dev/");
    const std::string mediaBaseName("media");
    std::string deviceName;

    DIR *dirp = opendir(mediaDirPath.c_str());
    struct dirent *dp;
    while ((dp = readdir(dirp))) {
        if (!strncmp(dp->d_name, mediaBaseName.c_str(),
                     mediaBaseName.length())) {
            std::string fullMediaPath = mediaDirPath + std::string(dp->d_name);
            mediaPaths.emplace_back(fullMediaPath);
        }
    }
    closedir(dirp);

    // Identify any eligible time of flight cameras
    for (const auto &media : mediaPaths) {
        DLOG(INFO) << "Looking at: " << media << " for an eligible TOF camera";

        std::vector<std::string> devPaths;
        std::vector<std::string> subdevPaths;
        std::vector<std::string> deviceNames;

        status = local::findDevicePathsAtMedia(media, devPaths, subdevPaths,
                                               deviceNames);
        if (status != Status::OK) {
            LOG(WARNING) << "failed to find device paths at media: " << media;
            continue;
        }

        for (size_t i = 0; i < devPaths.size(); ++i) {
            DLOG(INFO) << "Considering: " << devPaths[i] << " an eligible TOF camera";

            SensorInfo sInfo;

            if (deviceNames[i] == "adsd3500") {
                sInfo.sensorType = SensorType::SENSOR_ADSD3500;
            }

            sInfo.driverPath = devPaths[i];
            sInfo.subDevPath = subdevPaths[i];
            sInfo.captureDev = CAPTURE_DEVICE_NAME;
            m_sensorsInfo.emplace_back(sInfo);
        }
    }

    return status;
}