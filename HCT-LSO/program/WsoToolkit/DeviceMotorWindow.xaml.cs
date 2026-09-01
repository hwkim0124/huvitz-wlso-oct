using System;
using System.Collections.Generic;
using System.Text;
using System.Threading;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Shapes;
using WsoNativeLib;

namespace WsoToolkit
{
    using static utils.MsgBoxUtil;
    using static utils.NumberUtil;
    using static WsoNativeLib.WsoDevice;

    public partial class DeviceMotorWindow : Window
    {
        bool _isDraggingStarted = false;
        bool _isPositionChanged = false;


        public DeviceMotorWindow()
        {
            InitializeComponent();

            _octFocusChanged = new StepMotorPositionChanged(this.OnOctFocusPositionChanged);
            _octReferChanged = new StepMotorPositionChanged(this.OnOctReferPositionChanged);
            _octPolarChanged = new StepMotorPositionChanged(this.OnOctPolarPositionChanged);
            _octRefNdChanged = new StepMotorPositionChanged(this.OnOctRefNdPositionChanged);
            _lsoFocusChanged = new StepMotorPositionChanged(this.OnLsoFocusPositionChanged);
            _retMirrorChanged = new StepMotorPositionChanged(this.OnRetMirrorPositionChanged);
            _octAntLensChanged = new StepMotorPositionChanged(this.OnOctAntLensPositionChanged);
            _lsoFilterChanged = new StepMotorPositionChanged(this.OnLsoFilterPositionChanged);
            _swingChanged = new StepMotorPositionChanged(this.OnSwingPositionChanged);
            _xStageChanged = new StepMotorPositionChanged(this.OnXstagePositionChanged);
            _yStageChanged = new StepMotorPositionChanged(this.OnYstagePositionChanged);
            _zStageChanged = new StepMotorPositionChanged(this.OnZstagePositionChanged);
        }

        private void Window_Initialized(object sender, EventArgs e)
        {
            FetchAllMotorStatus();
        }

