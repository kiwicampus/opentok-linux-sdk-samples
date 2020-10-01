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
  std::cout << "Caught signal " << signum << std::endl;
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

  std::cout << "STARTING STREAMING LOOP" << std::endl;

  while (video_capturer->capturer_thread_exit.load() == false)
  {
    // Mat frame;
    // cap >> frame; // get a new frame from camera
    // std::cout << "Frame size: " << frame.rows << "x" << frame.cols << std::endl;

    // std::cout << "Loop..." << std::endl;
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
  std::cout << __FUNCTION__ << " callback function" << std::endl;

  g_is_connected = true;

  if ((session != nullptr) && (g_publisher != nullptr))
  {
    if (otc_session_publish(session, g_publisher) == OTC_SUCCESS)
    {
      g_is_publishing = true;
      return;
    }
    std::cout << "Could not publish successfully" << std::endl;
  }
  else
    std::cout << __FUNCTION__ << " session and publisher is null" << std::endl;
}

static void on_session_connection_created(otc_session *session,
                                          void *user_data,
                                          const otc_connection *connection)
{
  std::cout << __FUNCTION__ << " callback function" << std::endl;
}

static void on_session_connection_dropped(otc_session *session,
                                          void *user_data,
                                          const otc_connection *connection)
{
  std::cout << __FUNCTION__ << " callback function" << std::endl;
}

static void on_session_stream_received(otc_session *session,
                                       void *user_data,
                                       const otc_stream *stream)
{
  std::cout << __FUNCTION__ << " callback function" << std::endl;
}

static void on_session_stream_dropped(otc_session *session,
                                      void *user_data,
                                      const otc_stream *stream)
{
  std::cout << __FUNCTION__ << " callback function" << std::endl;
}

static void on_session_disconnected(otc_session *session, void *user_data)
{
  std::cout << __FUNCTION__ << " callback function" << std::endl;
}

static void on_session_error(otc_session *session,
                             void *user_data,
                             const char *error_string,
                             enum otc_session_error_code error)
{
  std::cout << __FUNCTION__ << " callback function" << std::endl;
  std::cout << "Session error. Error : " << error_string << std::endl;
}

static void on_publisher_stream_created(otc_publisher *publisher,
                                        void *user_data,
                                        const otc_stream *stream)
{
  std::cout << __FUNCTION__ << " callback function" << std::endl;
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
}

static void on_publisher_stream_destroyed(otc_publisher *publisher,
                                          void *user_data,
                                          const otc_stream *stream)
{
  std::cout << __FUNCTION__ << " callback function" << std::endl;
}

static void on_publisher_error(otc_publisher *publisher,
                               void *user_data,
                               const char *error_string,
                               enum otc_publisher_error_code error_code)
{
  std::cout << __FUNCTION__ << " callback function" << std::endl;
  std::cout << "Publisher error. Error code: " << error_string << std::endl;
}

static void on_otc_log_message(const char *message)
{
  std::cout << __FUNCTION__ << ":" << message << std::endl;
}

int main(int argc, char **argv)
{
  if (otc_init(nullptr) != OTC_SUCCESS)
  {
    std::cout << "Could not init OpenTok library" << std::endl;
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
  otc_log_enable(OTC_LOG_LEVEL_WARN);

  struct otc_session_callbacks session_callbacks = {0};
  session_callbacks.on_connected = on_session_connected;
  session_callbacks.on_connection_created = on_session_connection_created;
  session_callbacks.on_connection_dropped = on_session_connection_dropped;
  session_callbacks.on_stream_received = on_session_stream_received;
  session_callbacks.on_stream_dropped = on_session_stream_dropped;
  session_callbacks.on_disconnected = on_session_disconnected;
  session_callbacks.on_error = on_session_error;

  otc_session *session = nullptr;
  session = otc_session_new(API_KEY, SESSION_ID, &session_callbacks);

  if (session == nullptr)
  {
    std::cout << "Could not create OpenTok session successfully" << std::endl;
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

  g_publisher = otc_publisher_new("opentok-linux-sdk-samples",
                                  &(video_capturer->video_capturer_callbacks),
                                  &publisher_callbacks);
  if (g_publisher == nullptr)
  {
    std::cout << "Could not create OpenTok publisher successfully" << std::endl;
    otc_session_delete(session);
    return EXIT_FAILURE;
  }
  // renderer_manager.createRenderer(g_publisher);

  otc_session_connect(session, TOKEN);

  // renderer_manager.runEventLoop();

  // Register signal and signal handler
  signal(SIGINT, signal_callback_handler);
  while (RUN_LOOP)
  {
    // std::cout << "Program processing..." << std::endl;
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
