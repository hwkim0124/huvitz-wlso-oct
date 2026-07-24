using OxyPlot;
using OxyPlot.Annotations;
using OxyPlot.Axes;
using OxyPlot.Series;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;
using System.Windows.Threading;
using WsoNativeLib;

namespace WsoToolkit.controls
{
    using static WsoNativeLib.WsoOctDefs;
    public partial class OctSignalResampleGraph : UserControl
    {
        public PlotModel Model { get; private set; }
        public int AxisY_Max { get; set; } = LINE_CAMERA_PIXEL_VALUE_MAX;
        public int AxisY_Min { get; set; } = 0;
        public int AxisX_Max { get; set; } = LINE_CAMERA_CCD_PIXELS;
        public int AxisX_Min { get; set; } = 0;
        public int CenterX { get; set; } = LINE_CAMERA_CCD_PIXELS / 2;
        public int SelectedProfileIndex { get; set; } = 0;
        public int SpectrumKernelSize { get; set; } = 5;
        public bool IsRecordingValues { get; set; } = true;
        public int PeakThreshold { get; set; } = 3685;
        public int SubsThreshold { get; set; } = 3685;

        public List<int> PeakValueList { get; set; } = new();
        public List<int> PeakIndexList { get; set; } = new();

        public List<int> CalibratedIndexList { get; set; } = new();

        private const int DATA_WIDTH = LINE_CAMERA_CCD_PIXELS;
        private const int PEAK_AVERAGING_SIZE = 100;

        private List<ushort[]> _listSpectrumData = new();
        private List<float[]> _listResampleData = new();
        private List<ushort[]> _listSpectrumBuff = new();
        private List<float[]> _listResampleBuff = new();
        private List<int> _listProfileIndex = new();

        private ushort[] _spectrumLine = Array.Empty<ushort>();
        private float[] _resampleLine = Array.Empty<float>();

        private int _averagingSize = 5;
        private int _spectrumCount = 0;
        private int _resampleCount = 0;

        private int _fullMaxIndex = 0;
        private float _halfMaxIndex1 = 0.0f;
        private float _halfMaxIndex2 = 0.0f;

        private double _fullMaxValue = 0.0;
        private double _fullMinValue = 0.0;
        private double _halfMaxValue = 0.0;
        private double _fwhmWlenDist = 0.0;
        private double _fwhmIndexDist = 0.0;

        private double _totalSpectrum = 0.0;
        private double _sumFullMaxValue = 0.0;
        private double _avgFullMaxValue = 0.0;

        private double _maxFullMaxValue = 0.0;
        private double _minFullMaxValue = 0.0;
        private double _maxHalfMaxValue = 0.0;
        private double _maxTotalSpectrum = 0.0;
        private double _maxFwhmWlenDist = 0.0;
        private double _maxFwhmIndexDist = 0.0;

        // The graph repaints on a fixed cadence, decoupled from the frame/averaging
        // rate, and only when new data has arrived.
        private readonly DispatcherTimer _redrawTimer;
        private bool _dirty = false;

        public OctSignalResampleGraph()
        {
            InitializeComponent();

            Model = new PlotModel();
            plotView.Model = Model;

            UpdatePlotAxisRange();

            _redrawTimer = new DispatcherTimer(DispatcherPriority.Background)
            {
                Interval = TimeSpan.FromMilliseconds(50)   // ~20 Hz, independent of averaging size
            };
            _redrawTimer.Tick += OnRedrawTick;
            _redrawTimer.Start();
        }

        private void OnRedrawTick(object? sender, EventArgs e)
        {
            if (!_dirty)
            {
                return;
            }
            _dirty = false;
            RedrawGraph();
        }

        public void CallbackOctResampleDataCatpured(float[] data, int width, int height)
        {
            if (data == null || data.Length == 0)
            {
                return;
            }
            _resampleCount += 1;

            for (int i = 0; i < _listProfileIndex.Count; i++)
            {
                int index = _listProfileIndex[i];
                if (index >= 0 && index < data.Length)
                {
                    int start = index * width;
                    for (int j = start, k = 0; j < (start + width); j++, k++)
                    {
                        _listResampleBuff[i][k] += data[j];
                    }
                }
            }

            if (_resampleCount > _averagingSize && _resampleCount % _averagingSize == 0)
            {
                for (int i = 0; i < _listResampleData.Count; i++)
                {
                    for (int j = 0; j < DATA_WIDTH; j++)
                    {
                        _listResampleData[i][j] = _listResampleBuff[i][j] / _averagingSize;
                    }
                    _listResampleBuff[i] = new float[DATA_WIDTH];
                }
                UpdateResampleProfile();
            }
        }