        private void Window_Loaded(object sender, RoutedEventArgs e)
        {
            sliderOctFocus.Minimum = _msOctFocus.rangeMin;
            sliderOctFocus.Maximum = _msOctFocus.rangeMax;
            sliderOctFocus.TickFrequency = 100;

            sliderLsoFocus.Minimum = _msLsoFocus.rangeMin;
            sliderLsoFocus.Maximum = _msLsoFocus.rangeMax;
            sliderLsoFocus.TickFrequency = 100;

            sliderOctRefer.Minimum = _msOctRefer.rangeMin;
            sliderOctRefer.Maximum = _msOctRefer.rangeMax;
            sliderOctRefer.TickFrequency = 100;
            editOctReferStep.Text = "100";

            sliderOctPolar.Minimum = _msOctPolar.rangeMin;
            sliderOctPolar.Maximum = _msOctPolar.rangeMax;
            sliderOctPolar.TickFrequency = 50;

            sliderOctRefNd.Minimum = _msOctRefNd.rangeMin;
            sliderOctRefNd.Maximum = _msOctRefNd.rangeMax;
            sliderOctRefNd.TickFrequency = 10;

            sliderRetMirror.Minimum = _msRetMirror.rangeMin;
            sliderRetMirror.Maximum = _msRetMirror.rangeMax;
            sliderRetMirror.TickFrequency = 10;

            sliderOctAntLens.Minimum = _msOctAntLens.rangeMin;
            sliderOctAntLens.Maximum = _msOctAntLens.rangeMax;
            sliderOctAntLens.TickFrequency = 10;

            sliderLsoFilter.Minimum = _msLsoFilter.rangeMin;
            sliderLsoFilter.Maximum = _msLsoFilter.rangeMax;
            sliderLsoFilter.TickFrequency = 10;

            cmbRetMirror.ItemsSource = new string[] { "IN", "OUT" };
            cmbRetMirror.SelectedIndex = 0;
            cmbLsoFilter.ItemsSource = new string[] { "Color", "Blue", "Green", "ICGA" };
            cmbLsoFilter.SelectedIndex = 0;
            cmbOctRefNd.ItemsSource = new string[] { "IN", "OUT" };
            cmbOctRefNd.SelectedIndex = 0;
            cmbOctAntLens.ItemsSource = new string[] { "IN", "OUT" };
            cmbOctAntLens.SelectedIndex = 0;

            sliderSwing.Minimum = _msSwing.rangeMin;
            sliderSwing.Maximum = _msSwing.rangeMax;
            sliderSwing.TickFrequency = 10;

            sliderXstage.Minimum = _msXstage.rangeMin;
            sliderXstage.Maximum = _msXstage.rangeMax;
            sliderXstage.TickFrequency = 10;

            sliderYstage.Minimum = _msYstage.rangeMin;
            sliderYstage.Maximum = _msYstage.rangeMax;
            sliderYstage.TickFrequency = 10;

            sliderZstage.Minimum = _msZstage.rangeMin;
            sliderZstage.Maximum = _msZstage.rangeMax;
            sliderZstage.TickFrequency = 10;

            ConnectMotorCallbacks();

            DeviceMotors.MoveStepMotorPositionAsync(MotorType.OctFocus, _msOctFocus.currPos);
            DeviceMotors.MoveStepMotorPositionAsync(MotorType.LsoFocus, _msLsoFocus.currPos);
            DeviceMotors.MoveStepMotorPositionAsync(MotorType.OctRefer, _msOctRefer.currPos);
            DeviceMotors.MoveStepMotorPositionAsync(MotorType.OctPolar, _msOctPolar.currPos);
            DeviceMotors.MoveStepMotorPositionAsync(MotorType.RetMirror, _msRetMirror.currPos);
            DeviceMotors.MoveStepMotorPositionAsync(MotorType.OctRefNd, _msOctRefNd.currPos);
            DeviceMotors.MoveStepMotorPositionAsync(MotorType.OctAntLens, _msOctAntLens.currPos);
            DeviceMotors.MoveStepMotorPositionAsync(MotorType.LsoFilter, _msLsoFilter.currPos);
            DeviceMotors.MoveStepMotorPositionAsync(MotorType.Swing, _msSwing.currPos);
            DeviceMotors.MoveStepMotorPositionAsync(MotorType.StageX, _msXstage.currPos);
            DeviceMotors.MoveStepMotorPositionAsync(MotorType.StageY, _msYstage.currPos);
            DeviceMotors.MoveStepMotorPositionAsync(MotorType.StageZ, _msZstage.currPos);

            StartRefreshTimer();
        }

        private void Window_Closed(object sender, EventArgs e)
        {
            // Stop the refresh tick first: it polls the motors every 100 ms.
            _timer.Stop();
            _timer.Tick -= TimerRefreshTick;

            StopAllMotors();

            // Safe with moves still in flight. CallbackRegistry guards the set and the
            // invoke with one mutex, so once this returns no further position callback
            // can fire and the remaining native calls just unwind on their own.
            ReleaseMotorCallbacks();
        }

        private void Window_Closing(object sender, System.ComponentModel.CancelEventArgs e)
        {
            if (ShowQuestion("Are you sure to close this window?") == false)
            {
                e.Cancel = true;
            }
        }

        private void BtnClose_Click(object sender, RoutedEventArgs e)
        {
            Close();
        }


        private void SliderOctFocus_DragStarted(object sender, System.Windows.Controls.Primitives.DragStartedEventArgs e)
        {
            _isDraggingStarted = true;
        }

        private void SliderOctFocus_ValueChanged(object sender, RoutedPropertyChangedEventArgs<double> e)
        {
            if (!_isDraggingStarted && !_isPositionChanged)
            {
                DeviceMotors.MoveStepMotorPositionAsync(MotorType.OctFocus, (int)sliderOctFocus.Value);
            }
            _isPositionChanged = false;
        }

        private void SliderOctFocus_DragCompleted(object sender, System.Windows.Controls.Primitives.DragCompletedEventArgs e)
        {
            _isDraggingStarted = false;
            DeviceMotors.MoveStepMotorPositionAsync(MotorType.OctFocus, (int)sliderOctFocus.Value);
        }

        private void EditOctFocusPos_KeyDown(object sender, KeyEventArgs e)
        {
            if (e.Key == Key.Enter)
            {
                int value = ToInt(editOctFocusPos.Text);
                DeviceMotors.MoveStepMotorPositionAsync(MotorType.OctFocus, value);
            }
        }

