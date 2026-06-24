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
using WsoNativeLib;

namespace WsoToolkit.controls
{
    public partial class OctSignalIntensityGraph : UserControl
    {
        public PlotModel Model { get; private set; }
        public int AxisY_Max { get; set; } = 81920;
        public int AxisY_Min { get; set; } = 0;
        public int AxisX_Max { get; set; } = 1024;
        public int AxisX_Min { get; set; } = 0;


        private const int DATA_WIDTH = 4096;
        private const int PEAK_AVERAGE_SIZE = 10;
        private const int SNR_AVERAGE_SIZE = 100;
        private const double AXIAL_REF_INDEX = 1.36;

        private int _callbackCount = 0;
        private int _intensityCount = 0;
        private int _averagingSize = 1;
        private int _dataBuffIndex = 0;
        private int _dataWidth = 0;
        private int _dataHeight = 0;

        private int _peakTopIndex = 0;
        private int _peakHalfIndex1 = 0;
        private int _peakHalfIndex2 = 0;
        private int _peakTermIndex1 = 0;
        private int _peakTermIndex2 = 0;

        private float _maxPeakTopValue = 0.0f;
        private float _maxPeakSnrRatio = 0.0f;
        private float _avgPeakSnrRatio = 0.0f;
        private float _sumPeakSnrRatio = 0.0f;
        private float _avgPeakFwhmDist = 0.0f;
        private float _sumPeakFwhmDist = 0.0f;

        private float _peakTopValue = 0.0f;
        private float _peakHalfValue1 = 0.0f;
        private float _peakHalfValue2 = 0.0f;
        private float _peakFwhmDist = 0.0f;
        private float _peakFloorValue = 0.0f;

        private float _peakNoiseValue = 0.0f;
        private float _peakSignalValue = 0.0f;
        private float _peakSnrRatio = 0.0f;

        public int MultipleOfZeros { get; set; } = 1;
        public int CurrentLinePosition { get; set; } = 512;
        public int FindPeakOffset1 { get; set; } = 0;
        public int FindPeakOffset2 { get; set; } = 0;
        public float FixedNoiseLevel { get; set; } = 0.0f;

        private List<float[]> _listDataBuffer = new();
        private float[] _intensityLine = new float[DATA_WIDTH];

        public OctSignalIntensityGraph()
        {
            InitializeComponent();

            Model = new PlotModel();
            plotView.Model = Model;

            SetAveragingSize(10);
            UpdatePlotAxisRange();
        }

        public void CallbackOctIntensityDataCaptured(float[] data, int width, int height)
        {
            if (data == null || data.Length == 0)
            {
                return;
            }
            _dataWidth = width;
            _dataHeight = height;
            _callbackCount += 1;

            for (int i = 0; i < _dataHeight; i++)
            {
                _listDataBuffer[_dataBuffIndex][i] = data[i * _dataWidth + CurrentLinePosition];
            }
            _dataBuffIndex = (_averagingSize <= 1 ? 0 : (_dataBuffIndex + 1) % _averagingSize);

            UpdateIntensityLine();

            if (_callbackCount >= _averagingSize && _callbackCount % _averagingSize == 0)
            {
                RedrawGraph();
            }
        }

