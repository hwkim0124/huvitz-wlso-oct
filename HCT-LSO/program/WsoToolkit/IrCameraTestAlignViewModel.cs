using System;
using System.Threading;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Input;
using System.Windows.Threading;
using WsoNativeLib;
using WsoToolkit.utils;

namespace WsoToolkit
{
    using static utils.MsgBoxUtil;
    using static WsoNativeLib.WsoDevice;
    using static WsoNativeLib.WsoDomain;
    using static WsoNativeLib.WsoEyeAlign;

    // Eye alignment panel of the IR camera test window.
    //
    // Callback contract: the native EyeAlignEventOccurred delegate runs on the aligner's
    // dispatcher thread. The handler below only queues work on the WPF Dispatcher and
    // returns; it never blocks and never calls back into the aligner synchronously. A
    // measurement triggered on FocusAligned must call EyeAlign.HoldAlignment() first,
    // run the capture on a Task, and EyeAlign.ResumeAlignment() when it completes.
    public partial class IrCameraTestWindow
    {
        WsoCallback.EyeAlignEventOccurred _onEyeAlignEvent;
        WsoCallback.OptimizeButtonPressed _onOptimizeButtonPressed;

        const int MaxAlignEventLines = 300;

        // Manual stage move buttons: one press at a time, UI thread only.
        private const string MoveHoldOwner = "IrCamMove";
        private const int OverrideMoveIntervalMs = 50;
        private Button? _moveButton;


        ////////////////////////////////////////////////////////////////////////////////////////////////////////
        /// Controls
        ///
        private void InitializeAlignControls()
        {
            cmbAlignMode.ItemsSource = new string[] { "Manual", "Auto", "Full" };
            cmbAlignMode.SelectedIndex = 0;

            cmbAlignEyeSide.ItemsSource = new string[] { "OD (right eye)", "OS (left eye)" };
            cmbAlignEyeSide.SelectedIndex = (BoardDevice.GetCurrentEyeSide() == EyeSide.OS) ? 1 : 0;

            checkPupilOverlay.IsChecked = true;
            SetPupilOverlay(true);

            btnAlignHold.IsEnabled = false;
            lblAlignStatus.Content = "Idle";

            InitializeMoveControls();
        }

        private EyeAlignMode GetAlignModeSelected()
        {
            return cmbAlignMode.SelectedIndex switch
            {
                1 => EyeAlignMode.Auto,
                2 => EyeAlignMode.Full,
                _ => EyeAlignMode.Manual,
            };
        }

        private EyeSide GetAlignEyeSideSelected()
        {
            return cmbAlignEyeSide.SelectedIndex == 1 ? EyeSide.OS : EyeSide.OD;
        }

        private void SetPupilOverlay(bool enabled)
        {
            corneaPreview1.IsOverlayPupilMark = enabled;
            corneaPreview2.IsOverlayPupilMark = enabled;
            corneaPreview3.IsOverlayPupilMark = enabled;
        }

        private void SetStageClickControl(bool enabled)
        {
            corneaPreview1.IsStageControlEnabled = enabled;
            corneaPreview2.IsStageControlEnabled = enabled;
            corneaPreview3.IsStageControlEnabled = enabled;
        }

        private void ResetAlignControls()
        {
            btnAlignStart.Content = "Start Align";
            btnAlignHold.Content = "Hold";
            btnAlignHold.IsEnabled = false;
            cmbAlignMode.IsEnabled = true;
            cmbAlignEyeSide.IsEnabled = true;
            SetStageClickControl(true);
        }


        ////////////////////////////////////////////////////////////////////////////////////////////////////////
        /// Start / cancel
        ///
        private void StartEyeAlignment()
        {
            if (!IsCorneaCameraPreviewing())
            {
                ShowWarning("Start the cornea camera preview before aligning.");
                return;
            }

            var mode = GetAlignModeSelected();
            var side = GetAlignEyeSideSelected();

            var result = EyeAlign.StartAlignment(mode, side, _onEyeAlignEvent);
            if (result != EyeAlignResult.None)
            {
                ShowWarning($"Eye alignment refused: {result}");
                return;
            }

            // Click-to-jog must not interleave with the aligner's own motor commands.
            SetStageClickControl(mode == EyeAlignMode.Manual);

            btnAlignStart.Content = "Cancel Align";
            btnAlignHold.Content = "Hold";
            btnAlignHold.IsEnabled = false;
            cmbAlignMode.IsEnabled = false;
            cmbAlignEyeSide.IsEnabled = false;
            lblAlignStatus.Content = $"{mode} starting ({side})";
            AppendAlignEvent($"--- start {mode} {side}");
        }

