/**
* @file NetManagerTZ.cpp
* @brief Implementation of structures and functions for Http Connection
* @author : Dmitriy Vadnyov <d.vadnyov@samsung.com>
* @date : 2012/10/18
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <curl/curl.h>
#include "drm_licence.h"
#include "log.h"

#define HTTP_HEADER_LICGET      "Content-Type: text/xml; charset=utf-8\r\nSOAPAction: \"http://schemas.microsoft.com/DRM/2007/03/protocols/AcquireLicense\""
#define HTTP_HEADER_LICACK      "Content-Type: text/xml; charset=utf-8\r\nSOAPAction: \"http://schemas.microsoft.com/DRM/2007/03/protocols/AcknowledgeLicense\""
#define HTTP_HEADER_JOIN        "Content-Type: text/xml; charset=utf-8\r\nSOAPAction: \"http://schemas.microsoft.com/DRM/2007/03/protocols/JoinDomain\""
#define HTTP_HEADER_LEAVE       "Content-Type: text/xml; charset=utf-8\r\nSOAPAction: \"http://schemas.microsoft.com/DRM/2007/03/protocols/LeaveDomain\""
#define HTTP_HEADER_METERCERT   "Content-Type: text/xml; charset=utf-8\r\nSOAPAction: \"http://schemas.microsoft.com/DRM/2007/03/protocols/GetMeteringCertificate\""
#define HTTP_HEADER_METERDATA   "Content-Type: text/xml; charset=utf-8\r\nSOAPAction: \"http://schemas.microsoft.com/DRM/2007/03/protocols/ProcessMeteringData\""

#define HTTP_FALLBACK_HEADER    "Content-Type: application/x-www-form-urlencoded"
#define HTTP_REVOCATION_URL	"http://go.microsoft.com/fwlink/?LinkId=110086"

#define ChkCURLFAIL(expr) {	\
	if (expr != CURLE_OK)	\
	{						\
		LOG_INFO("Error %d \n", __LINE__);	\
		goto ErrorExit;		\
	}						\
}

#ifdef _MSC_VER

#define OEM_strnicmp(s1,s2,count)   _strnicmp(s1,s2,count)
#define OEM_stristr(s1,s2)      stristr(s1,s2)

#else

#define OEM_strnicmp(s1,s2,count)   strncasecmp(s1,s2,count)
#define OEM_stristr(s1,s2)      strcasestr(s1,s2)

#endif


typedef struct _S_DYNAMIC_BUF {
  unsigned char*  iData;
  size_t          iSize;
  size_t          iAllocated;
} S_DYNAMIC_BUF;


/**
 * @brief	This struct contains internal HTTP session information
 *
 */
typedef struct _HTTP_SESSION
{
	void*			curl_handle;		//curl_handle
	unsigned char*		postData;		//request body
	size_t			postDataLen;		//length of request body
	DRM_MSG_TYPE 		type;
	size_t			sendDataLen;		//length of send already
	S_DYNAMIC_BUF   	header;			//response header
	S_DYNAMIC_BUF		body;			//response body
	long			resCode;
} HTTP_SESSION;

#ifdef _MSC_VER
static const char* stristr(const char *String, const char *Pattern)
{
      char *pptr, *sptr, *start;
      unsigned   slen, plen;

      for (start = (char *)String,
           pptr  = (char *)Pattern,
           slen  = strlen(String),
           plen  = strlen(Pattern);

           /* while string length not shorter than pattern length */

           slen >= plen;

           start++, slen--)
      {
            /* find start of pattern in string */
            while (toupper(*start) != toupper(*Pattern))
            {
                  start++;
                  slen--;

                  /* if pattern longer than string */

                  if (slen < plen) return(NULL);
            }

            sptr = start;
            pptr = (char *)Pattern;

            while (toupper(*sptr) == toupper(*pptr))
            {
                  sptr++;
                  pptr++;

                  /* if end of pattern then pattern was found */

                  if ('\0' == *pptr) return (start);
            }
      }
      return(NULL);
}
#endif

