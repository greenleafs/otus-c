#include <stdio.h>
#include <stdbool.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/mman.h>

#include "homework.h"

// Wave Header
struct wave_header
{
    u_int8_t chunkId[4];                    // RIFF
    u_int32_t chunkSize;
    u_int8_t format[4];                     // WAVE

    u_int8_t subchunk1Id[4];                // fmt
    u_int32_t subchunk1Size;
    u_int16_t audioFormat;                  // Only value = 1 (for us)
    u_int16_t numChannels;
    u_int32_t sampleRate;
    u_int32_t byteRate;
    u_int16_t blockAlign;
    u_int16_t bitsPerSample;

    u_int8_t subchunk2Id[4];                // data
    u_int32_t subchunk2Size;
};

// function prototypes
static bool is_wave_file(const struct wave_header *wh);
static bool detect_wav_capabilities(GstWaveSrc *src, const struct wave_header *wh);
static void gst_wave_src_finalize(GObject *self);
static void gst_wave_src_set_property(GObject *self, guint prop_id,  const GValue *value, GParamSpec *pspec);
static void gst_wave_src_get_property(GObject *self, guint prop_id, GValue *value, GParamSpec *pspec);
static gboolean gst_wave_src_start(GstBaseSrc *self);
static gboolean gst_wave_src_stop(GstBaseSrc *self);
static gboolean gst_wave_src_is_seekable(__attribute__((unused)) GstBaseSrc *self);
static gboolean gst_wave_src_get_size(GstBaseSrc *self, guint64 *size);
static GstFlowReturn gst_wave_src_fill(GstBaseSrc *self, guint64 offset, guint length, GstBuffer *buf);
static gboolean gst_wave_src_negotiate(GstBaseSrc* self);

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

    // setup property functions
    gobject_class->set_property = gst_wave_src_set_property;
    gobject_class->get_property = gst_wave_src_get_property;
    gobject_class->finalize = gst_wave_src_finalize;

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

    // setup pad functions
    GstBaseSrcClass *gst_base_src_class = GST_BASE_SRC_CLASS(klass);
    gst_base_src_class->start = GST_DEBUG_FUNCPTR(gst_wave_src_start);
    gst_base_src_class->stop = GST_DEBUG_FUNCPTR(gst_wave_src_stop);
    gst_base_src_class->is_seekable = GST_DEBUG_FUNCPTR(gst_wave_src_is_seekable);
    gst_base_src_class->get_size = GST_DEBUG_FUNCPTR(gst_wave_src_get_size);
    gst_base_src_class->fill = GST_DEBUG_FUNCPTR(gst_wave_src_fill);
    gst_base_src_class->negotiate = GST_DEBUG_FUNCPTR(gst_wave_src_negotiate);
}

static void gst_wave_src_init(GstWaveSrc *self)
{
    self->filename = "";
    self->file_size = 0;
    self->fd = -1;
    self->content_start = NULL;

    gst_base_src_set_blocksize(GST_BASE_SRC(self), 8192);
}

static void gst_wave_src_finalize(GObject *self)
{
    GstWaveSrc *wave_src = GST_WAVE_SRC(self);
    if ('\0' != wave_src->filename[0])
        g_free(wave_src->filename);
    G_OBJECT_CLASS(gst_wave_src_parent_class)->finalize(self);
}

static void gst_wave_src_set_property(GObject *self, guint prop_id,  const GValue *value, GParamSpec *pspec)
{
    if (PROP_FILENAME == prop_id)
    {
        GstWaveSrc *wave_src = GST_WAVE_SRC(self);
        wave_src->filename = g_strdup(g_value_get_string(value));
        return;
    }

    G_OBJECT_WARN_INVALID_PROPERTY_ID(self, prop_id, pspec);
}

static void gst_wave_src_get_property(GObject *self, guint prop_id, GValue *value, GParamSpec *pspec)
{
    if (PROP_FILENAME == prop_id)
    {
        GstWaveSrc *wave_src = GST_WAVE_SRC(self);
        g_value_set_string(value, wave_src->filename);
        return;
    }

    G_OBJECT_WARN_INVALID_PROPERTY_ID(self, prop_id, pspec);
}

