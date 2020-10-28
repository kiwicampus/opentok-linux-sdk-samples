#include <opentok.h>

#include <atomic>
#include <cstdint>
#include <cstdlib>
#include <iostream>

#include "config.h"
#include "otk_thread.h"
#include <signal.h>

// #include "opencv2/opencv.hpp"

// using namespace cv;

static std::atomic<bool>
    g_is_connected(false);
static otc_publisher *g_publisher = nullptr;
static std::atomic<bool> g_is_publishing(false);

bool RUN_LOOP = true;
void signal_callback_handler(int signum)
{
  std::cout << "[MY CODE]: "
            << "Caught signal " << signum << std::endl;
  // Terminate program
  //  exit(signum);
  RUN_LOOP = false;
}

struct custom_video_capturer
{
  const otc_video_capturer *video_capturer;
  struct otc_video_capturer_callbacks video_capturer_callbacks;
  int width;
  int height;
  otk_thread_t capturer_thread;
  std::atomic<bool> capturer_thread_exit;
};

static int generate_random_integer()
{
  srand(time(nullptr));
  return rand();
}

static otk_thread_func_return_type capturer_thread_start_function(void *arg)
{
  struct custom_video_capturer *video_capturer = static_cast<struct custom_video_capturer *>(arg);
  if (video_capturer == nullptr)
  {
    otk_thread_func_return_value;
  }

  // VideoCapture cap(0); // open the default camera
  // if (!cap.isOpened()) // check if we succeeded
  //   otk_thread_func_return_value;

  uint8_t *buffer = (uint8_t *)malloc(sizeof(uint8_t) * video_capturer->width * video_capturer->height * 3);

  std::cout << "[MY CODE]: "
            << "STARTING STREAMING LOOP" << std::endl;

  while (video_capturer->capturer_thread_exit.load() == false)
  {
    // Mat frame;
    // cap >> frame; // get a new frame from camera
    // std::cout << "[MY CODE]: "<< "Frame size: " << frame.rows << "x" << frame.cols << std::endl;

    // std::cout << "[MY CODE]: "<< "Loop..." << std::endl;
    memset(buffer, generate_random_integer() & 0xFF, video_capturer->width * video_capturer->height * 3);
    // memcpy(buffer, frame.data, frame.total() * frame.elemSize());

    otc_video_frame *otc_frame = otc_video_frame_new(OTC_VIDEO_FRAME_FORMAT_RGB24, video_capturer->width, video_capturer->height, buffer);

    otc_video_capturer_provide_frame(video_capturer->video_capturer, 0, otc_frame);

    if (otc_frame != nullptr)
    {
      otc_video_frame_delete(otc_frame);
    }
    usleep(1000 / 30 * 1000);
  }

  if (buffer != nullptr)
  {
    free(buffer);
  }

  // cap.release();

  otk_thread_func_return_value;
}

static otc_bool video_capturer_init(const otc_video_capturer *capturer, void *user_data)
{
  struct custom_video_capturer *video_capturer = static_cast<struct custom_video_capturer *>(user_data);
  if (video_capturer == nullptr)
  {
    return OTC_FALSE;
  }

  video_capturer->video_capturer = capturer;

  return OTC_TRUE;
}

static otc_bool video_capturer_destroy(const otc_video_capturer *capturer, void *user_data)
{
  struct custom_video_capturer *video_capturer = static_cast<struct custom_video_capturer *>(user_data);
  if (video_capturer == nullptr)
  {
    return OTC_FALSE;
  }

  video_capturer->capturer_thread_exit = true;
  otk_thread_join(video_capturer->capturer_thread);

  return OTC_TRUE;
}

static otc_bool video_capturer_start(const otc_video_capturer *capturer, void *user_data)
{
  struct custom_video_capturer *video_capturer = static_cast<struct custom_video_capturer *>(user_data);
  if (video_capturer == nullptr)
  {
    return OTC_FALSE;
  }

  video_capturer->capturer_thread_exit = false;
  if (otk_thread_create(&(video_capturer->capturer_thread), &capturer_thread_start_function, (void *)video_capturer) != 0)
  {
    return OTC_FALSE;
  }

  return OTC_TRUE;
}

static otc_bool get_video_capturer_capture_settings(const otc_video_capturer *capturer,
                                                    void *user_data,
                                                    struct otc_video_capturer_settings *settings)
{
  struct custom_video_capturer *video_capturer = static_cast<struct custom_video_capturer *>(user_data);
  if (video_capturer == nullptr)
  {
    return OTC_FALSE;
  }

