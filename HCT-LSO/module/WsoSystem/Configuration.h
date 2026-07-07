#pragma once


#include "WsoSystem2.h"

#include <memory>
#include <mutex>


namespace wso_system
{
	class WSOSYSTEM_DLL_API Configuration
	{
	public:
		Configuration();
		virtual ~Configuration();

		Configuration(const Configuration& rhs) = delete;
		Configuration& operator=(const Configuration& rhs) = delete;

		static Configuration* getInstance(void);
		static std::mutex singleMutex_;

	public:
		bool loadSystemConfiguration(bool fetch = true);
		bool saveSystemConfiguration(bool write = true);
		bool applySystemConfiguration(void);

	private:
		struct ConfigurationImpl;
		std::unique_ptr<ConfigurationImpl> d_ptr;
		ConfigurationImpl& impl(void) const;
	};
}

