using OxyPlot.Series;
using OxyPlot;
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Shapes;
using OxyPlot.Axes;

namespace WsoToolkit
{
    /// <summary>
    /// Interaction logic for ImageControlWindow.xaml
    /// </summary>
    public partial class ImageAdjustmentWindow : Window
    {
        public delegate void ImageAdjustmentDelegate(ImageAdjustParam param);
        private ImageAdjustmentDelegate? _adjustmentCallback = null;

        private bool _isAdjusting = false;
        private bool _forceClose = false;

        // 현재 조정 값들
        ImageAdjustParam _currentParam = new ();

        // 디바운싱을 위한 타이머
        private System.Windows.Threading.DispatcherTimer _debounceTimer;
        private const int DEBOUNCE_INTERVAL_MS = 150; // 150ms 대기

        // OxyPlot 관련
        public PlotModel PlotModel { get; private set; }
        private LineSeries _curveSeries;
        private ScatterSeries _controlPointsSeries;
        private List<DataPoint> _curvePoints;
        private int _selectedPointIndex = -1;
        private bool _isDragging = false;

        public ImageAdjustmentWindow()
        {
            InitializeComponent();
            DataContext = this;

            // 디바운스 타이머 초기화
            _debounceTimer = new System.Windows.Threading.DispatcherTimer();
            _debounceTimer.Interval = TimeSpan.FromMilliseconds(DEBOUNCE_INTERVAL_MS);
            _debounceTimer.Tick += DebounceTimer_Tick;

            // Plot 초기화
            InitializePlot();
        }

        #region radial Correction

        private void InitializePlot()
        {
            PlotModel = new PlotModel
            {
                Title = "Radial Correction Curve",
                Background = OxyColors.White,
            };

            // X축 설정 (정규화된 거리: 0~1)
            var xAxis = new LinearAxis
            {
                Position = AxisPosition.Bottom,
                Title = "Normalized Distance (Center → Edge)",
                Minimum = -0.05,         // 살짝 줌 아웃 (5% 여유)
                Maximum = 1.05,          // 살짝 줌 아웃 (5% 여유)
                AbsoluteMinimum = -0.05, // 최소 범위 고정
                AbsoluteMaximum = 1.05,  // 최대 범위 고정
                IsPanEnabled = false,    // 팬(이동) 비활성화
                IsZoomEnabled = false,   // 줌 비활성화
                MajorGridlineStyle = LineStyle.Solid,
                MinorGridlineStyle = LineStyle.Dot,
                MajorGridlineColor = OxyColor.FromRgb(230, 230, 230),
                MinorGridlineColor = OxyColor.FromRgb(240, 240, 240),
            };

            // Y축 설정 (보정 계수: 0~1)
            var yAxis = new LinearAxis
            {
                Position = AxisPosition.Left,
                Title = "Brightness Correction (0 = Default)",
                Minimum = -0.1,         // 살짝 줌 아웃 (아래 5% 여유)
                Maximum = 1.1,           // 살짝 줌 아웃 (위 10% 여유)
                AbsoluteMinimum = -0.1, // 최소 범위 고정
                AbsoluteMaximum = 1.1,   // 최대 범위 고정
                IsPanEnabled = false,    // 팬(이동) 비활성화
                IsZoomEnabled = false,   // 줌 비활성화
                MajorGridlineStyle = LineStyle.Solid,
                MinorGridlineStyle = LineStyle.Dot,
                MajorGridlineColor = OxyColor.FromRgb(230, 230, 230),
                MinorGridlineColor = OxyColor.FromRgb(240, 240, 240),
            };

            PlotModel.Axes.Add(xAxis);
            PlotModel.Axes.Add(yAxis);

            // 기준선 (y=0, 기본 밝기)
            var baselineSeries = new LineSeries
            {
                Color = OxyColors.Gray,
                StrokeThickness = 1,
                LineStyle = LineStyle.Dash
            };
            baselineSeries.Points.Add(new DataPoint(0, 0));
            baselineSeries.Points.Add(new DataPoint(1, 0));
            PlotModel.Series.Add(baselineSeries);

            // 곡선 초기화 (2차 함수 곡선)
            _curvePoints = new List<DataPoint>();
            InitializeDefaultCurve();

            // 곡선 시리즈
            _curveSeries = new LineSeries
            {
                Color = OxyColor.FromRgb(41, 128, 185),  // 다크 블루 (#2980b9)
                StrokeThickness = 2,
                //Smooth = true  // 부드러운 곡선 표시
            };
            UpdateCurveSeries();
            PlotModel.Series.Add(_curveSeries);

            // 조절 포인트 시리즈
            _controlPointsSeries = new ScatterSeries
            {
                MarkerType = MarkerType.Circle,
                MarkerSize = 5,
                MarkerFill = OxyColor.FromRgb(46, 204, 113),     // 에메랄드 (#2ecc71)
                MarkerStroke = OxyColor.FromRgb(39, 174, 96),    // 진한 그린 (#27ae60)
                MarkerStrokeThickness = 2
            };
            UpdateControlPointsSeries();
            PlotModel.Series.Add(_controlPointsSeries);

            PlotModel.InvalidatePlot(true);

            plotView.ActualController.UnbindAll(); // 기본 자체 마우스 이벤트 제거 // mouse_down이 더블클릭으로 실행되는 문제때문에 적용
        }

