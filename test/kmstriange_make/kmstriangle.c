#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>

#include <errno.h>
#include <sys/select.h>

#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "kmstriangle.h"
#include "esUtil.h"

#include <inttypes.h>
#include <unistd.h>

static const struct egl *main_egl;
static const struct gbm *main_gbm;
static const struct drm *main_drm;
static struct drm drm;
static struct gbm gbm;

static struct {
    struct egl egl;
    GLfloat aspect;
    GLuint program;
    GLint modelviewmatrix, modelviewprojectionmatrix, normalmatrix;
    GLuint vbo;
    GLuint positionsoffset, colorsoffset, normalsoffset;
} gl;

WEAK uint64_t
gbm_bo_get_modifier(struct gbm_bo *bo);

WEAK int
gbm_bo_get_plane_count(struct gbm_bo *bo);

WEAK uint32_t
gbm_bo_get_stride_for_plane(struct gbm_bo *bo, int plane);

WEAK uint32_t
gbm_bo_get_offset(struct gbm_bo *bo, int plane);

static void drm_fb_destroy_callback(struct gbm_bo *bo, void *data)
{
    int drm_fd = gbm_device_get_fd(gbm_bo_get_device(bo));
    struct drm_fb *fb = data;

    if (fb->fb_id)
        drmModeRmFB(drm_fd, fb->fb_id);

    free(fb);
}
struct drm_fb * drm_fb_get_from_bo(struct gbm_bo *bo)
{
    int drm_fd = gbm_device_get_fd(gbm_bo_get_device(bo));
    struct drm_fb *fb = gbm_bo_get_user_data(bo);
    uint32_t width, height, format,
         strides[4] = {0}, handles[4] = {0},
         offsets[4] = {0}, flags = 0;
    int ret = -1;

    if (fb)
        return fb;

    fb = calloc(1, sizeof *fb);
    fb->bo = bo;

    width = gbm_bo_get_width(bo);
    height = gbm_bo_get_height(bo);
    format = gbm_bo_get_format(bo);

    if (gbm_bo_get_modifier && gbm_bo_get_plane_count &&
        gbm_bo_get_stride_for_plane && gbm_bo_get_offset) {

        uint64_t modifiers[4] = {0};
        modifiers[0] = gbm_bo_get_modifier(bo);
        const int num_planes = gbm_bo_get_plane_count(bo);
        for (int i = 0; i < num_planes; i++) {
            strides[i] = gbm_bo_get_stride_for_plane(bo, i);
            handles[i] = gbm_bo_get_handle(bo).u32;
            offsets[i] = gbm_bo_get_offset(bo, i);
            modifiers[i] = modifiers[0];
        }

        if (modifiers[0]) {
            flags = DRM_MODE_FB_MODIFIERS;
            printf("Using modifier %" PRIx64 "\n", modifiers[0]);
        }

        ret = drmModeAddFB2WithModifiers(drm_fd, width, height,
                format, handles, strides, offsets,
                modifiers, &fb->fb_id, flags);
    }
    if (ret) {
        if (flags)
            fprintf(stderr, "Modifiers failed!\n");

        memcpy(handles, (uint32_t [4]){gbm_bo_get_handle(bo).u32,0,0,0}, 16);
        memcpy(strides, (uint32_t [4]){gbm_bo_get_stride(bo),0,0,0}, 16);
        memset(offsets, 0, 16);
        ret = drmModeAddFB2(drm_fd, width, height, format,
                handles, strides, offsets, &fb->fb_id, 0);
    }

    if (ret) {
        printf("failed to create fb: %s\n", strerror(errno));
        free(fb);
        return NULL;
    }

    gbm_bo_set_user_data(bo, fb, drm_fb_destroy_callback);

    return fb;
}

