#ifndef __SUBTITLE_INFO_LISTENER_H__
#define __SUBTITLE_INFO_LISTENER_H__

class InfoChangeListener {
public:
    InfoChangeListener() {}
    virtual ~InfoChangeListener() {}

    /**
     * When from Data source, get total subtitle number, notify
     * to subtitle module for later processing.
     * If no subtitle retreived, no need start subtitle.
     */
    virtual void onSubtitleChanged(int newTotal) = 0;

    /**
     *  indicate current render time(PTS).
     */
    virtual void onRenderTimeChanged(int64_t renderTime) = 0;

    /**
     *  Normally, start play the PTS is 0, but some time is not
     *  Sync with this callback
     */
    virtual void onRenderStartTimestamp(int64_t renderTime) = 0;
    virtual void onTypeChanged(int newType) = 0;

    virtual void onGetLanguage(std::string &lang) = 0;
};
#endif