        private void InitializeDefaultCurve()
        {
            _curvePoints.Clear();

            // 3개의 포인트: 시작(0,0), 제어점(0.5,0), 끝(1,0)
            _curvePoints.Add(new DataPoint(0, 0));      // 시작점 (고정)
            _curvePoints.Add(new DataPoint(0.5, 0.5));    // 제어점 (이동 가능)
            _curvePoints.Add(new DataPoint(1, 1));      // 끝점 (고정)
        }

        private void UpdateCurveSeries()
        {
            _curveSeries.Points.Clear();

            // 2차 함수 곡선으로 보간하여 표시
            int resolution = 100;
            for (int i = 0; i <= resolution; i++)
            {
                double x = i / (double)resolution;
                double y = CalculateQuadraticCurve(x);
                _curveSeries.Points.Add(new DataPoint(x, y));
            }
        }

        private double CalculateQuadraticCurve(double x)
        {
            // 3개의 포인트를 지나는 2차 함수 계산
            // P0(x0,y0), P1(x1,y1), P2(x2,y2)를 지나는 포물선
            // y = ax² + bx + c 형태

            if (_curvePoints.Count != 3)
                return 0;

            double x0 = _curvePoints[0].X; // 0
            double y0 = _curvePoints[0].Y; // 0
            double x1 = _curvePoints[1].X; // 제어점 X (가변)
            double y1 = _curvePoints[1].Y; // 제어점 Y (가변)
            double x2 = _curvePoints[2].X; // 1
            double y2 = _curvePoints[2].Y; // 0

            // 연립방정식을 풀어 a, b, c 계산
            // y0 = a*x0² + b*x0 + c
            // y1 = a*x1² + b*x1 + c
            // y2 = a*x2² + b*x2 + c

            double x0_sq = x0 * x0;
            double x1_sq = x1 * x1;
            double x2_sq = x2 * x2;

            // 행렬식 방법으로 계산
            double denom = (x0 - x1) * (x0 - x2) * (x1 - x2);

            if (Math.Abs(denom) < 1e-10) // 점들이 일직선상에 있는 경우
            {
                // 선형 보간
                if (x <= x1)
                    return y0 + (y1 - y0) * (x - x0) / (x1 - x0);
                else
                    return y1 + (y2 - y1) * (x - x1) / (x2 - x1);
            }

            double a = (x2 * (y1 - y0) + x1 * (y0 - y2) + x0 * (y2 - y1)) / denom;
            double b = (x2_sq * (y0 - y1) + x1_sq * (y2 - y0) + x0_sq * (y1 - y2)) / denom;
            double c = (x1 * x2 * (x1 - x2) * y0 + x2 * x0 * (x2 - x0) * y1 + x0 * x1 * (x0 - x1) * y2) / denom;

            double y = a * x * x + b * x + c;

            return Math.Max(0, Math.Min(1, y)); // 0~1 범위로 제한
        }