/**
  * ReceiveHeader()
  * Receive HTTP response header
  *
  * @param		ptr     [in]    received data pointer of HTTP response header
  * @param      size    [in]    number of nmemb
  * @param      nmemb   [in]    size of record
  * @param      stream  [in]    HttpOpen栏肺 积己茄 HttpSession 器牢磐
  * @return		己傍矫 焊辰 size甫, 角菩矫 0阑 府畔茄促.
  * @see		SendBody, ReceiveBody
  * @remark
  * @version	1.0
  */
static size_t ReceiveHeader(void *ptr, size_t size, size_t nmemb, void *stream);


/**
  * ReceiveBody()
  * Receive HTTP response body
  *
  * @param		ptr     [in]    received data pointer of HTTP response body
  * @param      size    [in]    number of nmemb
  * @param      nmemb   [in]    size of record
  * @param      stream  [in]    HttpOpen栏肺 积己茄 HttpSession 器牢磐
  * @return		己傍矫 焊辰 size甫, 角菩矫 0阑 府畔茄促.
  * @see		ReceiveHeader, SendBody
  * @remark
  * @version	1.0
  */
static size_t ReceiveBody(void *ptr, size_t size, size_t nmemb, void *stream);

/**
  * SendBody()
  * Send HTTP POST body
  *
  * @param		ptr     [in]    data to send HTTP POST body
  * @param      size    [in]    number of nmemb
  * @param      nmemb   [in]    size of record
  * @param      stream  [in]    HttpOpen栏肺 积己茄 HttpSession 器牢磐
  * @return		己傍矫 焊辰 size甫, 角菩矫 0阑 府畔茄促.
  * @see		ReceiveHeader, ReceiveBody
  * @remark
  * @version	1.0
  */
static size_t SendBody(void *ptr, size_t size, size_t nmemb, void *stream);


static bool AppendData(S_DYNAMIC_BUF* aBuf,const void* aData,size_t aSize)
{
  size_t newSize = aBuf->iSize+aSize;
  if ( aBuf->iAllocated < newSize )
   {
     newSize += 1024;
     unsigned char* buf = (unsigned char*)realloc(aBuf->iData, newSize);
     if (!buf)
     {
		LOG_INFO("AppendData : realloc fail \n");
	 	return false;
     }
     aBuf->iData = buf; aBuf->iAllocated = newSize;
     LOG_INFO("AppendData : realloc aSize(%d), iSize(%d) aBuf->iAllocated(%d)\n", aSize, aBuf->iSize, aBuf->iAllocated);
   }
  memcpy( aBuf->iData+aBuf->iSize, aData, aSize );
  aBuf->iSize += aSize;

  return true;
}

static char* _GetRedirectLocation(const char* aHeaders, bool bSupportHttps)
{
    if ( !aHeaders )
    {
        return NULL;
    }

	const char* pLocation = OEM_stristr( aHeaders, "Location" );
    if ( !pLocation )
    {
        return NULL;
    }

	const char* ptr = pLocation + strlen( "Location" );

    while ( *ptr == ':' )
    {
        ptr++;
    }
    while ( *ptr == ' ' )
    {
        ptr++;
    }

	unsigned i = 0;
	while ( ptr[i] && (ptr[i] != ' ') && (ptr[i] != '\n') && (ptr[i] != '\r') )
    {
		i++;
    }

	if (bSupportHttps)
	{
		// [soyoung] get redirection location
		// for using https itself

		char* ret = (char*)malloc( i+1 );
        if ( !ret )
        {
            return NULL;
        }
		memcpy( ret, ptr, i ); ret[i] = 0;
		return ret;
	}
	else
	{
		// Convert Redirection Location from https to http
		// [soyoung]
		// Redirect location 捞 https 老 版快绰 http 肺 函券
		// If the petition URL contains "https," the client may use SSL for the connection. (For non-SSL transport, remove the "s" in "https" from the URL.)
		// If SSL is used, the client should check the server's certificate to ensure it is current, matches the domain, and is properly signed by a trusted authority.
		int len = i;
		const char* p = ptr+4;
		const char http_str[6] = "http\0";

        if ( i < 7 )
        {
            return NULL; // wrong location, no space even for http://
        }

		if ( OEM_strnicmp(ptr, "https", 5) == 0 ) { len--; p++; }

		char* ret = (char*)malloc( len+1 );
        if ( !ret )
        {
            return NULL;
        }

		//strncpy( ret, "http", 4 ); memcpy( ret+4, p, len-4 ); ret[len] = 0;
		memcpy( ret, http_str, 4 ); memcpy( ret+4, p, len-4 ); ret[len] = 0;
		return ret;
	}
}


