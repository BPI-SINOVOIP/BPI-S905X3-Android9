#define LOG_TAG "tests"
#include <signal.h>
#include <fcntl.h>
#include <poll.h>
#include <math.h>
#include <sys/syscall.h>
#include <utils/Log.h>
#include <utils/RefBase.h>

#include "SubtitleNativeAPI.h"

#define MAX_LINE 80 /* The maximum length command */
typedef void(*apiFunc)(char* args[]);
typedef struct {
    const char *api;
    apiFunc func;
} ApiFuncItem;

static AmlSubtitleHnd gSubtitleHandle = nullptr;

#define DEBUGP(...) printf(__VA_ARGS__)

static int parseInput(char * command, const char* delim, char* args[]) {
    char *s = strdup(command);
    char *token;
    int n = 0;
    for (token = strsep(&s, delim); token != NULL; token = strsep(&s, delim)) {
        if (strlen(token) > 0) {
            //args[n] = (char*) malloc(strlen(token) * sizeof(char));
            //strcpy(args[n++], token);
            args[n++] = strdup(token);
        }
    }
    args[n] = NULL;
    free(s);
    return n;
}

void subtitleCreate(char* args[]) {
    DEBUGP("calling %s\n", __func__);
    gSubtitleHandle = amlsub_Create();
    printf("create called, result session id:%p\n", gSubtitleHandle);
}

void subtitleDestory(char* args[]) {
    DEBUGP("calling %s\n", __func__);
    AmlSubtitleStatus r = amlsub_Destroy(gSubtitleHandle);
    printf("destroy session:%p, result is: %d\n", gSubtitleHandle, r);
    gSubtitleHandle = nullptr;
}

void subtitleOpen(char* args[]) {
    DEBUGP("calling %s\n", __func__);
    AmlSubtitleParam param;
    memset(&param, 0, sizeof(AmlSubtitleParam));
    param.extSubPath = nullptr;
    if (args[1] != nullptr) param.ioSource = (AmlSubtitleIOType)atoi(args[1]);
    if (args[2] != nullptr) param.subtitleType = atoi(args[2]);
    if (args[3] != nullptr) param.pid = atoi(args[3]);
    if (args[4] != nullptr) param.videoFormat = atoi(args[4]);
    if (args[5] != nullptr) param.channelId = atoi(args[5]);
    if (args[6] != nullptr) param.ancillaryPageId = atoi(args[6]);
    if (args[7] != nullptr) param.compositionPageId = atoi(args[7]);
    AmlSubtitleStatus r = amlsub_Open(gSubtitleHandle, &param);
    printf("open:%p, result is: %d\n", gSubtitleHandle, r);
}

void subtitleClose(char* args[]) {
    DEBUGP("calling %s\n", __func__);
    AmlSubtitleStatus r = amlsub_Close(gSubtitleHandle);
    printf("close:%p, result is: %d\n", gSubtitleHandle, r);
}
void subtitleReset(char* args[]) {
    DEBUGP("calling %s\n", __func__);
    AmlSubtitleStatus r = amlsub_Reset(gSubtitleHandle);
    printf("reset:%p, result is: %d\n", gSubtitleHandle, r);
}

void subtitleSetParameter(char* args[]) {
    DEBUGP("calling %s\n", __func__);
    //AmlSubtitleStatus amlsub_SetParameter(AmlSubtitleHnd handle, AmlSubtitleParamCmd cmd, void *value, int paramSize);
}

void subtitleGetParameter(char* args[]) {
    DEBUGP("calling %s\n", __func__);
//int amlsub_GetParameter(AmlSubtitleHnd handle, AmlSubtitleParamCmd cmd, void *value);
}

void subtitleTeletextControl(char* args[]) {
    DEBUGP("calling %s\n", __func__);
//AmlSubtitleStatus amlsub_TeletextControl(AmlSubtitleHnd handle, AmlTeletextCtrlParam *param);
}

void subtitleShow(char* args[]) {
    DEBUGP("calling %s\n", __func__);
    AmlSubtitleStatus r = amlsub_UiShow(gSubtitleHandle);
    printf("%s:%p, result is: %d\n", __func__, gSubtitleHandle, r);
}

void subtitleHide(char* args[]) {
    DEBUGP("calling %s\n", __func__);
    AmlSubtitleStatus r = amlsub_UiHide(gSubtitleHandle);
    printf("%s:%p, result is: %d\n", __func__, gSubtitleHandle, r);
}

void subtitleDisplayRect(char* args[]) {
    DEBUGP("calling %s\n", __func__);
    AmlSubtitleStatus r = amlsub_UiSetSurfaceViewRect(gSubtitleHandle,
            atoi(args[1]), atoi(args[2]), atoi(args[3]), atoi(args[4]));
    printf("%s:%p, result is: %d\n", __func__, gSubtitleHandle, r);

}

ApiFuncItem gFuncTable[] = {
    // session
    { "create", subtitleCreate },
    { "destroy", subtitleDestory },
    // control
    { "open", subtitleOpen },
    { "close", subtitleClose },
    { "reset", subtitleReset },
    // dtv:
    { "setParam", subtitleSetParameter},
    { "getParam", subtitleGetParameter},
    { "teletextControl", subtitleTeletextControl},

    //UI related.
    { "show", subtitleShow},
    { "hide", subtitleHide},

    { "displayRect", subtitleDisplayRect},

};


static bool gExitRequested = false;

int main(int argc, char **argv) {
    char *args[MAX_LINE/2 + 1]; /* command line arguments */
    int index = 0;
    int sizeInHistory = 0;
    char command[MAX_LINE];

    signal(SIGINT, [](int sig) {
            printf("%s\n", strsignal(sig));
            gExitRequested = true;
        });

    // initialze args:
    memset(args, 0, sizeof(args));

    while (!gExitRequested) {
        printf("subtitle > ");
        fflush(stdout);
        fgets(command, MAX_LINE, stdin);
        index = parseInput(command, " !&\n", args);

        if (args[0] == NULL) {
            continue;
        }

        if (strcmp(args[0], "exit") == 0) {
            gExitRequested = true;
            printf("exit requested, quiting...\n");
        } else {
            int i;
            for (i=0; i< sizeof(gFuncTable)/sizeof(ApiFuncItem); i++) {
                if (strcmp(args[0], gFuncTable[i].api) == 0) {
                    gFuncTable[i].func(args);
                    break;
                }
            }

            if (i == sizeof(gFuncTable)/sizeof(ApiFuncItem)) {
                printf("Error! not recognized command: %s\n", args[0]);
            }
            //execute(args, index, commandInHistory, sizeInHistory, command);
        }
        memset(command, 0, strlen(command) * sizeof(char));
        for (int i=0; i<MAX_LINE/2 + 1; i++) {
            if (args[i] != nullptr) {
                //printf("%d:%s ", i, args[i]);
                free(args[i]);
                args[i] = nullptr;
            }
        }
        //printf("\n");
    }


    return 0;
}