        private void UpdateControlPointsSeries()
        {
            _controlPointsSeries.Points.Clear();

            // 모든 제어점 표시 (시작점, 중앙점, 끝점)
            foreach (var point in _curvePoints)
            {
                _controlPointsSeries.Points.Add(new ScatterPoint(point.X, point.Y));
            }
        }

        private void PlotView_MouseDown(object sender, MouseButtonEventArgs e)
        {
            if (e.ChangedButton != MouseButton.Left)
                return;

            var position = e.GetPosition(plotView);
            var screenPoint = new ScreenPoint(position.X, position.Y);
            var dataPoint = _curveSeries.InverseTransform(screenPoint);

            // 가장 가까운 제어점 찾기
            _selectedPointIndex = FindNearestPointIndex(dataPoint.X, dataPoint.Y);
            if (_selectedPointIndex >= 0)
            {
                _isDragging = true;
                plotView.CaptureMouse();
                e.Handled = true;
            }
        }

        private void PlotView_MouseMove(object sender, MouseEventArgs e)
        {
            if (!_isDragging || _selectedPointIndex < 0)
                return;

            var position = e.GetPosition(plotView);
            var screenPoint = new ScreenPoint(position.X, position.Y);
            var dataPoint = _curveSeries.InverseTransform(screenPoint);

            // Y 값만 업데이트 (X는 고정)
            double newY = Math.Max(0, Math.Min(1, dataPoint.Y)); // Y: 0~1 범위

            if (_selectedPointIndex == 0)
            {
                // 시작점: X는 0으로 고정, Y만 변경
                _curvePoints[0] = new DataPoint(0, newY);
            }
            else if (_selectedPointIndex == 1)
            {
                // 중앙점: X와 Y 모두 변경 가능
                double newX = Math.Max(0.01, Math.Min(0.99, dataPoint.X)); // X: 0.1~0.9 범위
                _curvePoints[1] = new DataPoint(newX, newY);
            }
            else if (_selectedPointIndex == 2)
            {
                // 끝점: X는 1로 고정, Y만 변경
                _curvePoints[2] = new DataPoint(1, newY);
            }

            UpdateCurveSeries();
            UpdateControlPointsSeries();
            PlotModel.InvalidatePlot(true);

            // 보정 적용
            ApplyAdjustmentDebounced();
        }

        private void PlotView_MouseUp(object sender, MouseButtonEventArgs e)
        {
            if (_isDragging)
            {
                _isDragging = false;
                _selectedPointIndex = -1;
                plotView.ReleaseMouseCapture();
            }
        }

        private int FindNearestPointIndex(double x, double y)
        {
            // 모든 제어점에서 가장 가까운 점 찾기
            double minDistance = double.MaxValue;
            int nearestIndex = -1;
            double threshold = 0.08; // 선택 임계값

            for (int i = 0; i < _curvePoints.Count; i++)
            {
                double dx = _curvePoints[i].X - x;
                double dy = (_curvePoints[i].Y - y) * 0.5; // Y 스케일 조정
                double distance = Math.Sqrt(dx * dx + dy * dy);

                if (distance < minDistance && distance < threshold)
                {
                    minDistance = distance;
                    nearestIndex = i;
                }
            }

            return nearestIndex;
        }

        private double[] GetCorrectionCurveData()
        {
            // 곡선 데이터를 배열로 변환 (2차 함수 보간)
            int resolution = 100;
            double[] curveData = new double[resolution];

            for (int i = 0; i < resolution; i++)
            {
                double x = i / (double)(resolution - 1);
                curveData[i] = CalculateQuadraticCurve(x);
            }

            return curveData;
        }

        private double InterpolateCurve(double x)
        {
            // 2차 함수 곡선 사용
            return CalculateQuadraticCurve(x);
        }