static struct curl_slist* _slist_append( struct curl_slist* aList, const char* aString )
{
    if ( !aList )
    {
        return NULL;
    }

    struct curl_slist* newList = curl_slist_append( aList, aString );
    if ( !newList ) // allocation failed
    {
        curl_slist_free_all( aList );
    }

  return newList;
}

static DRM_RESULT _ComposePostData_TZ(HTTP_SESSION *hSession, const char *f_pbPostData, int f_cbPostData , const char *f_extSoapHeader)
{
	DRM_RESULT dr = DRM_SUCCESS;
	const char *p;
	char *dest;
	int dest_len;
	int remain;

	free( hSession->postData );
	hSession->postData = NULL;
	hSession->postDataLen = 0;

	int extSoapHeaderLen = f_extSoapHeader ? strlen(f_extSoapHeader) : 0;


	dest_len = f_cbPostData  ;

	if (extSoapHeaderLen > 0)
	{
		dest_len += extSoapHeaderLen + sizeof("<soap:Header>\r\n</soap:Header>\r\n");
	}


	hSession->postData = (unsigned char *) malloc( dest_len +1  );
	if (hSession->postData == NULL)
	{
        LOG_INFO("Failed to alloc post data.\n");
		return DRM_E_POINTER;
	}
	dest = (char*)hSession->postData;
	remain = f_cbPostData;

	if (extSoapHeaderLen > 0)
	{
		/* append to the last in an existing soap header */
		p = strstr(f_pbPostData, "</soap:Header>");
		if (p > f_pbPostData && p < f_pbPostData+remain)
		{
			int hd_len = p - f_pbPostData;
			memcpy(dest, f_pbPostData, hd_len);
			dest += hd_len; dest_len -= hd_len; remain -= hd_len;

			memcpy(dest, f_extSoapHeader, extSoapHeaderLen);
			dest += extSoapHeaderLen;
            if (*dest == '\0')
            {
                dest --;
            }
		}
		else
		{
			/* insert soap header in front of soap body */
			p = strstr(f_pbPostData, "<soap:Body>");
			if (p > f_pbPostData && p <  f_pbPostData+remain)
			{
				int hd_len = p - f_pbPostData;
				memcpy(dest, f_pbPostData, hd_len);
				dest += hd_len; dest_len -= hd_len; remain -= hd_len;
				*dest = '\0'; strncat(dest, "<soap:Header>", dest_len);
				hd_len = strlen(dest); dest += hd_len; dest_len -= hd_len;

				memcpy(dest, f_extSoapHeader, extSoapHeaderLen);
				hd_len = extSoapHeaderLen; dest += hd_len; dest_len -= hd_len;

				*dest = '\0'; strncat(dest, "</soap:Header>", dest_len);
				hd_len = strlen(dest); dest += hd_len; dest_len -= hd_len;
			}
			else
			{
				/* not a SOAP message */
				p = f_pbPostData;
			}
		}
	}
	else
	{
		p = f_pbPostData;
	}

	memcpy( dest, p, remain );
	dest += remain;
	*dest = '\0';

	hSession->postDataLen = dest - (char*)hSession->postData;
	if (extSoapHeaderLen > 0)
	{
        LOG_INFO("[soap header added %d ] %s \n", hSession->postDataLen, hSession->postData);
	}

	return dr;
}


