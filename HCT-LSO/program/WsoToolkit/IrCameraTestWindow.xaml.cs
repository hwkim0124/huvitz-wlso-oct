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
using WsoToolkit.utils;

namespace WsoToolkit
{
    using static utils.NumberUtil;

    public partial class IrCameraTestWindow : Window
    {
        // Save Frame 저장 폴더. 선택 전에는 기존 기본값(실행 폴더의 cornea)
        private string _saveFolder = ".\\cornea";

        public IrCameraTestWindow()
        {
            InitializeComponent();

            _onCorneaLeftFrameCaptured = new WsoCallback.CorneaCameraFrameCaptured(this.OnCorneaCameraLeftFrameCaptured);
            _onCorneaRightFrameCaptured = new WsoCallback.CorneaCameraFrameCaptured(this.OnCorneaCameraRightFrameCaptured);
            _onCorneaLowerFrameCaptured = new WsoCallback.CorneaCameraFrameCaptured(this.OnCorneaCameraLowerFrameCaptured);

            _onEyeAlignEvent = new WsoCallback.EyeAlignEventOccurred(this.OnEyeAlignEventOccurred);
            _onOptimizeButtonPressed = new WsoCallback.OptimizeButtonPressed(this.OnOptimizeButtonPressed);
        }

        // Windows Event handlers
        /////////////////////////////////////////////////////////////////////////////////////////////
        private void Window_Initialized(object sender, EventArgs e)
        {

        }
        
        private void InitializeWindowControls()
        {
            cmbCorneaChannel.ItemsSource = new string[] { "Lower", "Right", "Left" };
            cmbOutputMode.ItemsSource = new string[] { "Normal", "Test Pattern" };
            cmbTestPattern.ItemsSource = new string[] { "Standard", "TopBottomBar", "LeftRightBar", "Random", "Square", "Black" };
            cmbHorzFlip.ItemsSource = new string[] { "Off", "On" };
            cmbVertFlip.ItemsSource = new string[] { "Off", "On" };
            cmbViewFormat.ItemsSource = new string[] { "Lower", "Left", "Lower+Left", "Right", "Lower+Right", "Left+Right", "Lower+Left+Right" };

            checkCorneaAlign.IsChecked = false;
            checkCorneaFocus.IsChecked = false;
            checkRollingBar.IsChecked = false;
            checkTransparent.IsChecked = false;

            cmbCorneaChannel.SelectedIndex = 0;
            cmbViewFormat.SelectedIndex = 6;
        }

        private void Window_Loaded(object sender, RoutedEventArgs e)
        {
            InitializeWindowControls();
            ReflectCorneaChannelControls();

            corneaPreview1.IsOverlayAlignGuide = false;
            corneaPreview2.IsOverlayAlignGuide = false;
            corneaPreview3.IsOverlayAlignGuide = false;

            corneaPreview1.IsOverlayFocusGuide = false;
            corneaPreview2.IsOverlayFocusGuide= false;
            corneaPreview3.IsOverlayFocusGuide = false;

            corneaPreview1.IsStageControlEnabled = true;
            corneaPreview2.IsStageControlEnabled = true;
            corneaPreview3.IsStageControlEnabled = true;

            InitializeAlignControls();
            BoardDevice.ConnectOptimizeButtonPressed(_onOptimizeButtonPressed);
        }

        private void Window_Closed(object sender, EventArgs e)
        {
            Mouse.OverrideCursor = Cursors.Wait;
            _lightControlWindow?.Close();
            _focusMotorWindow?.Close();
            _stageMotorWindow?.Close();

            // Stop the aligner and drop the button hook before the camera preview goes away.
            EyeAlign.CancelAlignment();
            EyeAlign.ClearAllPupilDetectOverrides();
            BoardDevice.ReleaseOptimizeButtonPressed();

            CloseCorneaCameraPreview();

            Thread.Sleep(500);
            Mouse.OverrideCursor = null;
        }

        private void Window_Closing(object sender, System.ComponentModel.CancelEventArgs e)
        {
            if (MsgBoxUtil.ShowQuestion("Are you sure to close this window?") == false)
            {
                e.Cancel = true;
            }
        }

        // Operational button event handlers
        /////////////////////////////////////////////////////////////////////////////////////////////
        private void BtnStartCamera_Click(object sender, RoutedEventArgs e)
        {
            if (IsCorneaCameraPreviewing())
            {
                btnStartCamera.Content = "Start Camera";
                CloseCorneaCameraPreview();
            }
            else
            {
                btnStartCamera.Content = "Stop Camera";
                StartCorneaCameraPreview();
            }
        }

        private void BtnClose_Click(object sender, RoutedEventArgs e)
        {
            Close();
        }

        private void BtnShowFocusM_Click(object sender, RoutedEventArgs e)
        {
            if (_focusMotorWindow == null || _focusMotorWindow.IsLoaded)
            {
                _focusMotorWindow = new();
                _focusMotorWindow.Show();
            }
            else
            {
                _focusMotorWindow?.Focus();
            }
        }

        private void BtnShowStageM_Click(object sender, RoutedEventArgs e)
        {
            if (_stageMotorWindow == null || !_stageMotorWindow.IsLoaded)
            {
                _stageMotorWindow = new();
                _stageMotorWindow.Show();
            }
            else
            {
                _stageMotorWindow?.Focus();
            }
        }