static uint32_t find_crtc_for_encoder(const drmModeRes *resources,
        const drmModeEncoder *encoder) {
    int i;

    for (i = 0; i < resources->count_crtcs; i++) {
        /* possible_crtcs is a bitmask as described here:
         * https://dvdhrm.wordpress.com/2012/09/13/linux-drm-mode-setting-api
         */
        const uint32_t crtc_mask = 1 << i;
        const uint32_t crtc_id = resources->crtcs[i];
        if (encoder->possible_crtcs & crtc_mask) {
            return crtc_id;
        }
    }

    /* no match found */
    return -1;
}
static uint32_t find_crtc_for_connector(const struct drm *drm, const drmModeRes *resources,
        const drmModeConnector *connector) {
    int i;

    for (i = 0; i < connector->count_encoders; i++) {
        const uint32_t encoder_id = connector->encoders[i];
        drmModeEncoder *encoder = drmModeGetEncoder(drm->fd, encoder_id);

        if (encoder) {
            const uint32_t crtc_id = find_crtc_for_encoder(resources, encoder);

            drmModeFreeEncoder(encoder);
            if (crtc_id != 0) {
                return crtc_id;
            }
        }
    }

    /* no match found */
    return -1;
}

static int get_resources(int fd, drmModeRes **resources)
{
    *resources = drmModeGetResources(fd);
    if (*resources == NULL)
        return -1;
    return 0;
}
#define MAX_DRM_DEVICES 64

static int find_drm_device(drmModeRes **resources)
{
    drmDevicePtr devices[MAX_DRM_DEVICES] = { NULL };
    int num_devices, fd = -1;

    num_devices = drmGetDevices2(0, devices, MAX_DRM_DEVICES);
    if (num_devices < 0) {
        printf("drmGetDevices2 failed: %s\n", strerror(-num_devices));
        return -1;
    }

    for (int i = 0; i < num_devices; i++) {
        drmDevicePtr device = devices[i];
        int ret;

        if (!(device->available_nodes & (1 << DRM_NODE_PRIMARY)))
            continue;
        /* OK, it's a primary device. If we can get the
         * drmModeResources, it means it's also a
         * KMS-capable device.
         */
        fd = open(device->nodes[DRM_NODE_PRIMARY], O_RDWR);
        if (fd < 0)
            continue;
        ret = get_resources(fd, resources);
        if (!ret)
            break;
        close(fd);
        fd = -1;
    }
    drmFreeDevices(devices, num_devices);

    if (fd < 0)
        printf("no drm device found!\n");
    return fd;
}
int init_drm(struct drm *drm, const char *device, const char *mode_str,
        unsigned int vrefresh, unsigned int count)
{
    drmModeRes *resources;
    drmModeConnector *connector = NULL;
    drmModeEncoder *encoder = NULL;
    int i, ret, area;

    if (device) {
        drm->fd = open(device, O_RDWR);
        ret = get_resources(drm->fd, &resources);
        if (ret < 0 && errno == EOPNOTSUPP)
            printf("%s does not look like a modeset device\n", device);
    } else {
        drm->fd = find_drm_device(&resources);
    }

    if (drm->fd < 0) {
        printf("could not open drm device\n");
        return -1;
    }

    if (!resources) {
        printf("drmModeGetResources failed: %s\n", strerror(errno));
        return -1;
    }

    /* find a connected connector: */
    for (i = 0; i < resources->count_connectors; i++) {
        connector = drmModeGetConnector(drm->fd, resources->connectors[i]);
        if (connector->connection == DRM_MODE_CONNECTED) {
            /* it's connected, let's use this! */
            break;
        }
        drmModeFreeConnector(connector);
        connector = NULL;
    }

    if (!connector) {
        /* we could be fancy and listen for hotplug events and wait for
         * a connector..
         */
        printf("no connected connector!\n");
        return -1;
    }
    /* find user requested mode: */
    if (mode_str && *mode_str) {
        for (i = 0; i < connector->count_modes; i++) {
            drmModeModeInfo *current_mode = &connector->modes[i];

            if (strcmp(current_mode->name, mode_str) == 0) {
                if (vrefresh == 0 || current_mode->vrefresh == vrefresh) {
                    drm->mode = current_mode;
                    break;
                }
            }
        }
        if (!drm->mode)
            printf("requested mode not found, using default mode!\n");
    }

    /* find preferred mode or the highest resolution mode: */
    if (!drm->mode) {
        for (i = 0, area = 0; i < connector->count_modes; i++) {
            drmModeModeInfo *current_mode = &connector->modes[i];

            if (current_mode->type & DRM_MODE_TYPE_PREFERRED) {
                drm->mode = current_mode;
                break;
            }

            int current_area = current_mode->hdisplay * current_mode->vdisplay;
            if (current_area > area) {
                drm->mode = current_mode;
                area = current_area;
            }
        }
    }

    if (!drm->mode) {
        printf("could not find mode!\n");
        return -1;
    }

    /* find encoder: */
    for (i = 0; i < resources->count_encoders; i++) {
        encoder = drmModeGetEncoder(drm->fd, resources->encoders[i]);
        if (encoder->encoder_id == connector->encoder_id)
            break;
        drmModeFreeEncoder(encoder);
        encoder = NULL;
    }
    if (encoder) {
        drm->crtc_id = encoder->crtc_id;
    } else {
        uint32_t crtc_id = find_crtc_for_connector(drm, resources, connector);
        if (crtc_id == 0) {
            printf("no crtc found!\n");
            return -1;
        }

        drm->crtc_id = crtc_id;
    }

    for (i = 0; i < resources->count_crtcs; i++) {
        if (resources->crtcs[i] == drm->crtc_id) {
            drm->crtc_index = i;
            break;
        }
    }

    drmModeFreeResources(resources);

    drm->connector_id = connector->connector_id;
    drm->count = count;

    return 0;
}
static const GLfloat vVertices[] = {
-1.0f, -1.0f, +1.0f,
+1.0f, -1.0f, +1.0f,
-1.0f, +1.0f, +1.0f, //右下角
+1.0f, +1.0f, +1.0f,
};


