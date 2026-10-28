using System;
using System.Collections.Generic;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Controls.Primitives;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Threading;
using WsoNativeLib;
using WsoNativeLib.Presets;
using WsoToolkit.utils;

namespace WsoToolkit
{
    using static utils.NumberUtil;
    using static WsoNativeLib.WsoDevice;

    public partial class StageMotorPanel : UserControl
    {
        private const string ChinrestOwner = "chinrest";
        private static readonly Brush StatusErrorBrush = new SolidColorBrush(Color.FromRgb(0xB7, 0x1C, 0x1C));
        private static readonly Brush BadgeMovingBrush = new SolidColorBrush(Color.FromRgb(0xFF, 0xB7, 0x4D));
        private static readonly Brush BadgeLimitBrush = new SolidColorBrush(Color.FromRgb(0xE5, 0x73, 0x73));
        private static readonly Brush BadgeBusyBrush = new SolidColorBrush(Color.FromRgb(0xB0, 0xBE, 0xC5));

        private readonly StepMotorPositionChanged _swingChanged;
        private readonly StepMotorPositionChanged _xStageChanged;
        private readonly StepMotorPositionChanged _yStageChanged;
        private readonly StepMotorPositionChanged _zStageChanged;

        private StepMotorStatus _msSwing = new();
        private StepMotorStatus _msXstage = new();
        private StepMotorStatus _msYstage = new();
        private StepMotorStatus _msZstage = new();

        private bool _isActive;
        private bool _isDraggingStarted;
        private bool _isUpdatingFromDevice;

        // UI thread only. Last position reported by the board, used to put a refused slider back.
        private readonly Dictionary<MotorType, int> _lastPos = new();

        // Badge refresh while Loaded (4 Hz). It also reads the board position of an axis when its step
        // move ends, on every tick while a velocity hold (cornea click) owns it, and on the two ticks after
        // the hold ends: velocity moves raise no position callback. UI thread only.
        private readonly DispatcherTimer _badgeTimer;
        private readonly bool[] _wasMoving = new bool[3];
        private readonly int[] _holdRefreshTicks = new int[3];
        private const int HoldRefreshTicksAfterEnd = 2;
        private Window? _host;

        // Jog pad press (UI thread only): the button under the press and how many Clicks it produced.
        private RepeatButton? _padButton;
        private int _padClicks;

        // Chinrest hold (UI thread only). _chinGen guards the UI reset that the hold's stop posts,
        // which may come from StageJog.StopAll or the watchdog on another thread.
        private Button? _chinButton;
        private int _chinGen;

        public StageMotorPanel()
        {
            _swingChanged = new StepMotorPositionChanged(OnSwingPositionChanged);
            _xStageChanged = new StepMotorPositionChanged(OnXstagePositionChanged);
            _yStageChanged = new StepMotorPositionChanged(OnYstagePositionChanged);
            _zStageChanged = new StepMotorPositionChanged(OnZstagePositionChanged);

            _badgeTimer = new DispatcherTimer(DispatcherPriority.Background) { Interval = TimeSpan.FromMilliseconds(250) };
            _badgeTimer.Tick += BadgeTimer_Tick;

            InitializeComponent();
        }

        private void UserControl_Loaded(object sender, RoutedEventArgs e)
        {
            if (_isActive)
            {
                return;
            }

            FetchAllMotorStatus();

            configureSlider_(MotorType.Swing, _msSwing);
            configureSlider_(MotorType.StageX, _msXstage);
            configureSlider_(MotorType.StageY, _msYstage);
            configureSlider_(MotorType.StageZ, _msZstage);
            updateStepLabels_();

            ConnectMotorCallbacks();
            _isActive = true;

            // Esc reaches STOP ALL even when another handler (slit measure, Zoom Area) marked it
            // handled; this handler never sets Handled so those consumers keep working.
            _host = Window.GetWindow(this);
            if (_host != null)
            {
                _host.AddHandler(UIElement.PreviewKeyDownEvent, new KeyEventHandler(onHostKey_), true);
                _host.Deactivated += Host_Deactivated;
            }

            StageJog.HoldTimedOut += onHoldTimedOut_;
            StageJog.StopUnconfirmed += onStopUnconfirmed_;
            StageJog.HoldStopFailed += onHoldStopFailed_;
            _badgeTimer.Start();
        }

