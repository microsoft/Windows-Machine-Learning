// Copyright (c) Microsoft Corporation. 
// Licensed under the MIT license. 

using Lumia.Imaging;
using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.IO;
using System.Runtime.InteropServices.WindowsRuntime;
using System.Threading.Tasks;
using Windows.Foundation;
using Windows.Graphics.Imaging;
using Windows.Storage.Streams;
using Windows.UI.Xaml.Media.Imaging;

namespace Emoji8.Pages.PageHelpers
{
    class GifHelpers
    {       
        public static async Task<List<IImageProvider>> LoadImagesAsync(bool isBranded, ObservableCollection<Emoji> ResultsEmojis)
        {
            WriteableBitmap logoImg = null;
            var sources = new List<IImageProvider>();
            List<IRandomAccessStream> streams = new List<IRandomAccessStream>();
            List<Task> tasks = new List<Task>();

            if (isBranded)
            {
                logoImg = await BitmapFactory.FromContent(new Uri("ms-appx:///Assets/emoji8Small.png"));
            }

            foreach (Emoji emoji in ResultsEmojis)
            {
                WriteableBitmap img = emoji.BestPicWB;
                if (img != null)
                {

                    if (isBranded) //superimpose logo on images before combining them in a gif
                    {
                        img.Blit(new Rect(1350, 1150, 536, 250), logoImg, new Rect(0, 0, logoImg.PixelWidth, logoImg.PixelHeight));
                    }

                    var stream = new InMemoryRandomAccessStream();
                    streams.Add(stream);
                    tasks.Add(img.ToStream(stream, BitmapEncoder.GifEncoderId));

                }
            }

            await Task.WhenAll(tasks);
            foreach (var stream in streams)
            {
                stream.Seek(0);
                sources.Add(new StreamImageSource(stream.AsStream()));
            }

            return sources;
        }

        public static async Task<BitmapImage> CreateGifBitmapImageAsync(bool isBranded, List<IImageProvider> images, ObservableCollection<Emoji> ResultsEmojis)
        {
            BitmapImage bitmap;
            IBuffer buffer;

            double scaleForTwitter = 0.5;

            //get size of gif based on saved images
            var height = ResultsEmojis[0].BestPicWB.PixelHeight * scaleForTwitter;
            var width = ResultsEmojis[0].BestPicWB.PixelWidth * scaleForTwitter;

            using (var gifRenderer = new GifRenderer(images))
            {
                gifRenderer.Size = new Size(width, height);
                gifRenderer.Duration = 100;
                buffer = await gifRenderer.RenderAsync();
            }

            bitmap = new BitmapImage();
            await bitmap.SetSourceAsync(buffer.AsStream().AsRandomAccessStream());

            if (isBranded)
            {
                ResultsPage.GifStream = buffer.AsStream();
            }

            return bitmap;
        }
    }
}