static const char *vertex_shader_source =
                "attribute vec4 in_position;        \n"
                "void main()                        \n"
                "{                                  \n"
                "    gl_Position = vec4(in_position.x, in_position.y, in_position.z, 1.0);\n"
                "}                                  \n";

static const char *fragment_shader_source =
                "void main()                        \n"
                "{                                  \n"
                "    gl_FragColor = vec4(1.0f, 0.5f, 0.2f, 1.0f);  \n"
                "}                                  \n";


static void draw_triangle(unsigned i)
{
	/* clear the color buffer */
        glClearColor(0.5, 0.5, 0.5, 1.0);
        glClear(GL_COLOR_BUFFER_BIT);

        glDrawArrays(GL_TRIANGLE_STRIP, 0, 3);
}
WEAK struct gbm_surface *
gbm_surface_create_with_modifiers(struct gbm_device *gbm,
                                  uint32_t width, uint32_t height,
                                  uint32_t format,
                                  const uint64_t *modifiers,
                                  const unsigned int count);

const struct gbm * init_gbm(int drm_fd, int w, int h, uint32_t format, uint64_t modifier)
{
    gbm.dev = gbm_create_device(drm_fd);
    gbm.format = format;
    gbm.surface = NULL;

    if (gbm_surface_create_with_modifiers) {
        gbm.surface = gbm_surface_create_with_modifiers(gbm.dev, w, h,
                                gbm.format,
                                &modifier, 1);

    }

    if (!gbm.surface) {
        if (modifier != DRM_FORMAT_MOD_LINEAR) {
            fprintf(stderr, "Modifiers requested but support isn't available\n");
            return NULL;
        }
        gbm.surface = gbm_surface_create(gbm.dev, w, h,
                        gbm.format,
                        GBM_BO_USE_SCANOUT | GBM_BO_USE_RENDERING);

    }

    if (!gbm.surface) {
        printf("failed to create gbm surface\n");
        return NULL;
    }

    gbm.width = w;
    gbm.height = h;

    return &gbm;
}
static bool has_ext(const char *extension_list, const char *ext)
{
    const char *ptr = extension_list;
    int len = strlen(ext);

    if (ptr == NULL || *ptr == '\0')
        return false;

    while (true) {
        ptr = strstr(ptr, ext);
        if (!ptr)
            return false;

        if (ptr[len] == ' ' || ptr[len] == '\0')
            return true;

        ptr += len;
    }
}

