using System;
using System.Globalization;
using System.Windows;
using System.Windows.Input;
using System.Windows.Media;

namespace WsoToolkit.controls
{
    // Zoom and pan for the preview image: the DevKit ZoomPanPictureBox gestures on WPF transforms.
    //   wheel            zoom about the cursor, x1.2 a notch, x1..x60
    //   Ctrl+left drag   pan, while zoomed in
    //   middle click     back to the Uniform fit
    //   long left press  then drag moves the fixation marker (LsoScanImagePreview.Fixation.cs)
    // Plain clicks are left alone, so the slit measurement behaves as before; the wheel is
    // consumed by the zoom while an image is shown. The viewport grid is click-through (see
    // the constructor), so the gestures are read on their way down to it - this control's
    // Preview mouse events - and kept to the viewport rectangle.
    //
    // Only myZoomSurface carries the zoom. A frame only swaps imageViewport.Source, so the zoom
    // outlives live updates; the overlays stay in viewport space and are re-placed through the
    // image after every change, so their strokes do not grow with the zoom.
    public partial class LsoScanImagePreview
    {
        private const double ZoomStepPerNotch = 1.2;
        private const double MinZoom = 1.0;
        private const double MaxZoom = 60.0;

        // DevKit ClampPanOffset: the zoomed image always covers at least the middle 40% of the
        // viewport on each axis, so it cannot be panned out of sight.
        private const double PanMarginRatio = 0.7;

        // Mouse travel (DIPs) before a Ctrl+press counts as a drag and gets the hand cursor.
        private const double PanDragSlop = 3.0;

        private bool _isPanning;
        private Point _panStartMouse;
        private Vector _panStartOffset;

        private void UserControl_PreviewMouseWheel(object sender, MouseWheelEventArgs e)
        {
            if (imageViewport.Source == null || !isOverViewport_(e))
            {
                return;   // not ours: no image, or the pointer is outside the viewport
            }

            // x1.2 per notch; a finer-grained wheel or touchpad steps in proportion.
            double factor = Math.Pow(ZoomStepPerNotch, (double)e.Delta / Mouse.MouseWheelDeltaForOneLine);
            zoomAt_(e.GetPosition(myViewportHost), factor);
            e.Handled = true;
        }

        private void UserControl_PreviewMouseDown(object sender, MouseButtonEventArgs e)
        {
            if (!isOverViewport_(e))
            {
                return;
            }

            if (e.ChangedButton == MouseButton.Middle)
            {
                resetZoom_();
                e.Handled = true;
                return;
            }

            armFixationDrag_(e);

            // At x1 there is nothing to drag, so a Ctrl+click still goes to whoever took it
            // before - the slit measurement, while one runs.
            if (e.ChangedButton != MouseButton.Left || myZoomScale.ScaleX <= MinZoom
                || (Keyboard.Modifiers & ModifierKeys.Control) != ModifierKeys.Control)
            {
                return;
            }

            _panStartMouse = e.GetPosition(myViewportHost);
            _panStartOffset = new Vector(myZoomPan.X, myZoomPan.Y);
            _isPanning = CaptureMouse();
            e.Handled = true;
        }

        private void UserControl_PreviewMouseMove(object sender, MouseEventArgs e)
        {
            if (fixationMouseMove_(e) || !_isPanning)
            {
                return;
            }

            Vector moved = e.GetPosition(myViewportHost) - _panStartMouse;
            if (Math.Abs(moved.X) > PanDragSlop || Math.Abs(moved.Y) > PanDragSlop)
            {
                Cursor = Cursors.Hand;
            }

            myZoomPan.X = _panStartOffset.X + moved.X;
            myZoomPan.Y = _panStartOffset.Y + moved.Y;
            applyZoom_();
            e.Handled = true;
        }

        private void UserControl_PreviewMouseUp(object sender, MouseButtonEventArgs e)
        {
            if (fixationMouseUp_(e))
            {
                return;
            }

            if (_isPanning && e.ChangedButton == MouseButton.Left)
            {
                ReleaseMouseCapture();   // LostMouseCapture ends the drag
                e.Handled = true;
            }
        }