        private void EditOctFocusValue_KeyDown(object sender, KeyEventArgs e)
        {
            if (e.Key == Key.Enter)
            {
                float diopt = ToFloat(editOctFocusValue.Text);
                DeviceMotors.MoveStepMotorByValueAsync(MotorType.OctFocus, diopt);
            }
        }

        private void BtnOctFocusFwd_Click(object sender, RoutedEventArgs e)
        {
            DeviceMotors.MoveStepMotorByValueOffsetAsync(MotorType.OctFocus, +1.0f);
        }

        private void BtnOctFocusBwd_Click(object sender, RoutedEventArgs e)
        {
            DeviceMotors.MoveStepMotorByValueOffsetAsync(MotorType.OctFocus, -1.0f);
        }

        private void BtnOctFocusSetAs_Click(object sender, RoutedEventArgs e)
        {
            DeviceMotors.SetStepMotorCurrentPositionAsOrigin(MotorType.OctFocus);
        }

        private void BtnOctFocusOrigin_Click(object sender, RoutedEventArgs e)
        {
            DeviceMotors.MoveStepMotorToOriginAsync(MotorType.OctFocus);
        }

        private void SliderLsoFocus_DragStarted(object sender, System.Windows.Controls.Primitives.DragStartedEventArgs e)
        {
            _isDraggingStarted = true;
        }

        private void SliderLsoFocus_DragCompleted(object sender, System.Windows.Controls.Primitives.DragCompletedEventArgs e)
        {
            _isDraggingStarted = false;
            DeviceMotors.MoveStepMotorPositionAsync(MotorType.LsoFocus, (int)sliderLsoFocus.Value);
        }

        private void SliderLsoFocus_ValueChanged(object sender, RoutedPropertyChangedEventArgs<double> e)
        {
            if (!_isDraggingStarted && !_isPositionChanged)
            {
                DeviceMotors.MoveStepMotorPositionAsync(MotorType.LsoFocus, (int)sliderLsoFocus.Value);
            }
            _isPositionChanged = false;
        }

        private void EditLsoFocusValue_KeyDown(object sender, KeyEventArgs e)
        {
            if (e.Key == Key.Enter)
            {
                float diopt = ToFloat(editLsoFocusValue.Text);
                DeviceMotors.MoveStepMotorByValueAsync(MotorType.LsoFocus, diopt);
            }
        }

        private void BtnLsoFocusFwd_Click(object sender, RoutedEventArgs e)
        {
            DeviceMotors.MoveStepMotorByValueOffsetAsync(MotorType.LsoFocus, +1.0f);
        }

        private void BtnLsoFocusBwd_Click(object sender, RoutedEventArgs e)
        {
            DeviceMotors.MoveStepMotorByValueOffsetAsync(MotorType.LsoFocus, -1.0f);
        }

        private void BtnLsoFocusSetAs_Click(object sender, RoutedEventArgs e)
        {
            DeviceMotors.SetStepMotorCurrentPositionAsOrigin(MotorType.LsoFocus);
        }

        private void BtnLsoFocusOrigin_Click(object sender, RoutedEventArgs e)
        {
            DeviceMotors.MoveStepMotorToOriginAsync(MotorType.LsoFocus);
        }

        private void EditLsoFocusPos_KeyDown(object sender, KeyEventArgs e)
        {
            if (e.Key == Key.Enter)
            {
                int value = ToInt(editLsoFocusPos.Text);
                DeviceMotors.MoveStepMotorPositionAsync(MotorType.LsoFocus, value);
            }
        }

        private void SliderOctRefer_DragCompleted(object sender, System.Windows.Controls.Primitives.DragCompletedEventArgs e)
        {
            _isDraggingStarted = false;
            DeviceMotors.MoveStepMotorPositionAsync(MotorType.OctRefer, (int)sliderOctRefer.Value);
        }

        private void SliderOctRefer_DragStarted(object sender, System.Windows.Controls.Primitives.DragStartedEventArgs e)
        {
            _isDraggingStarted = true;
        }

