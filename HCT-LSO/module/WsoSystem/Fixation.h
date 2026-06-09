#pragma once


#include "WsoSystem2.h"

#include <memory>
#include <mutex>


namespace wso_system
{
	class WSOSYSTEM_DLL_API Fixation
	{
	public:
		Fixation();
		virtual ~Fixation();

		Fixation(const Fixation& rhs) = delete;
		Fixation& operator=(const Fixation& rhs) = delete;

		static Fixation* getInstance(void);
		static std::mutex singleMutex_;

	public:
		bool turnOnInternalLed(int row, int col);
		bool turnOnInternalLed(EyeSide side, FixationTarget target);
		bool turnOffInternalLed(void);
		bool getInternalPosition(EyeSide side, FixationTarget target, int& row, int& col) const;
		bool getCurrentInternalPosition(int& row, int& col) const;
		bool setupInternalFixation(const InternalFixationParam param) const;

	private:
		struct FixationImpl;
		std::unique_ptr<FixationImpl> d_ptr;
		FixationImpl& impl(void) const;
	};
}

