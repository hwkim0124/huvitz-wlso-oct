using OpenCvSharp;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace WsoToolkit.utils
{
    internal static class RadialCorrection
    {
        /// <summary>
        /// 안구 이미지의 방사형 밝기 보정을 수행합니다.
        /// 원의 중심에서 외곽으로 갈수록 어두워지는 현상을 보정합니다.
        /// </summary>
        /// <param name="source">입력 이미지 (정방형)</param>
        /// <param name="correctionStrength">보정 강도 (0.0~1.0, 기본값 0.3은 30% 보정)</param>
        /// <param name="centerX">원의 중심 X 좌표 (기본값: 이미지 중심)</param>
        /// <param name="centerY">원의 중심 Y 좌표 (기본값: 이미지 중심)</param>
        /// <param name="radius">유효 반지름 (기본값: 이미지 크기의 절반)</param>
        /// <returns>방사형 밝기가 보정된 이미지</returns>
        public static Mat ApplyRadialBrightnessCorrection(Mat source, double correctionStrength = 0.3,
            int? centerX = null, int? centerY = null, int? radius = null)
        {
            if (source.Empty())
            {
                throw new ArgumentException("Source image is empty.");
            }

            // 기본값 설정
            int imgWidth = source.Width;
            int imgHeight = source.Height;
            int cx = centerX ?? imgWidth / 2;
            int cy = centerY ?? imgHeight / 2;
            int r = radius ?? Math.Min(imgWidth, imgHeight) / 2;

            // 결과 이미지 생성
            Mat result = new Mat();
            source.CopyTo(result);

            // 방사형 보정 맵 생성
            using Mat correctionMap = CreateRadialCorrectionMap(imgWidth, imgHeight, cx, cy, r, correctionStrength);

            // 채널별로 보정 적용
            if (source.Channels() == 1)
            {
                // 그레이스케일 이미지
                ApplyRadialCorrectionToChannel(result, correctionMap);
            }
            else if (source.Channels() == 3)
            {
                // 컬러 이미지 - 각 채널별로 적용
                var channels = new Mat[3];
                Cv2.Split(result, out channels);

                for (int i = 0; i < 3; i++)
                {
                    ApplyRadialCorrectionToChannel(channels[i], correctionMap);
                }

                Cv2.Merge(channels, result);

                // 메모리 정리
                foreach (var channel in channels)
                    channel?.Dispose();
            }

            return result;
        }

        /// <summary>
        /// 방사형 보정 맵을 생성합니다. (Safe 버전)
        /// </summary>
        private static Mat CreateRadialCorrectionMap(int width, int height, int centerX, int centerY,
            int radius, double correctionStrength)
        {
            Mat correctionMap = new Mat(height, width, MatType.CV_32F, new Scalar(1.0f));

            // 원의 경계 박스 계산하여 불필요한 연산 줄이기
            int minX = Math.Max(0, centerX - radius);
            int maxX = Math.Min(width - 1, centerX + radius);
            int minY = Math.Max(0, centerY - radius);
            int maxY = Math.Min(height - 1, centerY + radius);

            double radiusSquared = radius * radius;

            for (int y = minY; y <= maxY; y++)
            {
                for (int x = minX; x <= maxX; x++)
                {
                    double dx = x - centerX;
                    double dy = y - centerY;
                    double distanceSquared = dx * dx + dy * dy;

                    if (distanceSquared <= radiusSquared)
                    {
                        double distance = Math.Sqrt(distanceSquared);
                        double normalizedDistance = distance / radius;
                        double correctionFactor = 1.0 + (correctionStrength * normalizedDistance);

                        correctionMap.Set<float>(y, x, (float)correctionFactor);
                    }
                }
            }

            return correctionMap;
        }


        /// <summary>
        /// 단일 채널에 방사형 보정을 적용합니다.
        /// </summary>
        private static void ApplyRadialCorrectionToChannel(Mat channel, Mat correctionMap)
        {
            // 채널을 float32로 변환
            using Mat floatChannel = new Mat();
            channel.ConvertTo(floatChannel, MatType.CV_32F);

            // 보정 맵과 곱셈
            using Mat corrected = new Mat();
            Cv2.Multiply(floatChannel, correctionMap, corrected);

            // 다시 원래 타입으로 변환 (0-255 범위로 클리핑)
            corrected.ConvertTo(channel, channel.Type());
        }

        /// <summary>
        /// 고급 방사형 밝기 보정 (비선형 보정 곡선 사용)
        /// </summary>
        /// <param name="source">입력 이미지</param>
        /// <param name="correctionStrength">보정 강도</param>
        /// <param name="gamma">감마 보정 값 (기본값 1.5, 클수록 부드러운 보정)</param>
        /// <param name="centerX">원의 중심 X</param>
        /// <param name="centerY">원의 중심 Y</param>
        /// <param name="radius">유효 반지름</param>
        /// <returns>보정된 이미지</returns>
        public static Mat ApplyAdvancedRadialBrightnessCorrection(Mat source, double correctionStrength = 0.3,
            double gamma = 1.5, int? centerX = null, int? centerY = null, int? radius = null)
        {
            if (source.Empty())
            {
                throw new ArgumentException("Source image is empty.");
            }

            // 기본값 설정
            int imgWidth = source.Width;
            int imgHeight = source.Height;
            int cx = centerX ?? imgWidth / 2;
            int cy = centerY ?? imgHeight / 2;
            int r = radius ?? Math.Min(imgWidth, imgHeight) / 2;

            // 결과 이미지 생성
            Mat result = new Mat();
            source.CopyTo(result);

            // 고급 방사형 보정 맵 생성
            using Mat correctionMap = CreateAdvancedRadialCorrectionMap(imgWidth, imgHeight, cx, cy, r,
                correctionStrength, gamma);

            // 채널별로 보정 적용
            if (source.Channels() == 1)
            {
                ApplyRadialCorrectionToChannel(result, correctionMap);
            }
            else if (source.Channels() == 3)
            {
                var channels = new Mat[3];
                Cv2.Split(result, out channels);

                for (int i = 0; i < 3; i++)
                {
                    ApplyRadialCorrectionToChannel(channels[i], correctionMap);
                }

                Cv2.Merge(channels, result);

                foreach (var channel in channels)
                    channel?.Dispose();
            }

            return result;
        }

        /// <summary>
        /// 고급 방사형 보정 맵 생성 (비선형 곡선 사용)
        /// </summary>
        private static Mat CreateAdvancedRadialCorrectionMap(int width, int height, int centerX, int centerY,
           int radius, double correctionStrength, double gamma)
        {
            Mat correctionMap = new Mat(height, width, MatType.CV_32F, new Scalar(1.0f));

            int minX = Math.Max(0, centerX - radius);
            int maxX = Math.Min(width - 1, centerX + radius);
            int minY = Math.Max(0, centerY - radius);
            int maxY = Math.Min(height - 1, centerY + radius);

            double radiusSquared = radius * radius;

            for (int y = minY; y <= maxY; y++)
            {
                for (int x = minX; x <= maxX; x++)
                {
                    double dx = x - centerX;
                    double dy = y - centerY;
                    double distanceSquared = dx * dx + dy * dy;

                    if (distanceSquared <= radiusSquared)
                    {
                        double distance = Math.Sqrt(distanceSquared);
                        double normalizedDistance = distance / radius;

                        // 감마 곡선을 사용한 부드러운 보정
                        double gammaCorrectedDistance = Math.Pow(normalizedDistance, 1.0 / gamma);
                        double correctionFactor = 1.0 + (correctionStrength * gammaCorrectedDistance);

                        correctionMap.Set<float>(y, x, (float)correctionFactor);
                    }
                }
            }

            return correctionMap;
        }

        /// <summary>
        /// 적응형 방사형 밝기 보정 (이미지 분석 기반)
        /// </summary>
        /// <param name="source">입력 이미지</param>
        /// <param name="analysisRadius">분석할 반지름 (기본값: 자동 계산)</param>
        /// <returns>보정된 이미지</returns>
        public static Mat ApplyAdaptiveRadialBrightnessCorrection(Mat source, int? analysisRadius = null)
        {
            if (source.Empty())
            {
                throw new ArgumentException("Source image is empty.");
            }

            int imgWidth = source.Width;
            int imgHeight = source.Height;
            int cx = imgWidth / 2;
            int cy = imgHeight / 2;
            int r = analysisRadius ?? Math.Min(imgWidth, imgHeight) / 2;

            // 그레이스케일 변환 (분석용)
            using Mat gray = new Mat();
            if (source.Channels() == 1)
            {
                source.CopyTo(gray);
            }
            else
            {
                Cv2.CvtColor(source, gray, ColorConversionCodes.BGR2GRAY);
            }

            // 중심부와 외곽부의 평균 밝기 계산
            double centerBrightness = CalculateRegionBrightness(gray, cx, cy, r / 4); // 중심 25% 영역
            double edgeBrightness = CalculateRegionBrightness(gray, cx, cy, r, r * 3 / 4); // 외곽 25% 영역

            // 보정 강도 자동 계산
            double brightnessRatio = centerBrightness / Math.Max(edgeBrightness, 1.0);
            double adaptiveCorrectionStrength = Math.Min((brightnessRatio - 1.0) * 0.8, 0.5); // 최대 50% 보정

            System.Diagnostics.Debug.WriteLine($"적응형 방사형 보정: 중심밝기={centerBrightness:F1}, " +
                $"외곽밝기={edgeBrightness:F1}, 보정강도={adaptiveCorrectionStrength:F3}");

            // 계산된 보정 강도로 보정 적용
            return ApplyAdvancedRadialBrightnessCorrection(source, adaptiveCorrectionStrength, 1.8, cx, cy, r);
        }

        /// <summary>
        /// 특정 영역의 평균 밝기를 계산합니다.
        /// </summary>
        private static double CalculateRegionBrightness(Mat grayImage, int centerX, int centerY,
            int outerRadius, int innerRadius = 0)
        {
            using Mat mask = new Mat(grayImage.Size(), MatType.CV_8U, Scalar.All(0));

            // 도넛 모양 마스크 생성
            Cv2.Circle(mask, new OpenCvSharp.Point(centerX, centerY), outerRadius, Scalar.All(255), -1);
            if (innerRadius > 0)
            {
                Cv2.Circle(mask, new OpenCvSharp.Point(centerX, centerY), innerRadius, Scalar.All(0), -1);
            }

            Scalar meanValue = Cv2.Mean(grayImage, mask);
            return meanValue.Val0;
        }

        /// <summary>
        /// 사용자 정의 곡선을 사용한 방사형 밝기 보정
        /// </summary>
        /// <param name="source">입력 이미지</param>
        /// <param name="correctionCurve">보정 곡선 배열 (0~1 범위의 정규화된 거리에 대한 보정 계수)</param>
        /// <param name="centerX">원의 중심 X</param>
        /// <param name="centerY">원의 중심 Y</param>
        /// <param name="radius">유효 반지름</param>
        /// <returns>보정된 이미지</returns>
        public static Mat ApplyCustomCurveRadialCorrection(Mat source, double[] correctionCurve,
            int? centerX = null, int? centerY = null, int? radius = null)
        {
            if (source.Empty())
            {
                throw new ArgumentException("Source image is empty.");
            }

            if (correctionCurve == null || correctionCurve.Length == 0)
            {
                return source.Clone();
            }

            // 기본값 설정
            int imgWidth = source.Width;
            int imgHeight = source.Height;
            int cx = centerX ?? imgWidth / 2;
            int cy = centerY ?? imgHeight / 2;
            int r = radius ?? Math.Min(imgWidth, imgHeight) / 2;

            // 결과 이미지 생성
            Mat result = new Mat();
            source.CopyTo(result);

            // 사용자 정의 곡선 기반 보정 맵 생성
            using Mat correctionMap = CreateCustomCurveRadialCorrectionMap(
                imgWidth, imgHeight, cx, cy, r, correctionCurve);

            // 채널별로 보정 적용
            if (source.Channels() == 1)
            {
                ApplyRadialCorrectionToChannel(result, correctionMap);
            }
            else if (source.Channels() == 3)
            {
                var channels = new Mat[3];
                Cv2.Split(result, out channels);

                for (int i = 0; i < 3; i++)
                {
                    ApplyRadialCorrectionToChannel(channels[i], correctionMap);
                }

                Cv2.Merge(channels, result);

                foreach (var channel in channels)
                    channel?.Dispose();
            }

            return result;
        }

        /// <summary>
        /// 사용자 정의 곡선 기반 방사형 보정 맵 생성
        /// </summary>
        private static Mat CreateCustomCurveRadialCorrectionMap(int width, int height,
            int centerX, int centerY, int radius, double[] correctionCurve)
        {
            Mat correctionMap = new Mat(height, width, MatType.CV_32F, new Scalar(1.0f));

            int minX = Math.Max(0, centerX - radius);
            int maxX = Math.Min(width - 1, centerX + radius);
            int minY = Math.Max(0, centerY - radius);
            int maxY = Math.Min(height - 1, centerY + radius);

            double radiusSquared = radius * radius;
            int curveLength = correctionCurve.Length;

            for (int y = minY; y <= maxY; y++)
            {
                for (int x = minX; x <= maxX; x++)
                {
                    double dx = x - centerX;
                    double dy = y - centerY;
                    double distanceSquared = dx * dx + dy * dy;

                    if (distanceSquared <= radiusSquared)
                    {
                        double distance = Math.Sqrt(distanceSquared);
                        double normalizedDistance = distance / radius;

                        // 곡선 배열에서 보정 계수 가져오기 (선형 보간)
                        double curveIndex = normalizedDistance * (curveLength - 1);
                        int lowerIndex = (int)Math.Floor(curveIndex);
                        int upperIndex = Math.Min(lowerIndex + 1, curveLength - 1);
                        double t = curveIndex - lowerIndex;

                        // UI의 곡선 값 (0~1 범위)을 가져옴
                        double curveValue = correctionCurve[lowerIndex] * (1 - t) +
                                           correctionCurve[upperIndex] * t;

                        // UI에서 0=보정없음이므로, 실제 보정 계수는 (1 + curveValue)
                        // curveValue=0 → correctionFactor=1.0 (보정 없음)
                        // curveValue=0.5 → correctionFactor=1.5 (50% 밝기 증가)
                        // curveValue=1.0 → correctionFactor=2.0 (100% 밝기 증가)
                        double correctionFactor = 1.0 + curveValue;

                        correctionMap.Set<float>(y, x, (float)correctionFactor);
                    }
                }
            }

            return correctionMap;
        }
    }
}