        private void UserControl_Unloaded(object sender, RoutedEventArgs e)
        {
            if (!_isActive)
            {
                return;
            }

            _isActive = false;
            _badgeTimer.Stop();
            StageJog.HoldTimedOut -= onHoldTimedOut_;
            StageJog.StopUnconfirmed -= onStopUnconfirmed_;
            StageJog.HoldStopFailed -= onHoldStopFailed_;
            if (_host != null)
            {
                _host.RemoveHandler(UIElement.PreviewKeyDownEvent, new KeyEventHandler(onHostKey_));
                _host.Deactivated -= Host_Deactivated;
                _host = null;
            }

            endPadPress_();
            endChinrestHold_();

            // Stop first while callbacks are still connected, then release them.
            StopAllMotors();
            ReleaseMotorCallbacks();
        }

        private void UserControl_IsEnabledChanged(object sender, DependencyPropertyChangedEventArgs e)
        {
            if (!(bool)e.NewValue)
            {
                endPadPress_();
                endChinrestHold_();
            }
        }

        private void Host_Deactivated(object? sender, EventArgs e)
        {
            endPadPress_();
            endChinrestHold_();
        }

        private void configureSlider_(MotorType type, StepMotorStatus status)
        {
            var (slider, edit, _) = controlsFor_(type);
            slider.Minimum = status.rangeMin;
            slider.Maximum = status.rangeMax;
            slider.TickFrequency = 10;
            slider.Value = status.currPos;
            edit.Text = status.currPos.ToString();
            setReadout_(type, status.currPos);
        }

        private void FetchAllMotorStatus()
        {
            DeviceMotors.FetchStepMotorStatus(MotorType.Swing, out _msSwing);
            DeviceMotors.FetchStepMotorStatus(MotorType.StageX, out _msXstage);
            DeviceMotors.FetchStepMotorStatus(MotorType.StageY, out _msYstage);
            DeviceMotors.FetchStepMotorStatus(MotorType.StageZ, out _msZstage);
        }

        private void ConnectMotorCallbacks()
        {
            DeviceMotors.ConnectStepMotorPositionChanged(MotorType.Swing, _swingChanged);
            DeviceMotors.ConnectStepMotorPositionChanged(MotorType.StageX, _xStageChanged);
            DeviceMotors.ConnectStepMotorPositionChanged(MotorType.StageY, _yStageChanged);
            DeviceMotors.ConnectStepMotorPositionChanged(MotorType.StageZ, _zStageChanged);
        }

        private static void ReleaseMotorCallbacks()
        {
            DeviceMotors.ReleaseStepMotorPositionChanged(MotorType.Swing);
            DeviceMotors.ReleaseStepMotorPositionChanged(MotorType.StageX);
            DeviceMotors.ReleaseStepMotorPositionChanged(MotorType.StageY);
            DeviceMotors.ReleaseStepMotorPositionChanged(MotorType.StageZ);
        }

        private static void StopAllMotors()
        {
            DeviceMotors.StopStepMotor(MotorType.Swing);
            DeviceMotors.StopStepMotor(MotorType.StageX);
            DeviceMotors.StopStepMotor(MotorType.StageY);
            DeviceMotors.StopStepMotor(MotorType.StageZ);
            AlignStage.StopChinrest();
        }

        #region Jog tab

        private void updateStepLabels_()
        {
            rbJogFine.Content = $"Fine ({stepText_(LsoJogSize.Fine)})";
            rbJogMed.Content = $"Med ({stepText_(LsoJogSize.Med)})";
            rbJogCoarse.Content = $"Coarse ({stepText_(LsoJogSize.Coarse)})";
        }

        private static string stepText_(LsoJogSize size)
        {
            int x = StageJog.StepFor(JogAxis.X, size);
            int y = StageJog.StepFor(JogAxis.Y, size);
            int z = StageJog.StepFor(JogAxis.Z, size);
            return x == z ? $"X/Z {x}, Y {y}" : $"X {x}, Y {y}, Z {z}";
        }

        private LsoJogSize selectedSize_()
        {
            if (rbJogFine.IsChecked == true)
            {
                return LsoJogSize.Fine;
            }
            return rbJogMed.IsChecked == true ? LsoJogSize.Med : LsoJogSize.Coarse;
        }