  settings->format = OTC_VIDEO_FRAME_FORMAT_ARGB32;
  settings->width = video_capturer->width;
  settings->height = video_capturer->height;
  settings->fps = 30;
  settings->mirror_on_local_render = OTC_FALSE;
  settings->expected_delay = 0;

  return OTC_TRUE;
}

static void on_session_connected(otc_session *session, void *user_data)
{
  std::cout << "[MY CODE]: " << __FUNCTION__ << " callback function" << std::endl;

  g_is_connected = true;

  if ((session != nullptr) && (g_publisher != nullptr))
  {
    if (otc_session_publish(session, g_publisher) == OTC_SUCCESS)
    {
      g_is_publishing = true;
      return;
    }
    std::cout << "[MY CODE]: "
              << "Could not publish successfully" << std::endl;
  }
  else
    std::cout << "[MY CODE]: " << __FUNCTION__ << " session and publisher is null" << std::endl;
}

static void on_session_connection_created(otc_session *session,
                                          void *user_data,
                                          const otc_connection *connection)
{
  std::cout << "[MY CODE]: " << __FUNCTION__ << " callback function" << std::endl;
}

static void on_session_connection_dropped(otc_session *session,
                                          void *user_data,
                                          const otc_connection *connection)
{
  std::cout << "[MY CODE]: " << __FUNCTION__ << " callback function" << std::endl;
}

static void on_session_stream_received(otc_session *session,
                                       void *user_data,
                                       const otc_stream *stream)
{
  std::cout << "[MY CODE]: " << __FUNCTION__ << " callback function" << std::endl;
}

static void on_session_stream_dropped(otc_session *session,
                                      void *user_data,
                                      const otc_stream *stream)
{
  std::cout << "[MY CODE]: " << __FUNCTION__ << " callback function" << std::endl;
}

static void on_session_disconnected(otc_session *session, void *user_data)
{
  std::cout << "[MY CODE]: " << __FUNCTION__ << " callback function" << std::endl;
}

static void on_session_error(otc_session *session,
                             void *user_data,
                             const char *error_string,
                             enum otc_session_error_code error)
{
  std::cout << "[MY CODE]: " << __FUNCTION__ << " callback function" << std::endl;
  std::cout << "[MY CODE]: "
            << "Session error. Error: " << error_string << " Code: " << error << std::endl;
}

static void on_session_reconnected(otc_session *session, void *user_data)
{
  std::cout << "[MY CODE]: " << __FUNCTION__ << " callback function" << std::endl;
}

static void on_session_reconnection_started(otc_session *session, void *user_data)
{
  std::cout << "[MY CODE]: " << __FUNCTION__ << " callback function" << std::endl;
}

static void on_session_signal_received(otc_session *session,
                                       void *user_data,
                                       const char *type,
                                       const char *signal,
                                       const otc_connection *connection)
{
  std::cout << "[MY CODE]: " << __FUNCTION__ << " callback function" << std::endl;
}

static void on_session_stream_has_audio_changed(otc_session *session,
                                                void *user_data,
                                                const otc_stream *stream,
                                                otc_bool has_audio)
{
  std::cout << "[MY CODE]: " << __FUNCTION__ << " callback function" << has_audio << std::endl;
}

/**
      Called when a stream toggles video on or off.

      @param session A pointer to the otc_session struct.

      @param user_data A pointer to the user_data you set for the session.

      @param stream Whether the stream now has video (OTC_TRUE) or not (OTC_FALSE).
*/

static void on_session_stream_has_video_changed(otc_session *session,
                                                void *user_data,
                                                const otc_stream *stream,
                                                otc_bool has_video)
{
  std::cout << "[MY CODE]: " << __FUNCTION__ << " callback function " << has_video << std::endl;
}

static void on_session_stream_video_dimensions_changed(otc_session *session,
                                                       void *user_data,
                                                       const otc_stream *stream,
                                                       int width,
                                                       int height)
{
  std::cout << "[MY CODE]: " << __FUNCTION__ << " callback function. Dimensions: " << width << "x" << height << std::endl;
}

// ************ Publisher callbacks

static void on_publisher_stream_created(otc_publisher *publisher,
                                        void *user_data,
                                        const otc_stream *stream)
{
  std::cout << "[MY CODE]: " << __FUNCTION__ << " callback function" << std::endl;
}

