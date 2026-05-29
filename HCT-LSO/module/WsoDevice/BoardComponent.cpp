#include "pch.h"
#include "BoardComponent.h"


using namespace wso_device;
using namespace std;


static IniFile* _iniConfig = nullptr;
static std::wstring CONFIG_INI_DEFAULT_PATH = L".//WsoDeviceCfg.ini";


struct BoardComponent::BoardComponentImpl
{
	MainBoard* board;
	IniFile* _iniConfig;

	BoardComponentImpl() {
		initBoardComponentImpl();
	}

	BoardComponentImpl(MainBoard* board) {
		BoardComponentImpl();
		this->board = board;
	}

	void initBoardComponentImpl(void) {
		board = nullptr;
	}
};


BoardComponent::BoardComponent() :
	d_ptr(make_unique<BoardComponentImpl>())
{
}


wso_device::BoardComponent::BoardComponent(MainBoard* board) :
	d_ptr(make_unique<BoardComponentImpl>(board))
{
}


BoardComponent::~BoardComponent()
{
}


// Copy constructor and assignment by const reference are not allowed here 
// becuase the innert implementation struct has unique_ptr member for exclusive-ownership.
// It needs additional definition of copy constructor and assignment for the struct to resolve 
// the situation. 
BoardComponent::BoardComponent::BoardComponent(BoardComponent&& rhs) = default;
BoardComponent& BoardComponent::BoardComponent::operator=(BoardComponent&& rhs) = default;


wso_device::BoardComponent::BoardComponent(const BoardComponent& rhs)
	: d_ptr(make_unique<BoardComponentImpl>(*rhs.d_ptr))
{
	openConfigIniFile(CONFIG_INI_DEFAULT_PATH);
}


BoardComponent& wso_device::BoardComponent::operator=(const BoardComponent& rhs)
{
	*d_ptr = *rhs.d_ptr;
	return *this;
}


bool wso_device::BoardComponent::loadCalibParamFromProfile(void)
{
	return true;
}


bool wso_device::BoardComponent::saveCalibParamToProfile(void)
{
	return true;
}


IniFile* wso_device::BoardComponent::getConfigIniFile(void)
{
	return _iniConfig;
}

bool wso_device::BoardComponent::openConfigIniFile(wstring path)
{
	_iniConfig = new IniFile(path);
	return false;
}

bool wso_device::BoardComponent::loadConfigFromIniFile(void)
{
	return true;
}

bool wso_device::BoardComponent::saveConfigToIniFile(void)
{
	return true;
}

BoardComponent::BoardComponentImpl& wso_device::BoardComponent::impl(void) const
{
	return *d_ptr;
}