        private (JogAxis Axis, int Direction) padAxis_(RepeatButton button)
        {
            if (ReferenceEquals(button, btnJogXplus)) return (JogAxis.X, 1);
            if (ReferenceEquals(button, btnJogXminus)) return (JogAxis.X, -1);
            if (ReferenceEquals(button, btnJogYplus)) return (JogAxis.Y, 1);
            if (ReferenceEquals(button, btnJogYminus)) return (JogAxis.Y, -1);
            if (ReferenceEquals(button, btnJogZplus)) return (JogAxis.Z, 1);
            if (ReferenceEquals(button, btnJogZminus)) return (JogAxis.Z, -1);
            throw new ArgumentOutOfRangeException(nameof(button));
        }

        // Runs before RepeatButton raises its first Click, so the press count starts at zero.
        private void PadButton_PreviewMouseLeftButtonDown(object sender, MouseButtonEventArgs e)
        {
            _padButton = (RepeatButton)sender;
            _padClicks = 0;
        }

        private void PadButton_Click(object sender, RoutedEventArgs e)
        {
            var button = (RepeatButton)sender;
            if (ReferenceEquals(button, _padButton))
            {
                _padClicks++;
            }

            var (axis, direction) = padAxis_(button);
            LsoJogSize size = selectedSize_();
            if (StageJog.TryStep(axis, size, direction, out int sent, out string refusal))
            {
                int nominal = StageJog.StepFor(axis, size);
                string clampNote = Math.Abs(sent) == nominal ? string.Empty : $" (limit, step {nominal})";
                setStatus_($"{button.Tag} {Math.Abs(sent)} sent{clampNote}", false);
            }
            else if (!(_padClicks >= 2 && refusal == "busy"))
            {
                // While auto-repeating, "busy" only means the previous step is still running.
                setStatus_($"{button.Tag} refused: {refusal}", true);
            }
        }

        private void PadButton_PreviewMouseLeftButtonUp(object sender, MouseButtonEventArgs e)
        {
            endPadPress_();
        }

        private void PadButton_LostMouseCapture(object sender, MouseEventArgs e)
        {
            endPadPress_();
        }

        // A single click's step always completes (it is bounded); only a press that auto-repeated
        // is stopped on release, so holding never runs past the release.
        private void endPadPress_()
        {
            RepeatButton? button = _padButton;
            int clicks = _padClicks;
            _padButton = null;
            _padClicks = 0;
            if (button == null)
            {
                return;
            }

            if (button.IsMouseCaptured)
            {
                button.ReleaseMouseCapture();
            }

            if (clicks >= 2)
            {
                var (axis, _) = padAxis_(button);
                StageJog.StopAxis(axis);
                setStatus_($"{button.Tag} released: {axis} stopped", false);
            }
        }

        private void BtnStopAll_Click(object sender, RoutedEventArgs e)
        {
            stopAll_("STOP ALL");
        }

        private void onHostKey_(object sender, KeyEventArgs e)
        {
            if (e.Key == Key.Escape)
            {
                stopAll_("STOP ALL (Esc)");
            }
        }

        private void stopAll_(string what)
        {
            // End the repeat first so no further Click can queue a new step after the stop.
            endPadPress_();
            IReadOnlyList<string> failed = StageJog.StopAll();
            if (failed.Count == 0)
            {
                setStatus_($"{what} sent", false);
            }
            else
            {
                setStatus_($"{what} sent, returned false: {string.Join(", ", failed)} - press STOP ALL again", true);
            }
        }

        private void onHoldTimedOut_(string owner)
        {
            Dispatcher.BeginInvoke(() => setStatus_($"{owner} held over {StageJog.HoldWatchdogMs / 1000} s: stopped all", true));
        }

        private void onStopUnconfirmed_(string motors)
        {
            Dispatcher.BeginInvoke(() => setStatus_($"stop not confirmed: {motors} still moving (stop sent again)", true));
        }

        private void onHoldStopFailed_(string owner, string message)
        {
            Dispatcher.BeginInvoke(() => setStatus_($"{owner} stop failed: {message} - press STOP ALL", true));
        }

