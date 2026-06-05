#include "pch.h"
#include "PatternFrame.h"
#include "LineTrace.h"

using namespace oct_pattern;


struct PatternFrame::PatternFrameImpl
{
	LineTraceVect lines;

	PatternFrameImpl()
	{
	}
};


PatternFrame::PatternFrame() :
	d_ptr(make_unique<PatternFrameImpl>())
{
}


oct_pattern::PatternFrame::~PatternFrame() = default;
oct_pattern::PatternFrame::PatternFrame(PatternFrame && rhs) = default;
PatternFrame & oct_pattern::PatternFrame::operator=(PatternFrame && rhs) = default;


oct_pattern::PatternFrame::PatternFrame(const PatternFrame & rhs)
	: d_ptr(make_unique<PatternFrameImpl>(*rhs.d_ptr))
{
}


PatternFrame & oct_pattern::PatternFrame::operator=(const PatternFrame & rhs)
{
	*d_ptr = *rhs.d_ptr;
	return *this;
}


void oct_pattern::PatternFrame::addLineTrace(LineTrace & trace)
{
	getImpl().lines.push_back(move(trace));
	return;
}


std::vector<LineTrace> & oct_pattern::PatternFrame::getLineTraceList(void) const
{
	return getImpl().lines;
}


int oct_pattern::PatternFrame::getLineTraceListSize(void) const
{
	return (int)getImpl().lines.size();
}


void oct_pattern::PatternFrame::clearLineTraceList(void)
{
	getImpl().lines.clear();
	return;
}


LineTrace * oct_pattern::PatternFrame::getLineTraceFirst(void) const
{
	return getLineTrace(0);
}


LineTrace * oct_pattern::PatternFrame::getLineTrace(int index) const
{
	if (index >= 0 && index < getLineTraceListSize()) {
		return &getImpl().lines[index];
	}
	return nullptr;
}


int oct_pattern::PatternFrame::getLateralSizeFirst(void) const
{
	return getLateralSizeList(false)[0];
}


std::vector<int> oct_pattern::PatternFrame::getLateralSizeList(bool repeat) const
{
	auto sizes = std::vector<int>();
	for (const auto& line : d_ptr->lines) {
		int points = line.getNumberOfScanPoints(false);
		if (repeat) {
			for (int k = 0; k < line.getNumberOfRepeats(); k++) {
				sizes.push_back(points);
			}
			// Each frame has only line repeated multiple times. 
			// break;
		}
		else {
			sizes.push_back(points);
		}
	}
	return sizes;
}


int oct_pattern::PatternFrame::getNumberOfLineTraces(bool repeat) const
{
	int count = 0;
	for (const auto& line : d_ptr->lines) {
		if (repeat) {
			count += line.getNumberOfRepeats();
		}
		else {
			count++;
		}
	}
	return count;
}


int oct_pattern::PatternFrame::getPositionIntervalX(int index1, int index2, int xPos)
{
	LineTrace* line1 = getLineTrace(index1);
	LineTrace* line2 = getLineTrace(index2);
	if (!line1 || !line2) {
		return 0;
	}

	int count1 = line1->getCountOfPositionsX();
	int count2 = line2->getCountOfPositionsX();
	if (count1 <= xPos || count2 <= xPos) {
		return 0;
	}

	short* pts1 = line1->getGalvanoPositionsX();
	short* pts2 = line2->getGalvanoPositionsX();

	int p1 = pts1[xPos];
	int p2 = pts2[xPos];
	int offset = (p2 - p1);

	LogD() << "Galvano x-offset, index: " << xPos << ", line-1: " << index1 << ", line-2: " << index2 << ", pos-1: " << p1 << ", pos-2: " << p2 << ", offset: " << (p2 - p1) ;
	return offset;
}


int oct_pattern::PatternFrame::getPositionIntervalY(int index1, int index2, int yPos)
{
	LineTrace* line1 = getLineTrace(index1);
	LineTrace* line2 = getLineTrace(index2);
	if (!line1 || !line2) {
		return 0;
	}

	int count1 = line1->getCountOfPositionsX();
	int count2 = line2->getCountOfPositionsX();
	if (count1 <= yPos || count2 <= yPos) {
		return 0;
	}

	short* pts1 = line1->getGalvanoPositionsY();
	short* pts2 = line2->getGalvanoPositionsY();

	int p1 = pts1[yPos];
	int p2 = pts2[yPos];
	int offset = (p2 - p1);

	LogD() << "Galvano y-offset, index: " << yPos << ", line-1: " << index1 << ", line-2: " << index2 << ", pos-1: " << p1 << ", pos-2: " << p2 << ", offset: " << (p2 - p1);
	return offset;
}


PatternFrame::PatternFrameImpl & oct_pattern::PatternFrame::getImpl(void) const
{
	return *d_ptr;
}
