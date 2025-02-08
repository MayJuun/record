#ifndef FLUTTER_PLUGIN_RECORD_LINUX_PLUGIN_H_
#define FLUTTER_PLUGIN_RECORD_LINUX_PLUGIN_H_

#include <flutter_linux/flutter_linux.h>
#include <glib-object.h>
#include <pulse/simple.h>
#include <stdio.h>
#include <stdint.h>

G_BEGIN_DECLS

typedef struct _RecordLinuxPlugin RecordLinuxPlugin;
typedef struct _RecordLinuxPluginClass RecordLinuxPluginClass;

struct _RecordLinuxPlugin {
  GObject parent_instance;

  // PulseAudio config
  pa_simple* s;
  pa_sample_spec ss;
  bool is_recording;

  // Background read thread
  GThread* record_thread_handle;

  // Buffer for reading audio
  uint8_t* buffer;

  // Flutter method channel
  FlMethodChannel* channel;

  // File-based recording
  FILE* file_handle;
  size_t total_data_bytes;
  gchar* file_path;
};

struct _RecordLinuxPluginClass {
  GObjectClass parent_class;
};

#define RECORD_LINUX_PLUGIN(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj), record_linux_plugin_get_type(), RecordLinuxPlugin))

GType record_linux_plugin_get_type();
void record_linux_plugin_register_with_registrar(FlPluginRegistrar* registrar);

// File-based recording
FlMethodResponse* start_recording_file(RecordLinuxPlugin* self, const gchar* path);
FlMethodResponse* stop_recording_file(RecordLinuxPlugin* self);

// Stream-based recording (if you are sending PCM back to Dart)
FlMethodResponse* start_recording_stream(RecordLinuxPlugin* self);

// Potentially you might want separate “stop_stream” or rely on the same “stop_recording_file()”
FlMethodResponse* pause_recording(RecordLinuxPlugin* self);
FlMethodResponse* resume_recording(RecordLinuxPlugin* self);
FlMethodResponse* cancel_recording(RecordLinuxPlugin* self);

// Utility methods
FlMethodResponse* list_input_devices(RecordLinuxPlugin* self);
FlMethodResponse* is_encoder_supported(RecordLinuxPlugin* self, const gchar* encoder);
FlMethodResponse* get_amplitude(RecordLinuxPlugin* self);
FlMethodResponse* has_permission(RecordLinuxPlugin* self);
FlMethodResponse* is_paused(RecordLinuxPlugin* self);
FlMethodResponse* is_recording(RecordLinuxPlugin* self);

// If you want to implement or stub "create", "dispose" etc:
FlMethodResponse* create_recorder(RecordLinuxPlugin* self);
FlMethodResponse* dispose_recorder(RecordLinuxPlugin* self);

G_END_DECLS

#endif  // FLUTTER_PLUGIN_RECORD_LINUX_PLUGIN_H_