        private void UpdateIntensityLine()
        {
            if (_callbackCount < _averagingSize)
            {
                Array.Clear(_intensityLine, 0, _intensityLine.Length);
            }
            else
            {
                if (_averagingSize <= 1)
                {
                    _intensityLine = _listDataBuffer[0];
                }
                else
                {
                    Array.Clear(_intensityLine, 0, _intensityLine.Length);
                    int size = _listDataBuffer.Count;
                    for (int i = 0; i < _dataHeight; i++)
                    {
                        for (int j = 0; j < size; j++)
                        {
                            _intensityLine[i] += _listDataBuffer[j][i];
                        }
                        _intensityLine[i] /= size;
                    }
                }
                _intensityCount += 1;

                int index1 = FindPeakOffset1;
                int index2 = _dataHeight - FindPeakOffset2;
                float maxValue = 0.0f;
                float sumValue = 0.0f;
                int maxIndex = 0;
                int count = 0;
                for (int i = index1 + 1; i < index2; i++)
                {
                    if (maxValue < _intensityLine[i])
                    {
                        maxValue = _intensityLine[i];
                        maxIndex = i;
                    }
                    sumValue += _intensityLine[i];
                    count += 1;
                }

                if (count == 0)
                {
                    return;
                }

                float avgValue = sumValue / count;
                float midValue = maxValue / 2.0f;

                _peakHalfIndex1 = 0;
                _peakHalfIndex2 = 0;
                _peakTermIndex1 = 0;
                _peakTermIndex2 = 0;
                _peakHalfValue1 = 0.0f;
                _peakHalfValue2 = 0.0f;

                for (int i = maxIndex - 1; i >= index1; i--)
                {
                    if (_intensityLine[i] <= midValue)
                    {
                        _peakHalfIndex1 = i;
                        _peakHalfValue1 = _intensityLine[i];
                        for (; i >= 0; i--)
                        {
                            _peakTermIndex1 = i;
                            if (_intensityLine[i] <= avgValue)
                            {
                                break;
                            }
                        }
                        break;
                    }
                }

                for (int i = maxIndex + 1; i <= index2; i++)
                {
                    if (_intensityLine[i] <= midValue)
                    {
                        _peakHalfIndex2 = i;
                        _peakHalfValue2 = _intensityLine[i];
                        for (; i < _dataHeight; i++)
                        {
                            _peakTermIndex2 = i;
                            if (_intensityLine[i] <= avgValue)
                            {
                                break;
                            }
                        }
                        break;
                    }
                }

                _peakFwhmDist = 0.0f;

                if (_peakHalfIndex1 > 0 && _peakHalfIndex2 > 0)
                {
                    float rate1 = (midValue - _peakHalfValue1) / (_intensityLine[_peakHalfIndex1 + 1] - _peakHalfValue1);
                    float dist1 = (_peakTopIndex - _peakHalfIndex1 - rate1);
                    float rate2 = (midValue - _peakHalfValue2) / (_intensityLine[_peakHalfIndex2 - 1] - _peakHalfValue2);
                    float dist2 = (_peakHalfIndex2 - _peakTopIndex - rate2);
                    _peakFwhmDist = dist1 + dist2;
                    _sumPeakFwhmDist += _peakFwhmDist;
                    if (_intensityCount >= PEAK_AVERAGE_SIZE && _intensityCount % PEAK_AVERAGE_SIZE == 0)
                    {
                        _avgPeakFwhmDist = _sumPeakFwhmDist / PEAK_AVERAGE_SIZE;
                        _sumPeakFwhmDist = 0.0f;
                    }
                }

                _peakFloorValue = avgValue;
                _peakTopValue = maxValue;
                _peakTopIndex = maxIndex;

                if (_peakTopValue > _maxPeakTopValue)
                {
                    _maxPeakTopValue = _peakTopValue;
                }

                sumValue = 0.0f;
                count = 0;
                for (int i = index1 + 1; i < index2; i++)
                {
                    if (_intensityLine[i] < _peakFloorValue)
                    {
                        sumValue += _intensityLine[i];
                        count += 1;
                    }
                }

                if (count > 0)
                {
                    float noise = sumValue / count;
                    float signal = _peakTopValue;
                    float ratio = 0.0f;

                    if (FixedNoiseLevel > 0.01f)
                    {
                        noise = FixedNoiseLevel;
                    }
                    if (noise > 0.0f)
                    {
                        ratio = (float)(20.0f * Math.Log10(signal / noise));
                    }

                    _peakNoiseValue = noise;
                    _peakSignalValue = signal;
                    _peakSnrRatio = ratio;

                    if (_peakSnrRatio > _maxPeakSnrRatio)
                    {
                        _maxPeakSnrRatio = _peakSnrRatio;
                    }

                    _sumPeakSnrRatio += ratio;
                    if (_intensityCount > SNR_AVERAGE_SIZE && _intensityCount % SNR_AVERAGE_SIZE == 0)
                    {
                        _avgPeakSnrRatio = _sumPeakSnrRatio / SNR_AVERAGE_SIZE;
                        _sumPeakSnrRatio = 0.0f;
                    }
                }
            }
        }