        private void SliderOctRefer_ValueChanged(object sender, RoutedPropertyChangedEventArgs<double> e)
        {
            if (!_isDraggingStarted && !_isPositionChanged)
            {
                DeviceMotors.MoveStepMotorPositionAsync(MotorType.OctRefer, (int)sliderOctRefer.Value);
            }
            _isPositionChanged = false;
        }

        private void EditOctReferPos_KeyDown(object sender, KeyEventArgs e)
        {
            if (e.Key == Key.Enter)
            {
                int value = ToInt(editOctReferPos.Text);
                DeviceMotors.MoveStepMotorPositionAsync(MotorType.OctRefer, value);
            }
        }

        private void EditOctReferStep_KeyDown(object sender, KeyEventArgs e)
        {

        }

        private void BtnOctReferFwd_Click(object sender, RoutedEventArgs e)
        {
            int step = ToInt(editOctReferStep.Text);
            step = Math.Max(Math.Min(step, 1000), 10) * -1;
            DeviceMotors.MoveStepMotorByPositionOffsetAsync(MotorType.OctRefer, step);
        }

        private void BtnOctReferBwd_Click(object sender, RoutedEventArgs e)
        {
            int step = ToInt(editOctReferStep.Text);
            step = Math.Max(Math.Min(step, 1000), 10) * +1;
            DeviceMotors.MoveStepMotorByPositionOffsetAsync(MotorType.OctRefer, step);
        }

        private void BtnOctReferSetAs_Click(object sender, RoutedEventArgs e)
        {
            int mode = checkOctReferCornea.IsChecked == true ? 1 : 0;
            DeviceMotors.SetStepMotorCurrentPositionAsOrigin(MotorType.OctRefer, mode);
        }

        private void BtnOctReferOrigin_Click(object sender, RoutedEventArgs e)
        {
            var mode = checkOctReferCornea.IsChecked == true ? OctReferMotorOrigin.Cornea : OctReferMotorOrigin.Retina;
            DeviceMotors.MoveStepMotorToOriginAsync(MotorType.OctRefer, (int)mode);
        }

        private void CheckOctReferAnterior_Click(object sender, RoutedEventArgs e)
        {

        }

        private void SliderOctPolar_DragCompleted(object sender, System.Windows.Controls.Primitives.DragCompletedEventArgs e)
        {
            _isDraggingStarted = false;
            DeviceMotors.MoveStepMotorPositionAsync(MotorType.OctPolar, (int)sliderOctPolar.Value);
        }

        private void sliderOctPolar_DragStarted(object sender, System.Windows.Controls.Primitives.DragStartedEventArgs e)
        {
            _isDraggingStarted = true;
        }

        private void sliderOctPolar_ValueChanged(object sender, RoutedPropertyChangedEventArgs<double> e)
        {
            if (!_isDraggingStarted && !_isPositionChanged)
            {
                DeviceMotors.MoveStepMotorPositionAsync(MotorType.OctPolar, (int)sliderOctPolar.Value);
            }
            _isPositionChanged = false;
        }

        private void EditOctPolarPos_KeyDown(object sender, KeyEventArgs e)
        {
            if (e.Key == Key.Enter)
            {
                int value = ToInt(editOctPolarPos.Text);
                DeviceMotors.MoveStepMotorPositionAsync(MotorType.OctPolar, value);
            }
        }

        private void BtnOctPolarOrigin_Click(object sender, RoutedEventArgs e)
        {
            DeviceMotors.MoveStepMotorToOriginAsync(MotorType.OctPolar);
        }

        private void BtnOctPolarSetAs_Click(object sender, RoutedEventArgs e)
        {
            DeviceMotors.SetStepMotorCurrentPositionAsOrigin(MotorType.OctPolar);
        }

        private void BtnOctPolarBwd_Click(object sender, RoutedEventArgs e)
        {
            float step = 15;
            DeviceMotors.MoveStepMotorByValueOffsetAsync(MotorType.OctPolar, step);
        }

        private void BtnOctPolarFwd_Click(object sender, RoutedEventArgs e)
        {
            float step = -15;
            DeviceMotors.MoveStepMotorByValueOffsetAsync(MotorType.OctPolar, step);
        }