/**
@fn struct curl_slist* SetHttpHeader (struct curl_slist *headers, CURL *pCurl, const DRM_CHAR *pszHeader)
@brief  Set the HTTP Header
@param  pCurl		[IN] Pointer to the curl structure
@param	pszHeader	[IN] additional header
@return curl_slist  list of httpheaders
*/
static struct curl_slist* SetHttpHeader( CURL* pCurl, DRM_MSG_TYPE f_type, const char* f_pCookie,  const char* f_pHttpHeader, const char* f_pUserAgent)
{
	struct curl_slist* headers;
	if(f_pUserAgent)
	{
		const char* userAgentPrefix = "User-Agent: ";
	    	unsigned prefixLen = strlen(userAgentPrefix);
	    	unsigned userAgentLen = strlen(f_pUserAgent);

	    	char *userAgent = (char *)malloc(prefixLen + userAgentLen + 1);

		if (userAgent)
		{
			memcpy(userAgent, userAgentPrefix, prefixLen);
			memcpy(userAgent + prefixLen, f_pUserAgent, userAgentLen);
			userAgent[prefixLen + userAgentLen] = '\0';
			LOG_INFO("SetHttpHeader :  user-agent added to header --- (%s)\n", userAgent);
			free(userAgent);
		}

		headers = curl_slist_append(NULL, f_pUserAgent); // allocates a new list
	}
	else
	{
		headers = curl_slist_append(NULL, "User-Agent: PlayReadyClient");  // allocates a new list
	}

    if ( !headers )
    {
        return NULL;
    }

    LOG_INFO("SetHttpHeader : f_type(%d), f_pCookie(%s), f_pHttpHeader(%s)\n", f_type, f_pCookie, f_pHttpHeader);

	const char* hdr = NULL;
	switch ( f_type ) {
		case CHALLENGE_GET_LICENSE:       hdr = HTTP_HEADER_LICGET; break;
		case CHALLENGE_JOIN_DOMAIN:       hdr = HTTP_HEADER_JOIN; break;
		case CHALLENGE_LEAVE_DOMAIN:      hdr = HTTP_HEADER_LEAVE; break;
		case CHALLENGE_GET_METERCERT:     hdr = HTTP_HEADER_METERCERT; break;
		case CHALLENGE_PROCESS_METERDATA: hdr = HTTP_HEADER_METERDATA; break;
		case ACK_LICENSE:                 hdr = HTTP_HEADER_LICACK; break;
		case FALLBACK:                    hdr = HTTP_FALLBACK_HEADER; break;
		default: break; // make compiler happy
	}

    if ( hdr )
    {
        headers = _slist_append( headers, hdr );
    }

	headers = _slist_append(headers, "Pragma: no-cache");
	headers = _slist_append(headers, "Accept:");

	if ( f_pCookie )
	{
        const char* cookiePrefix = "Cookie: ";
        unsigned prefixLen = strlen(cookiePrefix);
        unsigned cookieLen = strlen(f_pCookie);

        char *cookie = (char *)malloc(prefixLen + cookieLen + 1);

        if (cookie)
        {
            memcpy(cookie, cookiePrefix, prefixLen);
            memcpy(cookie + prefixLen, f_pCookie, cookieLen);
            cookie[prefixLen + cookieLen] = '\0';

		    headers = _slist_append( headers, cookie );

            LOG_INFO("SetHttpHeader :  cookie added to header --- (%s)\n", cookie);

            free(cookie);
        }
	}

	if (f_pHttpHeader)
	{
        LOG_INFO("SetHttpHeader :  HttpHeader added to header --- (%s)\n", f_pHttpHeader);
		headers = _slist_append (headers, f_pHttpHeader);
    }

    if ( headers )
    {
        curl_easy_setopt(pCurl, CURLOPT_HTTPHEADER, headers);
    }

	return headers;
}

/**
  * HttpOpen()
  * Start a libcurl easy session
  * @return	If this function returns NULL, something went wrong and you cannot use the other curl functions.
  * @see
  * @remark
  * @version 1.0
  */
static HTTP_SESSION* HttpOpen()
{
  HTTP_SESSION* pSession = NULL;

  CURL* pCurl = curl_easy_init();
    if ( pCurl )
    {
        pSession = (HTTP_SESSION*)malloc( sizeof(HTTP_SESSION) );
        if ( pSession )
        {
            memset( pSession, 0, sizeof(HTTP_SESSION) );
            pSession->curl_handle = pCurl;
            return pSession;
        }
        curl_easy_cleanup( pCurl );
    }
    LOG_INFO("Can't create CURL object, curl_global_init missed");
    return NULL;
}

int CURL_PROGRESS_CALLBACK(void* ptr, double TotalToDownload, double NowDownloaded, double TotalToUpload, double NowUploaded)
{
	bool *pCancelRequest = (bool*) ptr;

	if(pCancelRequest)
	{
        LOG_INFO("pCancelRequest : (%d)\n", *pCancelRequest);

		if(*pCancelRequest)
		{
            LOG_INFO("%s:%d curl works canceled.\n", __FUNCTION__, __LINE__);
			return 1;
		}
	}

	return 0;
}