        /// <summary>
        /// 입력된 강도 값에 따라 곡선의 포인트를 자동으로 조정
        /// </summary>
        /// <param name="intensity">끝점의 Y 값 (0~1)</param>
        private void UpdateCurveFromIntensity(float intensity)
        {
            if (_curvePoints.Count != 3)
                return;

            // 시작점: (0, 0) 고정
            _curvePoints[0] = new DataPoint(0, 0);

            // 끝점: (1, intensity) - 입력된 값으로 설정
            _curvePoints[2] = new DataPoint(1, intensity);

            // 중간점: 시작점과 끝점을 잇는 직선상의 중간 위치 (0.5, intensity/2)
            double midX = 0.5;
            double midY = intensity / 2.0; // 직선의 중간 높이
            _curvePoints[1] = new DataPoint(midX, midY);

            // UI 업데이트
            UpdateCurveSeries();
            UpdateControlPointsSeries();
            PlotModel.InvalidatePlot(true);
        }

        #endregion radial correction 


        private void DebounceTimer_Tick(object sender, EventArgs e)
        {
            _debounceTimer.Stop();
            ApplyAdjustment();
        }

        /// <summary>
        /// 이미지 조정 적용 (디바운스 적용)
        /// </summary>
        private void ApplyAdjustmentDebounced()
        {
            // 기존 타이머를 리셋하고 다시 시작
            _debounceTimer.Stop();
            _debounceTimer.Start();
        }

        /// <summary>
        /// 이미지 조정 콜백 설정
        /// </summary>
        public void SetAdjustmentCallback(ImageAdjustmentDelegate callback)
        {
            _adjustmentCallback = callback;
        }

        /// <summary>
        /// 슬라이더 값 초기화
        /// </summary>
        public void ResetValues()
        {
            _isAdjusting = true;

            _currentParam.Brightness = 0;
            mySliderBright.Value = _currentParam.Brightness;


            // Radial Correction
            _currentParam.IsRadialCorrection = false;
            _currentParam.RadialCorIntensity = 1;
            myCheckRadialCorrection.IsChecked = _currentParam.IsRadialCorrection;
            myTbRadialCorrection.Text = _currentParam.RadialCorIntensity.ToString();

            // 곡선도 초기화
            InitializeDefaultCurve();
            UpdateCurveSeries();
            UpdateControlPointsSeries();
            PlotModel.InvalidatePlot(true);

            _isAdjusting = false;
        }

        public void SetDefaultValues(ImageAdjustParam param)
        {
            _isAdjusting = true;

            _currentParam.Brightness = param.Brightness;
            _currentParam.BrightnessRangeMin = param.BrightnessRangeMin;
            _currentParam.BrightnessRangeMax = param.BrightnessRangeMax;
            _currentParam.IsRadialCorrection = param.IsRadialCorrection;
            _currentParam.RadialCorIntensity = param.RadialCorIntensity;

            mySliderBright.Value = param.Brightness;
            mySliderBright.Minimum = param.BrightnessRangeMin;
            mySliderBright.Maximum = param.BrightnessRangeMax;
            myCheckRadialCorrection.IsChecked = param.IsRadialCorrection;
            myTbRadialCorrection.Text = param.RadialCorIntensity.ToString();
            myCheckUseRemoveReflectedLight.IsChecked = param.IsRemoveReflectedLight;
            _currentParam.IsRemoveReflectedLight = param.IsRemoveReflectedLight;
            _currentParam.RadialCorrectionCurve = param.RadialCorrectionCurve;

            UpdateCurveFromIntensity(param.RadialCorIntensity);

            _isAdjusting = false;
        }

        /// <summary>
        /// 이미지 조정 적용
        /// </summary>
        private void ApplyAdjustment()
        {
            _currentParam.RadialCorrectionCurve = GetCorrectionCurveData();

            _adjustmentCallback?.Invoke(_currentParam);
        }

        #region Event Handler

        public void ForceClose()
        {
            _forceClose = true;
            Close();
        }

