#include "record_linux/record_linux_plugin.h"

#include <flutter_linux/flutter_linux.h>
#include <glib-object.h>
#include <gtk/gtk.h>
#include <pulse/error.h>
#include <pulse/simple.h>
#include <cstring>
#include <thread>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

////////////////////////////////////////////////////////////////////////////////
// G_DEFINE_TYPE
////////////////////////////////////////////////////////////////////////////////

G_DEFINE_TYPE(RecordLinuxPlugin, record_linux_plugin, G_TYPE_OBJECT)

// Called when the plugin is disposed
static void record_linux_plugin_dispose(GObject* object) {
  RecordLinuxPlugin* self = (RecordLinuxPlugin*)object;

  // Stop if recording
  g_mutex_lock(&self->state_mutex);
  self->is_recording = false;
  g_cond_broadcast(&self->pause_cond);
  g_mutex_unlock(&self->state_mutex);

  if (self->record_thread_handle) {
    g_thread_join(self->record_thread_handle);
    self->record_thread_handle = nullptr;
  }

  // Close file
  if (self->file_handle) {
    fclose(self->file_handle);
    self->file_handle = nullptr;
  }

  // Disconnect from PulseAudio
  if (self->pa_handle) {
    pa_simple_free(self->pa_handle);
    self->pa_handle = nullptr;
  }

  // Chain up
  G_OBJECT_CLASS(record_linux_plugin_parent_class)->dispose(object);
}

static void record_linux_plugin_class_init(RecordLinuxPluginClass* klass) {
  GObjectClass* object_class = G_OBJECT_CLASS(klass);
  object_class->dispose = record_linux_plugin_dispose;
}

static void record_linux_plugin_init(RecordLinuxPlugin* self) {
  // Initialize fields
  self->pa_handle = nullptr;
  self->is_recording = false;
  self->is_paused = false;
  self->is_stream_mode = false;
  self->record_thread_handle = nullptr;
  self->file_handle = nullptr;
  self->file_path.clear();
  self->total_data_bytes = 0;

  // init WAV header to zeros
  memset(&self->wav_header, 0, sizeof(WavHeader));

  g_mutex_init(&self->state_mutex);
  g_cond_init(&self->pause_cond);
}

////////////////////////////////////////////////////////////////////////////////
// Helpers for WAV & PulseAudio
////////////////////////////////////////////////////////////////////////////////

static void write_wav_header(FILE* file, const WavHeader& header) {
  fseek(file, 0, SEEK_SET);
  fwrite(&header, sizeof(header), 1, file);
  fflush(file);
}

// Build a fresh WavHeader
static WavHeader init_wav_header() {
  WavHeader hdr;
  memcpy(hdr.riff, "RIFF", 4);
  memcpy(hdr.wave, "WAVE", 4);
  memcpy(hdr.fmt_chunk_marker, "fmt ", 4);
  memcpy(hdr.data_chunk_header, "data", 4);

  hdr.overall_size = 0;
  hdr.length_of_fmt = 16;
  hdr.format_type = 1;     // PCM
  hdr.channels = 2;
  hdr.sample_rate = 44100;
  hdr.bits_per_sample = 16;
  hdr.byterate = hdr.sample_rate * hdr.channels * (hdr.bits_per_sample / 8);
  hdr.block_align = hdr.channels * (hdr.bits_per_sample / 8);
  hdr.data_size = 0;
  return hdr;
}

static void finalize_wav_header(RecordLinuxPlugin* self) {
  if (!self->file_handle) return;
  self->wav_header.data_size = (uint32_t)self->total_data_bytes;
  self->wav_header.overall_size = (uint32_t)(self->total_data_bytes + sizeof(WavHeader) - 8);

  write_wav_header(self->file_handle, self->wav_header);
}