        public void CallbackOctSpectrumDataCaptured(ushort[] data, int width, int height)
        {
            if (data == null || data.Length == 0 || width != DATA_WIDTH)
            {
                return;
            }
            _spectrumCount += 1;

            for (int i = 0; i < _listProfileIndex.Count; i++)
            {
                int index = _listProfileIndex[i];
                if (index >= 0 && index < data.Length)
                {
                    int start = index * width;
                    for (int j = start, k = 0; j < (start + width); j++, k++)
                    {
                        _listSpectrumBuff[i][k] += data[j];
                    }
                }
            }

            if (_spectrumCount > _averagingSize && _spectrumCount % _averagingSize == 0)
            {
                for (int i = 0; i < _listSpectrumData.Count; i++)
                {
                    for (int j = 0; j < DATA_WIDTH; j++)
                    {
                        _listSpectrumData[i][j] = (ushort)(_listSpectrumBuff[i][j] / _averagingSize);
                    }
                    _listSpectrumBuff[i] = new ushort[DATA_WIDTH];
                }
                UpdateSpectrumProfile();
                _dirty = true;   // actual repaint happens on the redraw timer
            }
        }

        private void UpdateResampleProfile()
        {
            if (_listResampleData.Count == 0 || SelectedProfileIndex < 0 || SelectedProfileIndex >= _listResampleData.Count)
            {
                return;
            }

            _resampleLine = _listResampleData[SelectedProfileIndex];
        }

        private void UpdateSpectrumProfile()
        {
            if (_listProfileIndex.Count == 0 || SelectedProfileIndex < 0 || SelectedProfileIndex >= _listSpectrumData.Count)
            {
                return;
            }

            ushort[] data = _listSpectrumData[SelectedProfileIndex];
            _spectrumLine = new ushort[DATA_WIDTH];

            _totalSpectrum = 0;
            for (int i = 0; i < DATA_WIDTH; i++)
            {
                int count = 0;
                int dsum = 0;
                for (int j = -SpectrumKernelSize / 2; j < SpectrumKernelSize / 2; j++)
                {
                    if (i + j >= 0 && i + j < DATA_WIDTH)
                    {
                        dsum += data[i + j];
                        count += 1;
                    }
                }
                if (count > 0)
                {
                    _spectrumLine[i] = (ushort)(dsum / count);
                    _totalSpectrum += _spectrumLine[i];
                }
            }

            _fullMaxValue = 0;
            _fullMaxIndex = 0;
            for (int i = 0; i < DATA_WIDTH; i++)
            {
                if (_spectrumLine[i] >= _fullMaxValue)
                {
                    _fullMaxValue = _spectrumLine[i];
                    _fullMaxIndex = i;
                }
            }
            _fullMinValue = _spectrumLine.Min();
            _halfMaxValue = _fullMaxValue / 2.0;
            _halfMaxIndex1 = 0.0f;
            _halfMaxIndex2 = 0.0f;

            for (int i = _fullMaxIndex - 1; i >= 0; i--)
            {
                if (_spectrumLine[i] < _halfMaxValue)
                {
                    _halfMaxIndex1 = (float)(i + (_halfMaxValue - _spectrumLine[i]) / (_spectrumLine[i + 1] - _spectrumLine[i] + 1e-9));
                    break;
                }
            }
            for (int i = _fullMaxIndex + 1; i < DATA_WIDTH; i++)
            {
                if (_spectrumLine[i] < _halfMaxValue)
                {
                    _halfMaxIndex2 = (float)(i - (_halfMaxValue - _spectrumLine[i]) / (_spectrumLine[i - 1] - _spectrumLine[i] + 1e-9));
                    break;
                }
            }

            double w1 = OctScanner.GetOctWavelengthAtPixelPosition(_halfMaxIndex1);
            double w2 = OctScanner.GetOctWavelengthAtPixelPosition(_halfMaxIndex2);
            _fwhmWlenDist = w2 - w1;
            _fwhmWlenDist = _halfMaxIndex2 - _halfMaxIndex1;

            _sumFullMaxValue += _fullMaxValue;
            int peakAvgSize = _averagingSize * 10;
            if (_spectrumCount > peakAvgSize && _spectrumCount % peakAvgSize == 0)
            {
                _avgFullMaxValue = _sumFullMaxValue / peakAvgSize;
                _sumFullMaxValue = 0;
            }

            if (IsRecordingValues)
            {
                _maxFullMaxValue = Math.Max(_maxFullMaxValue, _fullMaxValue);
                _minFullMaxValue = Math.Min(_minFullMaxValue, _fullMaxValue);
                _maxHalfMaxValue = Math.Max(_maxHalfMaxValue, _halfMaxValue);
                _maxTotalSpectrum = Math.Max(_maxTotalSpectrum, _totalSpectrum);
                _maxFwhmWlenDist = Math.Max(_maxFwhmWlenDist, _fwhmWlenDist);
                _maxFwhmIndexDist = Math.Max(_maxFwhmIndexDist, _fwhmIndexDist);
            }
        }

