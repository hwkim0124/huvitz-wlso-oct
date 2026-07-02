#pragma once

#include <type_traits>
#include <string>
#include <functional>


namespace wso_domain
{
	// External key callback 
	////////////////////////////////////////////////////////////////////////////////////////
	using JoystickButtonPressedCallback = std::add_pointer<void(__stdcall)(void)>::type;
	using OptimizeButtonPressedCallback = std::add_pointer<void(__stdcall)(void)>::type;
	using AdapterLensAttachedCallback = std::add_pointer<void(__stdcall)(int)>::type;


	// Device callbacks 
	///////////////////////////////////////////////////////////////////////////////////////////
	using StepMotorPositionChanged = std::add_pointer<void(__stdcall)(
		int,				// motor encoder position 
		float)>::type;		// logical value in the current position


	// Infrared Camera data callback 
	////////////////////////////////////////////////////////////////////////////////////////
	using InfraredCameraFrameCallback = std::add_pointer<void(__stdcall)(unsigned char*, unsigned int, unsigned int)>::type;
	using CorneaCameraImageCallback = std::add_pointer<void(__stdcall)(unsigned char*, unsigned int, unsigned int)>::type;
	using RetinaCameraImageCallback = std::add_pointer<void(__stdcall)(unsigned char*, unsigned int, unsigned int)>::type;


	// Infrared Camera
	///////////////////////////////////////////////////////////////////////////////////////////
	using CorneaCameraFrameCaptured = std::add_pointer<void(__stdcall)(
		unsigned char*,		// Image bits data
		int32_t,			// width
		int32_t				// height
		)>::type;

	using RetinaCameraFrameCaptured = std::add_pointer<void(__stdcall)(
		unsigned char*,		// Image bits data
		int32_t,			// width
		int32_t				// height
		)>::type;


	// Color Camera
	//////////////////////////////////////////////////////////////////////////////////////////////
	using ColorCameraFrameCaptured = std::add_pointer<void(__stdcall)(
		unsigned char*,     // Image bits data
		int32_t,			// width
		int32_t,			// height
		int32_t,			// frame count
		int32_t,			// flip mode
		int32_t,			// Pixel Format 
		int32_t				// bytes Per Pixel
		)>::type;

	using ColorCameraSeqROIFrameCaptured = std::add_pointer<void(__stdcall)(
		unsigned char*,     // Image bits data
		int32_t,			// width
		int32_t,			// height
		int32_t,			// offset X
		int32_t,			// offset Y
		int32_t,			// frame count
		int32_t,			// Total count
		int32_t,			// flip mode
		int32_t,			// Pixel Format 
		int32_t				// bytes Per Pixel
		)>::type;

	using ColorCameraOffsetROIFrameCaptured = std::add_pointer<void(__stdcall)(
		unsigned char*,     // Image bits data
		int32_t,			// width
		int32_t,			// height
		int32_t,			// offset X
		int32_t,			// offset Y
		int32_t,			// frame count
		int32_t,			// Total count
		int32_t,			// flip mode
		int32_t,			// Pixel Format 
		int32_t				// bytes Per Pixel
		)>::type;

	using ColorCameraRollSwTrigOverlapFrameCaptured = std::add_pointer<void(__stdcall)(
		unsigned char*,     // Image bits data
		int32_t,			// width
		int32_t,			// height
		int32_t,			// frame count
		int32_t,			// Total count
		int32_t,			// flip mode
		int32_t,			// Pixel Format 
		int32_t				// bytes Per Pixel
		)>::type;

	using ColorCameraImageCaptured = std::add_pointer<void(__stdcall)(
		unsigned char*,     // Image bits data
		int32_t,			// width
		int32_t,			// height
		int32_t,			// frame count
		int32_t,			// total frame count
		int32_t,			// flip mode
		int32_t,			// Pixel Format 
		int32_t				// bytes Per Pixel
		)>::type;

	// Color Camera Data Callback type
	/////////////////////////////////////////////////////////////////////////////////////////
	typedef std::function<void(unsigned char*, unsigned int, unsigned int, unsigned int, unsigned int)> ColorCameraFrameCallback;
	typedef std::function<void(unsigned char*, unsigned int, unsigned int, unsigned int)> ColorCameraImageCallback;


	// OCT Scanning Measure
	///////////////////////////////////////////////////////////////////////////////////////////
	using OctScanPatternAcquired = std::add_pointer<void(__stdcall)(
		bool				// result
		)>::type;

	using OctScanProtocolCompleted = std::add_pointer<void(__stdcall)(
		bool				// result
		)>::type;


	// OCT Scanning Preview
	///////////////////////////////////////////////////////////////////////////////////////////
	using OctSpectrumDataCaptured = std::add_pointer<void(__stdcall)(
		unsigned short*,	// data 
		int,		// width
		int			// height
		)>::type;

	using OctResampleDataCaptured = std::add_pointer<void(__stdcall)(
		float*,		// data 
		int,		// width
		int			// height
		)>::type;

	using OctIntensityDataCaptured = std::add_pointer<void(__stdcall)(
		float*,		// data 
		int,		// width
		int			// height
		)>::type;

	using OctPreviewImageCaptured = std::add_pointer<void(__stdcall)(
		unsigned char*,		// Image bits data
		int,				// width
		int,				// height
		float,				// Quality index 
		float,				// SNR
		int,				// Reference position 
		int					// Index of Image 
		)>::type;

	using OctMeasureImageCaptured = std::add_pointer<void(__stdcall)(
		unsigned char*,		// Image bits data
		int,				// width
		int,				// height
		float,				// Quality index 
		float,				// SNR
		int,				// Reference position 
		int					// Index of Image 
		)>::type;

	using OctEnfaceImageCaptured = std::add_pointer<void(__stdcall)(
		unsigned char*,		// Image bits data
		int,				// width
		int					// height
		)>::type;


	// Oct Optimizer 
	///////////////////////////////////////////////////////////////////////////////////////////
	using OctAutoFocusOptimized = std::add_pointer<void(__stdcall)(
		bool,				// Result 
		float,				// qualitiy
		float				// diopter
		)>::type;

	using OctAutoPolarOptimized = std::add_pointer<void(__stdcall)(
		bool,				// Result 
		float,				// quality
		float				// degree
		)>::type;

	using OctAutoReferOptimized = std::add_pointer<void(__stdcall)(
		bool,				// Result 
		float,				// quality
		int,				// reference point 
		int					// position 
		)>::type;

	using OctAutoScanOptimized = std::add_pointer<void(__stdcall)(
		bool				// Result
		)>::type;


	
	// Log Callback types 
	/////////////////////////////////////////////////////////////////////////////////////////////////////////
	// message (string), log msg type. 
	using WsoLogMsgCallback = std::add_pointer<void(__stdcall)(const wchar_t*, int)>::type;


	// Firmware Message Callback
	///////////////////////////////////////////////////////////////////////////////////////////
	// int currentState, int percent, bool bReset, bool bError, string message
	using FirmwareMsgCallback = std::add_pointer<void(__stdcall)(const wchar_t*, int, int, bool, bool)>::type;
}