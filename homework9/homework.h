#ifndef _HOMEWORK_H_
#define _HOMEWORK_H_

#include <gst/gst.h>
#include <gst/base/gstbasesrc.h>

G_BEGIN_DECLS

#define GST_TYPE_WAVE_SRC (gst_wave_src_get_type())
#define GST_WAVE_SRC(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), GST_TYPE_WAVE_SRC, GstWaveSrc))
#define GST_WAVE_SRC_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST((klass), GST_TYPE_WAVE_SRC, GstWaveSrcClass))
#define GST_IS_WAVE_SRC(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj), GST_TYPE_WAVE_SRC))
#define GST_IS_WAVE_SRC_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), GST_TYPE_WAVE_SRC))

typedef struct _GstWaveSrc GstWaveSrc;
typedef struct _GstWaveSrcClass GstWaveSrcClass;

struct _GstWaveSrc {
    GstBaseSrc parent;
    gchar *filename;

    // Private variables
    guint64 file_size;
    u_int8_t *content_start;
    int fd;

    // wave capabilities
    gchar *block_align;
    u_int32_t rate;
    u_int16_t num_channels;
    u_int8_t *data;
    u_int32_t data_size;
};

struct _GstWaveSrcClass {
    GstBaseSrcClass parent_class;
};

GType gst_wave_src_get_type(void);

enum {
    PROP_FILENAME = 1
};

G_END_DECLS
#endif

