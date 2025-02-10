#ifndef FLUTTER_PLUGIN_RECORD_LINUX_PLUGIN_H_
#define FLUTTER_PLUGIN_RECORD_LINUX_PLUGIN_H_

#include <flutter_linux/flutter_linux.h>
#include <glib-object.h>
#include <pulse/simple.h>
#include <stdio.h>
#include <stdint.h>
#include <string> // for std::string usage

G_BEGIN_DECLS

////////////////////////////////////////////////////////////////////////////////
//  WAV header struct used by both .h & .cc
////////////////////////////////////////////////////////////////////////////////
#pragma pack(push, 1)
typedef struct
{
  char riff[4];
  uint32_t overall_size;
  char wave[4];
  char fmt_chunk_marker[4];
  uint32_t length_of_fmt;
  uint16_t format_type;
  uint16_t channels;
  uint32_t sample_rate;
  uint32_t byterate;
  uint16_t block_align;
  uint16_t bits_per_sample;
  char data_chunk_header[4];
  uint32_t data_size;
} WavHeader;
#pragma pack(pop)

////////////////////////////////////////////////////////////////////////////////
//  Forward declarations of our GObject struct and class
////////////////////////////////////////////////////////////////////////////////
typedef struct _RecordLinuxPlugin RecordLinuxPlugin;
typedef struct _RecordLinuxPluginClass RecordLinuxPluginClass;

////////////////////////////////////////////////////////////////////////////////
//  The RecordLinuxPlugin struct
//  Must contain ALL your fields (like std::string file_path, buffer, etc.)
////////////////////////////////////////////////////////////////////////////////
struct _RecordLinuxPlugin
{
  GObject parent_instance; // MUST be first

  // PulseAudio
  pa_simple *pa_handle;
  pa_sample_spec pa_spec;

  // Recording state
  bool is_recording;
  bool is_paused;
  bool is_stream_mode;

  // Thread & synchronization
  GThread *record_thread_handle;
  GMutex state_mutex;
  GCond pause_cond;

  // Audio buffer
  static const size_t K_BUFFER_SIZE = 4096;
  uint8_t buffer[K_BUFFER_SIZE];

  // Flutter method channel
  FlMethodChannel *channel;

  // File-based recording
  FILE *file_handle;
  size_t total_data_bytes;
  std::string file_path; // can call file_path.clear() safely

  // WAV header
  WavHeader wav_header;
};

struct _RecordLinuxPluginClass
{
  GObjectClass parent_class;
};

#define RECORD_LINUX_PLUGIN(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj), record_linux_plugin_get_type(), RecordLinuxPlugin))

////////////////////////////////////////////////////////////////////////////////
//  We'll declare a manual "record_linux_plugin_get_type()" function
////////////////////////////////////////////////////////////////////////////////
GType record_linux_plugin_get_type(void);

#ifdef __cplusplus
extern "C"
{
#endif

  ////////////////////////////////////////////////////////////////////////////////
  //  Exported registration function so Flutter can call it
  ////////////////////////////////////////////////////////////////////////////////
  __attribute__((visibility("default"))) void record_linux_plugin_register_with_registrar(FlPluginRegistrar *registrar);

  ////////////////////////////////////////////////////////////////////////////////
  //  Non-static function declarations matching the .cc definitions
  ////////////////////////////////////////////////////////////////////////////////
  FlMethodResponse *create_recorder(RecordLinuxPlugin *self);
  FlMethodResponse *dispose_recorder(RecordLinuxPlugin *self);

  FlMethodResponse *start_recording_file(RecordLinuxPlugin *self, const gchar *path);
  FlMethodResponse *stop_recording_file(RecordLinuxPlugin *self);

  FlMethodResponse *start_recording_stream(RecordLinuxPlugin *self);
  FlMethodResponse *stop_recording_stream(RecordLinuxPlugin *self);

  FlMethodResponse *cancel_recording(RecordLinuxPlugin *self);
  FlMethodResponse *pause_recording(RecordLinuxPlugin *self);
  FlMethodResponse *resume_recording(RecordLinuxPlugin *self);

  FlMethodResponse *list_input_devices(RecordLinuxPlugin *self);
  FlMethodResponse *is_encoder_supported(RecordLinuxPlugin *self, const gchar *encoder);
  FlMethodResponse *get_amplitude(RecordLinuxPlugin *self);
  FlMethodResponse *has_permission(RecordLinuxPlugin *self);
  FlMethodResponse *is_paused_fn(RecordLinuxPlugin *self);
  FlMethodResponse *is_recording_fn(RecordLinuxPlugin *self);

  G_END_DECLS
#ifdef __cplusplus
}
#endif

#endif // FLUTTER_PLUGIN_RECORD_LINUX_PLUGIN_H_
