using OxyPlot;
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

namespace WsoToolkit.controls
{
    using static WsoNativeLib.WsoOctDefs;

    public partial class OctSignalSpectrumGraph : UserControl
    {
        public PlotModel Model { get; private set; }
        public int AxisY_Max { get; set; } = LINE_CAMERA_PIXEL_VALUE_MAX;
        public int AxisY_Min { get; set; } = -1024;
        public int AxisX_Max { get; set; } = LINE_CAMERA_CCD_PIXELS;
        public int AxisX_Min { get; set; } = 0;

        private const int DATA_WIDTH = LINE_CAMERA_CCD_PIXELS;
        private const int DATA_HEIGHT = SPECTRO_CALIB_BSCAN_WIDTH;
        private const int DATA_SIZE = DATA_WIDTH * DATA_HEIGHT;

        // private float[] _resampleData = new float[DATA_SIZE];
        // private ushort[] _spectrumData = new ushort[DATA_SIZE];
        private ushort[] _spectrumLine = Array.Empty<ushort>();
        private float[] _resampleLine = Array.Empty<float>();
        private double[] _detectorWlens = new double[DATA_WIDTH];

        private List<ushort[]> _listSpectrums = new();
        private List<float[]> _listResamples = new();

        private int _spectrumIndex = 0;
        private int _resampleIndex = 0;
        private int _averagingSize = 10;
        private int _spectrumCount = 0;
        private int _resampleCount = 0;

        private int _fullMaxValue = 0;
        private int _fullMinValue = 0;
        private int _halfMaxIndex1 = 0;
        private int _halfMaxIndex2 = 0;
        private int _halfMaxValue = 0;

        private double _halfMaxWlenDist = 0.0;
        private float _resampleMaxValue = 0.0f;
        private float _resampleMinValue = 0.0f;

        // The graph repaints on a fixed cadence, decoupled from the frame/averaging
        // rate, and only when new data has arrived.
        private readonly DispatcherTimer _redrawTimer;
        private bool _dirty = false;