static int
match_config_to_visual(EGLDisplay egl_display,
               EGLint visual_id,
               EGLConfig *configs,
               int count)
{
    int i;

    for (i = 0; i < count; ++i) {
        EGLint id;

        if (!eglGetConfigAttrib(egl_display,
                configs[i], EGL_NATIVE_VISUAL_ID,
                &id))
            continue;

        if (id == visual_id)
            return i;
    }

    return -1;
}
static bool
egl_choose_config(EGLDisplay egl_display, const EGLint *attribs,
                  EGLint visual_id, EGLConfig *config_out)
{
    EGLint count = 0;
    EGLint matched = 0;
    EGLConfig *configs;
    int config_index = -1;

    if (!eglGetConfigs(egl_display, NULL, 0, &count) || count < 1) {
        printf("No EGL configs to choose from.\n");
        return false;
    }
    configs = malloc(count * sizeof *configs);
    if (!configs)
        return false;

    if (!eglChooseConfig(egl_display, attribs, configs,
                  count, &matched) || !matched) {
        printf("No EGL configs with appropriate attributes.\n");
        goto out;
    }

    if (!visual_id)
        config_index = 0;

    if (config_index == -1)
        config_index = match_config_to_visual(egl_display,
                              visual_id,
                              configs,
                              matched);

    if (config_index != -1)
        *config_out = configs[config_index];

out:
    free(configs);
    if (config_index == -1)
        return false;

    return true;
}
int init_egl(struct egl *egl, const struct gbm *gbm, int samples)
{
    EGLint major, minor;

    static const EGLint context_attribs[] = {
        EGL_CONTEXT_CLIENT_VERSION, 2,
        EGL_NONE
    };

    const EGLint config_attribs[] = {
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_RED_SIZE, 1,
        EGL_GREEN_SIZE, 1,
        EGL_BLUE_SIZE, 1,
        EGL_ALPHA_SIZE, 0,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
        EGL_SAMPLES, samples,
        EGL_NONE
    };
    const char *egl_exts_client, *egl_exts_dpy, *gl_exts;

#define get_proc_client(ext, name) do { \
        if (has_ext(egl_exts_client, #ext)) \
            egl->name = (void *)eglGetProcAddress(#name); \
    } while (0)
#define get_proc_dpy(ext, name) do { \
        if (has_ext(egl_exts_dpy, #ext)) \
            egl->name = (void *)eglGetProcAddress(#name); \
    } while (0)

#define get_proc_gl(ext, name) do { \
        if (has_ext(gl_exts, #ext)) \
            egl->name = (void *)eglGetProcAddress(#name); \
    } while (0)

    egl_exts_client = eglQueryString(EGL_NO_DISPLAY, EGL_EXTENSIONS);
    get_proc_client(EGL_EXT_platform_base, eglGetPlatformDisplayEXT);

    if (egl->eglGetPlatformDisplayEXT) {
        egl->display = egl->eglGetPlatformDisplayEXT(EGL_PLATFORM_GBM_KHR,
                gbm->dev, NULL);
    } else {
        egl->display = eglGetDisplay((void *)gbm->dev);
    }

    if (!eglInitialize(egl->display, &major, &minor)) {
        printf("failed to initialize\n");
        return -1;
    }

    egl_exts_dpy = eglQueryString(egl->display, EGL_EXTENSIONS);
    get_proc_dpy(EGL_KHR_image_base, eglCreateImageKHR);
    get_proc_dpy(EGL_KHR_image_base, eglDestroyImageKHR);
    get_proc_dpy(EGL_KHR_fence_sync, eglCreateSyncKHR);
    get_proc_dpy(EGL_KHR_fence_sync, eglDestroySyncKHR);
    get_proc_dpy(EGL_KHR_fence_sync, eglWaitSyncKHR);
    get_proc_dpy(EGL_KHR_fence_sync, eglClientWaitSyncKHR);
    get_proc_dpy(EGL_ANDROID_native_fence_sync, eglDupNativeFenceFDANDROID);
    egl->modifiers_supported = has_ext(egl_exts_dpy,
                       "EGL_EXT_image_dma_buf_import_modifiers");

    printf("Using display %p with EGL version %d.%d\n",
            egl->display, major, minor);

    printf("===================================\n");
    printf("EGL information:\n");
    printf("  version: \"%s\"\n", eglQueryString(egl->display, EGL_VERSION));
    printf("  vendor: \"%s\"\n", eglQueryString(egl->display, EGL_VENDOR));
    printf("  client extensions: \"%s\"\n", egl_exts_client);
    printf("  display extensions: \"%s\"\n", egl_exts_dpy);
    printf("===================================\n");

    if (!eglBindAPI(EGL_OPENGL_ES_API)) {
        printf("failed to bind api EGL_OPENGL_ES_API\n");
        return -1;
    }

    if (!egl_choose_config(egl->display, config_attribs, gbm->format,
                               &egl->config)) {
        printf("failed to choose config\n");
        return -1;
    }

    egl->context = eglCreateContext(egl->display, egl->config,
            EGL_NO_CONTEXT, context_attribs);
    if (egl->context == NULL) {
        printf("failed to create context\n");
        return -1;
    }

    egl->surface = eglCreateWindowSurface(egl->display, egl->config,
            (EGLNativeWindowType)gbm->surface, NULL);
    if (egl->surface == EGL_NO_SURFACE) {
        printf("failed to create egl surface\n");
        return -1;
    }

    /* connect the context to the surface */
    eglMakeCurrent(egl->display, egl->surface, egl->surface, egl->context);

    gl_exts = (char *) glGetString(GL_EXTENSIONS);
    printf("OpenGL ES 2.x information:\n");
    printf("  version: \"%s\"\n", glGetString(GL_VERSION));
    printf("  shading language version: \"%s\"\n", glGetString(GL_SHADING_LANGUAGE_VERSION));
    printf("  vendor: \"%s\"\n", glGetString(GL_VENDOR));
    printf("  renderer: \"%s\"\n", glGetString(GL_RENDERER));
    printf("  extensions: \"%s\"\n", gl_exts);
    printf("===================================\n");

    get_proc_gl(GL_OES_EGL_image, glEGLImageTargetTexture2DOES);
    /*
    get_proc_gl(GL_AMD_performance_monitor, glGetPerfMonitorGroupsAMD);
    get_proc_gl(GL_AMD_performance_monitor, glGetPerfMonitorCountersAMD);
    get_proc_gl(GL_AMD_performance_monitor, glGetPerfMonitorGroupStringAMD);
    get_proc_gl(GL_AMD_performance_monitor, glGetPerfMonitorCounterStringAMD);
    get_proc_gl(GL_AMD_performance_monitor, glGetPerfMonitorCounterInfoAMD);
    get_proc_gl(GL_AMD_performance_monitor, glGenPerfMonitorsAMD);
    get_proc_gl(GL_AMD_performance_monitor, glDeletePerfMonitorsAMD);
    get_proc_gl(GL_AMD_performance_monitor, glSelectPerfMonitorCountersAMD);
    get_proc_gl(GL_AMD_performance_monitor, glBeginPerfMonitorAMD);
    get_proc_gl(GL_AMD_performance_monitor, glEndPerfMonitorAMD);
    get_proc_gl(GL_AMD_performance_monitor, glGetPerfMonitorCounterDataAMD);
    */

    return 0;
}

int create_program(const char *vs_src, const char *fs_src)
{
    GLuint vertex_shader, fragment_shader, program;
    GLint ret;

    vertex_shader = glCreateShader(GL_VERTEX_SHADER);

    glShaderSource(vertex_shader, 1, &vs_src, NULL);
    glCompileShader(vertex_shader);

    glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &ret);
    if (!ret) {
        char *log;

        printf("vertex shader compilation failed!:\n");
        glGetShaderiv(vertex_shader, GL_INFO_LOG_LENGTH, &ret);
        if (ret > 1) {
            log = malloc(ret);
            glGetShaderInfoLog(vertex_shader, ret, NULL, log);
            printf("%s", log);
            free(log);
        }

        return -1;
    }
    fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);

    glShaderSource(fragment_shader, 1, &fs_src, NULL);
    glCompileShader(fragment_shader);

    glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &ret);
    if (!ret) {
        char *log;

        printf("fragment shader compilation failed!:\n");
        glGetShaderiv(fragment_shader, GL_INFO_LOG_LENGTH, &ret);

        if (ret > 1) {
            log = malloc(ret);
            glGetShaderInfoLog(fragment_shader, ret, NULL, log);
            printf("%s", log);
            free(log);
        }

        return -1;
    }

    program = glCreateProgram();

    glAttachShader(program, vertex_shader);
    glAttachShader(program, fragment_shader);

    return program;
}
int link_program(unsigned program)
{
    GLint ret;

    glLinkProgram(program);

    glGetProgramiv(program, GL_LINK_STATUS, &ret);
    if (!ret) {
        char *log;

        printf("program linking failed!:\n");
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &ret);

        if (ret > 1) {
            log = malloc(ret);
            glGetProgramInfoLog(program, ret, NULL, log);
            printf("%s", log);
            free(log);
        }

        return -1;
    }

    return 0;
}

int64_t get_time_ns(void)
{
    struct timespec tv;
    clock_gettime(CLOCK_MONOTONIC, &tv);
    return tv.tv_nsec + tv.tv_sec * NSEC_PER_SEC;
}


static void page_flip_handler(int fd, unsigned int frame,
          unsigned int sec, unsigned int usec, void *data)
{
    /* suppress 'unused parameter' warnings */
    (void)fd, (void)frame, (void)sec, (void)usec;

    int *waiting_for_flip = data;
    *waiting_for_flip = 0;
}
static int legacy_run(const struct gbm *gbm, const struct egl *egl)
{
    fd_set fds;
    drmEventContext evctx = {
            .version = 2,
            .page_flip_handler = page_flip_handler,
    };
    struct gbm_bo *bo;
    struct drm_fb *fb;
    uint32_t i = 0;
    int64_t start_time, report_time, cur_time;
    int ret;

    printf("liukui debug app eglSwapBuffers in\n");
    eglSwapBuffers(egl->display, egl->surface);
    printf("liukui debug app eglSwapBuffers end\n");
    bo = gbm_surface_lock_front_buffer(gbm->surface);
    fb = drm_fb_get_from_bo(bo);
    if (!fb) {
        fprintf(stderr, "Failed to get a new framebuffer BO\n");
        return -1;
    }

    /* set mode: */
    ret = drmModeSetCrtc(drm.fd, drm.crtc_id, fb->fb_id, 0, 0,
            &drm.connector_id, 1, drm.mode);
    if (ret) {
        printf("failed to set mode: %s\n", strerror(errno));
        return ret;
    }

    start_time = report_time = get_time_ns();

    while (i < drm.count) {
        struct gbm_bo *next_bo;
        int waiting_for_flip = 1;

        /* Start fps measuring on second frame, to remove the time spent
         * compiling shader, etc, from the fps:
         */
        if (i == 1) {
            start_time = report_time = get_time_ns();
        }

        egl->draw(i++);

	printf("liukui debug  while eglSwapBuffers pre\n");
        eglSwapBuffers(egl->display, egl->surface);
	printf("liukui debug while eglSwapBuffers end\n");
        next_bo = gbm_surface_lock_front_buffer(gbm->surface);
        fb = drm_fb_get_from_bo(next_bo);
        if (!fb) {
            fprintf(stderr, "Failed to get a new framebuffer BO\n");
            return -1;
        }
        /*
         * Here you could also update drm plane layers if you want
         * hw composition
         */

        ret = drmModePageFlip(drm.fd, drm.crtc_id, fb->fb_id,
                DRM_MODE_PAGE_FLIP_EVENT, &waiting_for_flip);
        if (ret) {
            printf("failed to queue page flip: %s\n", strerror(errno));
            return -1;
        }

        while (waiting_for_flip) {
            FD_ZERO(&fds);
            FD_SET(0, &fds);
            FD_SET(drm.fd, &fds);

            ret = select(drm.fd + 1, &fds, NULL, NULL, NULL);
            if (ret < 0) {
                printf("select err: %s\n", strerror(errno));
                return ret;
            } else if (ret == 0) {
                printf("select timeout!\n");
                return -1;
            } else if (FD_ISSET(0, &fds)) {
                printf("user interrupted!\n");
                return 0;
            }
            drmHandleEvent(drm.fd, &evctx);
        }

        cur_time = get_time_ns();
        if (cur_time > (report_time + 2 * NSEC_PER_SEC)) {
            double elapsed_time = cur_time - start_time;
            double secs = elapsed_time / (double)NSEC_PER_SEC;
            unsigned frames = i - 1;  /* first frame ignored */
            printf("Rendered %u frames in %f sec (%f fps)\n",
                frames, secs, (double)frames/secs);
            report_time = cur_time;
        }

        /* release last buffer to render on again: */
        gbm_surface_release_buffer(gbm->surface, bo);
        bo = next_bo;
    }

    //finish_perfcntrs();

    cur_time = get_time_ns();
    double elapsed_time = cur_time - start_time;
    double secs = elapsed_time / (double)NSEC_PER_SEC;
    unsigned frames = i - 1;  /* first frame ignored */
    printf("Rendered %u frames in %f sec (%f fps)\n",
        frames, secs, (double)frames/secs);

    //dump_perfcntrs(frames, elapsed_time);

    return 0;
}
const struct drm * init_drm_legacy(const char *device, const char *mode_str,
        unsigned int vrefresh, unsigned int count)
{
    int ret;

    ret = init_drm(&drm, device, mode_str, vrefresh, count);
    if (ret)
        return NULL;

    drm.run = legacy_run;

    return &drm;
}

int main()
{
    const char *device = NULL;
    char mode_str[DRM_DISPLAY_MODE_LEN] = "";
    uint32_t format = DRM_FORMAT_XRGB8888;
    uint64_t modifier = DRM_FORMAT_MOD_LINEAR;
    int samples = 0;
    int atomic = 0;
    int ret;

    unsigned int vrefresh = 0;
    unsigned int count = ~0;

    main_drm = init_drm_legacy(device, mode_str, vrefresh, count);
    if (!main_drm) {
        printf("failed to initialize %s DRM\n", atomic ? "atomic" : "legacy");
        return -1;
    }

    main_gbm = init_gbm(main_drm->fd, main_drm->mode->hdisplay, main_drm->mode->vdisplay,
            format, modifier);
    if (!main_gbm) {
        printf("failed to initialize GBM\n");
        return -1;
    }

    ret = init_egl(&gl.egl, main_gbm, samples);
    if (ret)
        return -1;

    gl.aspect = (GLfloat)(main_gbm->height) / (GLfloat)(main_gbm->width);
    printf("height[%d],width[%d]\n",main_gbm->height,main_gbm->width);
    ret = create_program(vertex_shader_source, fragment_shader_source);
    if (ret < 0)
	    return -1;
    gl.program = ret;

    ret = link_program(gl.program);
    if (ret)
	    return -1;

    glUseProgram(gl.program);
    glViewport(0, 0, 1280, 800);
    glEnable(GL_CULL_FACE);

    glGenBuffers(1, &gl.vbo);

    glBindBuffer(GL_ARRAY_BUFFER, gl.vbo);

    glBufferData(GL_ARRAY_BUFFER, sizeof(vVertices), vVertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), 0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);

    gl.egl.draw = draw_triangle;
    main_egl = &gl.egl;


    /* clear the color buffer */
    glClearColor(0.5, 0.5, 0.5, 1.0);
    glClear(GL_COLOR_BUFFER_BIT);

    return  main_drm->run(main_gbm, main_egl);
}
