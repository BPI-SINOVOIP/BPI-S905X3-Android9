#define LOG_NDEBUG 0
#define LOG_TAG "ITsPlayer"

#include "ITsPlayer.h"
#include "CTC_Utils.h"
#include "CTC_Log.h"
#include "CTsPlayerImpl.h"
#include "CTC_Version.h"
#include <mutex>

using namespace android;

void version()
{
    ALOGE("====================================================================");
    //ALOGE("CTC <%p>", this);
    ALOGE("version: " CTC_GIT);
    ALOGE("workdir_status: " CTC_WORKDIR_STATUS);
    ALOGE("build_date: " CTC_BUILD_DATE);
    ALOGE("user: " CTC_BUILD_USER);
    ALOGE("android version: %d", ANDROID_PLATFORM_SDK_VERSION);
    ALOGE("arch: %s",
#ifdef __arm__
            "ARM"
#elif __aarch64__
            "ARM64"
#else
            "UNKNOWN"
#endif
        );
    ALOGE("====================================================================");
}

ITsPlayerFactory::ITsPlayerFactory()
{

}

ITsPlayerFactory::~ITsPlayerFactory()
{

}

ITsPlayerFactory* ITsPlayerFactory::sITsPlayerFactory = nullptr;
static std::once_flag g_OnceFlag;

ITsPlayerFactory& ITsPlayerFactory::instance()
{
    if (sITsPlayerFactory == nullptr) {
        std::call_once(g_OnceFlag, [] {
            sITsPlayerFactory = new ITsPlayerFactory();
        });
    }

    return *sITsPlayerFactory;
}


ITsPlayer* ITsPlayerFactory::createITsPlayer(int channelNo, CTC_InitialParameter* initialParam)
{
    int omxDebug = CTC_getConfig("media.ctcplayer.omxdebug", 0);
    int display_mode = CTC_getConfig("media.ctc.display.mode", 0);

    bool preferOmxPlayer = false;
    if (initialParam != nullptr) {
        preferOmxPlayer = initialParam->useOmx;
    }


    if (preferOmxPlayer) {
        return nullptr;
    }

    auto player = new CTsPlayer();
    player->incStrong(player);

    return player;
}