        private void Chinrest_PreviewMouseLeftButtonDown(object sender, MouseButtonEventArgs e)
        {
            var button = (Button)sender;
            bool up = ReferenceEquals(button, btnJogChinUp) || ReferenceEquals(button, btnChinrestUp);
            string dir = up ? "up" : "down";

            if (_chinButton != null)
            {
                e.Handled = true;
                return;
            }

            if (LsoCamera.IsCapturing())
            {
                setStatus_($"chinrest {dir} refused: capture running", true);
                e.Handled = true;
                return;
            }

            if (!button.CaptureMouse())
            {
                setStatus_($"chinrest {dir} refused: no mouse capture", true);
                e.Handled = true;
                return;
            }

            int gen = ++_chinGen;
            _chinButton = button;
            HoldStartResult result = StageJog.StartVelocityHold(
                ChinrestOwner,
                () => stopChinrestHold_(gen),
                () => up ? AlignStage.MoveChinrestUp() : AlignStage.MoveChinrestDown(),
                AlignStage.StopChinrest);
            Console.WriteLine($"[LSO-JOG] chinrest {dir} {result}");
            switch (result)
            {
                case HoldStartResult.Started:
                    setStatus_($"chinrest {dir} (release to stop)", false);
                    return;
                case HoldStartResult.Busy:
                    _chinButton = null;
                    button.ReleaseMouseCapture();
                    setStatus_($"chinrest {dir} refused: busy", true);
                    break;
                case HoldStartResult.Rejected:
                    endChinrestHold_();
                    setStatus_($"chinrest {dir} rejected", true);
                    break;
                default:
                    // The stop that removed the hold may have preceded the move; the second stop's result
                    // is the one that counts, so the reset that stop posted is dropped (generation bump).
                    _chinGen++;
                    _chinButton = null;
                    if (button.IsMouseCaptured)
                    {
                        button.ReleaseMouseCapture();
                    }
                    bool stopped = result == HoldStartResult.RacedStopped;
                    setStatus_(stopped ? $"chinrest {dir} stopped by STOP ALL" : "chinrest STOP FAILED - press STOP ALL", !stopped);
                    break;
            }
            e.Handled = true;
        }

        private void Chinrest_PreviewMouseLeftButtonUp(object sender, MouseButtonEventArgs e)
        {
            if (ReferenceEquals(sender, _chinButton))
            {
                endChinrestHold_();
            }
        }

        private void Chinrest_LostMouseCapture(object sender, MouseEventArgs e)
        {
            if (ReferenceEquals(sender, _chinButton))
            {
                endChinrestHold_();
            }
        }

        private void endChinrestHold_()
        {
            Button? button = _chinButton;
            if (button == null)
            {
                return;
            }

            _chinButton = null;
            StageJog.EndVelocityHold(ChinrestOwner);
            if (button.IsMouseCaptured)
            {
                button.ReleaseMouseCapture();
            }
        }

        // The hold's stop: device call only, then a UI reset posted to the dispatcher. Runs on the UI
        // thread (release paths, STOP ALL) or on the StageJog watchdog thread.
        private void stopChinrestHold_(int gen)
        {
            bool stopped = AlignStage.StopChinrest();
            Console.WriteLine(stopped ? "[LSO-JOG] chinrest stop" : "[LSO-JOG] chinrest stop returned false");
            Dispatcher.BeginInvoke(() =>
            {
                if (gen != _chinGen)
                {
                    return;
                }

                Button? button = _chinButton;
                _chinButton = null;
                if (button != null && button.IsMouseCaptured)
                {
                    button.ReleaseMouseCapture();
                }
                setStatus_(stopped ? "chinrest stopped" : "chinrest stop returned false", !stopped);
            });
        }

        private void BadgeTimer_Tick(object? sender, EventArgs e)
        {
            updateBadge_(JogAxis.X, bdgJogX, txtJogXBadge, () => AlignStage.IsHeadAtLeftEnd() || AlignStage.IsHeadAtRightEnd());
            updateBadge_(JogAxis.Y, bdgJogY, txtJogYBadge, () => AlignStage.IsHeadAtLowerEnd() || AlignStage.IsHeadAtUpperEnd());
            updateBadge_(JogAxis.Z, bdgJogZ, txtJogZBadge, () => AlignStage.IsHeadAtBackwardEnd() || AlignStage.IsHeadAtForwardEnd());
        }