        private void Window_Closing(object sender, System.ComponentModel.CancelEventArgs e)
        {
            if (_forceClose)
                return;

            e.Cancel = true;  // 실제 닫기 취소
            this.Hide();      // 숨기기만 함
        }

        private void myBtReset_Click(object sender, RoutedEventArgs e)
        {
            _debounceTimer.Stop(); // 타이머 정지
            ResetValues();

            ApplyAdjustment();
        }

        private void myBtClose_Click(object sender, RoutedEventArgs e)
        {
            _debounceTimer.Stop(); // 타이머 정지
            this.Hide();
        }

        private void mySliderBright_ValueChanged(object sender, RoutedPropertyChangedEventArgs<double> e)
        {
            if (_isAdjusting) return;

            _currentParam.Brightness = (int)mySliderBright.Value;
            ApplyAdjustmentDebounced(); // 디바운스 적용
        }

        private void myTbBright_KeyDown(object sender, KeyEventArgs e)
        {
            if (e.Key == Key.Enter)
            {
                int value;
                if (int.TryParse(myTbBright.Text, out value))
                {
                    mySliderBright.Value = value;
                }
                else
                {
                    mySliderBright.Value = 0;
                }
            }
        }

        private void myCheckRadialCorrection_CheckedNUnChecked(object sender, RoutedEventArgs e)
        {
            if (_isAdjusting) return;

            _currentParam.IsRadialCorrection = myCheckRadialCorrection.IsChecked ?? false;

            float nIntensity;
            if (float.TryParse(myTbRadialCorrection.Text, out nIntensity) == false)
            {
                nIntensity = 1;
                myTbRadialCorrection.Text = nIntensity.ToString();
            }

            _currentParam.RadialCorIntensity = nIntensity;
            ApplyAdjustmentDebounced(); // 디바운스 적용
        }

        private void myTbRadialCorrection_KeyDown(object sender, KeyEventArgs e)
        {
            if (e.Key == Key.Enter)
            {
                if (_isAdjusting) return;

                float nIntensity;
                if (float.TryParse(myTbRadialCorrection.Text, out nIntensity))
                {
                    // 값 범위 제한 (0~1)
                    nIntensity = Math.Max(0, Math.Min(1, nIntensity));
                    myTbRadialCorrection.Text = nIntensity.ToString();
                }
                else
                {
                    nIntensity = 1;
                    myTbRadialCorrection.Text = nIntensity.ToString();
                }

                _currentParam.RadialCorIntensity = nIntensity;

                // 곡선 포인트를 자동으로 조정
                UpdateCurveFromIntensity(nIntensity);

                ApplyAdjustmentDebounced(); // 디바운스 적용
            }
        }
        private void myCheckUse_CheckedNUnChecked(object sender, RoutedEventArgs e)
        {
            if (_isAdjusting) return;

            _currentParam.IsRemoveReflectedLight = myCheckUseRemoveReflectedLight.IsChecked ?? false;

            ApplyAdjustmentDebounced(); // 디바운스 적용
        }

        #endregion Event Handler
    }

    public class ImageAdjustParam
    {
        public int Brightness { get; set; }
        public int BrightnessRangeMin { get; set; }
        public int BrightnessRangeMax { get; set; }

        public bool IsRadialCorrection { get; set; }

        public float RadialCorIntensity { get; set; }

        public double[]? RadialCorrectionCurve { get; set; }

        public bool IsRemoveReflectedLight { get; set; }

        public ImageAdjustParam(int nBrightness = 0, int nPixelDepth = 0) // 생성자
        {
            Brightness = nBrightness;

            if (nPixelDepth == 2) // 16bit
            {
                BrightnessRangeMin = -32767;
                BrightnessRangeMax = 32767;
            }
            else // nPixelDepth == 0 // 8bit
            {
                BrightnessRangeMin = -127;
                BrightnessRangeMax = 127;
            }

            IsRadialCorrection = false;
            RadialCorIntensity = 1;
            RadialCorrectionCurve = null!;
            IsRemoveReflectedLight = false;
        }
    }
}