        private void EditOctPolarValue_KeyDown(object sender, KeyEventArgs e)
        {
            if (e.Key == Key.Enter)
            {
                float value = ToFloat(editOctPolarValue.Text);
                DeviceMotors.MoveStepMotorByValueAsync(MotorType.OctPolar, value);
            }
        }

        private void SliderRetMirror_DragCompleted(object sender, System.Windows.Controls.Primitives.DragCompletedEventArgs e)
        {
            _isDraggingStarted = false;
            DeviceMotors.MoveStepMotorPositionAsync(MotorType.RetMirror, (int)sliderRetMirror.Value);
        }

        private void SliderRetMirror_DragStarted(object sender, System.Windows.Controls.Primitives.DragStartedEventArgs e)
        {
            _isDraggingStarted = true;
        }

        private void SliderRetMirror_ValueChanged(object sender, RoutedPropertyChangedEventArgs<double> e)
        {
            if (!_isDraggingStarted && !_isPositionChanged)
            {
                DeviceMotors.MoveStepMotorPositionAsync(MotorType.RetMirror, (int)sliderRetMirror.Value);
            }
            _isPositionChanged = false;
        }

        private void EditRetMirrorPos_KeyDown(object sender, KeyEventArgs e)
        {
            if (e.Key == Key.Enter)
            {
                int value = ToInt(editRetMirrorPos.Text);
                DeviceMotors.MoveStepMotorPositionAsync(MotorType.RetMirror, value);
            }
        }

        private void CmbRetMirror_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
        }

        private void BtnRetMirrorSetAs_Click(object sender, RoutedEventArgs e)
        {
            int index = cmbRetMirror.SelectedIndex;
            var mode = index == 0 ? MirrorMotorMode.MirrorIn : MirrorMotorMode.MirrorOut;
            DeviceMotors.SetStepMotorCurrentPositionAsOrigin(MotorType.RetMirror, (int)mode);
        }

        private void BtnRetMirrorOut_Click(object sender, RoutedEventArgs e)
        {
            DeviceMotors.MoveStepMotorToOriginAsync(MotorType.RetMirror, 1);
        }

        private void BtnRetMirrorIn_Click(object sender, RoutedEventArgs e)
        {
            DeviceMotors.MoveStepMotorToOriginAsync(MotorType.RetMirror, 0);
        }

        private void SliderOctRefNd_DragCompleted(object sender, System.Windows.Controls.Primitives.DragCompletedEventArgs e)
        {
            _isDraggingStarted = false;
            DeviceMotors.MoveStepMotorPositionAsync(MotorType.OctRefNd, (int)sliderOctRefNd.Value);
        }

        private void sliderOctRefNd_DragStarted(object sender, System.Windows.Controls.Primitives.DragStartedEventArgs e)
        {
            _isDraggingStarted = true;
        }

        private void sliderOctRefNd_ValueChanged(object sender, RoutedPropertyChangedEventArgs<double> e)
        {
            if (!_isDraggingStarted && !_isPositionChanged)
            {
                DeviceMotors.MoveStepMotorPositionAsync(MotorType.OctRefNd, (int)sliderOctRefNd.Value);
            }
            _isPositionChanged = false;
        }

        private void EditOctRefNdPos_KeyDown(object sender, KeyEventArgs e)
        {
            if (e.Key == Key.Enter)
            {
                int value = ToInt(editOctRefNdPos.Text);
                DeviceMotors.MoveStepMotorPositionAsync(MotorType.OctRefNd, value);
            }
        }

        private void BtnOctRefNdSetAs_Click(object sender, RoutedEventArgs e)
        {
            int mode = cmbOctRefNd.SelectedIndex;
            DeviceMotors.SetStepMotorCurrentPositionAsOrigin(MotorType.OctRefNd, mode);
        }

        private void BtnOctRefNdOut_Click(object sender, RoutedEventArgs e)
        {
            DeviceMotors.MoveStepMotorToOriginAsync(MotorType.OctRefNd, 1);
        }

        private void BtnOctRefNdIn_Click(object sender, RoutedEventArgs e)
        {
            DeviceMotors.MoveStepMotorToOriginAsync(MotorType.OctRefNd, 0);
        }