/**
  * HttpStartTransaction()
  * Start a libcurl easy session
  * @param		handle            [in] struct passed to libcurl
  * @param		pUrl              [in] url of destation for posting http message
  * @param		pPostData         [in] http body for posting http message
  * @param		postLen           [in] length of http body for posting http message
  * @param		httpInd           [in] accomdate for http message passed from libcurl
  * @param		httpAbortInd      [in] not used
  * @param		httpChunkInd      [in] accomdate for chunked http message passed from libcurl
  * @param		httpAddChunkInd   [in] not used
  * @return	If this function returns CURLE_OK or CURLE_PARTIAL_FILE, something went well.
  * @see
  * @remark
  * @version 1.0
  */
static DRM_RESULT HttpStartTransaction(
    HTTP_SESSION* hSession,
    const char* f_pUrl,
    const void* f_pbPostData,
    unsigned f_cbPostData,
    DRM_MSG_TYPE f_type,
    const char* f_pCookie,
    const char *f_pSoapHeader,
    const char *f_pHttpHeader,
    const char *f_pUserAgent,
    bool* pCancelRequest    )
{
	CURLcode fRes = CURLE_OK;
	struct curl_slist *headers = NULL;
	CURL* pCurl = hSession->curl_handle;

	// 1. Set Post Data
	hSession->postDataLen = f_cbPostData;
	hSession->sendDataLen = 0;
	hSession->body.iSize = 0;
	hSession->header.iSize = 0;

    LOG_INFO("HttpStartTransaction : f_type(%d)\n", f_type);

	// 2. Set Header type
	hSession->type = f_type;

	headers = SetHttpHeader( pCurl, f_type, f_pCookie,  f_pHttpHeader, f_pUserAgent);

	if ( !headers )
	{
        LOG_INFO("Failed to set HTTP header.\n");
		return DRM_E_NETWORK_HEADER;
	}

	curl_easy_setopt(pCurl, CURLOPT_VERBOSE, 0L);

	// Check
	curl_easy_setopt(pCurl, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_1_0);

	int soap_flag = 0;

	if ( f_pbPostData && f_cbPostData > 0 )
	{
		if (f_pSoapHeader != NULL)
		{
			DRM_RESULT dr = _ComposePostData_TZ(hSession, (char *)f_pbPostData, f_cbPostData, f_pSoapHeader);
			if (dr != DRM_SUCCESS)
			{
				LOG_INFO("Failed to compose post data, dr : 0x%lx\n", dr);
				return dr;
			}
			else if(dr == DRM_SUCCESS)
			{
				soap_flag = 1;
			}
		}

		fRes = curl_easy_setopt(pCurl, CURLOPT_POST, 1L);
		ChkCURLFAIL(fRes);

		if(soap_flag == 0)
		{
			if ( !(hSession->postData = (unsigned char*)malloc( f_cbPostData )) )
			{
				if (headers != NULL)
				{
					curl_slist_free_all (headers);
				}
				LOG_INFO("Failed to alloc post data.\n");
				return DRM_E_POINTER;
			}
			memcpy( hSession->postData, f_pbPostData , f_cbPostData );

			hSession->postDataLen = f_cbPostData;
		}

		fRes = curl_easy_setopt(pCurl, CURLOPT_READFUNCTION, SendBody);
		ChkCURLFAIL(fRes);

		fRes = curl_easy_setopt(pCurl, CURLOPT_POSTFIELDSIZE, hSession->postDataLen);
		ChkCURLFAIL(fRes);

		fRes = curl_easy_setopt(pCurl, CURLOPT_READDATA, hSession);
		ChkCURLFAIL(fRes);
	}
	else
	{
		curl_easy_setopt (pCurl, CURLOPT_HTTPGET , 1L);
	}

	curl_easy_setopt(pCurl, CURLOPT_USE_SSL, 1L);
	curl_easy_setopt(pCurl, CURLOPT_SSL_VERIFYPEER, 1L);
	curl_easy_setopt(pCurl, CURLOPT_SSL_VERIFYHOST, 2L);

	// set timeout 10 seconds
	curl_easy_setopt(pCurl, CURLOPT_TIMEOUT, 10);

	fRes = curl_easy_setopt(pCurl, CURLOPT_URL, f_pUrl);
	ChkCURLFAIL(fRes);

	fRes = curl_easy_setopt(pCurl, CURLOPT_NOPROGRESS, 0L);
	ChkCURLFAIL(fRes);
	fRes = curl_easy_setopt(pCurl, CURLOPT_PROGRESSFUNCTION, CURL_PROGRESS_CALLBACK);
	ChkCURLFAIL(fRes);
	fRes = curl_easy_setopt(pCurl, CURLOPT_PROGRESSDATA, pCancelRequest);
	ChkCURLFAIL(fRes);

	fRes = curl_easy_setopt(pCurl, CURLOPT_HEADERFUNCTION, ReceiveHeader);
	ChkCURLFAIL(fRes);

	fRes = curl_easy_setopt(pCurl, CURLOPT_BUFFERSIZE, 1024L*20L);
	ChkCURLFAIL(fRes);

	fRes = curl_easy_setopt(pCurl, CURLOPT_WRITEFUNCTION, ReceiveBody);
	ChkCURLFAIL(fRes);

	fRes = curl_easy_setopt(pCurl, CURLOPT_WRITEHEADER, hSession);
	ChkCURLFAIL(fRes);

	fRes = curl_easy_setopt(pCurl, CURLOPT_WRITEDATA, hSession);
	ChkCURLFAIL(fRes);

	fRes = curl_easy_setopt(pCurl, CURLOPT_NOSIGNAL, 1); //Add by SJKIM 2013.12.18 for signal safe [according to guide]
	ChkCURLFAIL(fRes);

	fRes = curl_easy_perform(pCurl);

	/*CURLE_COULDNT_RESOLVE_PROXY:
	CURLE_COULDNT_RESOLVE_HOST
	CURLE_COULDNT_CONNECT
	CURLE_REMOTE_ACCESS_DENIED
	CURLE_OPERATION_TIMEDOUT
	CURLE_SSL_CONNECT_ERROR
	CURLE_SEND_ERROR
	CURLE_RECV_ERROR*/

	if (fRes == CURLE_OK)
	{
		LOG_INFO(" after curl_easy_perform : fRes(%d)\n", fRes);
		curl_easy_getinfo(pCurl, CURLINFO_RESPONSE_CODE, (long *)&hSession->resCode);
		LOG_INFO(" after curl_easy_perform : hSession->resCode(%ld)\n", hSession->resCode);
	}

	// Secure Clock Petition Server returns wrong size ..
	else if (fRes == CURLE_PARTIAL_FILE)
	{
		LOG_INFO(" after curl_easy_perform : fRes(%d)\n", fRes);
		curl_easy_getinfo(pCurl, CURLINFO_RESPONSE_CODE, (long *)&hSession->resCode);
		LOG_INFO(" after curl_easy_perform : hSession->resCode(%ld)\n", hSession->resCode);
		fRes = CURLE_OK;
	}
	else if (fRes == CURLE_SEND_ERROR)
	{
		LOG_INFO(" after curl_easy_perform : fRes(%d)\n", fRes);
		curl_easy_getinfo(pCurl, CURLINFO_RESPONSE_CODE, (long *)&hSession->resCode);
		LOG_INFO(" after curl_easy_perform : hSession->resCode(%ld)\n", hSession->resCode);
		fRes = CURLE_OK;
	}
	else
	{
		LOG_INFO(" after curl_easy_perform : fRes(%d)\n", fRes);
		curl_easy_getinfo(pCurl, CURLINFO_RESPONSE_CODE, (long *)&hSession->resCode);
		LOG_INFO(" after curl_easy_perform : hSession->resCode(%ld)\n", hSession->resCode);
		if (fRes == CURLE_OPERATION_TIMEDOUT)
		{
			LOG_INFO("CURLE_OPERATION_TIMEDOUT occured\n");
		}

		if ( headers != NULL )
		{
			curl_slist_free_all (headers);
		}

		if(fRes == CURLE_OUT_OF_MEMORY)
		{
            LOG_INFO("Failed to alloc from curl.\n");
			return DRM_E_POINTER;
		}
		else if (fRes == CURLE_ABORTED_BY_CALLBACK )
		{
			*pCancelRequest = false;
            LOG_INFO("Network job canceled by caller.\n");
			return DRM_E_NETWORK_CANCELED;
		}
		else
		{
            LOG_INFO("Failed from curl, curl message : %s\n", curl_easy_strerror(fRes));
			return DRM_E_NETWORK_CURL;
		}
	}

ErrorExit:
	if (headers != NULL) {
		//INFO_CURL_HEADERS(headers);
		curl_slist_free_all (headers);
	}

	if (fRes != CURLE_OK)
	{
		if(fRes == CURLE_OUT_OF_MEMORY)
		{
            LOG_INFO("Failed to alloc from curl.\n");
			return DRM_E_POINTER;
		}
		else
		{
            LOG_INFO("Failed from curl, curl message : %s\n", curl_easy_strerror(fRes));
			return DRM_E_NETWORK_CURL;
		}
	}

	return DRM_SUCCESS;
}

