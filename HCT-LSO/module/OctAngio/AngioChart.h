#pragma once

#include "OctAngio.h"


namespace oct_angio
{
	class OCTANGIO_DLL_API AngioChart
	{
	public:
		AngioChart();
		virtual ~AngioChart();

		AngioChart(AngioChart&& rhs);
		AngioChart& operator=(AngioChart&& rhs);
		AngioChart(const AngioChart& rhs);
		AngioChart& operator=(const AngioChart& rhs);

	public:
		virtual void setupAngioChart(cpp_util::CvImage& image, float rangeX, float rangeY, int imageW, int imageH);
		virtual void setCenterPosition(float xmm, float ymm);
		virtual bool updateAngioChart(void);
		
		void setEyeSide(EyeSide side);
		void setThreshold(float threshold);
		float getThreshold(void) const;

		EyeSide eyeSide(void) const;
		int centerPosX(void) const;
		int centerPosY(void) const;
		float centerX(void) const;
		float centerY(void) const;

		float pixelsPerXmm(void) const;
		float pixelsPerYmm(void) const;

	protected:
		cpp_util::CvImage& getAngioImage(void);

	private:
		struct AngioChartImpl;
		std::unique_ptr<AngioChartImpl> d_ptr;
		AngioChartImpl& getImpl(void) const;
	};
}

