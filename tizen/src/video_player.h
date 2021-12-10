#ifndef VIDEO_PLAYER_H_
#define VIDEO_PLAYER_H_

#include <flutter/encodable_value.h>
#include <flutter/event_channel.h>
#include <flutter/plugin_registrar.h>
#include <flutter_tizen.h>
#include <player.h>

//DRM Function
//#include <player_product.h>
//#include <drmManagerAPI.h>
//
#include <mutex>
#include <string>

#include "video_player_options.h"
typedef void* DRMSessionHandle_t;

typedef enum
{
    DM_TYPE_NONE = 0,               /**< None */
    DM_TYPE_PLAYREADY = 1,          /**< Playready */
    DM_TYPE_MARLINMS3 = 2,          /**< Marlinms3 */
    DM_TYPE_VERIMATRIX = 3,         /**< Verimatrix */
    DM_TYPE_WIDEVINE_CLASSIC = 4,   /**< Widevine classic */
    DM_TYPE_SECUREMEDIA = 5,        /**< Securemedia */
    DM_TYPE_SDRM = 6,               /**< SDRM */
    DM_TYPE_VUDU = 7,               /**< Vudu */
    DM_TYPE_WIDEVINE = 8,           /**< Widevine cdm */
    DM_TYPE_LYNK = 9,               /**< Lynk */
    DM_TYPE_CLEARKEY = 13,          /**< Clearkey */
    DM_TYPE_EME = 14,               /**< EME */
    //...
    DM_TYPE_MAX,
} dm_type_e;

typedef struct SetDataParam_s
{
    void* param1;   /**< Parameter 1 */
    void* param2;   /**< Parameter 2 */
    void* param3;   /**< Parameter 3 */
    void* param4;   /**< Parameter 4 */
} SetDataParam_t;

typedef enum {
	PLAYER_DRM_TYPE_NONE = 0,
	PLAYER_DRM_TYPE_PLAYREADY,
	PLAYER_DRM_TYPE_MARLIN,
	PLAYER_DRM_TYPE_VERIMATRIX,
	PLAYER_DRM_TYPE_WIDEVINE_CLASSIC,
	PLAYER_DRM_TYPE_SECUREMEDIA,
	PLAYER_DRM_TYPE_SDRM,
	PLAYER_DRM_TYPE_VUDU,
	PLAYER_DRM_TYPE_WIDEVINE_CDM,
	PLAYER_DRM_TYPE_AES128,
	PLAYER_DRM_TYPE_HDCP,
	PLAYER_DRM_TYPE_DTCP,
	PLAYER_DRM_TYPE_SCSA,
	PLAYER_DRM_TYPE_CLEARKEY,
	PLAYER_DRM_TYPE_EME,
	PLAYER_DRM_TYPE_MAX_COUNT,
} player_drm_type_e;

#define DM_ERROR_NONE 0

using SeekCompletedCb = std::function<void()>;
typedef bool (*security_init_complete_cb)(int* drmhandle,unsigned int length,unsigned char* psshdata, void* user_data);

class VideoPlayer {
 public:
  VideoPlayer(FlutterDesktopPluginRegistrarRef registrar_ref,
              flutter::PluginRegistrar *plugin_registrar,
              const std::string &uri, VideoPlayerOptions &options);
  ~VideoPlayer();

  long getTextureId();
  void play();
  void pause();
  void setLooping(bool is_looping);
  void setVolume(double volume);
  void setPlaybackSpeed(double speed);
  void seekTo(int position,
              const SeekCompletedCb &seek_completed_cb);  // milliseconds
  int getPosition();                                      // milliseconds
  void dispose();
  void setDisplayRoi(int x, int y, int w, int h);

 private:
  void initialize();
  void setupEventChannel(flutter::BinaryMessenger *messenger);
  void sendInitialized();
  void sendBufferingStart();
  void sendBufferingUpdate(int position);  // milliseconds
  void sendBufferingEnd();
  void sendSeeking(bool seeking);

  static void onPrepared(void *data);
  static void onBuffering(int percent, void *data);
  static void onSeekCompleted(void *data);
  static void onPlayCompleted(void *data);
  static void onInterrupted(player_interrupted_code_e code, void *data);
  static void onErrorOccurred(int code, void *data);
  //DRM Function
  static void DM_Error_CB(long err_code , char* err_msg , void * user_data);
  static int DM_EME_challenge_Data_CB(void* session_id, int msg_type , void* msg, int msg_len, void *user_data);
  void Drm_Init(player_h m_hPlayer, const std::string &uri, dm_type_e dm_type);
  void Drm_Release();

  DRMSessionHandle_t  m_DRMSession;
  int m_DrmType;
  std::string m_LicenseUrl;
  //

  bool is_initialized_;
  player_h player_;
  std::unique_ptr<flutter::EventChannel<flutter::EncodableValue>>
      event_channel_;
  std::unique_ptr<flutter::EventSink<flutter::EncodableValue>> event_sink_;
  long texture_id_;
  SeekCompletedCb on_seek_completed_;
  bool is_interrupted_;
};

#endif  // VIDEO_PLAYER_H_