        private void RedrawGraph()
        {
            var line1 = new LineSeries
            {
                Title = "Spectrum",
                Color = OxyColors.Green,
                LineStyle = LineStyle.Solid,
                StrokeThickness = 1
            };
            var line2 = new LineSeries
            {
                Title = "Resample",
                Color = OxyColors.SteelBlue,
                StrokeThickness = 1,
                LineStyle = LineStyle.Dot
            };
            var line3 = new LineSeries
            {
                Title = "Peak",
                Color = OxyColors.Blue,
                StrokeThickness = 1,
                LineStyle = LineStyle.Dash
            };
            var line4 = new LineSeries
            {
                Title = "FWHM1",
                Color = OxyColors.SkyBlue,
                StrokeThickness = 1,
                LineStyle = LineStyle.Dash
            };
            var line5 = new LineSeries
            {
                Title = "FWHM2",
                Color = OxyColors.SkyBlue,
                StrokeThickness = 1,
                LineStyle = LineStyle.Dash
            };
            var line6 = new LineSeries
            {
                Title = "Peak Threshold",
                Color = OxyColors.Red,
                StrokeThickness = 1,
                LineStyle = LineStyle.Dot,
            };
            var line7 = new LineSeries
            {
                Title = "Subs Threshold",
                Color = OxyColors.Maroon,
                StrokeThickness = 1,
                LineStyle = LineStyle.Dot
            };


            if (_spectrumLine.Length > 0)
            {
                line1.Points.AddRange(_spectrumLine.Select((x, i) => new DataPoint(i, x)));
                // line2.Points.AddRange(_resampleLine.Select((x, i) => new DataPoint(i, x)));

                for (int y = AxisY_Min; y < AxisY_Max; y++)
                {
                    line3.Points.Add(new DataPoint(_fullMaxIndex, y));
                    line4.Points.Add(new DataPoint((int)_halfMaxIndex1, y));
                    line5.Points.Add(new DataPoint((int)_halfMaxIndex2, y));
                }

                for (int x = 0; x < AxisX_Max; x++)
                {
                    line6.Points.Add(new DataPoint(x, PeakThreshold));
                    line7.Points.Add(new DataPoint(x, SubsThreshold));
                }
            }

            var peaks = new ScatterSeries
            {
                MarkerType = MarkerType.Cross,
                MarkerSize = 5,
                MarkerFill = OxyColors.BlueViolet,
                MarkerStroke = OxyColors.Black,
                MarkerStrokeThickness = 1
            };

            for (int i = 0; i < PeakIndexList.Count; i++)
            {
                var point = new ScatterPoint(PeakIndexList[i], PeakValueList[i]);
                var wlen = OctScanner.GetOctWavelengthAtPixelPosition(PeakIndexList[i]);
                var s = String.Format("{0:F2}", wlen);

                peaks.Points.Add(point);
                Model.Annotations.Add(new TextAnnotation
                {
                    Text = s,
                    TextPosition = new DataPoint(PeakIndexList[i], PeakValueList[i] + 64),
                    TextColor = OxyColors.BlueViolet,
                    StrokeThickness = 0,
                });
            }

            Model.Series.Clear();
            Model.Series.Add(line1);
            Model.Series.Add(line2);
            Model.Series.Add(line3);
            Model.Series.Add(line4);
            Model.Series.Add(line5);
            Model.Series.Add(line6);
            Model.Series.Add(line7);
            Model.Series.Add(peaks);

            for (int i = 0; i < CalibratedIndexList.Count; i++)
            {
                var line = new LineSeries
                {
                    Title = String.Format("Calibrated {0}", i),
                    Color = OxyColors.DarkOrange,
                    StrokeThickness = 1,
                    LineStyle = LineStyle.Dash
                };

                for (int y = AxisY_Min; y < AxisY_Max; y++)
                {
                    line.Points.Add(new DataPoint(CalibratedIndexList[i], y));
                }

                Model.Series.Add(line);
            }

            Model.InvalidatePlot(true);

            string s1 = String.Format("Peak value: {0:F0} at {1}, max: {2:F0}, min: {3:F0}, avg: {4:F0}", _fullMaxValue, _fullMaxIndex, _maxFullMaxValue, _minFullMaxValue, _avgFullMaxValue);
            string s2 = String.Format("FWHM value: {0:F0} by {1:F0}, max: {2:F0} by {3:F2}", _halfMaxValue, _fwhmWlenDist, _maxHalfMaxValue, _maxFwhmWlenDist);
            string s3 = String.Format("Total spectrum: {0:F0}, max: {1:F0}", _totalSpectrum, _maxTotalSpectrum);
            string s4 = String.Format("FWHM range: {0:F2} ~ {1:F2}", _halfMaxIndex1, _halfMaxIndex2);

            // Called on the UI thread (redraw timer, or user-driven Set*/Clear* calls).
            lblStatus1.Content = s1;
            lblStatus2.Content = s2;
            lblStatus3.Content = s3;
            lblStatus4.Content = s4;
        }


