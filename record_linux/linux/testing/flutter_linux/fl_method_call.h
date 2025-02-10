// testing/flutter_linux/fl_method_call.h
#ifndef FL_METHOD_CALL_H
#define FL_METHOD_CALL_H

#include <glib-object.h>
#include "fl_value.h"

G_BEGIN_DECLS

struct _FlMethodCall;
typedef struct _FlMethodCall FlMethodCall;

#define FL_TYPE_METHOD_CALL (fl_method_call_get_type())
#define FL_METHOD_CALL(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), FL_TYPE_METHOD_CALL, FlMethodCall))

GType fl_method_call_get_type(void) G_GNUC_CONST;

// Function declarations using correct types
const gchar* fl_method_call_get_name(FlMethodCall* method_call);
FlValue* fl_method_call_get_args(FlMethodCall* method_call);
void fl_method_call_respond(FlMethodCall* method_call,
                          FlMethodResponse* response,
                          GError** error);

// Method call handler type
typedef void (*FlMethodCallHandler)(FlMethodChannel* channel,
                                  FlMethodCall* method_call,
                                  gpointer user_data);

G_END_DECLS

#endif // FL_METHOD_CALL_H