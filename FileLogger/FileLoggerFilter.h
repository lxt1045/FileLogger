#pragma once
#include <fltKernel.h>


NTSTATUS InitFilterRules();
NTSTATUS FreeFilterRules();
NTSTATUS ResetFilterRules();
BOOLEAN FilterFLogger(PWCHAR wszProcessName, PWCHAR wszSourcePath, PWCHAR wszTargetPath, PWCHAR wszUserName);

