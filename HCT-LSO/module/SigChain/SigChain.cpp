// SigChain.cpp : Defines the exported functions for the DLL.
//

#include "pch.h"
#include "framework.h"
#include "SigChain.h"


// This is an example of an exported variable
SIGCHAIN_API int nSigChain=0;

// This is an example of an exported function.
SIGCHAIN_API int fnSigChain(void)
{
    return 0;
}

// This is the constructor of a class that has been exported.
CSigChain::CSigChain()
{
    return;
}
