#include "retracer/glws.hpp"

#include "dispatch/eglproc_auto.hpp"
#include "retracer/retracer.hpp"
#include "retracer/forceoffscreen/offscrmgr.h"

using namespace retracer;

namespace retracer {

Context::~Context()
{
    if (_shareContext != NULL)
        _shareContext->release();
}

}
