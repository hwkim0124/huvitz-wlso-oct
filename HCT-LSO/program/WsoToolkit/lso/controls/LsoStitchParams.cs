using System;
using System.Collections.Generic;

namespace WsoToolkit.controls
{
    public enum StitchFeatherCurve
    {
        Linear = 0,
        Cosine = 1,
    }

    // Everything the subframe stitch is allowed to be tuned with, plus the geometry those
    // values produce. The merge and the Bands-view guide lines both read it here so they can
    // never disagree, and there is no WPF in this file - the arithmetic can be checked with
    // no window, no camera and no board.
    //
    // The band model, as measured on the instrument: frame j is lit over one band of the
    // sensor, the bands walk down the frame, and neighbouring frames still carry usable
    // signal a little beyond their own band. So a band is cut with a margin on both sides and
    // the margins are cross-faded into each other instead of being cut off hard.
    public sealed class LsoStitchParams
    {
        // A dark margin row divided by a small weight would explode, so the falloff
        // compensation is capped.
        public const double MaxCompensationGain = 4.0;

        // One band's height. 0 = derive it from the capture (frame height / subframe count),
        // which is what the scan is supposed to produce; type a value when the lit band is
        // measurably taller or shorter than that.
        public int BandHeightRows { get; set; }

        // How much is taken beyond the band on each side, as a fraction of the band height.
        // 0 = no overlap (the old hard cut), 0.5 = half a band above and below, so two
        // neighbours share a full band's worth of rows.
        public double BandMarginRatio { get; set; } = 0.5;

        public StitchFeatherCurve Curve { get; set; } = StitchFeatherCurve.Linear;

        // How much darker the band's edge is than its centre, in percent. The merge divides
        // that profile back out. 0 = leave the image as captured.
        public double FalloffCompensationPercent { get; set; }

        // How far outside its own band a frame still carries usable signal. It caps the
        // margin: taking more than this would blend black rows into the seam. 0 = no cap.
        public int EdgeTailRows { get; set; }

        // Shifts every band's crop together, for when the lit bands sit systematically above
        // or below where the subframe arithmetic puts them.
        public int BandOffsetRows { get; set; }

        // --- Measured-profile settings (LsoBandProfiler) ----------------------------------
        // The values above describe the band the scan was SUPPOSED to produce. These control
        // measuring the band the frame actually got, which is what a merge should weight by.

        // Rows searched either side of the expected band centre.
        public int ProfileSearchRadiusRows { get; set; } = 200;

        // A column counts as covered when its band signal reaches this percent of the
        // strongest column. The lit field is a disc, so the outer columns of the top and
        // bottom bands carry nothing.
        public double ValidColumnPercent { get; set; } = 5.0;

        // Degrees of the polynomials smoothing the per-column centre and amplitude. The
        // centre needs at least 2 to express the band's bow; 3 covers the measured shape.
        public int CenterFitDegree { get; set; } = 3;

        public int AmplitudeFitDegree { get; set; } = 4;

        // Every Nth column is sampled for the cross-section median. 7 keeps ~500 samples on a
        // 3648-wide frame, which is plenty and keeps the measurement well under a second.
        public int ShapeStrideColumns { get; set; } = 7;

        // Where one band comes from and how far it is trusted.
        //   Core0..Core1 : the rows this band alone is responsible for
        //   Src0..Src1   : the rows actually cut out of the frame (core + margin, clamped)
        public readonly struct BandGeometry
        {
            public BandGeometry(int core0, int core1, int src0, int src1, int margin)
            {
                Core0 = core0;
                Core1 = core1;
                Src0 = src0;
                Src1 = src1;
                Margin = margin;
            }

            public int Core0 { get; }

            public int Core1 { get; }

            public int Src0 { get; }

            public int Src1 { get; }

            public int Margin { get; }

            public int CoreHeight => Core1 - Core0;

            public int SrcHeight => Src1 - Src0;
        }

        public int EffectiveBandHeight(int frameHeight, int subFrames)
        {
            if (BandHeightRows > 0)
            {
                return BandHeightRows;
            }

            return subFrames > 0 ? frameHeight / subFrames : frameHeight;
        }

