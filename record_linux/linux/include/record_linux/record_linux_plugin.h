#ifndef FLUTTER_PLUGIN_RECORD_LINUX_PLUGIN_H_
#define FLUTTER_PLUGIN_RECORD_LINUX_PLUGIN_H_

#include <flutter_linux/flutter_linux.h>
#include <pulse/pulseaudio.h>
#include <pulse/simple.h>
#include <glib-object.h>
#include <stdint.h>

G_BEGIN_DECLS

#ifdef FLUTTER_PLUGIN_IMPL
#define FLUTTER_PLUGIN_EXPORT __attribute__((visibility("default")))
#else
#define FLUTTER_PLUGIN_EXPORT
#endif

// Forward declarations for our types.
typedef struct _RecordLinuxPlugin RecordLinuxPlugin;
typedef struct _RecordLinuxPluginClass RecordLinuxPluginClass;

#define RECORD_LINUX_PLUGIN(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj), record_linux_plugin_get_type(), RecordLinuxPlugin))

struct _RecordLinuxPlugin {
  GObject parent_instance;

  // PulseAudio stream and configuration.
  pa_simple* s;
  pa_sample_spec ss;
  bool is_recording;
  uint8_t* buffer;

  // Flutter method channel for communicating with Dart.
  FlMethodChannel* channel;

  // Thread handle for the background recording thread.
  GThread* record_thread_handle;
};

struct _RecordLinuxPluginClass {
  GObjectClass parent_class;
};

FLUTTER_PLUGIN_EXPORT GType record_linux_plugin_get_type();

FLUTTER_PLUGIN_EXPORT void record_linux_plugin_register_with_registrar(
    FlPluginRegistrar* registrar);

// API methods to start and stop recording.
FLUTTER_PLUGIN_EXPORT FlMethodResponse* start_recording(RecordLinuxPlugin* self);
FLUTTER_PLUGIN_EXPORT FlMethodResponse* stop_recording(RecordLinuxPlugin* self);

// Declaration of the thread function.
FLUTTER_PLUGIN_EXPORT gpointer record_thread(gpointer data);

G_END_DECLS

#endif  // FLUTTER_PLUGIN_RECORD_LINUX_PLUGIN_H_