static bool connect_to_pulse(RecordLinuxPlugin* self, GError** gerror) {
  self->pa_spec.format = PA_SAMPLE_S16LE;
  self->pa_spec.rate = 44100;
  self->pa_spec.channels = 2;

  int error = 0;
  self->pa_handle = pa_simple_new(nullptr,
                                  "record_linux_plugin",
                                  PA_STREAM_RECORD,
                                  nullptr,
                                  "recording",
                                  &self->pa_spec,
                                  nullptr,
                                  nullptr,
                                  &error);
  if (!self->pa_handle) {
    if (gerror) {
      *gerror = g_error_new_literal(
          g_quark_from_static_string("pulse-error"),
          error,
          pa_strerror(error));
    }
    return false;
  }
  return true;
}

static void disconnect_from_pulse(RecordLinuxPlugin* self) {
  if (self->pa_handle) {
    pa_simple_free(self->pa_handle);
    self->pa_handle = nullptr;
  }
}

////////////////////////////////////////////////////////////////////////////////
// The background recording thread
////////////////////////////////////////////////////////////////////////////////

static gpointer record_thread_func(gpointer user_data) {
  RecordLinuxPlugin* self = (RecordLinuxPlugin*)user_data;
  int error = 0;

  while (true) {
    g_mutex_lock(&self->state_mutex);
    bool should_record = self->is_recording;
    bool paused = self->is_paused;
    bool stream = self->is_stream_mode;
    g_mutex_unlock(&self->state_mutex);

    if (!should_record) break;

    if (paused) {
      g_mutex_lock(&self->state_mutex);
      while (self->is_paused && self->is_recording) {
        g_cond_wait(&self->pause_cond, &self->state_mutex);
      }
      g_mutex_unlock(&self->state_mutex);
      continue;
    }

    // Read from PulseAudio
    ssize_t r = pa_simple_read(self->pa_handle, self->buffer,
                               RecordLinuxPlugin::K_BUFFER_SIZE, &error);
    if (r < 0) {
      g_warning("pa_simple_read() failed: %s", pa_strerror(error));
      break;
    }

    if (!stream) {
      // File-based
      if (self->file_handle) {
        fwrite(self->buffer, 1, RecordLinuxPlugin::K_BUFFER_SIZE, self->file_handle);
        self->total_data_bytes += RecordLinuxPlugin::K_BUFFER_SIZE;
      }
    } else {
      // Stream-based => send data back to Dart
      auto* chunk = new std::vector<uint8_t>(self->buffer,
                                             self->buffer + RecordLinuxPlugin::K_BUFFER_SIZE);

      auto send_chunk = [](gpointer data) -> gboolean {
        auto pair = static_cast<std::pair<RecordLinuxPlugin*, std::vector<uint8_t>*>*>(data);
        RecordLinuxPlugin* plugin = pair->first;
        std::vector<uint8_t>* bytesVec = pair->second;

        FlValue* typed_data = fl_value_new_uint8_list(bytesVec->data(), bytesVec->size());
        fl_method_channel_invoke_method(plugin->channel,
                                        "audioData",
                                        typed_data,
                                        nullptr,
                                        nullptr,
                                        nullptr);

        delete bytesVec;
        delete pair;
        return G_SOURCE_REMOVE;
      };

      auto* pair = new std::pair<RecordLinuxPlugin*, std::vector<uint8_t>*>(self, chunk);
      g_idle_add_full(G_PRIORITY_DEFAULT, send_chunk, pair, nullptr);
    }
  }

  return nullptr;
}

////////////////////////////////////////////////////////////////////////////////
// Implement the non-static plugin methods
////////////////////////////////////////////////////////////////////////////////

FlMethodResponse* create_recorder(RecordLinuxPlugin* self) {
  // Possibly no-op
  return FL_METHOD_RESPONSE(fl_method_success_response_new(nullptr));
}

FlMethodResponse* dispose_recorder(RecordLinuxPlugin* self) {
  // Stop recording if in progress
  g_mutex_lock(&self->state_mutex);
  self->is_recording = false;
  g_cond_broadcast(&self->pause_cond);
  g_mutex_unlock(&self->state_mutex);

  if (self->record_thread_handle) {
    g_thread_join(self->record_thread_handle);
    self->record_thread_handle = nullptr;
  }
  if (self->file_handle) {
    fclose(self->file_handle);
    self->file_handle = nullptr;
  }
  disconnect_from_pulse(self);

  return FL_METHOD_RESPONSE(fl_method_success_response_new(nullptr));
}

