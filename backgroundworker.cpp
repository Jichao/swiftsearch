#include "stdafx.h"
#include "BackgroundWorker.hpp"

BackgroundWorker * BackgroundWorker::create(bool coInitialize)
{
	return new BackgroundWorkerImpl(coInitialize);
}
