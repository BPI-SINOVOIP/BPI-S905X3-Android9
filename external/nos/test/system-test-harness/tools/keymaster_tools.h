#ifndef KEYMASTER_TOOLS_H
#define KEYMASTER_TOOLS_H

#include <app_nugget.h>
#include <application.h>
#include <keymaster.h>
#include <nos/debug.h>
#include <nos/NuggetClientInterface.h>

#include <memory>
#include <string>

namespace keymaster_tools {

void SetRootOfTrust(nos::NuggetClientInterface *client);

}  // namespace keymaster_tools

#endif  // KEYMASTER_TOOLS_H