        public void SetupProfileIndexList(List<int> list)
        {
            _listProfileIndex = list;

            _listSpectrumData.Clear();
            _listResampleData.Clear();
            _listSpectrumBuff.Clear();
            _listResampleBuff.Clear();

            for (int i = 0; i < list.Count; i++)
            {
                _listSpectrumData.Add(new ushort[DATA_WIDTH]);
                _listResampleData.Add(new float[DATA_WIDTH]);
                _listSpectrumBuff.Add(new ushort[DATA_WIDTH]);
                _listResampleBuff.Add(new float[DATA_WIDTH]);
            }
            _dirty = false;
        }

        public void SetPeakThresholds(int peak, int subs)
        {
            PeakThreshold = peak;
            SubsThreshold = subs;
            RedrawGraph();
        }

        public void SetDataPeakList(List<int> values, List<int> indices)
        {
            PeakValueList = values;
            PeakIndexList = indices;
            RedrawGraph();
        }

        public void SetCalibratedIndexList(List<int> list)
        {
            CalibratedIndexList = list;
            RedrawGraph();
        }

        public void SetAveragingSize(int size)
        {
            _averagingSize = Math.Max(size, 1);
            _sumFullMaxValue = 0;
        }

        public ushort[] GetSpectrumData()
        {
            return _spectrumLine;
        }

        public void ClearRecordingValues()
        {
            _maxFullMaxValue = 0;
            _minFullMaxValue = 0;
            _maxHalfMaxValue = 0;
            _maxTotalSpectrum = 0;
            _maxFwhmWlenDist = 0;
            _maxFwhmIndexDist = 0;
        }

        public void ClearDataPeakList()
        {
            PeakValueList.Clear();
            PeakIndexList.Clear();
            CalibratedIndexList.Clear();
            Model.Annotations.Clear();
            RedrawGraph();
        }


        public void UpdatePlotAxisRange()
        {
            var axisX = new LinearAxis
            {
                Position = AxisPosition.Bottom,
                Minimum = AxisX_Min,
                Maximum = AxisX_Max,
                MajorStep = AxisX_Max / 4,
                MinorStep = AxisX_Max / 8,
                //MajorGridlineStyle = LineStyle.Dot,
                //MinorGridlineStyle = LineStyle.Dot
            };
            var axisY = new LinearAxis
            {
                Position = AxisPosition.Left,
                Minimum = AxisY_Min,
                Maximum = AxisY_Max,
                MajorStep = AxisY_Max / 4,
                MinorStep = AxisY_Max / 8,
                //MajorGridlineStyle = LineStyle.Dot,
                //MinorGridlineStyle = LineStyle.Dot
            };

            Model.Axes.Clear();
            Model.Axes.Add(axisX);
            Model.Axes.Add(axisY);
            Model.InvalidatePlot(true);
        }
    }
}
