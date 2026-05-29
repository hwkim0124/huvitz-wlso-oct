#pragma once

#include "WsoDevice2.h"
#include "BoardComponent.h"

#include <memory>
#include <string>

#include "wso_domain.h"


namespace wso_device
{
	class MainBoard;
	class TraceProfile;


	class WSODEVICE_DLL_API Galvanometer : public BoardComponent
	{
	public:
		Galvanometer(MainBoard* board);
		virtual ~Galvanometer();

		Galvanometer(Galvanometer&& rhs);
		Galvanometer& operator=(Galvanometer&& rhs);
		// Prevent copy construction and assignment. 
		Galvanometer(const Galvanometer& rhs) = delete;
		Galvanometer& operator=(const Galvanometer& rhs) = delete;


	public:
		void initializeGalvanometer(void);
		bool isInitiated(void) const;

		short getStepXat(float mm, bool cornea);
		short getStepXatRetina(float mm);
		short getStepXatCornea(float mm);

		short getStepYat(float mm, bool cornea);
		short getStepYatRetina(float mm);
		short getStepYatCornea(float mm);

		void setScaleX(double scale);
		void setScaleY(double scale);

		void setTriggerTimeStep(float timeStep);
		void setTriggerTimeDelay(std::uint32_t timeDelay);
		void setTriggerForePaddings(OctScanSpeed speed, std::uint32_t padds);
		void setTriggerPostPaddings(OctScanSpeed speed, std::uint32_t padds);

		float getTriggerTimeStep(void);
		std::uint32_t getTriggerTimeDelay(void);
		std::uint32_t getTriggerForePaddings(OctScanSpeed speed);
		std::uint32_t getTriggerPostPaddings(OctScanSpeed speed);

		void setRetinaPositionFunctionCoefficientsX(double coeffs[GALVANO_POSITION_FUNCTION_DEGREE]);
		void setCorneaPositionFunctionCoefficientsX(double coeffs[GALVANO_POSITION_FUNCTION_DEGREE]);
		void setRetinaPositionFunctionCoefficientsY(double coeffs[GALVANO_POSITION_FUNCTION_DEGREE]);
		void setCorneaPositionFunctionCoefficientsY(double coeffs[GALVANO_POSITION_FUNCTION_DEGREE]);

		TraceProfile* getTraceProfile(int traceId);
		bool setTraceProfile(int traceId, short forePadd, short numPoints, short postPadd, short numRepeats);
		bool setTraceProfile(int traceId, short forePadd, short numPoints, short postPadd, short numRepeats,
			float timeStep, uint32_t timeDelay);
		bool setTracePositionsX(int traceId, short* posXs, short count);
		bool setTracePositionsY(int traceId, short* posYs, short count);

		bool buildTracePositionsX(EyeSide eyeSide, short forePadd, short numPoints, short postPadd, float start, float close, bool cornea, bool circle, short* buffer, short* count);
		bool buildTracePositionsY(EyeSide eyeSide, short forePadd, short numPoints, short postPadd, float start, float close, bool cornea, bool circle, short* buffer, short* count);

		bool scanMoveXY(int traceId);
		bool scanRepeatX(int traceId, int repeats, short offsetX = 0, short offsetY = 0);
		bool scanRepeatY(int traceId, int repeats, short offsetX = 0, short offsetY = 0);
		bool scanRepeatFastX(int traceId, int repeats, short offsetX = 0, short offsetY = 0);
		bool scanRepeatFastY(int traceId, int repeats, short offsetX = 0, short offsetY = 0);

		bool slewMoveXY(float xpos, float ypos, bool cornea);
		bool slewMoveXY(short xpos, short ypos);

	protected:
		MainBoard* getMainBoard(void) const;

	private:
		struct GalvanometerImpl;
		std::unique_ptr<GalvanometerImpl> d_ptr;
		GalvanometerImpl& impl(void) const;
	};
}

