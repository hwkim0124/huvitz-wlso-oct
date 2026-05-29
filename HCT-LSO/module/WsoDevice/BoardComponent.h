#pragma once

#include "WsoDevice2.h"

#include <memory>
#include <string>


namespace wso_device
{
	class MainBoard;

	class WSODEVICE_DLL_API BoardComponent
	{
	public:
		BoardComponent();
		BoardComponent(MainBoard* board);
		virtual ~BoardComponent();

		BoardComponent(BoardComponent&& rhs);
		BoardComponent& operator=(BoardComponent&& rhs);
		BoardComponent(const BoardComponent& rhs);
		BoardComponent& operator=(const BoardComponent& rhs);

	public:
		virtual bool loadCalibParamFromProfile(void) ;
		virtual bool saveCalibParamToProfile(void) ;

		virtual bool loadConfigFromIniFile(void);
		virtual bool saveConfigToIniFile(void);

	protected:
		static IniFile* getConfigIniFile(void);
		static bool openConfigIniFile(wstring path);

	private:
		struct BoardComponentImpl;
		std::unique_ptr<BoardComponentImpl> d_ptr;
		BoardComponentImpl& impl(void) const;

	};

}


