#pragma once


#include <type_traits>
#include <string>
#include <functional>


namespace wso_domain
{
	/*
	// SLO Scan Grabbing
	///////////////////////////////////////////////////////////////////////////////////////////
	typedef std::function<void(
		unsigned short*[3],	// data 
		int,		// width
		int,		// height
		int,		// channels
		int			// pixel size.
	)> SloFrameBufferGrabbedEvent;

	typedef std::function<void(
		unsigned char*,	// data 
		int,		// width
		int,		// height
		int,		// channels
		float,		// quality index
		float		// intensity mean
	)> SloFrameImageGrabbedEvent;


	// SLO Scan Image Processing
	///////////////////////////////////////////////////////////////////////////////////////////
	typedef std::function<void(
		unsigned char*,		// bits 
		int,		// width
		int,		// height
		int,		// channels
		float		// quality index
		)> SloPreviewImageProcessedEvent;

	typedef std::function<void(
		unsigned char*,		// bits 
		int,		// width
		int,		// height
		int,		// channels
		float		// quality index
		)> SloMeasureImageProcessedEvent;

	typedef std::function<void(
		bool		// result
		)> SloCaptureMeasureCompletedEvent;
	*/

	// OCT Scan Grabbing 
	///////////////////////////////////////////////////////////////////////////////////////////
	typedef std::function<void(
		unsigned short*,	// data 
		int,		// width
		int,		// height
		int			// index
		)> OctPreviewBufferGrabbedEvent;

	typedef std::function<void(
		unsigned short*,	// data 
		int,		// width
		int,		// height
		int			// index
		)> OctMeasureBufferGrabbedEvent;

	typedef std::function<void(
		unsigned short*,	// data 
		int,		// width
		int,		// height
		int			// index
		)> OctEnfaceBufferGrabbedEvent;


	// OCT Scan Image Processing
	///////////////////////////////////////////////////////////////////////////////////////////
	typedef std::function<void(
		unsigned char*,		// bits 
		unsigned int,		// width
		unsigned int		// height
		)> OctEnfaceImageProcessedEvent;

	typedef std::function<void(
		unsigned char*,		// Image bits data
		int,				// width
		int,				// height
		float,				// Quality index 
		float,				// SNR
		int,				// Reference position 
		int					// Index of Image 
		)> OctPreviewImageProcessedEvent;

	typedef std::function<void(
		unsigned char*,		// Image bits data
		int,				// width
		int,				// height
		float,				// Quality index 
		float,				// SNR
		int,				// Reference position 
		int,				// Index of Image 
		unsigned char*		// lateral strip data. 
		)> OctMeasureImageProcessedEvent;

	typedef std::function<void(
		bool		// result
		)> OctProtocolPatternAcquiredEvent;

	typedef std::function<void(
		bool		// result
		)> OctProtocolMeasureCompletedEvent;

}