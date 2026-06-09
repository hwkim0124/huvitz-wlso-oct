#pragma once

#include "OctResult2.h"
#include "BscanBundle.h"

#include <memory>
#include <vector>


namespace oct_result
{
	class BscanSection;
	class BscanBundle;

	class OCTRESULT_DLL_API AngioOutput : public BscanBundle
	{
	public:
		AngioOutput();
		virtual ~AngioOutput();

		AngioOutput(AngioOutput&& rhs);
		AngioOutput& operator=(AngioOutput&& rhs);
		AngioOutput(const AngioOutput& rhs) = delete;
		AngioOutput& operator=(const AngioOutput& rhs) = delete;

	public:
		bool setAmplitudes(std::vector<std::vector<CvImage>>&& ampls);
		std::vector<std::vector<CvImage>>& getAmplitudes(void) const;

	private:
		struct AngioOutputImpl;
		std::unique_ptr<AngioOutputImpl> d_ptr;
		AngioOutputImpl& impl(void) const;
	};
}