        private void BtnShowLights_Click(object sender, RoutedEventArgs e)
        {
            if (_lightControlWindow == null || !_lightControlWindow.IsLoaded)
            {
                _lightControlWindow = new();
                _lightControlWindow.Show();
            }
            else
            {
                _lightControlWindow.Focus();
            }
        }
        
        private void BtnShowConfig_Click(object sender, RoutedEventArgs e)
        {
            var window = new SystemConfigurationWindow
            {
                Owner = this
            };
            window.ShowDialog();
        }

        private void BtnSaveFrame_Click(object sender, RoutedEventArgs e)
        {
            // Title을 입력하면 "Title_카메라이름.png", 비어 있으면 기존대로 "카메라이름_시각.png"
            string title = txtFileName.Text.Trim();
            if (title.IndexOfAny(System.IO.Path.GetInvalidFileNameChars()) >= 0)
            {
                MsgBoxUtil.ShowInfo("Title contains characters that cannot be used in a file name.");
                return;
            }

            foreach (var preview in new[] { corneaPreview1, corneaPreview2, corneaPreview3 })
            {
                preview.SaveFrameImage(title.Length == 0 ? "" : $"{title}_{preview.GetCameraName()}.png", _saveFolder);
            }
            MsgBoxUtil.ShowInfo($"Cornea frame images saved in {System.IO.Path.GetFullPath(_saveFolder)}");
        }

        private void BtnSelectFolder_Click(object sender, RoutedEventArgs e)
        {
            var dialog = new Microsoft.Win32.OpenFolderDialog
            {
                Title = "Select folder to save frame images",
                InitialDirectory = System.IO.Path.GetFullPath(_saveFolder),
            };
            if (dialog.ShowDialog(this) == true)
            {
                _saveFolder = dialog.FolderName;
                btnSelectFolder.ToolTip = _saveFolder;
            }
        }

        private void CmbCorneaChannel_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            var comboBox = sender as ComboBox;
            if (comboBox != null && comboBox.IsDropDownOpen)
            {
                ReflectCorneaChannelControls();
            }
        }

        private void CmbOutputMode_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            var comboBox = sender as ComboBox;
            if (comboBox != null && comboBox.IsDropDownOpen)
            {
                UpdateCorneaOutputMode();
            }
        }

        private void CmbTestPattern_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            var comboBox = sender as ComboBox;
            if (comboBox != null && comboBox.IsDropDownOpen)
            {
                UpdateCorneaOutputMode();
            }
        }

        private void CmbHorzFlip_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            var comboBox = sender as ComboBox;
            if (comboBox != null && comboBox.IsDropDownOpen)
            {
                UpdateCorneaMirrorMode();
            }
        }

        private void CmbVertFlip_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            var comboBox = sender as ComboBox;
            if (comboBox != null && comboBox.IsDropDownOpen)
            {
                UpdateCorneaMirrorMode();
            }
        }

        private void CheckCorneaAlign_Click(object sender, RoutedEventArgs e)
        {
            corneaPreview1.IsOverlayAlignGuide = checkCorneaAlign.IsChecked == true;
            corneaPreview2.IsOverlayAlignGuide = checkCorneaAlign.IsChecked == true;
            corneaPreview3.IsOverlayAlignGuide = checkCorneaAlign.IsChecked == true;
        }

        private void CheckCorneaFocus_Click(object sender, RoutedEventArgs e)
        {
            corneaPreview1.IsOverlayFocusGuide = checkCorneaFocus.IsChecked == true;
            corneaPreview2.IsOverlayFocusGuide = checkCorneaFocus.IsChecked == true;
            corneaPreview3.IsOverlayFocusGuide = checkCorneaFocus.IsChecked == true;
        }

        private void EditCorneaAgain_KeyDown(object sender, KeyEventArgs e)
        {
            if (e.Key == Key.Enter)
            {
                var type = GetCameraTypeSelected();
                var again = ToFloat(editCorneaAgain.Text);
                CorneaCamera.SetAnalogGain(type, again);
            }
        }

        private void EditCorneaDgain_KeyDown(object sender, KeyEventArgs e)
        {
            if (e.Key == Key.Enter)
            {
                var type = GetCameraTypeSelected();
                var dgain = ToFloat(editCorneaDgain.Text);
                CorneaCamera.SetDigitalGain(type, dgain);
            }
        }
        private void EditCorneaExposure_KeyDown(object sender, KeyEventArgs e)
        {
            if (e.Key == Key.Enter)
            {
                var type = GetCameraTypeSelected();
                var value = ToFloat(editCorneaExposure.Text);
                CorneaCamera.SetExposureTime(type, value);
            }
        }

        private void CmbViewFormat_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            int index = cmbViewFormat.SelectedIndex;
            if (index < 0)
            {
                return;
            }

            var comboBox = sender as ComboBox;
            if (comboBox != null && comboBox.IsDropDownOpen)
            {
                CorneaCamera.SetCameraViewPreset((WsoDevice.IrCameraViewPreset)(index + 1));
            }
        }

        private void CheckRollingBar_Click(object sender, RoutedEventArgs e)
        {
            UpdateCorneaOutputMode();
        }

        private void CheckTransparent_Click(object sender, RoutedEventArgs e)
        {
            UpdateCorneaOutputMode();
        }
    }
}