        private void updateBadge_(JogAxis axis, Border badge, TextBlock text, Func<bool> atLimit)
        {
            MotorType type = StageJog.ToMotor(axis);
            bool moving = DeviceMotors.IsStepMotorMoving(type);
            bool held = StageJog.IsAxisHeld(axis);
            int i = (int)axis;
            if (held)
            {
                _holdRefreshTicks[i] = HoldRefreshTicksAfterEnd;
            }
            if (_wasMoving[i] && !moving)
            {
                refreshPosition_(type);
            }
            else if (_holdRefreshTicks[i] > 0 && !moving)
            {
                // The stage may still decelerate after the hold's stop, so the read repeats after the end.
                refreshPosition_(type);
            }
            if (!held && _holdRefreshTicks[i] > 0)
            {
                _holdRefreshTicks[i]--;
            }
            _wasMoving[i] = moving;

            string label;
            Brush brush;
            if (moving)
            {
                (label, brush) = ("moving", BadgeMovingBrush);
            }
            else if (!StageJog.IsAxisFree(axis, out string refusal) && refusal == "busy")
            {
                (label, brush) = ("busy", BadgeBusyBrush);
            }
            else if (atLimit())
            {
                (label, brush) = ("limit", BadgeLimitBrush);
            }
            else
            {
                (label, brush) = (string.Empty, Brushes.Transparent);
            }

            text.Text = label;
            badge.Background = brush;
        }

        private void refreshPosition_(MotorType type)
        {
            if (DeviceMotors.FetchStepMotorStatus(type, out StepMotorStatus status))
            {
                applyPosition_(type, status.currPos);
            }
        }

        private void setReadout_(MotorType type, int pos)
        {
            _lastPos[type] = pos;
            TextBlock? readout = type switch
            {
                MotorType.StageX => txtJogXPos,
                MotorType.StageY => txtJogYPos,
                MotorType.StageZ => txtJogZPos,
                _ => null,
            };
            if (readout != null)
            {
                readout.Text = pos.ToString();
            }
        }

        private void setStatus_(string text, bool isError)
        {
            txtJogStatus.Text = text;
            txtJogStatus.Foreground = isError ? StatusErrorBrush : SystemColors.ControlTextBrush;
        }

        #endregion

        #region Advanced tab

        private (Slider Slider, TextBox Position, TextBox Step) controlsFor_(MotorType type) => type switch
        {
            MotorType.StageX => (sliderXstage, editXstagePos, editXstageValue),
            MotorType.StageY => (sliderYstage, editYstagePos, editYstageValue),
            MotorType.StageZ => (sliderZstage, editZstagePos, editZstageValue),
            MotorType.Swing => (sliderSwing, editSwingPos, editSwingValue),
            _ => throw new ArgumentOutOfRangeException(nameof(type)),
        };

        private static string nameOf_(MotorType type) => type switch
        {
            MotorType.StageX => "X",
            MotorType.StageY => "Y",
            MotorType.StageZ => "Z",
            MotorType.Swing => "Swing",
            _ => type.ToString(),
        };

        // Same interlock as the Jog tab: no new move while capturing or while the axis is busy
        // (moving, or held by a velocity move such as the cornea click). Refusals are shown, not queued.
        private bool canMoveAdvanced_(MotorType type, string action)
        {
            string? refusal = null;
            if (LsoCamera.IsCapturing())
            {
                refusal = "capture running";
            }
            else
            {
                JogAxis? axis = type switch
                {
                    MotorType.StageX => JogAxis.X,
                    MotorType.StageY => JogAxis.Y,
                    MotorType.StageZ => JogAxis.Z,
                    _ => null,
                };
                if (axis is JogAxis a && !StageJog.IsAxisFree(a, out string busy))
                {
                    refusal = busy;
                }
            }

            if (refusal == null)
            {
                return true;
            }

            Console.WriteLine($"[LSO-JOG] advanced {nameOf_(type)} {action} refused: {refusal}");
            setStatus_($"{nameOf_(type)} {action} refused: {refusal}", true);
            return false;
        }

        private void reportAdvanced_(MotorType type, string action, bool ok)
        {
            Console.WriteLine($"[LSO-JOG] advanced {nameOf_(type)} {action} {(ok ? "ok" : "rejected")}");
            setStatus_($"{nameOf_(type)} {action} {(ok ? "sent" : "rejected")}", !ok);
        }

