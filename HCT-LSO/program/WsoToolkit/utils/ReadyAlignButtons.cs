using System;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Input;
using System.Windows.Threading;
using WsoNativeLib;
using static WsoNativeLib.WsoEyeAlign;

namespace WsoToolkit.utils
{
    // The "Move To" row of a scan window's Target Eye group: Ready OD / Ready OS / Middle.
    //
    // Each button starts an eye alignment run that only moves the head to a ready position
    // (EyeAlignMode.Ready for an eye side, EyeAlignMode.ReadyMiddle for the middle) and ends by
    // itself on arrival. While the run lasts the pressed button reads "Cancel", the other two are
    // disabled and the owning window shows the hourglass; pressing "Cancel" ends the run.
    //
    // UI thread only, except the native event callback, which marshals to the Dispatcher. The
    // callback delegate lives in a field of this object: the owning window must keep the object
    // in a field of its own for as long as it can start a run.
    internal sealed class ReadyAlignButtons
    {
        private readonly Window _window;
        private readonly Button _btnOd;
        private readonly Button _btnOs;
        private readonly Button _btnMiddle;
        private readonly Action<bool> _setStageControlEnabled;
        private readonly WsoCallback.EyeAlignEventOccurred _onEyeAlignEvent;

        private Button? _runningButton = null;      // the button that started the running move
        private object? _runningCaption = null;     // its caption, shown again when the move has ended
        private bool _isCancelling = false;         // CancelAlignment() in flight on a worker thread

        // setStageControlEnabled switches the window's click-to-jog on the cornea preview, which
        // must not interleave with the aligner's own motor commands.
        public ReadyAlignButtons(Window window, Button btnOd, Button btnOs, Button btnMiddle, Action<bool> setStageControlEnabled)
        {
            _window = window;
            _btnOd = btnOd;
            _btnOs = btnOs;
            _btnMiddle = btnMiddle;
            _setStageControlEnabled = setStageControlEnabled;
            _onEyeAlignEvent = new WsoCallback.EyeAlignEventOccurred(OnEyeAlignEventOccurred);
        }

        public bool IsRunning => _runningButton != null;

        // Click handler of all three buttons: starts the move, or cancels the one in progress.
        public void OnButtonClick(Button button)
        {
            if (IsRunning)
            {
                Cancel(button);
            }
            else
            {
                Start(button);
            }
        }

        // Window closing: ends a move these buttons started. A cancel already in flight is left alone.
        public void CancelAtClose()
        {
            if (IsRunning && !_isCancelling)
            {
                EyeAlign.CancelAlignment();
            }
        }

        private void Start(Button button)
        {
            var mode = ReferenceEquals(button, _btnMiddle) ? EyeAlignMode.ReadyMiddle : EyeAlignMode.Ready;
            var side = ReferenceEquals(button, _btnOs) ? WsoDomain.EyeSide.OS : WsoDomain.EyeSide.OD;

            var result = EyeAlign.StartAlignment(mode, side, _onEyeAlignEvent);
            if (result != EyeAlignResult.None)
            {
                MsgBoxUtil.ShowWarning($"Failed to start the move to the ready position: {result}");
                return;
            }

            _runningButton = button;
            _runningCaption = button.Content;
            button.Content = "Cancel";
            foreach (var other in new[] { _btnOd, _btnOs, _btnMiddle })
            {
                other.IsEnabled = ReferenceEquals(other, button);
            }
            _setStageControlEnabled(false);
            SetBusyCursor(true);
        }

        private async void Cancel(Button button)
        {
            if (!ReferenceEquals(button, _runningButton) || _isCancelling)
            {
                return;
            }
            _isCancelling = true;
            button.IsEnabled = false;
            try
            {
                // Returns when the run has ended, its Z retreat to the ready position included:
                // kept off the UI thread. The buttons are reset here, not by the terminal event,
                // so a new move cannot start while this call is still joining the run's threads.
                await Task.Run(() => EyeAlign.CancelAlignment());
            }
            finally
            {
                _isCancelling = false;
                Reset();
            }
        }

        private void Reset()
        {
            if (_runningButton != null)
            {
                _runningButton.Content = _runningCaption;
            }
            _runningButton = null;
            _runningCaption = null;
            _btnOd.IsEnabled = true;
            _btnOs.IsEnabled = true;
            _btnMiddle.IsEnabled = true;
            _setStageControlEnabled(true);
            SetBusyCursor(false);
        }

        // Hourglass over the owning window while the head moves. Set on the window (forced onto
        // its children) rather than through Mouse.OverrideCursor, which the scan and optimize
        // handlers of the scan windows clear when they finish.
        private void SetBusyCursor(bool busy)
        {
            _window.Cursor = busy ? Cursors.Wait : null;
            _window.ForceCursor = busy;
        }

        private void OnEyeAlignEventOccurred(int evt, int mode, int phase, int arg, int posX, int posY, int posZ, int posSwing)
        {
            // Native dispatcher thread: only the end of the run matters here; marshal and return.
            var alignEvent = (EyeAlignEvent)evt;
            if (!EyeAlign.IsTerminalEvent(alignEvent))
            {
                return;
            }
            _window.Dispatcher.BeginInvoke(() =>
            {
                // A cancel in flight resets the buttons itself once CancelAlignment() has returned.
                if (!_isCancelling)
                {
                    Reset();
                }
                if (alignEvent == EyeAlignEvent.AlignFailed)
                {
                    MsgBoxUtil.ShowError($"Failed to move to the ready position: {(EyeAlignResult)arg}");
                }
            }, DispatcherPriority.Normal);
        }
    }
}