static gboolean gst_wave_src_start(GstBaseSrc *self)
{
    GstWaveSrc *wave_src = GST_WAVE_SRC(self);

    wave_src->fd = open(wave_src->filename,O_RDONLY);
    if (-1 == wave_src->fd)
    {
        GST_ELEMENT_ERROR(wave_src, RESOURCE, OPEN_READ, (NULL),
            ("%s: \"%s\"", wave_src->filename, strerror(errno)));
        return false;
    }

    struct stat st;
    int res = fstat(wave_src->fd, &st);
    if (-1 == res)
    {
        close(wave_src->fd);
        GST_ELEMENT_ERROR(wave_src, RESOURCE, OPEN_READ, (NULL),
            ("%s: \"%s\"", wave_src->filename, strerror(errno)));
        return false;
    }

    wave_src->file_size = st.st_size;

    wave_src->content_start = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, wave_src->fd, 0);
    if (MAP_FAILED == wave_src->content_start)
    {
        close(wave_src->fd);
        GST_ELEMENT_ERROR(wave_src, RESOURCE, OPEN_READ, (NULL),
            ("%s: \"%s\"", wave_src->filename, strerror(errno)));
        return false;
    }

    if(!is_wave_file((struct wave_header*)wave_src->content_start))
    {
        close(wave_src->fd);
        munmap(wave_src->content_start, wave_src->file_size);
        GST_ELEMENT_ERROR(wave_src, RESOURCE, OPEN_READ, (NULL),
            ("%s: \"%s\"", wave_src->filename, "it's not wav file or we don't understand its format!"));
        return false;

    }

    if(!detect_wav_capabilities(wave_src, (struct wave_header*)wave_src->content_start))
    {
        close(wave_src->fd);
        munmap(wave_src->content_start, wave_src->file_size);
        GST_ELEMENT_ERROR(wave_src, RESOURCE, OPEN_READ, (NULL),
            ("%s: \"%s\"", wave_src->filename, "Cant detect this wav file capabilities!"));
        return false;

    }

    return true;
}

static gboolean gst_wave_src_stop(GstBaseSrc *self)
{
    GstWaveSrc *wave_src = GST_WAVE_SRC(self);

    if (wave_src->fd >= 0)
    {
        close(wave_src->fd);
        munmap(wave_src->content_start, wave_src->file_size);
    }

    wave_src->file_size = 0;
    wave_src->fd = -1;
    wave_src->content_start = NULL;
    return true;
}

static gboolean gst_wave_src_is_seekable(__attribute__((unused)) GstBaseSrc *self)
{
    return true;
}

static gboolean gst_wave_src_get_size(GstBaseSrc *self, guint64 *size)
{
    GstWaveSrc *wave_src = GST_WAVE_SRC(self);
    *size = wave_src->data_size;
    return true;
}

static GstFlowReturn gst_wave_src_fill(GstBaseSrc *self, guint64 offset, guint length, GstBuffer *buf)
{
    GstWaveSrc *wave_src = GST_WAVE_SRC(self);

    GstMapInfo info;
    gst_buffer_map(buf, &info, GST_MAP_WRITE);
    memcpy(info.data, wave_src->data + offset, length);
    gst_buffer_unmap(buf, &info);

    return GST_FLOW_OK;
}

static gboolean gst_wave_src_negotiate(GstBaseSrc* self)
{
    GstWaveSrc *wave_src = GST_WAVE_SRC(self);
    GstPad *src_pad;

    src_pad = gst_element_get_static_pad(GST_ELEMENT(self), "src");
    gst_pad_set_caps(
        src_pad,
        gst_caps_new_simple(
            "audio/x-raw",
            "rate", G_TYPE_INT, wave_src->rate,
            "channels", G_TYPE_INT, wave_src->num_channels,
            "format", G_TYPE_STRING, wave_src->block_align,
            "layout", G_TYPE_STRING, "interleaved",
            NULL
        )
    );
    gst_object_unref(src_pad);
    return true;
}

// Wave functions
static bool is_wave_file(const struct wave_header *wh)
{
    if (memcmp(wh->chunkId, "RIFF", 4) != 0)
        return false;

    if (memcmp(wh->format, "WAVE", 4) != 0)
        return false;

    if (memcmp(wh->subchunk2Id, "data", 4) != 0)
        return false;

    if (wh->subchunk1Size != 16)  // Only PCM we understand
        return false;

    if (wh->audioFormat != 1)  // Only uncompressed PCM format file we understand
        return false;

    return true;
}

// Very simplest detecting
static bool detect_wav_capabilities(GstWaveSrc *src, const struct wave_header *wh)
{
    src->num_channels = wh->numChannels;
    src->rate = wh->sampleRate;
    src->data = (u_int8_t*)wh + sizeof(struct wave_header);
    src->data_size = wh->subchunk2Size;
    switch (wh->bitsPerSample)
    {
        case 32: src->block_align = "S32LE";
                 break;
        case 24: src->block_align = "S24LE";
                 break;
        case 20: src->block_align = "S20LE";
                 break;
        case 18: src->block_align = "S18LE";
                 break;
        case 16: src->block_align = "S16LE";
                 break;
        case  8: src->block_align = "S8";
                 break;
        default: src->block_align = NULL;
    }

    if (!src->block_align)
        return false;

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

// RUN
// gst-launch-1.0 --gst-plugin-path=. wavesrc filename=test.wav  ! autoaudiosink
//