        private void CancelEyeAlignment()
        {
            // Blocks until the run's threads have finished; the terminal event that arrives
            // through the Dispatcher resets the controls.
            EyeAlign.CancelAlignment();
        }


        ////////////////////////////////////////////////////////////////////////////////////////////////////////
        /// Event handling
        ///
        private void OnEyeAlignEventOccurred(int evt, int mode, int phase, int arg, int posX, int posY, int posZ, int posSwing)
        {
            // Native dispatcher thread: marshal and return immediately.
            Dispatcher.BeginInvoke(() =>
            {
                HandleEyeAlignEvent((EyeAlignEvent)evt, (EyeAlignMode)mode, (EyeAlignPhase)phase, arg, posX, posY, posZ, posSwing);
            }, DispatcherPriority.Normal);
        }

        private void HandleEyeAlignEvent(EyeAlignEvent evt, EyeAlignMode mode, EyeAlignPhase phase, int arg, int posX, int posY, int posZ, int posSwing)
        {
            string line = string.Format("{0:HH:mm:ss.fff} {1,-26} {2,-13} {3,-10} X={4} Y={5} Z={6} S={7}",
                DateTime.Now, evt, phase, DescribeAlignArg(evt, arg), posX, posY, posZ, posSwing);
            AppendAlignEvent(line);
            lblAlignStatus.Content = $"{mode}: {phase} - {evt}";

            switch (evt)
            {
                case EyeAlignEvent.EyeFound:
                    btnAlignHold.IsEnabled = (mode != EyeAlignMode.Manual);
                    break;
                case EyeAlignEvent.AlignHeld:
                    btnAlignHold.Content = "Resume";
                    break;
                case EyeAlignEvent.AlignResumed:
                    btnAlignHold.Content = "Hold";
                    break;
                case EyeAlignEvent.FocusAligned:
                    // A measurement trigger would go here: EyeAlign.HoldAlignment(), then the
                    // capture on a Task, then EyeAlign.ResumeAlignment() when it completes.
                    break;
            }

            if (EyeAlign.IsTerminalEvent(evt))
            {
                ResetAlignControls();
                lblAlignStatus.Content = $"{evt} ({(EyeAlignResult)arg})";
            }
        }

        private static string DescribeAlignArg(EyeAlignEvent evt, int arg)
        {
            switch (evt)
            {
                case EyeAlignEvent.AlignCancelled:
                case EyeAlignEvent.AlignFailed:
                    return ((EyeAlignResult)arg).ToString();
                case EyeAlignEvent.EyeFound:
                case EyeAlignEvent.EyeNotFound:
                case EyeAlignEvent.LowerEyeFound:
                case EyeAlignEvent.LowerEyeNotFound:
                case EyeAlignEvent.EyeLost:
                    return ((CameraType)arg).ToString();
                case EyeAlignEvent.EndOfLeft:
                case EyeAlignEvent.EndOfRight:
                case EyeAlignEvent.EndOfTop:
                case EyeAlignEvent.EndOfBottom:
                case EyeAlignEvent.EndOfForward:
                case EyeAlignEvent.EndOfBackward:
                case EyeAlignEvent.EndOfSwingLeft:
                case EyeAlignEvent.EndOfSwingRight:
                case EyeAlignEvent.MotorMoveFailed:
                    return ((MotorType)arg).ToString();
                case EyeAlignEvent.FullAlignStarted:
                case EyeAlignEvent.AutoAlignStarted:
                case EyeAlignEvent.ManualAlignStarted:
                case EyeAlignEvent.ReadyPositionReached:
                case EyeAlignEvent.EyeSidePositionReached:
                case EyeAlignEvent.SwingAtTilt:
                    return ((EyeSide)arg).ToString();
                case EyeAlignEvent.PhaseTimeout:
                case EyeAlignEvent.AlignTimeout:
                    return ((EyeAlignPhase)arg).ToString();
                default:
                    return arg.ToString();
            }
        }

