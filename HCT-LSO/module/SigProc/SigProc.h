// The following ifdef block is the standard way of creating macros which make exporting
// from a DLL simpler. All files within this DLL are compiled with the SIGPROC_EXPORTS
// symbol defined on the command line. This symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see
// SIGPROC_API functions as being imported from a DLL, whereas this DLL sees symbols
// defined with this macro as being exported.
#ifdef SIGPROC_EXPORTS
#define SIGPROC_API __declspec(dllexport)
#else
#define SIGPROC_API __declspec(dllimport)
#endif

// This class is exported from the dll
class SIGPROC_API CSigProc {
public:
	CSigProc(void);
	// TODO: add your methods here.
};

extern SIGPROC_API int nSigProc;

SIGPROC_API int fnSigProc(void);
