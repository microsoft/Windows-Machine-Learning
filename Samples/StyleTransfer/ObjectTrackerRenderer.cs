// Copyright (C) Microsoft Corporation. All rights reserved.
using System.Collections.Generic;
using System.Linq;
using Windows.Foundation;
using Windows.UI;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Media;
using Windows.UI.Xaml.Shapes;

namespace ObjectDetectorSkillSample
{
    /// <summary>
    /// Helper struct for storing tracker results
    /// </summary>
    internal struct DetectionResult
    {
        public string label;
        public Rect bbox;
        public double prob;
    }

    /// <summary>
    /// Helper class to render object detections
    /// </summary>
    internal class BoundingBoxRenderer
    {
        private Canvas m_canvas;

        // Cache the original Rects we get for resizing purposes
        private List<Rect> m_rawRects;

        // Pre-populate rectangles/textblocks to avoid clearing and re-creating on each frame
        private Rectangle[] m_rectangles;
        private TextBlock[] m_textBlocks;
        private Border[] m_borders;

        /// <summary>
        /// </summary>
        /// <param name="canvas"></param>
        /// <param name="maxBoxes"></param>
        /// <param name="lineThickness"></param>
        /// <param name="colorBrush">Default Colors.SpringGreen color brush if not specified</param>
        public BoundingBoxRenderer(Canvas canvas, int maxBoxes = 50, int lineThickness = 2)
        {
            m_rawRects = new List<Rect>();
            m_rectangles = new Rectangle[maxBoxes];
            m_borders = new Border[maxBoxes];
            m_textBlocks = new TextBlock[maxBoxes];
            var foregroundColorBrush = new SolidColorBrush(Colors.SpringGreen);
            var backgroundColorBrush = new SolidColorBrush(Colors.Black);

            m_canvas = canvas;
            for (int i = 0; i < maxBoxes; i++)
            {
                // Default configuration.
                m_rectangles[i] = new Rectangle();
                m_rectangles[i].Stroke = foregroundColorBrush;
                m_rectangles[i].StrokeThickness = lineThickness;
                // Hide
                m_rectangles[i].Visibility = Visibility.Collapsed;
                // Add to canvas
                m_canvas.Children.Add(m_rectangles[i]);

                // Create Bordered Textblocks

                // Default configuration for textblock
                m_textBlocks[i] = new TextBlock();
                m_textBlocks[i].Foreground = foregroundColorBrush;
                m_textBlocks[i].FontSize = 18;
                // Hide
                m_textBlocks[i].Visibility = Visibility.Collapsed;

                // Default configuration for border
                m_borders[i] = new Border();
                m_borders[i].Background = backgroundColorBrush;
                m_borders[i].Child = m_textBlocks[i];

                // Add to canvas
                m_canvas.Children.Add(m_borders[i]);
            }
        }

        /// <summary>
        /// Render bounding boxes from ObjectDetections
        /// </summary>
        /// <param name="detections"></param>
        public void Render(IReadOnlyList<DetectionResult> detections)
        {
            int i = 0;
            m_rawRects.Clear();
            // Render detections up to MAX_BOXES
            for (i = 0; i < detections.Count && i < m_rectangles.Length; i++)
            {
                // Cache rect
                m_rawRects.Add(detections[i].bbox);

                // Render bounding box
                m_rectangles[i].Width = detections[i].bbox.Width * m_canvas.ActualWidth;
                m_rectangles[i].Height = detections[i].bbox.Height * m_canvas.ActualHeight;
                Canvas.SetLeft(m_rectangles[i], detections[i].bbox.X * m_canvas.ActualWidth);
                Canvas.SetTop(m_rectangles[i], detections[i].bbox.Y * m_canvas.ActualHeight);
                m_rectangles[i].Visibility = Visibility.Visible;

                // Render text label
                m_textBlocks[i].Text = $"{detections[i].label}";
                Canvas.SetLeft(m_borders[i], detections[i].bbox.X * m_canvas.ActualWidth + 2);
                Canvas.SetTop(m_borders[i], detections[i].bbox.Y * m_canvas.ActualHeight + 2);
                m_textBlocks[i].Visibility = Visibility.Visible;
            }
            // Hide all remaining boxes
            for (; i < m_rectangles.Length; i++)
            {
                // Early exit: Everything after i will already be collapsed
                if (m_rectangles[i].Visibility == Visibility.Collapsed)
                {
                    break;
                }
                m_rectangles[i].Visibility = Visibility.Collapsed;
                m_textBlocks[i].Visibility = Visibility.Collapsed;
            }
        }

        /// <summary>
        /// Resize canvas and rendered bounding boxes
        /// </summary>
        public void ResizeContent(SizeChangedEventArgs args)
        {
            // Resize rendered bboxes
            for (int i = 0; i < m_rectangles.Length && m_rectangles[i].Visibility == Visibility.Visible; i++)
            {
                // Update bounding box
                m_rectangles[i].Width = m_rawRects[i].Width * m_canvas.Width;
                m_rectangles[i].Height = m_rawRects[i].Height * m_canvas.Height;
                Canvas.SetLeft(m_rectangles[i], m_rawRects[i].X * m_canvas.Width);
                Canvas.SetTop(m_rectangles[i], m_rawRects[i].Y * m_canvas.Height);

                // Update text label
                Canvas.SetLeft(m_borders[i], m_rawRects[i].X * m_canvas.Width + 2);
                Canvas.SetTop(m_borders[i], m_rawRects[i].Y * m_canvas.Height + 2);
            }
        }
    }
}
