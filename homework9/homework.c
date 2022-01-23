#include <stdbool.h>
#include "homework.h"

// function prototypes
static void gst_wave_src_set_property(GObject *object, guint prop_id,  const GValue *value, GParamSpec *pspec);
static void gst_wave_src_get_property(GObject *object, guint prop_id, GValue *value, GParamSpec *pspec);
static gboolean gst_wave_src_start(GstBaseSrc *basesrc);
static gboolean gst_wave_src_stop(GstBaseSrc *basesrc);
static gboolean gst_wave_src_is_seekable(GstBaseSrc *src);
static gboolean gst_wave_src_get_size(GstBaseSrc *src, guint64 *size);
// static GstFlowReturn gst_wave_src_fill(GstBaseSrc *src, guint64 offset, guint length, GstBuffer *buf);

// Type define
G_DEFINE_TYPE(GstWaveSrc, gst_wave_src, GST_TYPE_BASE_SRC)

// Pad define (src)
static GstStaticPadTemplate src_template = GST_STATIC_PAD_TEMPLATE("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS_ANY
);

static void gst_wave_src_class_init(GstWaveSrcClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

    gobject_class->set_property = gst_wave_src_set_property;
    gobject_class->get_property = gst_wave_src_get_property;

    // Add property
    g_object_class_install_property(
        gobject_class, PROP_FILENAME,
        g_param_spec_string(
            "filename", "filename", "File name", "",
            G_PARAM_READABLE | G_PARAM_WRITABLE | G_PARAM_STATIC_STRINGS
        )
    );

    // register pad and pad metadata
    GstElementClass *gst_element_class = GST_ELEMENT_CLASS(klass);
    gst_element_class_add_static_pad_template(gst_element_class, &src_template);
    gst_element_class_set_static_metadata(gst_element_class, "Wav files source", "Generic",
                                          "Open and decode wav files", "greenday");

    // pad functions
    GstBaseSrcClass *gst_base_src_class = GST_BASE_SRC_CLASS(klass);
    gst_base_src_class->start = GST_DEBUG_FUNCPTR (gst_wave_src_start);
    gst_base_src_class->stop = GST_DEBUG_FUNCPTR (gst_wave_src_stop);
    gst_base_src_class->is_seekable = GST_DEBUG_FUNCPTR (gst_wave_src_is_seekable);
    gst_base_src_class->get_size = GST_DEBUG_FUNCPTR (gst_wave_src_get_size);
//    gst_basesrc_class->fill = GST_DEBUG_FUNCPTR (gst_file_src_fill);
}

static void gst_wave_src_init(GstWaveSrc *self)
{
    self->filename = "";
}

static void gst_wave_src_set_property(GObject *object, guint prop_id,  const GValue *value, GParamSpec *pspec)
{
    if (PROP_FILENAME == prop_id)
    {
        GstWaveSrc *wave_src = GST_WAVE_SRC(object);
        wave_src->filename = g_strdup(g_value_get_string(value));
        return;
    }

    G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
}

static void gst_wave_src_get_property(GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
    if (PROP_FILENAME == prop_id)
    {
        GstWaveSrc *wave_src = GST_WAVE_SRC(object);
        g_value_set_string(value, wave_src->filename);
        return;
    }

    G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
}

static gboolean gst_wave_src_start(GstBaseSrc *basesrc)
{
    g_print("START\n");
    return true;
}

static gboolean gst_wave_src_stop(GstBaseSrc *basesrc)
{
    g_print("STOP\n");
    return true;
}

static gboolean gst_wave_src_is_seekable(GstBaseSrc *src)
{
    g_print("IS_SEEKABLE\n");
    return true;
}

static gboolean gst_wave_src_get_size(GstBaseSrc *src, guint64 *size)
{
    g_print("GET_SIZE\n");

    *size = 0;
    return true;
}

// ---------------------------------------------------------
#define VERSION "0.0.1"
#define PACKAGE_NAME "wavesrc"
#define PACKAGE "wavesrc"
#define GST_PACKAGE_ORIGIN "greenday"

static gboolean plugin_init (GstPlugin *plugin)
{
    return gst_element_register(plugin, "wavesrc", GST_RANK_NONE, gst_wave_src_get_type());
}

GST_PLUGIN_DEFINE(
    GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    wavesrc,
    "OTUS C learn course plugin",
    plugin_init,
    VERSION,
    "LGPL",
    PACKAGE_NAME,
    GST_PACKAGE_ORIGIN
)