/**
  * HttpClose()
  * End a libcurl easy session
  * @param	handle   [in] struct passed to libcurl
  * @return	void
  * @see
  * @remark
  * @version 2.0
  */
static void HttpClose(HTTP_SESSION* hSession)
{
    if ( !hSession )
    {
        return;
    }

    if ( hSession->curl_handle != NULL )
    {
        curl_easy_cleanup( hSession->curl_handle );
    }
    if ( hSession->postData )
    {
         free( hSession->postData );
    }
    if ( hSession->body.iData )
    {
        free( hSession->body.iData );
    }
    if ( hSession->header.iData )
    {
        free( hSession->header.iData );
    }
  free( hSession );
}


/**
  * SendBody()

  * @param		prt       [out] http body for posting
  * @param		size      [in]
  * @param		nmemb     [in]
  * @param		pStream   [in] HTTP_SESSION
  * @return	void
  * @see
  * @remark
  * @version 2.0
  */
static size_t SendBody(void* ptr, size_t size, size_t nmemb, void* pStream)
{
	HTTP_SESSION* pSession = (HTTP_SESSION*)pStream;

	size_t availData = pSession->postDataLen - pSession->sendDataLen;
	size_t canSend = size * nmemb;

	if ( availData == 0 )
	{
		return 0;
	}

	if ( canSend > availData )
	{
		canSend = availData;
	}

	memcpy( ptr, pSession->postData + pSession->sendDataLen, canSend );
	pSession->sendDataLen += canSend;
	return canSend;
}