        public int EffectiveMargin(int frameHeight, int subFrames)
        {
            int segH = EffectiveBandHeight(frameHeight, subFrames);
            int margin = (int)Math.Round(segH * Math.Max(0.0, BandMarginRatio));
            if (EdgeTailRows > 0)
            {
                margin = Math.Min(margin, EdgeTailRows);
            }

            return Math.Max(0, margin);
        }

        // Empty when the inputs cannot describe bands at all; the caller then leaves review on
        // the raw frames, exactly as it did before there was a stitch.
        public List<BandGeometry> Plan(int frameHeight, int subFrames)
        {
            var bands = new List<BandGeometry>();
            if (frameHeight <= 1 || subFrames <= 1)
            {
                return bands;
            }

            int segH = EffectiveBandHeight(frameHeight, subFrames);
            if (segH <= 0)
            {
                return bands;
            }

            int margin = EffectiveMargin(frameHeight, subFrames);

            for (int j = 0; j < subFrames; ++j)
            {
                int core0 = Math.Clamp((j * segH) + BandOffsetRows, 0, frameHeight - 1);

                // The last band absorbs whatever the division left over, as it always has.
                int core1 = j == subFrames - 1
                    ? frameHeight
                    : ((j + 1) * segH) + BandOffsetRows;
                core1 = Math.Clamp(core1, core0 + 1, frameHeight);

                int src0 = Math.Max(0, core0 - margin);
                int src1 = Math.Min(frameHeight, core1 + margin);
                bands.Add(new BandGeometry(core0, core1, src0, src1, margin));
            }

            return bands;
        }

        // 1 where this band is alone, ramping to 0 across the 2 x margin zone centred on each
        // seam, 0 outside the cut. The first and last band keep their outer edge at 1 - there
        // is no neighbour out there to fade into.
        public double WeightAt(IReadOnlyList<BandGeometry> bands, int index, int y)
        {
            if (bands == null || index < 0 || index >= bands.Count)
            {
                return 0.0;
            }

            BandGeometry g = bands[index];
            if (y < g.Src0 || y >= g.Src1)
            {
                return 0.0;
            }

            if (g.Margin <= 0)
            {
                return 1.0;
            }

            double span = 2.0 * g.Margin;
            double weight = 1.0;

            if (index > 0)
            {
                weight = Math.Min(weight, ramp_((y - (g.Core0 - g.Margin)) / span));
            }

            if (index < bands.Count - 1)
            {
                weight = Math.Min(weight, ramp_(((g.Core1 + g.Margin) - y) / span));
            }

            return Math.Clamp(weight, 0.0, 1.0);
        }

        // Divides the in-band brightness profile back out: the edge of a band is darker than
        // its centre by FalloffCompensationPercent, so the edge gets the bigger gain.
        public double GainAt(BandGeometry band, int y)
        {
            double drop = Math.Clamp(FalloffCompensationPercent / 100.0, 0.0, 0.9);
            if (drop <= 0.0)
            {
                return 1.0;
            }

            double half = Math.Max(1.0, band.CoreHeight / 2.0);
            double u = Math.Min(1.0, Math.Abs(y - ((band.Core0 + band.Core1) / 2.0)) / half);
            double denominator = 1.0 - (drop * u * u);
            return denominator <= 1e-6
                ? MaxCompensationGain
                : Math.Min(MaxCompensationGain, 1.0 / denominator);
        }

        public LsoStitchParams Clone()
        {
            return new LsoStitchParams
            {
                BandHeightRows = BandHeightRows,
                BandMarginRatio = BandMarginRatio,
                Curve = Curve,
                FalloffCompensationPercent = FalloffCompensationPercent,
                EdgeTailRows = EdgeTailRows,
                BandOffsetRows = BandOffsetRows,
            };
        }

        public void CopyFrom(LsoStitchParams source)
        {
            if (source == null)
            {
                return;
            }

            BandHeightRows = source.BandHeightRows;
            BandMarginRatio = source.BandMarginRatio;
            Curve = source.Curve;
            FalloffCompensationPercent = source.FalloffCompensationPercent;
            EdgeTailRows = source.EdgeTailRows;
            BandOffsetRows = source.BandOffsetRows;
        }

        private double ramp_(double t)
        {
            double clamped = Math.Clamp(t, 0.0, 1.0);
            return Curve == StitchFeatherCurve.Cosine
                ? 0.5 - (0.5 * Math.Cos(Math.PI * clamped))
                : clamped;
        }
    }
}
