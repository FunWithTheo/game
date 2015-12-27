#include "cbase.h"
#include "filesystem.h"

class MomentumUtil
{
public:

    void PostTimeCallback(HTTPRequestCompleted_t*, bool);
    void DownloadCallback(HTTPRequestCompleted_t*, bool);
    void PostTime(const char* URL);
    void DownloadMap(const char*);

    void CreateAndSendHTTPReq(const char*, CCallResult<MomentumUtil, HTTPRequestCompleted_t>*,
        CCallResult<MomentumUtil, HTTPRequestCompleted_t>::func_t);

    CCallResult<MomentumUtil, HTTPRequestCompleted_t> cbDownloadCallback;
    CCallResult<MomentumUtil, HTTPRequestCompleted_t> cbPostTimeCallback;

};

extern MomentumUtil mom_UTIL;