        private void revertSlider_(MotorType type)
        {
            if (!_lastPos.TryGetValue(type, out int pos))
            {
                return;
            }

            var (slider, edit, _) = controlsFor_(type);
            _isUpdatingFromDevice = true;
            try
            {
                slider.Value = pos;
                edit.Text = pos.ToString();
            }
            finally
            {
                _isUpdatingFromDevice = false;
            }
        }

        private void SliderSwing_ValueChanged(object sender, RoutedPropertyChangedEventArgs<double> e)
        {
            moveSlider_(MotorType.Swing, sliderSwing);
        }

        private void SliderXstage_ValueChanged(object sender, RoutedPropertyChangedEventArgs<double> e)
        {
            moveSlider_(MotorType.StageX, sliderXstage);
        }

        private void SliderYstage_ValueChanged(object sender, RoutedPropertyChangedEventArgs<double> e)
        {
            moveSlider_(MotorType.StageY, sliderYstage);
        }

        private void SliderZstage_ValueChanged(object sender, RoutedPropertyChangedEventArgs<double> e)
        {
            moveSlider_(MotorType.StageZ, sliderZstage);
        }

        private void moveSlider_(MotorType type, Slider slider)
        {
            if (_isActive && !_isDraggingStarted && !_isUpdatingFromDevice)
            {
                moveToSlider_(type, slider);
            }
        }

        private void moveToSlider_(MotorType type, Slider slider)
        {
            int target = (int)slider.Value;
            string action = $"to {target}";
            if (!canMoveAdvanced_(type, action))
            {
                revertSlider_(type);
                return;
            }

            bool ok = DeviceMotors.MoveStepMotorPositionAsync(type, target);
            reportAdvanced_(type, action, ok);
            if (!ok)
            {
                revertSlider_(type);
            }
        }

        private void SliderSwing_DragStarted(object sender, DragStartedEventArgs e)
        {
            _isDraggingStarted = true;
        }

        private void SliderXstage_DragStarted(object sender, DragStartedEventArgs e)
        {
            _isDraggingStarted = true;
        }

        private void SliderYstage_DragStarted(object sender, DragStartedEventArgs e)
        {
            _isDraggingStarted = true;
        }

        private void SliderZstage_DragStarted(object sender, DragStartedEventArgs e)
        {
            _isDraggingStarted = true;
        }

        private void SliderSwing_DragCompleted(object sender, DragCompletedEventArgs e)
        {
            completeDrag_(MotorType.Swing, sliderSwing);
        }

        private void SliderXstage_DragCompleted(object sender, DragCompletedEventArgs e)
        {
            completeDrag_(MotorType.StageX, sliderXstage);
        }

        private void SliderYstage_DragCompleted(object sender, DragCompletedEventArgs e)
        {
            completeDrag_(MotorType.StageY, sliderYstage);
        }

        private void SliderZstage_DragCompleted(object sender, DragCompletedEventArgs e)
        {
            completeDrag_(MotorType.StageZ, sliderZstage);
        }

        private void completeDrag_(MotorType type, Slider slider)
        {
            _isDraggingStarted = false;
            moveToSlider_(type, slider);
        }

        private void BtnSwingOrigin_Click(object sender, RoutedEventArgs e)
        {
            moveToOrigin_(MotorType.Swing);
        }

        private void BtnXstageOrigin_Click(object sender, RoutedEventArgs e)
        {
            moveToOrigin_(MotorType.StageX);
        }

        private void BtnYstageOrigin_Click(object sender, RoutedEventArgs e)
        {
            moveToOrigin_(MotorType.StageY);
        }

        private void BtnZstageOrigin_Click(object sender, RoutedEventArgs e)
        {
            moveToOrigin_(MotorType.StageZ);
        }

        private void moveToOrigin_(MotorType type)
        {
            if (!MsgBoxUtil.ShowQuestion($"Move {nameOf_(type)} to its origin?"))
            {
                return;
            }

            // Checked after the dialog: capture or another move may have started while it was open.
            if (canMoveAdvanced_(type, "origin"))
            {
                reportAdvanced_(type, "origin", DeviceMotors.MoveStepMotorToOriginAsync(type));
            }
        }

        private void BtnSwingBwd_Click(object sender, RoutedEventArgs e)
        {
            moveBy_(MotorType.Swing, -1);
        }

        private void BtnSwingFwd_Click(object sender, RoutedEventArgs e)
        {
            moveBy_(MotorType.Swing, 1);
        }