FlMethodResponse* start_recording_file(RecordLinuxPlugin* self, const gchar* path) {
  g_mutex_lock(&self->state_mutex);
  if (self->is_recording) {
    g_mutex_unlock(&self->state_mutex);
    return (FlMethodResponse*) fl_method_error_response_new(
        "already_recording", "A recording session is already in progress.", nullptr);
  }
  g_mutex_unlock(&self->state_mutex);

  GError* gerror = nullptr;
  if (!connect_to_pulse(self, &gerror)) {
    if (gerror) {
      auto resp = fl_method_error_response_new("pulse_error", gerror->message, nullptr);
      g_error_free(gerror);
      return (FlMethodResponse*) resp;
    }
    return (FlMethodResponse*) fl_method_error_response_new("pulse_error",
                                                           "Unknown PulseAudio error",
                                                           nullptr);
  }

  self->wav_header = init_wav_header();
  self->file_path = path ? path : "";
  self->file_handle = fopen(self->file_path.c_str(), "wb");
  if (!self->file_handle) {
    disconnect_from_pulse(self);
    return (FlMethodResponse*) fl_method_error_response_new(
        "file_io_error", "Failed to open the file for writing.", nullptr);
  }

  fwrite(&self->wav_header, sizeof(self->wav_header), 1, self->file_handle);
  fflush(self->file_handle);

  self->total_data_bytes = 0;
  self->is_stream_mode = false;

  g_mutex_lock(&self->state_mutex);
  self->is_recording = true;
  self->is_paused = false;
  self->record_thread_handle = g_thread_new("record_thread", record_thread_func, self);
  g_mutex_unlock(&self->state_mutex);

  return FL_METHOD_RESPONSE(fl_method_success_response_new(nullptr));
}

FlMethodResponse* stop_recording_file(RecordLinuxPlugin* self) {
  g_mutex_lock(&self->state_mutex);
  bool was_recording = self->is_recording;
  self->is_recording = false;
  g_cond_broadcast(&self->pause_cond);
  g_mutex_unlock(&self->state_mutex);

  if (was_recording && self->record_thread_handle) {
    g_thread_join(self->record_thread_handle);
    self->record_thread_handle = nullptr;
  }

  // finalize WAV
  finalize_wav_header(self);

  if (self->file_handle) {
    fclose(self->file_handle);
    self->file_handle = nullptr;
  }
  disconnect_from_pulse(self);

  FlValue* result = fl_value_new_string(self->file_path.c_str());
  return FL_METHOD_RESPONSE(fl_method_success_response_new(result));
}

FlMethodResponse* start_recording_stream(RecordLinuxPlugin* self) {
  g_mutex_lock(&self->state_mutex);
  if (self->is_recording) {
    g_mutex_unlock(&self->state_mutex);
    return (FlMethodResponse*) fl_method_error_response_new(
        "already_recording", "A recording session is already in progress.", nullptr);
  }
  g_mutex_unlock(&self->state_mutex);

  GError* gerror = nullptr;
  if (!connect_to_pulse(self, &gerror)) {
    if (gerror) {
      auto resp = fl_method_error_response_new("pulse_error", gerror->message, nullptr);
      g_error_free(gerror);
      return (FlMethodResponse*) resp;
    }
    return (FlMethodResponse*) fl_method_error_response_new("pulse_error",
                                                           "Unknown PulseAudio error",
                                                           nullptr);
  }

  self->file_path.clear();
  self->file_handle = nullptr;
  self->total_data_bytes = 0;
  self->is_stream_mode = true;

  g_mutex_lock(&self->state_mutex);
  self->is_recording = true;
  self->is_paused = false;
  self->record_thread_handle = g_thread_new("record_thread", record_thread_func, self);
  g_mutex_unlock(&self->state_mutex);

  return FL_METHOD_RESPONSE(fl_method_success_response_new(nullptr));
}