        private void AppendAlignEvent(string line)
        {
            listAlignEvents.Items.Add(line);
            while (listAlignEvents.Items.Count > MaxAlignEventLines)
            {
                listAlignEvents.Items.RemoveAt(0);
            }
            listAlignEvents.ScrollIntoView(listAlignEvents.Items[listAlignEvents.Items.Count - 1]);
        }

        private void OnOptimizeButtonPressed()
        {
            // Hardware button toggles start / cancel of the selected mode.
            Dispatcher.BeginInvoke(() =>
            {
                if (EyeAlign.IsAligning())
                {
                    CancelEyeAlignment();
                }
                else
                {
                    StartEyeAlignment();
                }
            }, DispatcherPriority.Normal);
        }


        ////////////////////////////////////////////////////////////////////////////////////////////////////////
        /// Button handlers
        ///
        private void BtnAlignStart_Click(object sender, RoutedEventArgs e)
        {
            if (EyeAlign.IsAligning())
            {
                CancelEyeAlignment();
            }
            else
            {
                StartEyeAlignment();
            }
        }

        private void BtnAlignHold_Click(object sender, RoutedEventArgs e)
        {
            if (!EyeAlign.IsAligning())
            {
                return;
            }
            if (EyeAlign.IsHeld())
            {
                if (!EyeAlign.ResumeAlignment())
                {
                    AppendAlignEvent("resume refused");
                }
            }
            else
            {
                if (!EyeAlign.HoldAlignment())
                {
                    AppendAlignEvent("hold refused (eye not found yet, or manual mode)");
                }
            }
        }

        private void CheckPupilOverlay_Click(object sender, RoutedEventArgs e)
        {
            SetPupilOverlay(checkPupilOverlay.IsChecked == true);
        }

        private void BtnClearOverride_Click(object sender, RoutedEventArgs e)
        {
            EyeAlign.ClearAllPupilDetectOverrides();
            AppendAlignEvent("--- pupil overrides cleared");
        }

        private void BtnClearEvents_Click(object sender, RoutedEventArgs e)
        {
            listAlignEvents.Items.Clear();
        }


        ////////////////////////////////////////////////////////////////////////////////////////////////////////
        /// Manual stage move (Move X / Y / Z, held buttons)
        ///
        /// Unchecked: one synchronous offset move per press. Override: while held, an override move to
        /// current + step is sent every OverrideMoveIntervalMs, each superseding the one in flight.
        /// Jogging: while held, a velocity move whose direction is the sign of the step.
        /// Held moves register with StageJog so STOP ALL and the hold watchdog stop them too.
        private void InitializeMoveControls()
        {
            editOffsetX.Text = "2000";
            editOffsetY.Text = "2000";
            editOffsetZ.Text = "1500";
            checkOverride.IsChecked = false;
            checkJogging.IsChecked = false;
        }

        private (JogAxis Axis, TextBox Edit) MoveAxisOf(Button button)
        {
            if (ReferenceEquals(button, btnMoveX)) return (JogAxis.X, editOffsetX);
            if (ReferenceEquals(button, btnMoveY)) return (JogAxis.Y, editOffsetY);
            if (ReferenceEquals(button, btnMoveZ)) return (JogAxis.Z, editOffsetZ);
            throw new ArgumentOutOfRangeException(nameof(button));
        }