        private void SliderOctAntLens_DragCompleted(object sender, System.Windows.Controls.Primitives.DragCompletedEventArgs e)
        {
            _isDraggingStarted = false;
            DeviceMotors.MoveStepMotorPositionAsync(MotorType.OctAntLens, (int)sliderOctAntLens.Value);
        }

        private void sliderOctAntLens_DragStarted(object sender, System.Windows.Controls.Primitives.DragStartedEventArgs e)
        {
            _isDraggingStarted = true;
        }

        private void sliderOctAntLens_ValueChanged(object sender, RoutedPropertyChangedEventArgs<double> e)
        {
            if (!_isDraggingStarted && !_isPositionChanged)
            {
                DeviceMotors.MoveStepMotorPositionAsync(MotorType.OctAntLens, (int)sliderOctAntLens.Value);
            }
            _isPositionChanged = false;
        }

        private void EditOctAntLensPos_KeyDown(object sender, KeyEventArgs e)
        {
            if (e.Key == Key.Enter)
            {
                int value = ToInt(editOctAntLensPos.Text);
                DeviceMotors.MoveStepMotorPositionAsync(MotorType.OctAntLens, value);
            }
        }

        private void BtnOctAntLensSetAs_Click(object sender, RoutedEventArgs e)
        {
            int index = cmbOctAntLens.SelectedIndex;
            var mode = index == 0 ? MirrorMotorMode.MirrorIn : MirrorMotorMode.MirrorOut;
            DeviceMotors.SetStepMotorCurrentPositionAsOrigin(MotorType.OctAntLens, (int)mode);
        }

        private void BtnOctAntLensOut_Click(object sender, RoutedEventArgs e)
        {
            DeviceMotors.MoveStepMotorToOriginAsync(MotorType.OctAntLens, 1);
        }

        private void BtnOctAntLensIn_Click(object sender, RoutedEventArgs e)
        {
            DeviceMotors.MoveStepMotorToOriginAsync(MotorType.OctAntLens, 0);
        }

        private void SliderLsoFilter_DragCompleted(object sender, System.Windows.Controls.Primitives.DragCompletedEventArgs e)
        {
            _isDraggingStarted = false;
            DeviceMotors.MoveStepMotorPositionAsync(MotorType.LsoFilter, (int)sliderLsoFilter.Value);
        }

        private void sliderLsoFilter_DragStarted(object sender, System.Windows.Controls.Primitives.DragStartedEventArgs e)
        {
            _isDraggingStarted = true;
        }

        private void sliderLsoFilter_ValueChanged(object sender, RoutedPropertyChangedEventArgs<double> e)
        {
            if (!_isDraggingStarted && !_isPositionChanged)
            {
                DeviceMotors.MoveStepMotorPositionAsync(MotorType.LsoFilter, (int)sliderLsoFilter.Value);
            }
            _isPositionChanged = false;
        }

        private void EditLsoFilterPos_KeyDown(object sender, KeyEventArgs e)
        {
            if (e.Key == Key.Enter)
            {
                int value = ToInt(editLsoFilterPos.Text);
                DeviceMotors.MoveStepMotorPositionAsync(MotorType.LsoFilter, value);
            }
        }

        private void CmbLsoFilter_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {

        }

        private void BtnLsoFilterSetAs_Click(object sender, RoutedEventArgs e)
        {
            int index = cmbLsoFilter.SelectedIndex;
            var mode = index == 0 ? LsoFilterMode.Color : index == 1 ? LsoFilterMode.BlueFaf : index == 2 ? LsoFilterMode.GreenFaf : LsoFilterMode.Icga;
            DeviceMotors.SetStepMotorCurrentPositionAsOrigin(MotorType.LsoFilter, (int)mode);
        }

        private void BtnLsoFilterIcga_Click(object sender, RoutedEventArgs e)
        {
            DeviceMotors.MoveStepMotorToOriginAsync(MotorType.LsoFilter, (int)LsoFilterMode.Icga);
        }

        private void BtnLsoFilterGreen_Click(object sender, RoutedEventArgs e)
        {
            DeviceMotors.MoveStepMotorToOriginAsync(MotorType.LsoFilter, (int)LsoFilterMode.GreenFaf);
        }

