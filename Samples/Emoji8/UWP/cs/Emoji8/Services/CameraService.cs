// Copyright (c) Microsoft Corporation. 
// Licensed under the MIT license. 

using Microsoft.Toolkit.Uwp.Helpers;
using System;
using System.Diagnostics;
using System.Linq;
using System.Threading.Tasks;
using Windows.Graphics.Imaging;
using Windows.Media;
using Windows.Media.Capture;
using Windows.Media.Capture.Frames;

namespace Emoji8.Services
{
    class CameraService
    {
        public event EventHandler<SoftwareBitmapEventArgs> SoftwareBitmapFrameCaptured;

        private static CameraService _current;
        public static CameraService Current => _current ?? (_current = new CameraService());

        private CameraHelper _cameraHelper;
        private bool _isInitialized = false;
        private bool _isProcessing = false;

        private void Current_IntelligenceServiceProcessingCompleted(object sender, EventArgs e)
        {
            _isProcessing = false;
        }

        public async Task<CameraHelperResult> InitializeAsync()
        {
            if (!_isInitialized)
            {
                var group = await GetFrameSourceGroupAsync();
                if (group != null)
                {
                    _cameraHelper = new CameraHelper() { FrameSourceGroup = group };
                    var result = await _cameraHelper.InitializeAndStartCaptureAsync();
                    if (result == CameraHelperResult.Success)
                    {
                        _cameraHelper.FrameArrived += CameraHelper_FrameArrived;
                        IntelligenceService.Current.IntelligenceServiceProcessingCompleted += Current_IntelligenceServiceProcessingCompleted;
                        _isInitialized = true;
                    }
                    return result;
                } else
                {
                    return CameraHelperResult.NoFrameSourceGroupAvailable;
                }
            } else
            {
                return CameraHelperResult.Success;
            }
        }

        private async Task<MediaFrameSourceGroup> GetFrameSourceGroupAsync()
        {
            var availableFrameSourceGroups = await CameraHelper.GetFrameSourceGroupsAsync();
            if (availableFrameSourceGroups != null)
            {
                //get a front-facing camera if one is available
                var selectedGroup = availableFrameSourceGroups.Select(group =>
                   new
                   {
                       sourceGroup = group,
                       colorSourceInfo = group.SourceInfos.FirstOrDefault((sourceInfo) =>
                       {
                           return
                               (sourceInfo.MediaStreamType == MediaStreamType.VideoPreview ||
                               sourceInfo.MediaStreamType == MediaStreamType.VideoRecord)
                           && sourceInfo.SourceKind == MediaFrameSourceKind.Color
                           && (sourceInfo.DeviceInformation.EnclosureLocation != null && sourceInfo.DeviceInformation.EnclosureLocation.Panel == Windows.Devices.Enumeration.Panel.Front);
                       })
                   })
                    .Where(t => t.colorSourceInfo != null)
                    .FirstOrDefault();

                // if we have no front facing camera, take any camera that is available
                if (selectedGroup == null)
                {
                    selectedGroup = availableFrameSourceGroups.Select(group =>
                    new
                    {
                        sourceGroup = group,
                        colorSourceInfo = group.SourceInfos.FirstOrDefault((sourceInfo) =>
                        {
                            return
                                (sourceInfo.MediaStreamType == MediaStreamType.VideoPreview ||
                                sourceInfo.MediaStreamType == MediaStreamType.VideoRecord)
                            && sourceInfo.SourceKind == MediaFrameSourceKind.Color;
                        })
                    })
                    .Where(t => t.colorSourceInfo != null)
                    .FirstOrDefault();
                }
                if (selectedGroup != null)
                {
                    return selectedGroup.sourceGroup;
                }
            }
            return null;
        }

        private void CameraHelper_FrameArrived(object sender, FrameEventArgs e)
        {
            Debug.WriteLine("A frame arrived in the CameraService");

            if (_isProcessing)
            {
                Debug.WriteLine("A frame already processing in the CameraService.  Throwing away...");
                return;
            }

            // Gets the current video frame
            VideoFrame currentVideoFrame = e.VideoFrame;

            // Gets the software bitmap image
            SoftwareBitmap softwareBitmap = currentVideoFrame.SoftwareBitmap;
            
            if (currentVideoFrame.SoftwareBitmap != null && !_isProcessing && SoftwareBitmapFrameCaptured != null)
            {
                _isProcessing = true;
                SoftwareBitmapFrameCaptured.Invoke(this, new SoftwareBitmapEventArgs(currentVideoFrame.SoftwareBitmap));
                Debug.WriteLine("The frame should have been sent to the Intelligence Service");
            }
            else if (currentVideoFrame.SoftwareBitmap != null)
            {
                currentVideoFrame.SoftwareBitmap.Dispose();            
            }            
        }

        public async Task CleanUpAsync()
        {
            if (!_isInitialized)
            {
                throw new InvalidOperationException("CameraService not initialized.");
            }

            _cameraHelper.FrameArrived -= CameraHelper_FrameArrived;
            IntelligenceService.Current.IntelligenceServiceProcessingCompleted -= Current_IntelligenceServiceProcessingCompleted;
            _current = null;
            await _cameraHelper.CleanUpAsync();
            _cameraHelper.Dispose();
            _cameraHelper = null;
            _isProcessing = false;
            _isInitialized = false;
        }
    }
}
