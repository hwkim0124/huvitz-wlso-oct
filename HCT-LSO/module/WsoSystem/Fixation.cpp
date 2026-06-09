#include "pch.h"
#include "Fixation.h"
#include "Hardware.h"


using namespace wso_system;
using namespace std;

std::mutex Fixation::singleMutex_;


struct Fixation::FixationImpl
{
	FixationImpl() {
		initializeFixationImpl();
	}

	void initializeFixationImpl(void) {
	}
};



wso_system::Fixation::Fixation() :
	d_ptr(std::make_unique<FixationImpl>())
{
}


wso_system::Fixation::~Fixation()
{
}


Fixation* wso_system::Fixation::getInstance(void)
{
	// Instance is constructed by public static method.
	// Static local variable initialization is thread-safe 
	// and will be initailized only once. 
	static Fixation instance;
	return &instance;
}


bool wso_system::Fixation::turnOnInternalLed(int row, int col)
{
	if (auto* board = Hardware::getInstance()->getMainBoard(); board) {
		row = min(max(row, LCD_FIXATION_ROW_MIN), LCD_FIXATION_ROW_MAX);
		col = min(max(col, LCD_FIXATION_COL_MIN), LCD_FIXATION_COL_MAX);
		return board->setLcdFixationOn(true, row, col);
	}
	return false;
}

bool wso_system::Fixation::turnOnInternalLed(EyeSide side, FixationTarget target)
{
	int row, col;
	if (!getInternalPosition(side, target, row, col)) {
		return false;
	}

	return turnOnInternalLed(row, col);
}

bool wso_system::Fixation::turnOffInternalLed(void)
{
	if (auto* board = Hardware::getInstance()->getMainBoard(); board) {
		board->setLcdFixationOn(false);
		return true;
	}
	return false;
}

bool wso_system::Fixation::getInternalPosition(EyeSide side, FixationTarget target, int& row, int& col) const
{
	std::pair<int, int> fixp;

	if (auto* config = SystemConfig::getInstance(); config) {
		if (auto* sett = config->getFixationSetting(); sett) {
			switch (target) {
				using enum FixationTarget;
			case CENTER:
				fixp = sett->getCenter(side);
				break;
			case FUNDUS:
				fixp = sett->getFundus(side);
				break;
			case OPTIC_DISC:
				fixp = sett->getOpticDisc(side);
				break;
			case LEFT_SIDE:
				fixp = sett->getLeftSide(side);
				break;
			case LEFT_TOP:
				fixp = sett->getLeftTop(side);
				break;
			case LEFT_BOTTOM:
				fixp = sett->getLeftBottom(side);
				break;
			case RIGHT_SIDE:
				fixp = sett->getRightSide(side);
				break;
			case RIGHT_TOP:
				fixp = sett->getRightTop(side);
				break;
			case RIGHT_BOTTOM:
				fixp = sett->getRightBottom(side);
				break;
			default:
				return false;
			}

			row = fixp.first;
			col = fixp.second;

			// Except center fixation, other fixation position is relative to center fixation.
			if (target != FixationTarget::CENTER) {
				row += sett->getCenter(side).first;
				col += sett->getCenter(side).second;
			}
			return true;
		}
	}
	return false;
}


bool wso_system::Fixation::getCurrentInternalPosition(int& row, int& col) const
{
	if (auto* board = Hardware::getInstance()->getMainBoard(); board) {
		return board->getLcdFixationOn(row, col);
	}
	return false;
}

bool wso_system::Fixation::setupInternalFixation(const InternalFixationParam param) const
{
	if (auto* board = Hardware::getInstance()->getMainBoard(); board) {
		return board->updateLcdParameters(param);
	}
	return false;
}


Fixation::FixationImpl& wso_system::Fixation::impl(void) const
{
	return *d_ptr;
}