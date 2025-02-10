// testing/flutter_linux/fl_method_response.h
#ifndef FL_METHOD_RESPONSE_H
#define FL_METHOD_RESPONSE_H

#include <glib-object.h>
#include "fl_value.h"

G_BEGIN_DECLS

struct _FlMethodResponse;
typedef struct _FlMethodResponse FlMethodResponse;

#define FL_TYPE_METHOD_RESPONSE (fl_method_response_get_type())
#define FL_METHOD_RESPONSE(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), FL_TYPE_METHOD_RESPONSE, FlMethodResponse))

GType fl_method_response_get_type(void) G_GNUC_CONST;

FlMethodResponse* fl_method_success_response_new(FlValue* result);
FlMethodResponse* fl_method_error_response_new(const gchar* code,
                                             const gchar* message,
                                             FlValue* details);
FlMethodResponse* fl_method_not_implemented_response_new(void);

G_END_DECLS

#endif // FL_METHOD_RESPONSE_H