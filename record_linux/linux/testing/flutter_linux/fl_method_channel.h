// testing/flutter_linux/fl_method_channel.h
#ifndef FL_METHOD_CHANNEL_H
#define FL_METHOD_CHANNEL_H

#include <glib-object.h>
#include <gio/gio.h>
#include "fl_method_call.h"
#include "fl_method_codec.h"
#include "fl_binary_messenger.h"

G_BEGIN_DECLS

struct _FlMethodChannel;
typedef struct _FlMethodChannel FlMethodChannel;

#define FL_TYPE_METHOD_CHANNEL (fl_method_channel_get_type())
#define FL_METHOD_CHANNEL(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), FL_TYPE_METHOD_CHANNEL, FlMethodChannel))

GType fl_method_channel_get_type(void) G_GNUC_CONST;

FlMethodChannel* fl_method_channel_new(FlBinaryMessenger* messenger,
                                     const gchar* name,
                                     FlMethodCodec* codec);

void fl_method_channel_set_method_call_handler(FlMethodChannel* channel,
                                             FlMethodCallHandler handler,
                                             gpointer user_data,
                                             GDestroyNotify destroy_notify);

void fl_method_channel_invoke_method(FlMethodChannel* channel,
                                   const gchar* name,
                                   FlValue* args,
                                   GCancellable* cancellable,
                                   GAsyncReadyCallback callback,
                                   gpointer user_data);

G_END_DECLS

#endif // FL_METHOD_CHANNEL_H