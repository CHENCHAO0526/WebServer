//
// Created by cc on 8/10/21.
//

#include "Logging.h"
#include <errno.h>
#include <string.h>

__thread char t_errnobuf[512];


const char* strerror_tl(int savedErrno)
{
    return strerror_r(savedErrno, t_errnobuf, sizeof t_errnobuf);
}
