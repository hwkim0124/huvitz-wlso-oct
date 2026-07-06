using System.Collections.ObjectModel;
using System.Windows;
using static WsoNativeLib.LsoCamera;

namespace WsoToolkit
{
    /// <summary>
    /// Interaction logic for CaptureROISettingWindow.xaml
    /// </summary>
    public partial class CaptureROISettingWindow : Window
    {
        private readonly LsoScanTestModel _scanTestModel;

        public int FrameCount { get; set; }

        public ObservableCollection<FrameInfo> FrameItems { get; set; } = new ObservableCollection<FrameInfo>();

        public CaptureROISettingWindow(LsoScanTestModel scanTestModel)
        {
            _scanTestModel = scanTestModel;
            InitializeComponent();
            initControls_();
        }

        private void Window_Loaded(object sender, RoutedEventArgs e)
        {
            loadCameraParam_();
            loadFrameParam_();
            updateDataGrid_();
            updateControls_();
        }

        private void mybtApply_Click(object sender, RoutedEventArgs e)
        {
            uploadCaptureFrameROISettings_();
            _scanTestModel.SaveConfigToIniFile();
        }

        private void mybtClose_Click(object sender, RoutedEventArgs e)
        {
            Close();
        }
    }
}
