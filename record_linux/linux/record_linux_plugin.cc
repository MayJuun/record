#include <record_linux/record_linux_plugin.h>

#include <pulse/pulseaudio.h>
#include <pulse/simple.h>
#include <stdlib.h>
#include <string.h>

// Define the type using GObject's type system.
G_DEFINE_TYPE(RecordLinuxPlugin, record_linux_plugin, G_TYPE_OBJECT)

// Helper function to send audio data to Flutter on the main thread.
static gboolean send_audio_data(gpointer data) {
  RecordLinuxPlugin* plugin = RECORD_LINUX_PLUGIN(data);

  // Create a new FlValue (byte list) from the buffer.
  g_autoptr(FlValue) args = fl_value_new_uint8_list(plugin->buffer, 1024);

  if (plugin->channel) {
    fl_method_channel_invoke_method(plugin->channel,
                                    "audioData",
                                    args,
                                    /* args */ nullptr,
                                    /* response_handle */ nullptr,
                                    /* error */ nullptr);
  }

  return G_SOURCE_REMOVE;
}

// Dispose method for cleaning up resources.
static void record_linux_plugin_dispose(GObject* object) {
  RecordLinuxPlugin* self = RECORD_LINUX_PLUGIN(object);

  // If recording is still active, stop it.
  if (self->is_recording) {
    self->is_recording = false;
    if (self->record_thread_handle != nullptr) {
      g_thread_join(self->record_thread_handle);
      self->record_thread_handle = nullptr;
    }
  }

  if (self->s) {
    pa_simple_free(self->s);
    self->s = nullptr;
  }

  if (self->buffer) {
    g_free(self->buffer);
    self->buffer = nullptr;
  }

  // Chain up to the parent dispose.
  G_OBJECT_CLASS(record_linux_plugin_parent_class)->dispose(object);
}

static void record_linux_plugin_class_init(RecordLinuxPluginClass* klass) {
  GObjectClass* object_class = G_OBJECT_CLASS(klass);
  object_class->dispose = record_linux_plugin_dispose;
}

static void record_linux_plugin_init(RecordLinuxPlugin* self) {
  self->s = nullptr;
  self->is_recording = false;
  self->buffer = nullptr;
  self->channel = nullptr;
  self->record_thread_handle = nullptr;

  // Set the sample specification.
  self->ss.format = PA_SAMPLE_S16LE;
  self->ss.channels = 1;
  self->ss.rate = 16000;
}

// The background thread that reads audio data from PulseAudio.
gpointer record_thread(gpointer data) {
  RecordLinuxPlugin* self = RECORD_LINUX_PLUGIN(data);
  int error;

  while (self->is_recording) {
    if (pa_simple_read(self->s, self->buffer, 1024, &error) < 0) {
      g_warning("Failed to read audio data: %s", pa_strerror(error));
      break;
    }

    // Post the audio data to the main thread.
    g_idle_add_full(
      G_PRIORITY_DEFAULT,
      send_audio_data,
      g_object_ref(self),             // pass a ref'd pointer
      (GDestroyNotify)g_object_unref  // unref when idle callback is dropped
    );
  }

  return nullptr;
}

FlMethodResponse* start_recording(RecordLinuxPlugin* self) {
  int error;
  self->s = pa_simple_new(
      nullptr,               // Use the default server.
      "record_linux",        // Application name.
      PA_STREAM_RECORD,      // Recording stream.
      nullptr,               // Default device.
      "record",              // Stream description.
      &self->ss,             // Sample format.
      nullptr,               // Use default channel map.
      nullptr,               // Use default buffering attributes.
      &error
  );

  if (!self->s) {
    return FL_METHOD_RESPONSE(
      fl_method_error_response_new(
        "RECORD_ERROR",
        g_strdup_printf("Failed to create PulseAudio stream: %s", pa_strerror(error)),
        nullptr
      )
    );
  }

  self->is_recording = true;
  self->buffer = static_cast<uint8_t*>(g_malloc(1024));

  // Start the audio capture thread.
  self->record_thread_handle = g_thread_new("audio_capture", record_thread, self);
  
  return FL_METHOD_RESPONSE(fl_method_success_response_new(nullptr));
}

FlMethodResponse* stop_recording(RecordLinuxPlugin* self) {
  if (!self->is_recording) {
    return FL_METHOD_RESPONSE(
      fl_method_error_response_new(
        "RECORD_NOT_RUNNING",
        "Recording is not currently running.",
        nullptr
      )
    );
  }
  
  self->is_recording = false;

  // Wait for the recording thread to finish.
  if (self->record_thread_handle != nullptr) {
    g_thread_join(self->record_thread_handle);
    self->record_thread_handle = nullptr;
  }

  if (self->s) {
    pa_simple_free(self->s);
    self->s = nullptr;
  }

  if (self->buffer) {
    g_free(self->buffer);
    self->buffer = nullptr;
  }
  
  return FL_METHOD_RESPONSE(fl_method_success_response_new(nullptr));
}

void record_linux_plugin_register_with_registrar(FlPluginRegistrar* registrar) {
  // Create a new instance of the plugin.
  RecordLinuxPlugin* plugin = RECORD_LINUX_PLUGIN(
      g_object_new(record_linux_plugin_get_type(), nullptr));

  // Create the method channel to communicate with Dart.
  g_autoptr(FlStandardMethodCodec) codec = fl_standard_method_codec_new();
  plugin->channel = fl_method_channel_new(
      fl_plugin_registrar_get_messenger(registrar),
      "record_linux",
      FL_METHOD_CODEC(codec)
  );

  // Set the method call handler.
  fl_method_channel_set_method_call_handler(
    plugin->channel,
    [](FlMethodChannel* channel, FlMethodCall* method_call, gpointer user_data) {
      RecordLinuxPlugin* self = RECORD_LINUX_PLUGIN(user_data);
      const gchar* method = fl_method_call_get_name(method_call);
      FlMethodResponse* response = nullptr;

      if (g_strcmp0(method, "startRecording") == 0) {
        response = start_recording(self);
      } else if (g_strcmp0(method, "stopRecording") == 0) {
        response = stop_recording(self);
      } else {
        response = FL_METHOD_RESPONSE(fl_method_not_implemented_response_new());
      }

      fl_method_call_respond(method_call, response, nullptr);
    },
    plugin,
    g_object_unref
  );

  g_object_unref(plugin);
}