        private void RedrawGraph()
        {
            if (_intensityLine.Length == 0)
            {
                return;
            }

            var line1 = new LineSeries
            {
                StrokeThickness = 1,
                Title = "Intensity",
                Color = OxyColors.Red
            };

            var line2 = new LineSeries
            {
                Title = "Background",
                Color = OxyColors.SteelBlue,
                StrokeThickness = 1,
                LineStyle = LineStyle.Dot
            };

            line1.Points.AddRange(_intensityLine.Select((y, x) => new DataPoint(x, y)));

            for (int i = 0; i < _intensityLine.Length; i++)
            {
                line2.Points.Add(new DataPoint(i, _peakFloorValue));
            }

            Model.Series.Clear();
            Model.Series.Add(line1);
            Model.Series.Add(line2);
            Model.InvalidatePlot(true);

            var resol1 = OctScanner.GetOctAxialPixelResolution(_peakFwhmDist, AXIAL_REF_INDEX) / MultipleOfZeros;
            var resol2 = OctScanner.GetOctAxialPixelResolution(_avgPeakFwhmDist, AXIAL_REF_INDEX) / MultipleOfZeros;

            string s1 = String.Format("Peak value: {0:F0} at {1}, signal {2:F2}, noise {3:F2}", _peakTopValue, _peakTopIndex, _peakSignalValue, _peakNoiseValue);
            string s2 = String.Format("FWHM value: {0:F2} at ({1},{2}), resol {3:F4}", _peakFwhmDist, _peakHalfIndex1, _peakHalfIndex2, resol1);
            string s3 = String.Format("SNR ratio: {0:F2} db, max: {1:F2} db, avg: {2:F2} db", _peakSnrRatio, _maxPeakSnrRatio, _avgPeakSnrRatio);
            string s4 = String.Format("FWHM avg.: {0:F2}, resol {1:F4}", _avgPeakFwhmDist, resol2);

            Dispatcher.Invoke(delegate
            {
                lblStatus1.Content = s1;
                lblStatus2.Content = s2;
                lblStatus3.Content = s3;
                lblStatus4.Content = s4;
            });
        }

        public void UpdatePlotAxisRange()
        {
            var axisX = new LinearAxis
            {
                Position = AxisPosition.Bottom,
                Minimum = AxisX_Min,
                Maximum = AxisX_Max,
                MajorStep = AxisX_Max / 4,
                MinorStep = AxisX_Max / 8
            };
            var axisY = new LinearAxis
            {
                Position = AxisPosition.Left,
                Minimum = AxisY_Min,
                Maximum = AxisY_Max,
                MajorStep = AxisY_Max / 8,
                MinorStep = AxisY_Max / 16
            };
            axisY.MajorGridlineStyle = LineStyle.Dot;

            Model.Axes.Clear();
            Model.Axes.Add(axisX);
            Model.Axes.Add(axisY);
            Model.InvalidatePlot(false);
        }

        public void SetAveragingSize(int size)
        {
            _averagingSize = (size <= 1 ? 1 : (size + 1));
            _listDataBuffer.Clear();

            for (int i = 0; i < _averagingSize; i++)
            {
                _listDataBuffer.Add(new float[DATA_WIDTH]);
            }
            _callbackCount = 0;
        }

        public int GetPeakTopIndex()
        {
            return _peakTopIndex;
        }

        public void ClearChartValues()
        {
            _maxPeakTopValue = 0.0f;
            _maxPeakSnrRatio = 0.0f;
            _avgPeakSnrRatio = 0.0f;
            _sumPeakSnrRatio = 0.0f;
            _avgPeakFwhmDist = 0.0f;
            _sumPeakFwhmDist = 0.0f;

            _peakTopValue = 0.0f;
            _peakHalfValue1 = 0.0f;
            _peakHalfValue2 = 0.0f;
            _peakFwhmDist = 0.0f;
            _peakFloorValue = 0.0f;

            _peakNoiseValue = 0.0f;
            _peakSignalValue = 0.0f;
            _peakSnrRatio = 0.0f;

            _intensityCount = 0;
        }
    }
}
