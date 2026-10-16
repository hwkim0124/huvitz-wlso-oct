#pragma once

#include <cmath>

namespace wso_device
{
	namespace detail
	{
		enum class CameraGeometryNode
		{
			Width,
			Height,
			OffsetX,
			OffsetY,
			BinningHorizontal,
			BinningVertical,
		};

		struct CameraIntegerRange
		{
			long long minimum = 0;
			long long maximum = 0;
			long long increment = 1;
		};

		struct ColorCameraGeometryTarget
		{
			long long binningHorizontal = 1;
			long long binningVertical = 1;
			long long width = 0;
			long long height = 0;
			long long offsetX = 0;
			long long offsetY = 0;
		};

		inline bool validCameraIntegerRange(const CameraIntegerRange& range)
		{
			return range.maximum >= range.minimum && range.increment > 0;
		}

		inline long long alignCameraInteger(long long requested, const CameraIntegerRange& range)
		{
			const long long clamped = requested < range.minimum ? range.minimum
				: (requested > range.maximum ? range.maximum : requested);
			return range.minimum + ((clamped - range.minimum) / range.increment) * range.increment;
		}

		// Geometry node limits are dynamic: WidthMax=2736 at 2x2 becomes 5472 at 1x1,
		// and OffsetMax changes again after Width is installed. Make the live ROI harmless
		// first, establish the requested binning, then read and honour each current range.
		template <typename Write, typename ReadRange>
		bool runColorCameraGeometryApply(const ColorCameraGeometryTarget& requested,
			Write&& write, ReadRange&& readRange, ColorCameraGeometryTarget* applied = nullptr)
		{
			ColorCameraGeometryTarget result = requested;
			CameraIntegerRange range;

			auto writeMinimum = [&](CameraGeometryNode node) {
				return readRange(node, range) && validCameraIntegerRange(range)
					&& write(node, range.minimum);
			};
			if (!writeMinimum(CameraGeometryNode::OffsetX)
				|| !writeMinimum(CameraGeometryNode::OffsetY)
				|| !writeMinimum(CameraGeometryNode::Width)
				|| !writeMinimum(CameraGeometryNode::Height)) {
				return false;
			}

			if (!write(CameraGeometryNode::BinningHorizontal, requested.binningHorizontal)
				|| !write(CameraGeometryNode::BinningVertical, requested.binningVertical)) {
				return false;
			}

			auto writeRequested = [&](CameraGeometryNode node, long long value, long long& actual) {
				if (!readRange(node, range) || !validCameraIntegerRange(range)) {
					return false;
				}
				actual = alignCameraInteger(value, range);
				return write(node, actual);
			};
			if (!writeRequested(CameraGeometryNode::Width, requested.width, result.width)
				|| !writeRequested(CameraGeometryNode::Height, requested.height, result.height)
				|| !writeRequested(CameraGeometryNode::OffsetX, requested.offsetX, result.offsetX)
				|| !writeRequested(CameraGeometryNode::OffsetY, requested.offsetY, result.offsetY)) {
				return false;
			}

			if (applied != nullptr) {
				*applied = result;
			}
			return true;
		}

		struct ColorCameraFullApplyVerdict
		{
			bool extendedVerified = false;
			bool baseVerified = false;
		};

		inline double clampCameraFloat(double value, double minimum, double maximum)
		{
			return value < minimum ? minimum : (value > maximum ? maximum : value);
		}

		inline bool cameraFloatReadbackMatches(double target, double actual, double increment)
		{
			// A node that publishes no increment still quantises to its own grid: Spinnaker's
			// Gain has HasInc() == false, and a requested 27.04 dB reads back as 27.045664. A
			// flat 0.001 is tighter than that grid, so a device doing exactly what it was told
			// failed the apply. The fallback therefore scales with the value (0.1%) and keeps
			// 0.001 as the floor for values near zero.
			const double scaled = std::abs(target) * 0.001;
			const double fallback = scaled > 0.001 ? scaled : 0.001;
			const double tolerance = increment > 0.0 ? increment : fallback;
			return std::abs(actual - target) <= tolerance;
		}

		template <typename Apply>
		bool applyManualCameraFloat(bool automatic, Apply&& apply)
		{
			return automatic || apply();
		}

		// Device-free executor for the dependency-sensitive portions of applyFullSettings.
		// The callbacks are the actual production operations; tests can supply stateful fake
		// camera nodes and observe the resulting values and read-back verdicts.
		template <typename SensorPhase, typename BasePhase, typename DevicePhase,
			typename ExposurePhase, typename GainPhase>
		ColorCameraFullApplyVerdict runColorCameraFullApply(SensorPhase&& sensorPhase,
			BasePhase&& basePhase, DevicePhase&& devicePhase, ExposurePhase&& exposurePhase,
			GainPhase&& gainPhase)
		{
			ColorCameraFullApplyVerdict verdict;
			verdict.extendedVerified = sensorPhase();
			verdict.baseVerified = basePhase();
			if (!devicePhase()) {
				verdict.extendedVerified = false;
			}
			const bool exposureVerified = exposurePhase();
			const bool gainVerified = gainPhase();
			verdict.baseVerified = verdict.baseVerified && exposureVerified && gainVerified;
			return verdict;
		}
	}
}