        // Also ends a drag when something else takes the mouse midway (a dialog, Alt+Tab).
        // Children losing their own capture bubble through here too; those are not a drag.
        private void UserControl_LostMouseCapture(object sender, MouseEventArgs e)
        {
            fixationLostCapture_();
            if (!_isPanning)
            {
                return;
            }

            _isPanning = false;
            ClearValue(CursorProperty);
        }

        // A frame laid out at another size moves the image under the zoom, so the pan limits
        // and the viewport-space overlays are recomputed for it.
        private void imageViewport_SizeChanged(object sender, SizeChangedEventArgs e)
        {
            applyZoom_();
        }

        private bool isOverViewport_(MouseEventArgs e)
        {
            Point p = e.GetPosition(myViewportHost);
            return p.X >= 0 && p.Y >= 0 && p.X < myViewportHost.ActualWidth && p.Y < myViewportHost.ActualHeight;
        }

        // Zooms by factor about a viewport point, keeping the image point under it in place.
        private void zoomAt_(Point anchor, double factor)
        {
            double oldZoom = myZoomScale.ScaleX;
            double newZoom = Math.Clamp(oldZoom * factor, MinZoom, MaxZoom);
            if (Math.Abs(newZoom - oldZoom) < 0.001)
            {
                return;
            }

            if (newZoom <= MinZoom + 0.001)
            {
                resetZoom_();
                return;
            }

            double ratio = newZoom / oldZoom;
            myZoomPan.X = anchor.X - ((anchor.X - myZoomPan.X) * ratio);
            myZoomPan.Y = anchor.Y - ((anchor.Y - myZoomPan.Y) * ratio);
            myZoomScale.ScaleX = newZoom;
            myZoomScale.ScaleY = newZoom;
            applyZoom_();
        }

        private void resetZoom_()
        {
            myZoomScale.ScaleX = MinZoom;
            myZoomScale.ScaleY = MinZoom;
            myZoomPan.X = 0.0;
            myZoomPan.Y = 0.0;
            applyZoom_();
        }

        // Everything that follows a zoom or pan change: keep the image findable, pick the
        // scaling filter, update the readout and re-place the overlays drawn in viewport space.
        private void applyZoom_()
        {
            clampPan_();

            double zoom = myZoomScale.ScaleX;
            bool zoomed = zoom > MinZoom;

            // Magnified pixels are drawn as hard blocks, as in the DevKit zoom view; at the fit
            // the image keeps the default smooth downscale.
            if (zoomed)
            {
                RenderOptions.SetBitmapScalingMode(imageViewport, BitmapScalingMode.NearestNeighbor);
            }
            else
            {
                imageViewport.ClearValue(RenderOptions.BitmapScalingModeProperty);
            }

            myZoomBadge.Visibility = zoomed ? Visibility.Visible : Visibility.Collapsed;
            myZoomText.Text = string.Format(CultureInfo.InvariantCulture, "x{0:F1}", zoom);

            if (IsOverlayAlignGuide)
            {
                DrawAlignGuide();
            }

            mySlitOverlay.Refresh();
            drawFixationOverlay_();
        }

        private void clampPan_()
        {
            double zoom = myZoomScale.ScaleX;
            if (zoom <= MinZoom)
            {
                myZoomPan.X = 0.0;
                myZoomPan.Y = 0.0;
                return;
            }

            // Where the fitted image sits on the surface (its letterbox offset), before the zoom.
            Point origin = imageViewport.TranslatePoint(new Point(0.0, 0.0), myZoomSurface);
            myZoomPan.X = clampPanAxis_(myZoomPan.X, zoom, origin.X, imageViewport.ActualWidth, myViewportHost.ActualWidth);
            myZoomPan.Y = clampPanAxis_(myZoomPan.Y, zoom, origin.Y, imageViewport.ActualHeight, myViewportHost.ActualHeight);
        }

        // The zoomed image's leading edge lands at zoom * origin + pan. It may come in no further
        // than 30% of the view, and its trailing edge may go back no further than 70%.
        private static double clampPanAxis_(double pan, double zoom, double origin, double imageSize, double viewSize)
        {
            double margin = viewSize * PanMarginRatio;
            double lo = margin - (zoom * imageSize);
            double hi = viewSize - margin;
            double edge = Math.Clamp((zoom * origin) + pan, Math.Min(lo, hi), Math.Max(lo, hi));
            return edge - (zoom * origin);
        }
    }
}