static void on_publisher_render_frame(otc_publisher *publisher,
                                      void *user_data,
                                      const otc_video_frame *frame)
{
  // RendererManager *render_manager = static_cast<RendererManager *>(user_data);
  // if (render_manager == nullptr)
  // {
  //   return;
  // }
  // render_manager->addFrame(publisher, frame);
  // std::cout << "[MY CODE]: " << __FUNCTION__ << " callback function" << std::endl;
}

static void on_publisher_stream_destroyed(otc_publisher *publisher,
                                          void *user_data,
                                          const otc_stream *stream)
{
  std::cout << "[MY CODE]: " << __FUNCTION__ << " callback function" << std::endl;
}

static void on_publisher_error(otc_publisher *publisher,
                               void *user_data,
                               const char *error_string,
                               enum otc_publisher_error_code error)
{
  std::cout << "[MY CODE]: " << __FUNCTION__ << " callback function" << std::endl;
  std::cout << "[MY CODE]: "
            << "Publisher error. Error: " << error_string << " Code: " << error << std::endl;
}

static void on_audio_stats(otc_publisher *publisher,
                           void *user_data,
                           struct otc_publisher_audio_stats audio_stats[],
                           size_t number_of_stats)
{
  std::cout << "[MY CODE]: " << __FUNCTION__ << " callback function. # Stats: " << number_of_stats << std::endl;
}

/**
      Called periodically to report video statistics for the publisher.

      @param publisher A pointer to the publisher.
      @param user_data A pointer to the user_data you set for the publisher.
      @param video_stats An array of publisher video stats.
      @param number_of_stats The number of video stats in the array.
*/
// struct otc_publisher_video_stats {
//   const char* connection_id; /**< The connection ID of the client subscribing to the stream. */
//   const char* subscriber_id; /**< The subscriber ID of the client subscribing to the stream (in a relayed session). */
//   int64_t packets_lost; /**< The total number of video packets packets that did not reach the subscriber (or the OpenTok Media Router). */
//   int64_t packets_sent; /**< The total number of video packets sent sent to the subscriber (or to the OpenTok Media Router). */
//   int64_t bytes_sent; /**< The total number of video bytes bytes sent to the subscriber (or to the OpenTok Media Router). */
//   double timestamp; /**< The timestamp, in milliseconds since the Unix epoch, for when these stats were gathered. */
//   double start_time; /**< The timestamp, in milliseconds since the Unix epoch, from which the cumulative totals started accumulating. */
// };
static void on_video_stats(otc_publisher *publisher,
                           void *user_data,
                           struct otc_publisher_video_stats video_stats[],
                           size_t number_of_stats)
{
  std::cout << "[MY CODE]: " << __FUNCTION__ << " callback function. # Stats: " << number_of_stats << std::endl;
  for (int i = 0; i < number_of_stats; ++i)
  {
    struct otc_publisher_video_stats stats = video_stats[i];
    std::cout << "[MY CODE]: " << __FUNCTION__ << " Connection_id : " << stats.connection_id;
    // std::cout << " subscriber_id : " << stats.subscriber_id;
    std::cout << " packets_lost : " << stats.packets_lost;
    std::cout << " packets_sent : " << stats.packets_sent;
    std::cout << " bytes_sent : " << stats.bytes_sent;
    // std::cout << " timestamp : " << stats.timestamp;
    // std::cout << " start_time : " << stats.start_time << std::endl;
  }
}

// General callback

static void on_otc_log_message(const char *message)
{
  std::cout << __FUNCTION__ << ":" << message << std::endl;
}

