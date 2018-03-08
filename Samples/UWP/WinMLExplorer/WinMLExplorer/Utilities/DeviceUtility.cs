using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using Windows.Devices.Enumeration;

namespace WinMLExplorer.Utilities
{
    public static class DeviceUtility
    {
        public static async Task<IEnumerable<string>> GetAvailableCameraNamesAsync()
        {
            DeviceInformationCollection deviceInfo = await DeviceInformation.FindAllAsync(DeviceClass.VideoCapture);
            return deviceInfo.Select(d => GetCameraName(d, deviceInfo)).OrderBy(name => name);
        }

        public static string GetCameraName(DeviceInformation cameraInfo, DeviceInformationCollection allCameras)
        {
            bool isCameraNameUnique = allCameras.Count(c => c.Name == cameraInfo.Name) == 1;
            return isCameraNameUnique ? cameraInfo.Name : string.Format("{0} [{1}]", cameraInfo.Name, cameraInfo.Id);
        }
    }
}
