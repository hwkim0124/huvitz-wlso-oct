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
		bool initializeBoardComponent(void);
		bool isInitiated(void) const;
		void setInitiated(bool flag); 

		virtual bool loadCalibParamFromProfile(void) ;
		virtual bool saveCalibParamToProfile(void) ;

	protected:
		MainBoard* getMainBoard(void) const;
		static IniFile* getConfigIniFile(void);
		static bool openConfigIniFile(wstring path);

		virtual bool loadConfigFromIniFile(void);
		virtual bool saveConfigToIniFile(void);

	private:
		struct BoardComponentImpl;
		std::unique_ptr<BoardComponentImpl> d_ptr;
		BoardComponentImpl& impl(void) const;

	};

}


