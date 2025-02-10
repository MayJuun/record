// testing/flutter_linux/fl_value.h
#ifndef FL_VALUE_H
#define FL_VALUE_H

#include <glib-object.h>
#include <cstdint>
#include <gio/gio.h>

G_BEGIN_DECLS

// Value types
typedef enum {
  FL_VALUE_TYPE_NULL,
  FL_VALUE_TYPE_BOOL,
  FL_VALUE_TYPE_INT,
  FL_VALUE_TYPE_FLOAT,
  FL_VALUE_TYPE_STRING,
  FL_VALUE_TYPE_UINT8_LIST,
  FL_VALUE_TYPE_INT32_LIST,
  FL_VALUE_TYPE_INT64_LIST,
  FL_VALUE_TYPE_FLOAT_LIST,
  FL_VALUE_TYPE_LIST,
  FL_VALUE_TYPE_MAP,
} FlValueType;

// Forward declare types
typedef struct _FlValue FlValue;
typedef struct _FlMethodResponse FlMethodResponse;

// Value functions
FlValue* fl_value_new_null(void);
FlValue* fl_value_new_bool(gboolean value);
FlValue* fl_value_new_int(int64_t value);
FlValue* fl_value_new_float(double value);
FlValue* fl_value_new_string(const gchar* value);
FlValue* fl_value_new_uint8_list(const uint8_t* value, size_t length);
FlValue* fl_value_new_list(void);
FlValue* fl_value_new_map(void);

void fl_value_set_string_take(FlValue* value, const gchar* key, FlValue* item);
FlValueType fl_value_get_type(const FlValue* self);
const gchar* fl_value_get_string(const FlValue* self);
gboolean fl_value_get_bool(const FlValue* self);

G_END_DECLS

#endif // FL_VALUE_H