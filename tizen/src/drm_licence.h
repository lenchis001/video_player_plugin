//
// Created by wangbiao on 2021/10/20.
//

#ifndef VIDEO_PLAYER_PLUGIN_DRM_LICENCE_H
#define VIDEO_PLAYER_PLUGIN_DRM_LICENCE_H
#include <stdbool.h>

typedef enum  {
    NO_TYPE=0,
    CHALLENGE_GET_LICENSE = 1,
    CHALLENGE_JOIN_DOMAIN = 2,
    CHALLENGE_LEAVE_DOMAIN = 3,
    CHALLENGE_GET_METERCERT = 4,
    CHALLENGE_PROCESS_METERDATA =5,
    CHALLENGE_SECURECLOCK = 6,
    ACK_LICENSE =7,
    FALLBACK = 8,
    GETSECURECLOCKSERVER_URL =9,
    CHALLENGE_GET_REVOCATIONPACKAGE =10
}DRM_MSG_TYPE;


#define DRM_SUCCESS                      ((DRM_RESULT)0x00000000L)
#define DRM_E_POINTER                    ((DRM_RESULT)0x80004003L)
#define DRM_E_INVALIDARG                 ((DRM_RESULT)0x80070057L)

typedef char DRM_CHAR;               /* 1 byte              1 byte */
typedef long DRM_RESULT;
typedef unsigned char DRM_BYTE;               /* 1 byte              1 byte */
typedef unsigned short DRM_WORD;               /* 2 bytes             1 byte */
typedef unsigned long  DRM_DWORD;              /* 4 bytes             2 bytes
*/



#define DRM_E_NETWORK					((DRM_RESULT)0x91000000L)
#define DRM_E_NETWORK_CURL				((DRM_RESULT)0x91000001L)
#define DRM_E_NETWORK_HOST				((DRM_RESULT)0x91000002L)
#define DRM_E_NETWORK_CLIENT			((DRM_RESULT)0x91000003L)
#define DRM_E_NETWORK_SERVER			((DRM_RESULT)0x91000004L)
#define DRM_E_NETWORK_HEADER			((DRM_RESULT)0x91000005L)
#define DRM_E_NETWORK_REQUEST			((DRM_RESULT)0x91000006L)
#define DRM_E_NETWORK_RESPONSE			((DRM_RESULT)0x91000007L)
#define DRM_E_NETWORK_CANCELED			((DRM_RESULT)0x91000008L)


/* See. struct _DRM_INIT_OVERRIDE_DATA *pOverrideData; */
typedef struct __PRExtensionCtx_TZ
{
    char *pSoapHeader;
    char *pHttpHeader;
    char *pUserAgent;
    char *pCustomData_Res;
    char *pRightsIssuerURL;
    char KID_base64[25];
    bool cancelRequest;
} PRExtensionCtx_TZ;

DRM_RESULT PRNetManager_DoTransaction_TZ (
    const char* pServerUrl,
    const void* f_pbChallenge,
    unsigned f_cbChallenge,
    unsigned char** f_ppbResponse,
    unsigned* f_pcbResponse,
    DRM_MSG_TYPE f_type,
    const char* f_pCookie,
    PRExtensionCtx_TZ *pExtCtx
);
#endif //VIDEO_PLAYER_PLUGIN_DRM_LICENCE_H