/**
  * ReceiveHeader()
  * The header data must be written when using this callback function
  * @param		prt       [out] string with the header data
  * @param		size      [in] size of data block
  * @param		nmemb     [in] number of data block
  * @param		pStream   [in] string with the header data to be written
  * @return	    Return the number of bytes written.
  * @see
  * @remark
  * @version 2.0
  */
static size_t ReceiveHeader(void* ptr, size_t size, size_t nmemb, void* pStream)
{
	size_t dataSize = size * nmemb;

	if ( dataSize > 0 )
	{
		HTTP_SESSION* pSession = (HTTP_SESSION*)pStream;

		if ( !AppendData( &pSession->header, ptr, dataSize ) )
		{
			return 0;
		}
	}
	return dataSize;
}

/**
  * ReceiveBody()
  * The body data must be written when using this callback function
  * @param		prt       [out] string with the body data
  * @param		size      [in] size of data block
  * @param		nmemb     [in] number of data block
  * @param		pStream   [in] string with the body data to be written
  * @return	    Return the number of bytes written.
  * @see
  * @remark
  * @version 2.0
  */
static size_t ReceiveBody(void* ptr, size_t size, size_t nmemb, void* pStream)
{
	size_t dataSize = size * nmemb;

	if ( dataSize > 0 )
	{
		HTTP_SESSION* pSession = (HTTP_SESSION*)pStream;

		if ( !AppendData( &pSession->body, ptr, dataSize ) )
		{
			return 0;
		}
	}
	return dataSize;
}