int main(int argc, char **argv)
{
  if (otc_init(nullptr) != OTC_SUCCESS)
  {
    std::cout << "[MY CODE]: "
              << "Could not init OpenTok library" << std::endl;
    return EXIT_FAILURE;
  }

  // OTC_LOG_LEVEL_DISABLED = 0, /**< No messages enum value. */
  // OTC_LOG_LEVEL_FATAL = 2, /**< Fatal level messages. */
  // OTC_LOG_LEVEL_ERROR = 3, /**< Error level messages. */
  // OTC_LOG_LEVEL_WARN  = 4, /**< Warning level messages. */
  // OTC_LOG_LEVEL_INFO  = 5, /**< Info level messages. */
  // OTC_LOG_LEVEL_DEBUG = 6, /**< Debug level messages. */
  // OTC_LOG_LEVEL_MSG = 7, /**< Message level messages. */
  // OTC_LOG_LEVEL_TRACE = 8, /**< Trace level messages. */
  // OTC_LOG_LEVEL_ALL = 100 /**< All messages. */
  otc_log_set_logger_callback(on_otc_log_message);
  // otc_log_enable(OTC_LOG_LEVEL_INFO);
  // otc_log_enable(OTC_LOG_LEVEL_WARN);
  // otc_log_enable(OTC_LOG_LEVEL_ALL);

  struct otc_session_callbacks session_callbacks = {0};
  session_callbacks.on_connected = on_session_connected;
  session_callbacks.on_connection_created = on_session_connection_created;
  session_callbacks.on_connection_dropped = on_session_connection_dropped;
  session_callbacks.on_stream_received = on_session_stream_received;
  session_callbacks.on_stream_dropped = on_session_stream_dropped;
  session_callbacks.on_disconnected = on_session_disconnected;
  session_callbacks.on_error = on_session_error;
  session_callbacks.on_reconnected = on_session_reconnected;
  session_callbacks.on_reconnection_started = on_session_reconnection_started;
  session_callbacks.on_signal_received = on_session_signal_received;
  session_callbacks.on_stream_has_audio_changed = on_session_stream_has_audio_changed;
  session_callbacks.on_stream_has_video_changed = on_session_stream_has_video_changed;
  session_callbacks.on_stream_video_dimensions_changed = on_session_stream_video_dimensions_changed;

  otc_session *session = nullptr;
  session = otc_session_new(API_KEY, SESSION_ID, &session_callbacks);

  if (session == nullptr)
  {
    std::cout << "[MY CODE]: "
              << "Could not create OpenTok session successfully" << std::endl;
    return EXIT_FAILURE;
  }

  struct custom_video_capturer *video_capturer = (struct custom_video_capturer *)malloc(sizeof(struct custom_video_capturer));
  video_capturer->video_capturer_callbacks = {0};
  video_capturer->video_capturer_callbacks.user_data = static_cast<void *>(video_capturer);
  video_capturer->video_capturer_callbacks.init = video_capturer_init;
  video_capturer->video_capturer_callbacks.destroy = video_capturer_destroy;
  video_capturer->video_capturer_callbacks.start = video_capturer_start;
  video_capturer->video_capturer_callbacks.get_capture_settings = get_video_capturer_capture_settings;
  video_capturer->width = 800;
  video_capturer->height = 600;

  // RendererManager renderer_manager;
  struct otc_publisher_callbacks publisher_callbacks = {0};
  // publisher_callbacks.user_data = &renderer_manager;
  publisher_callbacks.on_stream_created = on_publisher_stream_created;
  publisher_callbacks.on_render_frame = on_publisher_render_frame;
  publisher_callbacks.on_stream_destroyed = on_publisher_stream_destroyed;
  publisher_callbacks.on_error = on_publisher_error;
  // publisher_callbacks.on_audio_stats = on_audio_stats;
  // publisher_callbacks.on_video_stats = on_video_stats;

  g_publisher = otc_publisher_new("opentok-linux-sdk-samples",
                                  &(video_capturer->video_capturer_callbacks),
                                  &publisher_callbacks);
  if (g_publisher == nullptr)
  {
    std::cout << "[MY CODE]: "
              << "Could not create OpenTok publisher successfully" << std::endl;
    otc_session_delete(session);
    return EXIT_FAILURE;
  }

  // otc_publisher_set_publish_audio(g_publisher, false);

  // renderer_manager.createRenderer(g_publisher);

  otc_session_connect(session, TOKEN);

  // renderer_manager.runEventLoop();

  // Register signal and signal handler
  signal(SIGINT, signal_callback_handler);
  while (RUN_LOOP)
  {
    // std::cout << "[MY CODE]: "<< "Program processing..." << std::endl;
    sleep(1);
  }

  // renderer_manager.destroyRenderer(g_publisher);

  if ((session != nullptr) && (g_publisher != nullptr) && g_is_publishing.load())
  {
    otc_session_unpublish(session, g_publisher);
  }

  if (g_publisher != nullptr)
  {
    otc_publisher_delete(g_publisher);
  }

  if ((session != nullptr) && g_is_connected.load())
  {
    otc_session_disconnect(session);
  }

  if (session != nullptr)
  {
    otc_session_delete(session);
  }

  if (video_capturer != nullptr)
  {
    free(video_capturer);
  }

  otc_destroy();

  return EXIT_SUCCESS;
}
