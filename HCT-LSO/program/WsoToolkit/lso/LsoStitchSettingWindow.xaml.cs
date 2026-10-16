using System;
using System.Globalization;
using System.Windows;
using WsoToolkit.controls;
using static WsoToolkit.utils.NumberUtil;

namespace WsoToolkit
{
    /// <summary>
    /// The subframe stitch's tuning values. Edits the preview's live
    /// <see cref="LsoStitchParams"/>; Apply re-stitches what is already in review, so the
    /// numbers can be tried without shooting another capture.
    /// </summary>
    public partial class LsoStitchSettingWindow : Window
    {
        private readonly LsoStitchParams _params;
        private readonly Action _onApply;
        private readonly Func<string> _summary;

        public LsoStitchSettingWindow(LsoStitchParams stitchParams, Action onApply, Func<string> summary)
        {
            _params = stitchParams;
            _onApply = onApply;
            _summary = summary;

            InitializeComponent();

            myCbCurve.Items.Add("Linear");
            myCbCurve.Items.Add("Cosine");

            showParams_();
        }

        private void showParams_()
        {
            myTbBandHeight.Text = _params.BandHeightRows.ToString(CultureInfo.InvariantCulture);
            myTbMarginRatio.Text = _params.BandMarginRatio.ToString("0.##", CultureInfo.InvariantCulture);
            myCbCurve.SelectedIndex = _params.Curve == StitchFeatherCurve.Cosine ? 1 : 0;
            myTbFalloff.Text = _params.FalloffCompensationPercent.ToString("0.#", CultureInfo.InvariantCulture);
            myTbEdgeTail.Text = _params.EdgeTailRows.ToString(CultureInfo.InvariantCulture);
            myTbBandOffset.Text = _params.BandOffsetRows.ToString(CultureInfo.InvariantCulture);
            showSummary_();
        }

        private void showSummary_()
        {
            myLbSummary.Text = _summary?.Invoke() ?? string.Empty;
        }

        private void myBtApply_Click(object sender, RoutedEventArgs e)
        {
            _params.BandHeightRows = Math.Max(0, ToInt(myTbBandHeight.Text));
            _params.BandMarginRatio = Math.Clamp(ToDouble_(myTbMarginRatio.Text), 0.0, 1.0);
            _params.Curve = myCbCurve.SelectedIndex == 1
                ? StitchFeatherCurve.Cosine
                : StitchFeatherCurve.Linear;
            _params.FalloffCompensationPercent = Math.Clamp(ToDouble_(myTbFalloff.Text), 0.0, 90.0);
            _params.EdgeTailRows = Math.Max(0, ToInt(myTbEdgeTail.Text));
            _params.BandOffsetRows = ToInt(myTbBandOffset.Text);

            _onApply?.Invoke();

            // Clamped values are written back so the fields show what is actually in effect.
            showParams_();
        }

        private void myBtDefaults_Click(object sender, RoutedEventArgs e)
        {
            _params.CopyFrom(new LsoStitchParams());
            showParams_();
            _onApply?.Invoke();
            showSummary_();
        }

        private void myBtClose_Click(object sender, RoutedEventArgs e)
        {
            Close();
        }

        private static double ToDouble_(string text)
        {
            return double.TryParse(text, NumberStyles.Float, CultureInfo.InvariantCulture, out double value)
                ? value
                : 0.0;
        }
    }
}
