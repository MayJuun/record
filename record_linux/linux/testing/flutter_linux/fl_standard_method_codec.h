// testing/flutter_linux/fl_standard_method_codec.h
#ifndef FL_STANDARD_METHOD_CODEC_H
#define FL_STANDARD_METHOD_CODEC_H

#include <glib-object.h>
#include "fl_method_codec.h"

G_BEGIN_DECLS

struct _FlStandardMethodCodec;
typedef struct _FlStandardMethodCodec FlStandardMethodCodec;

#define FL_TYPE_STANDARD_METHOD_CODEC (fl_standard_method_codec_get_type())
#define FL_STANDARD_METHOD_CODEC(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), FL_TYPE_STANDARD_METHOD_CODEC, FlStandardMethodCodec))

GType fl_standard_method_codec_get_type(void) G_GNUC_CONST;

FlMethodCodec* fl_standard_method_codec_new(void);

G_END_DECLS

#endif // FL_STANDARD_METHOD_CODEC_H