        private void BtnMove_PreviewMouseLeftButtonDown(object sender, MouseButtonEventArgs e)
        {
            var button = (Button)sender;
            if (_moveButton != null)
            {
                e.Handled = true;
                return;
            }

            var (axis, edit) = MoveAxisOf(button);
            MotorType type = StageJog.ToMotor(axis);
            string? refusal = null;
            if (!int.TryParse(edit.Text.Trim(), out int offset) || offset == 0)
            {
                refusal = "step must be a non-zero integer";
            }
            else if (EyeAlign.IsAligning() && GetAlignModeSelected() != EyeAlignMode.Manual)
            {
                refusal = "auto alignment running";
            }
            else if (!StageJog.IsAxisFree(axis, out string busy))
            {
                refusal = busy;
            }
            if (refusal != null)
            {
                AppendAlignEvent($"move {axis} refused: {refusal}");
                e.Handled = true;
                return;
            }

            bool jogging = checkJogging.IsChecked == true;
            bool overriding = checkOverride.IsChecked == true;
            if (!jogging && !overriding)
            {
                // One bounded move per press; the native call waits for arrival on a worker thread.
                bool ok = DeviceMotors.MoveStepMotorByPositionOffsetAsync(type, offset);
                AppendAlignEvent(ok ? $"move {axis} {offset:+0;-0} sent (sync)" : $"move {axis} {offset:+0;-0} rejected");
                return;
            }

            if (!button.CaptureMouse())
            {
                AppendAlignEvent($"move {axis} refused: no mouse capture");
                e.Handled = true;
                return;
            }
            _moveButton = button;

            HoldStartResult result;
            string what;
            if (jogging)
            {
                int direction = offset > 0 ? 1 : 0;
                what = $"jog {axis} dir {direction}";
                result = StageJog.StartVelocityHold(
                    MoveHoldOwner,
                    () => DeviceMotors.StopStepMotorInVelocityMode(type),
                    () => DeviceMotors.MoveStepMotorInVelocityMode(type, direction),
                    () => DeviceMotors.StopStepMotorInVelocityMode(type),
                    axis);
            }
            else
            {
                what = $"override {axis} {offset:+0;-0}";
                var cts = new CancellationTokenSource();
                result = StageJog.StartVelocityHold(
                    MoveHoldOwner,
                    () => { cts.Cancel(); DeviceMotors.StopStepMotor(type); },
                    () => { _ = RunOverrideMoveLoopAsync(type, axis, offset, cts.Token); return true; },
                    () => { cts.Cancel(); DeviceMotors.StopStepMotor(type); return true; },
                    axis);
            }

            switch (result)
            {
                case HoldStartResult.Started:
                    AppendAlignEvent($"{what} (release to stop)");
                    return;
                case HoldStartResult.Busy:
                    _moveButton = null;
                    button.ReleaseMouseCapture();
                    AppendAlignEvent($"{what} refused: busy");
                    break;
                case HoldStartResult.Rejected:
                    EndMovePress();
                    AppendAlignEvent($"{what} rejected");
                    break;
                default:
                    // A STOP ALL or watchdog removed the hold before the move went out; it was stopped again.
                    _moveButton = null;
                    if (button.IsMouseCaptured)
                    {
                        button.ReleaseMouseCapture();
                    }
                    AppendAlignEvent(result == HoldStartResult.RacedStopped
                        ? $"{what} stopped by STOP ALL"
                        : $"{what} STOP FAILED - press Stop All");
                    break;
            }
            e.Handled = true;
        }

        // Sequential override moves OverrideMoveIntervalMs apart until cancelled, then a stop so the
        // last target is not run to completion. Continuations run on the UI thread; the native call
        // (a status read plus the move command) does not.
        private async Task RunOverrideMoveLoopAsync(MotorType type, JogAxis axis, int offset, CancellationToken token)
        {
            int sent = 0;
            try
            {
                while (!token.IsCancellationRequested)
                {
                    bool ok = await Task.Run(() => DeviceMotors.MoveStepMotorByPositionOffsetOverride(type, offset));
                    if (!ok)
                    {
                        AppendAlignEvent($"override {axis} rejected after {sent} sent");
                        break;
                    }
                    sent++;
                    await Task.Delay(OverrideMoveIntervalMs, token);
                }
            }
            catch (OperationCanceledException)
            {
            }
            finally
            {
                DeviceMotors.StopStepMotor(type);
            }
            AppendAlignEvent($"override {axis} ended: {sent} sent");
        }

        private void BtnMove_PreviewMouseLeftButtonUp(object sender, MouseButtonEventArgs e)
        {
            EndMovePress();
        }

        private void BtnMove_LostMouseCapture(object sender, MouseEventArgs e)
        {
            EndMovePress();
        }

        // Idempotent: ReleaseMouseCapture re-enters through LostMouseCapture with _moveButton already null.
        private void EndMovePress()
        {
            Button? button = _moveButton;
            _moveButton = null;
            if (button == null)
            {
                return;
            }

            if (button.IsMouseCaptured)
            {
                button.ReleaseMouseCapture();
            }
            // Runs the hold's stop (velocity stop, or cancel of the override loop plus a stop).
            StageJog.EndVelocityHold(MoveHoldOwner);
        }

        private void BtnStopAll_Click(object sender, RoutedEventArgs e)
        {
            EndMovePress();
            var failed = StageJog.StopAll();
            AppendAlignEvent(failed.Count == 0
                ? "--- stop all sent"
                : $"--- stop all sent, returned false: {string.Join(", ", failed)} - press Stop All again");
        }
    }
}
