// SigProc.cpp : Defines the exported functions for the DLL.
//

#include "pch.h"
#include "framework.h"
#include "SigProc.h"


// This is an example of an exported variable
SIGPROC_API int nSigProc=0;

// This is an example of an exported function.
SIGPROC_API int fnSigProc(void)
{
    return 0;
}

// This is the constructor of a class that has been exported.
CSigProc::CSigProc()
{
    return;
}
