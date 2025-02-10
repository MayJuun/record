// testing/flutter_linux/fl_method_codec.h
#ifndef FL_METHOD_CODEC_H
#define FL_METHOD_CODEC_H

#include <glib-object.h>
#include "fl_value.h"

G_BEGIN_DECLS

struct _FlMethodCodec;
typedef struct _FlMethodCodec FlMethodCodec;

#define FL_TYPE_METHOD_CODEC (fl_method_codec_get_type())
#define FL_METHOD_CODEC(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), FL_TYPE_METHOD_CODEC, FlMethodCodec))

GType fl_method_codec_get_type(void) G_GNUC_CONST;

G_END_DECLS

#endif // FL_METHOD_CODEC_H