        private void BtnLsoFilterBlue_Click(object sender, RoutedEventArgs e)
        {
            DeviceMotors.MoveStepMotorToOriginAsync(MotorType.LsoFilter, (int)LsoFilterMode.BlueFaf);
        }

        private void BtnLsoFilterColor_Click(object sender, RoutedEventArgs e)
        {
            DeviceMotors.MoveStepMotorToOriginAsync(MotorType.LsoFilter, (int)LsoFilterMode.Color);
        }

        private void SliderSwing_ValueChanged(object sender, RoutedPropertyChangedEventArgs<double> e)
        {
            if (!_isDraggingStarted && !_isPositionChanged)
            {
                DeviceMotors.MoveStepMotorPositionAsync(MotorType.Swing, (int)sliderSwing.Value);
            }
            _isPositionChanged = false;
        }

        private void SliderSwing_DragStarted(object sender, System.Windows.Controls.Primitives.DragStartedEventArgs e)
        {
            _isDraggingStarted = true;
        }

        private void SliderSwing_DragCompleted(object sender, System.Windows.Controls.Primitives.DragCompletedEventArgs e)
        {
            _isDraggingStarted = false;
            DeviceMotors.MoveStepMotorPositionAsync(MotorType.Swing, (int)sliderSwing.Value);
        }
        private void BtnSwingOrigin_Click(object sender, RoutedEventArgs e)
        {
            DeviceMotors.MoveStepMotorToOriginAsync(MotorType.Swing);
        }

        private void BtnSwingSetAs_Click(object sender, RoutedEventArgs e)
        {
            DeviceMotors.SetStepMotorCurrentPositionAsOrigin(MotorType.Swing);
        }

        private void BtnSwingBwd_Click(object sender, RoutedEventArgs e)
        {
            int step = ToInt(editSwingValue.Text);
            step = Math.Max(Math.Min(step, 1000), 10) * -1;
            DeviceMotors.MoveStepMotorByPositionOffsetAsync(MotorType.Swing, step);
        }

        private void BtnSwingFwd_Click(object sender, RoutedEventArgs e)
        {
            int step = ToInt(editSwingValue.Text);
            step = Math.Max(Math.Min(step, 1000), 10) * +1;
            DeviceMotors.MoveStepMotorByPositionOffsetAsync(MotorType.Swing, step);
        }

        private void SliderXstage_ValueChanged(object sender, RoutedPropertyChangedEventArgs<double> e)
        {
            if (!_isDraggingStarted && !_isPositionChanged)
            {
                DeviceMotors.MoveStepMotorPositionAsync(MotorType.StageX, (int)sliderXstage.Value);
            }
            _isPositionChanged = false;
        }

        private void SliderXstage_DragStarted(object sender, System.Windows.Controls.Primitives.DragStartedEventArgs e)
        {
            _isDraggingStarted = true;
        }

        private void SliderXstage_DragCompleted(object sender, System.Windows.Controls.Primitives.DragCompletedEventArgs e)
        {
            _isDraggingStarted = false;
            DeviceMotors.MoveStepMotorPositionAsync(MotorType.StageX, (int)sliderXstage.Value);
        }
        private void BtnXstageOrigin_Click(object sender, RoutedEventArgs e)
        {
            DeviceMotors.MoveStepMotorToOriginAsync(MotorType.StageX);
        }

        private void BtnXstageSetAs_Click(object sender, RoutedEventArgs e)
        {
            DeviceMotors.SetStepMotorCurrentPositionAsOrigin(MotorType.StageX);
        }

        private void BtnXstageBwd_Click(object sender, RoutedEventArgs e)
        {
            int step = ToInt(editXstageValue.Text);
            step = Math.Max(Math.Min(step, 1000), 10) * -1;
            DeviceMotors.MoveStepMotorByPositionOffsetAsync(MotorType.StageX, step);
        }

        private void BtnXstageFwd_Click(object sender, RoutedEventArgs e)
        {
            int step = ToInt(editXstageValue.Text);
            step = Math.Max(Math.Min(step, 1000), 10) * +1;
            DeviceMotors.MoveStepMotorByPositionOffsetAsync(MotorType.StageX, step);
        }

