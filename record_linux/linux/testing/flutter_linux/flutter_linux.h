// testing/flutter_linux/flutter_linux.h
#ifndef FLUTTER_LINUX_FLUTTER_LINUX_H_
#define FLUTTER_LINUX_FLUTTER_LINUX_H_

#include <glib-object.h>

#include "fl_method_call.h"
#include "fl_method_codec.h"
#include "fl_method_channel.h"
#include "fl_method_response.h"
#include "fl_standard_method_codec.h"
#include "fl_types.h"
#include "fl_value.h"

G_BEGIN_DECLS

// Basic types we need
typedef struct _FlMethodCall FlMethodCall;
typedef struct _FlMethodResponse FlMethodResponse;
typedef struct _FlMethodChannel FlMethodChannel;
typedef struct _FlPluginRegistrar FlPluginRegistrar;
typedef struct _FlValue FlValue;
typedef struct _FlBinaryMessenger FlBinaryMessenger;
typedef struct _FlMethodCodec FlMethodCodec;

// Method Response functions
FlMethodResponse* fl_method_success_response_new(FlValue* result);
FlMethodResponse* fl_method_error_response_new(const gchar* code,
                                             const gchar* message,
                                             FlValue* details);
FlMethodResponse* fl_method_not_implemented_response_new(void);

// Method Channel functions
FlMethodChannel* fl_method_channel_new(FlBinaryMessenger* messenger,
                                     const gchar* name,
                                     FlMethodCodec* codec);

// Value functions
FlValue* fl_value_new_bool(gboolean value);
FlValue* fl_value_new_int(int64_t value);
FlValue* fl_value_new_float(double value);
FlValue* fl_value_new_string(const gchar* value);
FlValue* fl_value_new_map(void);
void fl_value_set_string_take(FlValue* value, const gchar* key, FlValue* item);

// Plugin registrar functions
FlBinaryMessenger* fl_plugin_registrar_get_messenger(FlPluginRegistrar* registrar);

// Method call functions
const gchar* fl_method_call_get_name(FlMethodCall* method_call);
FlValue* fl_method_call_get_args(FlMethodCall* method_call);
void fl_method_call_respond(FlMethodCall* method_call,
                          FlMethodResponse* response,
                          GError** error);

G_END_DECLS

#endif  // FLUTTER_LINUX_FLUTTER_LINUX_H_