DRM_RESULT PRNetManager_DoTransaction_TZ(const char* pServerUrl,
													 const void* f_pbChallenge,
													 unsigned f_cbChallenge,
													 unsigned char** f_ppbResponse,
													 unsigned* f_pcbResponse,
													 DRM_MSG_TYPE f_type,
													 const char* f_pCookie,
													 PRExtensionCtx_TZ *pExtCtx)
{
	if ( !f_ppbResponse || !f_pcbResponse )
	{
        LOG_INFO("Invalid arg.\n");
		return DRM_E_INVALIDARG;
	}

	*f_ppbResponse = NULL; *f_pcbResponse = 0;

#ifdef PR_STANDALONE_BUILD
	CURLcode err = curl_global_init(CURL_GLOBAL_ALL);
	if (err != CURLE_OK)
	{
        LOG_INFO("curl_global_init fail, curl message : %s", curl_easy_strerror(err));
		return DRM_E_NETWORK_CURL;
	}
#endif

	const char* pUrl = pServerUrl;
	HTTP_SESSION* pSession;
	char* szRedirectUrl = NULL;

	DRM_RESULT dr = DRM_SUCCESS;

	// Redirection 3 times..
	for (int i = 0 ; i < 3 ; i++)
	{
		if( !(pSession = HttpOpen()) )
		{
            LOG_INFO("Failed to open HTTP session.\n");
			break;
		}

		char *pSoapHdr = NULL;
		char *pHttpHdr = NULL;
		char *pUserAgent = NULL;
		bool *pCancelRequest = NULL;

		if (pExtCtx != NULL)
		{
			if (pExtCtx->pSoapHeader)
			{
				pSoapHdr = pExtCtx->pSoapHeader;
			}

			if (pExtCtx->pHttpHeader)
			{
				pHttpHdr = pExtCtx->pHttpHeader;
			}

			if (pExtCtx->pUserAgent)
			{
				pUserAgent = pExtCtx->pUserAgent;
			}

			pCancelRequest = &(pExtCtx->cancelRequest);
		}

		dr = HttpStartTransaction( pSession, pUrl, f_pbChallenge, f_cbChallenge, f_type,
									f_pCookie, pSoapHdr, pHttpHdr, pUserAgent, pCancelRequest); //need  soapheader
		if ( dr != DRM_SUCCESS )
		{
            LOG_INFO("Failed on network transaction(%d/%d), dr : 0x%lx", i+1, 3, dr);
			break;
		}

		if (pSession->resCode == 301 || pSession->resCode == 302)
		{
			if ( szRedirectUrl )
			{
				free(szRedirectUrl);
				szRedirectUrl = NULL;
			}
			// Convert https to http for GETSECURECLOCKSERVER_URL
			szRedirectUrl = _GetRedirectLocation( (const char*)pSession->header.iData, f_type != GETSECURECLOCKSERVER_URL );

			HttpClose(pSession); pSession = NULL;
			if ( !szRedirectUrl )
			{
                LOG_INFO("Failed to get redirect URL\n");
				break;
			}
			pUrl = szRedirectUrl;
		}
		else
		{
			if ( pSession->resCode != 200 )
			{
                LOG_INFO("Server returns response Code %ld\n[%s][%d]\n", pSession->resCode , pSession->body.iData , pSession->body.iSize);

				if( pSession->resCode >= 400 && pSession->resCode < 500)
				{
					dr = DRM_E_NETWORK_CLIENT;
				}
				else if( pSession->resCode >= 500 && pSession->resCode < 600)
				{
					dr = DRM_E_NETWORK_SERVER;
				}
				else
				{
					dr = DRM_E_NETWORK;
				}
				break;
			}

            *f_ppbResponse = pSession->body.iData; *f_pcbResponse = pSession->body.iSize;

            pSession->body.iData = NULL; pSession->body.iSize = 0; pSession->body.iAllocated = 0;
            dr = DRM_SUCCESS;
			break;
		}
	}

	if ( szRedirectUrl )
	{
		free(szRedirectUrl);
		szRedirectUrl = NULL;
	}

	HttpClose(pSession);

#ifdef PR_STANDALONE_BUILD
	curl_global_cleanup();
#endif

	if( dr != DRM_SUCCESS )
	{
        LOG_INFO("Failed on network transaction, dr : 0x%lx\n", dr);
	}

	return dr;
}