FlMethodResponse* stop_recording_stream(RecordLinuxPlugin* self) {
  g_mutex_lock(&self->state_mutex);
  bool was_recording = self->is_recording;
  self->is_recording = false;
  g_cond_broadcast(&self->pause_cond);
  g_mutex_unlock(&self->state_mutex);

  if (was_recording && self->record_thread_handle) {
    g_thread_join(self->record_thread_handle);
    self->record_thread_handle = nullptr;
  }

  disconnect_from_pulse(self);
  return FL_METHOD_RESPONSE(fl_method_success_response_new(nullptr));
}

FlMethodResponse* cancel_recording(RecordLinuxPlugin* self) {
  bool fileMode = !self->is_stream_mode;
  FlMethodResponse* stopResp;
  if (fileMode) {
    stopResp = stop_recording_file(self);
  } else {
    stopResp = stop_recording_stream(self);
  }

  if (fileMode && !self->file_path.empty()) {
    remove(self->file_path.c_str());
    self->file_path.clear();
  }

  return stopResp;
}

FlMethodResponse* pause_recording(RecordLinuxPlugin* self) {
  g_mutex_lock(&self->state_mutex);
  if (!self->is_recording) {
    g_mutex_unlock(&self->state_mutex);
    return (FlMethodResponse*) fl_method_error_response_new(
        "not_recording", "No active recording session to pause.", nullptr);
  }
  self->is_paused = true;
  g_mutex_unlock(&self->state_mutex);

  return FL_METHOD_RESPONSE(fl_method_success_response_new(nullptr));
}

FlMethodResponse* resume_recording(RecordLinuxPlugin* self) {
  g_mutex_lock(&self->state_mutex);
  if (!self->is_recording) {
    g_mutex_unlock(&self->state_mutex);
    return (FlMethodResponse*) fl_method_error_response_new(
        "not_recording", "No active recording session to resume.", nullptr);
  }
  self->is_paused = false;
  g_cond_broadcast(&self->pause_cond);
  g_mutex_unlock(&self->state_mutex);

  return FL_METHOD_RESPONSE(fl_method_success_response_new(nullptr));
}

FlMethodResponse* list_input_devices(RecordLinuxPlugin* self) {
  // Not implemented => return empty list
  FlValue* devices = fl_value_new_list();
  return FL_METHOD_RESPONSE(fl_method_success_response_new(devices));
}

FlMethodResponse* is_encoder_supported(RecordLinuxPlugin* self, const gchar* encoder) {
  bool supported = false;
  if (encoder && strcmp(encoder, "wav") == 0) {
    supported = true;
  }
  FlValue* result = fl_value_new_bool(supported);
  return FL_METHOD_RESPONSE(fl_method_success_response_new(result));
}

FlMethodResponse* get_amplitude(RecordLinuxPlugin* self) {
  // Stub => always -160 dB
  FlValue* amplitude = fl_value_new_map();
  fl_value_set_string_take(amplitude, "current", fl_value_new_float(-160.0));
  fl_value_set_string_take(amplitude, "max", fl_value_new_float(-160.0));
  return FL_METHOD_RESPONSE(fl_method_success_response_new(amplitude));
}

FlMethodResponse* has_permission(RecordLinuxPlugin* self) {
  FlValue* result = fl_value_new_bool(true);
  return FL_METHOD_RESPONSE(fl_method_success_response_new(result));
}

FlMethodResponse* is_paused_fn(RecordLinuxPlugin* self) {
  g_mutex_lock(&self->state_mutex);
  bool paused = self->is_paused;
  g_mutex_unlock(&self->state_mutex);

  FlValue* result = fl_value_new_bool(paused);
  return FL_METHOD_RESPONSE(fl_method_success_response_new(result));
}

FlMethodResponse* is_recording_fn(RecordLinuxPlugin* self) {
  g_mutex_lock(&self->state_mutex);
  bool rec = (self->is_recording && !self->is_paused);
  g_mutex_unlock(&self->state_mutex);

  FlValue* result = fl_value_new_bool(rec);
  return FL_METHOD_RESPONSE(fl_method_success_response_new(result));
}