        private void SliderYstage_ValueChanged(object sender, RoutedPropertyChangedEventArgs<double> e)
        {
            if (!_isDraggingStarted && !_isPositionChanged)
            {
                DeviceMotors.MoveStepMotorPositionAsync(MotorType.StageY, (int)sliderYstage.Value);
            }
            _isPositionChanged = false;
        }

        private void SliderYstage_DragStarted(object sender, System.Windows.Controls.Primitives.DragStartedEventArgs e)
        {
            _isDraggingStarted = true;
        }

        private void SliderYstage_DragCompleted(object sender, System.Windows.Controls.Primitives.DragCompletedEventArgs e)
        {
            _isDraggingStarted = false;
            DeviceMotors.MoveStepMotorPositionAsync(MotorType.StageY, (int)sliderYstage.Value);
        }
        private void BtnYstageOrigin_Click(object sender, RoutedEventArgs e)
        {
            DeviceMotors.MoveStepMotorToOriginAsync(MotorType.StageY);
        }

        private void BtnYstageSetAs_Click(object sender, RoutedEventArgs e)
        {
            DeviceMotors.SetStepMotorCurrentPositionAsOrigin(MotorType.StageY);
        }

        private void BtnYstageBwd_Click(object sender, RoutedEventArgs e)
        {
            int step = ToInt(editYstageValue.Text);
            step = Math.Max(Math.Min(step, 1000), 10) * -1;
            DeviceMotors.MoveStepMotorByPositionOffsetAsync(MotorType.StageY, step);
        }

        private void BtnYstageFwd_Click(object sender, RoutedEventArgs e)
        {
            int step = ToInt(editYstageValue.Text);
            step = Math.Max(Math.Min(step, 1000), 10) * +1;
            DeviceMotors.MoveStepMotorByPositionOffsetAsync(MotorType.StageY, step);
        }

        private void SliderZstage_ValueChanged(object sender, RoutedPropertyChangedEventArgs<double> e)
        {
            if (!_isDraggingStarted && !_isPositionChanged)
            {
                DeviceMotors.MoveStepMotorPositionAsync(MotorType.StageZ, (int)sliderZstage.Value);
            }
            _isPositionChanged = false;
        }

        private void SliderZstage_DragStarted(object sender, System.Windows.Controls.Primitives.DragStartedEventArgs e)
        {
            _isDraggingStarted = true;
        }

        private void SliderZstage_DragCompleted(object sender, System.Windows.Controls.Primitives.DragCompletedEventArgs e)
        {
            _isDraggingStarted = false;
            DeviceMotors.MoveStepMotorPositionAsync(MotorType.StageZ, (int)sliderZstage.Value);
        }
        private void BtnZstageOrigin_Click(object sender, RoutedEventArgs e)
        {
            DeviceMotors.MoveStepMotorToOriginAsync(MotorType.StageZ);
        }

        private void BtnZstageSetAs_Click(object sender, RoutedEventArgs e)
        {
            DeviceMotors.SetStepMotorCurrentPositionAsOrigin(MotorType.StageZ);
        }

        private void BtnZstageBwd_Click(object sender, RoutedEventArgs e)
        {
            int step = ToInt(editZstageValue.Text);
            step = Math.Max(Math.Min(step, 1000), 10) * -1;
            DeviceMotors.MoveStepMotorByPositionOffsetAsync(MotorType.StageZ, step);
        }

        private void BtnZstageFwd_Click(object sender, RoutedEventArgs e)
        {
            int step = ToInt(editZstageValue.Text);
            step = Math.Max(Math.Min(step, 1000), 10) * +1;
            DeviceMotors.MoveStepMotorByPositionOffsetAsync(MotorType.StageZ, step);
        }

        private void BtnChinrestStop_Click(object sender, RoutedEventArgs e)
        {
            StageMotors.StopChinrestMove();
        }

        private void BtnChinrestDown_Click(object sender, RoutedEventArgs e)
        {
            StageMotors.MoveChinrestDown();
        }
        
        private void BtnChinrestUp_Click(object sender, RoutedEventArgs e)
        {
            StageMotors.MoveChinrestUp();
        }
    }
}
