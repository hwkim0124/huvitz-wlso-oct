#pragma once

#include "WsoSystem2.h"

#include <memory>
#include <mutex>


namespace wso_system
{
	class WSOSYSTEM_DLL_API Controller
	{
	public:
		Controller();
		virtual ~Controller();

		Controller(const Controller& rhs) = delete;
		Controller& operator=(const Controller& rhs) = delete;

		static Controller* getInstance(void);
		static std::mutex singleMutex_;

	public:
		bool changeOctDiopterFocus(float diopt);
		bool changeOctPolarization(float degree);

		bool moveOctReferenceToOrigin(bool is_cornea);
		bool moveOctDiopterFocusToOrigin(void);
		bool moveOctPolarizationToOrigin(void);
		// bool moveFixationToOrigin(void);

	private:
		struct ControllerImpl;
		std::unique_ptr<ControllerImpl> d_ptr;
		ControllerImpl& impl(void) const;
	};
}