////////////////////////////////////////////////////////////////////////////////
// The method-call dispatcher
////////////////////////////////////////////////////////////////////////////////

static void record_linux_plugin_handle_method_call(RecordLinuxPlugin* self,
                                                   FlMethodCall* method_call) {
  const gchar* method = fl_method_call_get_name(method_call);
  FlMethodResponse* response = nullptr;

  if (strcmp(method, "create") == 0) {
    response = create_recorder(self);
  } else if (strcmp(method, "dispose") == 0) {
    response = dispose_recorder(self);
  } else if (strcmp(method, "startRecordingFile") == 0) {
    // parse file path => call start_recording_file(self, path)
    FlValue* args = fl_method_call_get_args(method_call);
    if (args && fl_value_get_type(args) == FL_VALUE_TYPE_STRING) {
      const gchar* path = fl_value_get_string(args);
      response = start_recording_file(self, path);
    } else {
      response = FL_METHOD_RESPONSE(fl_method_error_response_new(
          "argument_error", "Expected path string", nullptr));
    }
  } else if (strcmp(method, "stopRecordingFile") == 0) {
    response = stop_recording_file(self);
  } else if (strcmp(method, "startRecording") == 0) {
    response = start_recording_stream(self);
  } else if (strcmp(method, "stopRecording") == 0) {
    if (self->is_stream_mode) {
      response = stop_recording_stream(self);
    } else {
      response = stop_recording_file(self);
    }
  } else if (strcmp(method, "cancelRecording") == 0) {
    response = cancel_recording(self);
  } else if (strcmp(method, "pauseRecording") == 0) {
    response = pause_recording(self);
  } else if (strcmp(method, "resumeRecording") == 0) {
    response = resume_recording(self);
  } else if (strcmp(method, "listInputDevices") == 0) {
    response = list_input_devices(self);
  } else if (strcmp(method, "isEncoderSupported") == 0) {
    FlValue* args = fl_method_call_get_args(method_call);
    const gchar* enc = (args && fl_value_get_type(args) == FL_VALUE_TYPE_STRING)
                       ? fl_value_get_string(args)
                       : nullptr;
    response = is_encoder_supported(self, enc);
  } else if (strcmp(method, "getAmplitude") == 0) {
    response = get_amplitude(self);
  } else if (strcmp(method, "hasPermission") == 0) {
    response = has_permission(self);
  } else if (strcmp(method, "isPaused") == 0) {
    response = is_paused_fn(self);
  } else if (strcmp(method, "isRecording") == 0) {
    response = is_recording_fn(self);
  } else {
    response = FL_METHOD_RESPONSE(fl_method_not_implemented_response_new());
  }

  fl_method_call_respond(method_call, response, nullptr);
}

////////////////////////////////////////////////////////////////////////////////
// The plugin registration
////////////////////////////////////////////////////////////////////////////////

extern "C" FLUTTER_PLUGIN_EXPORT void record_linux_plugin_register_with_registrar(
    FlPluginRegistrar* registrar) {
  // Create instance
  RecordLinuxPlugin* plugin = (RecordLinuxPlugin*)g_object_new(
      record_linux_plugin_get_type(), nullptr);

  // Create a method channel
  g_autoptr(FlStandardMethodCodec) codec = fl_standard_method_codec_new();
  g_autoptr(FlMethodChannel) channel =
      fl_method_channel_new(fl_plugin_registrar_get_messenger(registrar),
                            "record_linux",
                            FL_METHOD_CODEC(codec));

  plugin->channel = channel;

  // Set up method call handler
  fl_method_channel_set_method_call_handler(
      channel,
      [](FlMethodChannel* channel, FlMethodCall* method_call, gpointer user_data) {
        record_linux_plugin_handle_method_call(
            (RecordLinuxPlugin*)user_data, method_call);
      },
      plugin,
      nullptr);
}
