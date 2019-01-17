// Copyright (c) Microsoft Corporation. 
// Licensed under the MIT license. 

using System;
using System.Threading.Tasks;
using Windows.UI.Popups;

namespace Emoji8.Services
{
    public class MessageDialogService
    {
        private MessageDialogService() { }

        private static MessageDialogService _current;

        public static MessageDialogService Current => _current ?? (_current = new MessageDialogService());

        public async Task WriteMessage(string message)
        {
            await new MessageDialog(message).ShowAsync();
        }
    }
}