        private void BtnXstageBwd_Click(object sender, RoutedEventArgs e)
        {
            moveBy_(MotorType.StageX, -1);
        }

        private void BtnXstageFwd_Click(object sender, RoutedEventArgs e)
        {
            moveBy_(MotorType.StageX, 1);
        }

        private void BtnYstageBwd_Click(object sender, RoutedEventArgs e)
        {
            moveBy_(MotorType.StageY, -1);
        }

        private void BtnYstageFwd_Click(object sender, RoutedEventArgs e)
        {
            moveBy_(MotorType.StageY, 1);
        }

        private void BtnZstageBwd_Click(object sender, RoutedEventArgs e)
        {
            moveBy_(MotorType.StageZ, -1);
        }

        private void BtnZstageFwd_Click(object sender, RoutedEventArgs e)
        {
            moveBy_(MotorType.StageZ, 1);
        }

        // Legacy step box kept on purpose: empty or unparsable = 10, clamped to 10..1000.
        private void moveBy_(MotorType type, int direction)
        {
            int step = Math.Clamp(ToInt(controlsFor_(type).Step.Text), 10, 1000) * direction;
            string action = $"{step:+0;-0}";
            if (canMoveAdvanced_(type, action))
            {
                reportAdvanced_(type, action, DeviceMotors.MoveStepMotorByPositionOffsetAsync(type, step));
            }
        }

        private void EditSwingPos_KeyDown(object sender, KeyEventArgs e)
        {
            setPositionOnEnter_(e, MotorType.Swing);
        }

        private void EditXstagePos_KeyDown(object sender, KeyEventArgs e)
        {
            setPositionOnEnter_(e, MotorType.StageX);
        }

        private void EditYstagePos_KeyDown(object sender, KeyEventArgs e)
        {
            setPositionOnEnter_(e, MotorType.StageY);
        }

        private void EditZstagePos_KeyDown(object sender, KeyEventArgs e)
        {
            setPositionOnEnter_(e, MotorType.StageZ);
        }

        // Setting the slider sends the move through moveSlider_, which applies the interlock.
        private void setPositionOnEnter_(KeyEventArgs e, MotorType type)
        {
            if (e.Key != Key.Enter)
            {
                return;
            }

            var (slider, edit, _) = controlsFor_(type);
            if (!int.TryParse(edit.Text.Trim(), out int value) || value < slider.Minimum || value > slider.Maximum)
            {
                setStatus_($"{nameOf_(type)} position must be a whole number {slider.Minimum}..{slider.Maximum}", true);
                return;
            }

            slider.Value = value;
        }

        private void BtnChinrestStop_Click(object sender, RoutedEventArgs e)
        {
            StageJog.EndVelocityHold(ChinrestOwner);
            bool stopped = AlignStage.StopChinrest();
            Console.WriteLine(stopped ? "[LSO-JOG] chinrest stop" : "[LSO-JOG] chinrest stop returned false");
            setStatus_(stopped ? "chinrest stopped" : "chinrest stop returned false", !stopped);
        }

        #endregion

        private void OnSwingPositionChanged(int pos, float value)
        {
            updatePosition_(MotorType.Swing, pos);
        }

        private void OnXstagePositionChanged(int pos, float value)
        {
            updatePosition_(MotorType.StageX, pos);
        }

        private void OnYstagePositionChanged(int pos, float value)
        {
            updatePosition_(MotorType.StageY, pos);
        }

        private void OnZstagePositionChanged(int pos, float value)
        {
            updatePosition_(MotorType.StageZ, pos);
        }

        private void updatePosition_(MotorType type, int pos)
        {
            if (!_isActive)
            {
                return;
            }

            Dispatcher.BeginInvoke(() =>
            {
                if (!_isActive)
                {
                    return;
                }

                applyPosition_(type, pos);
            }, DispatcherPriority.Normal);
        }

        private void applyPosition_(MotorType type, int pos)
        {
            var (slider, edit, _) = controlsFor_(type);
            _isUpdatingFromDevice = true;
            try
            {
                edit.Text = pos.ToString();
                if (pos != slider.Value)
                {
                    slider.Value = pos;
                }
                setReadout_(type, pos);
            }
            finally
            {
                _isUpdatingFromDevice = false;
            }
        }
    }
}
