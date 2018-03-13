using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

using Windows.Storage;

namespace WinMLExplorer.Utilities
{
    public static class StorageUtility
    {
        public static async Task<IReadOnlyList<StorageFile>> GetFilesAsync(string folderPath)
        {
            // Get folder
            StorageFolder folder = await StorageFolder.GetFolderFromPathAsync(folderPath);

            // Initialize input files list
            return await folder.GetFilesAsync();
        }
    }
}