        public OctSignalSpectrumGraph()
        {
            InitializeComponent();

            Model = new PlotModel();
            plotView.Model = Model;

            SetAveragingSize(10);
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

        public void CallbackOctSpectrumDataCaptured(ushort[] data, int width, int height)
        {
            if (data == null || data.Length == 0)
            {
                return;
            }
            _spectrumCount += 1;

            int start = (height / 2) * width;
            _listSpectrums[_spectrumIndex] = data[start..(start + width)];
            _spectrumIndex = (_averagingSize <= 1 ? 0 : (_spectrumIndex + 1) % _averagingSize);

            // Array.Copy(data, _spectrumData, DATA_SIZE);

            UpdateSpectrumLine();

            if (_spectrumCount >= _averagingSize)
            {
                _dirty = true;   // actual repaint happens on the redraw timer
            }
        }

        public void CallbackOctResampleDataCatpured(float[] data, int width, int height)
        {
            if (data == null || data.Length == 0)
            {
                return;
            }
            _resampleCount += 1;

            int start = (height / 2) * width;
            _listResamples[_resampleIndex] = data[start..(start + width)];
            _resampleIndex = (_averagingSize <= 1 ? 0 : (_resampleIndex + 1) % _averagingSize);

            // Array.Copy(data, _resampleData, DATA_SIZE);

            UpdateResampleLine();
        }

        private void UpdateResampleLine()
        {
            if (_resampleCount < _averagingSize)
            {
                _resampleLine = Array.Empty<float>();
            }
            else
            {
                if (_averagingSize <= 1)
                {
                    _resampleLine = _listResamples[0];
                }
                else
                {
                    _resampleLine = new float[DATA_WIDTH];
                    int size = _listResamples.Count;
                    for (int i = 0; i < DATA_WIDTH; i++)
                    {
                        float sum = 0;
                        for (int j = 0; j < size; j++)
                        {
                            sum += _listResamples[j][i];
                        }
                        _resampleLine[i] = sum / _averagingSize;
                    }
                }

                _resampleMaxValue = _resampleLine.Max();
                _resampleMinValue = _resampleLine.Min();
            }
        }

        private void UpdateSpectrumLine()
        {
            if (_spectrumCount < _averagingSize)
            {
                _spectrumLine = Array.Empty<ushort>();
            }
            else
            {
                if (_averagingSize <= 1)
                {
                    _spectrumLine = _listSpectrums[0];
                }
                else
                {
                    _spectrumLine = new ushort[DATA_WIDTH];
                    int size = _listSpectrums.Count;
                    for (int i = 0; i < DATA_WIDTH; i++)
                    {
                        int sum = 0;
                        for (int j = 0; j < size; j++)
                        {
                            sum += _listSpectrums[j][i];
                        }
                        _spectrumLine[i] = (ushort)(sum / _averagingSize);
                    }
                }

                _fullMaxValue = _spectrumLine.Max();
                _fullMinValue = _spectrumLine.Min();
                _halfMaxValue = (_fullMaxValue + _fullMinValue) / 2;
                _halfMaxIndex1 = 0;
                _halfMaxIndex2 = 0;

                for (int i = 0; i < DATA_WIDTH; i++)
                {
                    if (_spectrumLine[i] >= _halfMaxValue)
                    {
                        _halfMaxIndex1 = i;
                        break;
                    }
                }
                for (int i = DATA_WIDTH - 1; i >= 0; i--)
                {
                    if (_spectrumLine[i] >= _halfMaxValue)
                    {
                        _halfMaxIndex2 = i;
                        break;
                    }
                }

                if (_detectorWlens.Length == DATA_WIDTH)
                {
                    _halfMaxWlenDist = _detectorWlens[_halfMaxIndex2] - _detectorWlens[_halfMaxIndex1];
                }
            }
        }

        private void RedrawGraph()
        {
            if (_spectrumLine.Length == 0)
            {
                return;
            }

            var line1 = new LineSeries
            {
                Title = "Spectrum",
                Color = OxyColors.Green,
                StrokeThickness = 1,
                LineStyle = LineStyle.Solid
            };

            var line2 = new LineSeries
            {
                Title = "HalfMax",
                Color = OxyColors.Red,
                StrokeThickness = 1,
                LineStyle = LineStyle.Dash
            };

            var line3 = new LineSeries
            {
                Title = "Wavelength",
                Color = OxyColors.SteelBlue,
                StrokeThickness = 1,
                LineStyle = LineStyle.Dot
            };

            var line4 = new LineSeries
            {
                Title = "Resample",
                Color = OxyColors.Blue,
                StrokeThickness = 1,
                LineStyle = LineStyle.Solid
            };

            line1.Points.AddRange(_spectrumLine.Select((y, x) => new DataPoint(x, y)));

            for (int i = _halfMaxIndex1; i <= _halfMaxIndex2; i++)
            {
                line2.Points.Add(new DataPoint(i, _halfMaxValue));
            }

            line3.Points.AddRange(_detectorWlens.Select((y, x) => new DataPoint(x, y)));
            line4.Points.AddRange(_resampleLine.Select((y, x) => new DataPoint(x, y)));

            Model.Series.Clear();
            Model.Series.Add(line1);
            Model.Series.Add(line2);
            Model.Series.Add(line3);
            Model.Series.Add(line4);
            Model.InvalidatePlot(true);

            string s = String.Format("Spectrum: {0} ~ {1}, Fwhm: {2:F2} ({3},{4}), Resample: {5:F2} ~ {6:F2}",
                _fullMinValue, _fullMaxValue, _halfMaxWlenDist, _halfMaxIndex1, _halfMaxIndex2, _resampleMinValue, _resampleMaxValue);

            // Called from the redraw timer, which already runs on the UI thread.
            lblStatus.Content = s;
        }

        private void UpdatePlotAxisRange()
        {
            var axisX = new LinearAxis
            {
                Position = AxisPosition.Bottom,
                Minimum = AxisX_Min,
                Maximum = AxisX_Max,
                MajorStep = 512,
                MinorStep = 256
            };
            var axisY = new LinearAxis
            {
                Position = AxisPosition.Left,
                Minimum = AxisY_Min,
                Maximum = AxisY_Max,
                MajorStep = 1024,
                MinorStep = 512
            };
            axisY.MajorGridlineStyle = LineStyle.Dot;

            Model.Axes.Clear();
            Model.Axes.Add(axisX);
            Model.Axes.Add(axisY);
            Model.InvalidatePlot(true);
        }

        private void SetAveragingSize(int size)
        {
            _averagingSize = (size <= 1 ? 1 : (size + 1));
            _listSpectrums.Clear();
            _listResamples.Clear();

            for (int i = 0; i < _averagingSize; i++)
            {
                _listSpectrums.Add(new ushort[DATA_WIDTH]);
                _listResamples.Add(new float[DATA_WIDTH]);
            }
            _spectrumIndex = 0;
            _resampleIndex = 0;
            _dirty = false;
        }

        public void SetDetectorWavelengths(double[] wlen)
        {
            if (wlen.Length == DATA_WIDTH)
            {
                Array.Copy(wlen, _detectorWlens, wlen.Length);
            }
        }
    }
}
