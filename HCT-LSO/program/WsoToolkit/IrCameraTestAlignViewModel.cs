using System;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Threading;
using WsoNativeLib;

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
    }
}
