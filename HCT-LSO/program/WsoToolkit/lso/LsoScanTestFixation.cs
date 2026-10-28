using System;
using WsoNativeLib;
using static WsoNativeLib.WsoDomain;

namespace WsoToolkit
{
    // Internal fixation commands and their echo on screen. Every command goes through a send
    // method here and prints exactly one [LSO-FIX] line, so a console capture counts what an
    // action sent. The position read back afterwards reaches the sliders, the number boxes and the
    // preview marker through syncFixationUi_, which keeps the slider ValueChanged handlers from
    // sending it to the device a second time. UI thread only.
    public partial class LsoScanTestWindow
    {
        // True while syncFixationUi_ writes the device position into the sliders.
        private bool _isFixationSyncing;

        private void sendFixation_(int row, int col, string source)
        {
            bool sent = Fixation.TurnOnInternalFixation(row, col);
            logFixation_($"{row},{col}", source, sent);
        }

        // The device resolves a preset from the system configuration, so the line reports the
        // position read back after the command; the same read updates the UI.
        private void sendFixationPreset_(EyeSide side, FixationTarget target)
        {
            bool sent = Fixation.TurnOnInternalFixationWithTarget(side, target);
            bool read = Fixation.GetCurrentInternalFixation(out int row, out int col);
            logFixation_(read ? $"{row},{col}" : "?,?", $"preset-{target}-{side}", sent);
            if (read)
            {
                syncFixationUi_(row, col);
            }
        }

        private static void logFixation_(string at, string source, bool sent)
        {
            Console.WriteLine(sent ? $"[LSO-FIX] {at} {source}" : $"[LSO-FIX] {at} {source} FAILED");
        }

        private void syncFixationUi_(int row, int col)
        {
            _isFixationSyncing = true;
            try
            {
                mySliderFixationRow.Value = row;
                mySliderFixationCol.Value = col;
                myTbFixationRow.Text = row.ToString();
                myTbFixationCol.Text = col.ToString();
            }
            finally
            {
                _isFixationSyncing = false;
            }

            myColorPreview.Image.SetFixationPosition(row, col);
        }

        // The preview's long-press drag: each request is one send, then the read-back position
        // reaches the sliders and the marker without a second send.
        private void onFixationDragRequested_(int row, int col, bool isFinal)
        {
            sendFixation_(row, col, isFinal ? "drag-end" : "drag");
            getCurrentInternalFixation_();
        }
    }
}
