/*! \brief Cairo library interface code
    \file program-fb.c
    \author Seungwoo Kang (ki6080@gmail.com)
    \version 0.1
    \date 2019-11-22
    
    \copyright Copyright (c) 2019. Seungwoo Kang. All rights reserved.
 */
#include "core/program.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <linux/fb.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <cairo.h>
#include <string.h>
#include <stdbool.h>
#include <signal.h>
#include "core/internal/program-types.h"

// -- Resource descriptors
// Image descriptors
typedef struct cairo_font_face_t rsrc_font_t;
typedef struct cairo_surface_t rsrc_image_t;

typedef struct
{
    cairo_surface_t *screen;
    void *backbuffer_memory[RENDERER_NUM_MAX_BUFFER];
    cairo_surface_t *backbuffer[RENDERER_NUM_MAX_BUFFER];

    float w, h;
    cairo_t *context;
} program_cairo_wrapper_t;

static cairo_surface_t *cairo_linuxfb_surface_create(const char *fb_name);

void *Internal_PInst_InitFB(UProgramInstance *s, char const *fb)
{
    program_cairo_wrapper_t *v = malloc(sizeof(program_cairo_wrapper_t));
    v->screen = cairo_linuxfb_surface_create(fb);

    size_t w = cairo_image_surface_get_width(v->screen);
    size_t h = cairo_image_surface_get_height(v->screen);
    size_t strd = cairo_image_surface_get_stride(v->screen);
    size_t fmt = cairo_image_surface_get_format(v->screen);
    v->w = w;
    v->h = h;

    lvlog(LOGLEVEL_INFO,
          "Image info: \n"
          "w, h= [%d, %d] \n[strd: %d], fmt: %d\n",
          w, h, strd, fmt);

    for (size_t i = 0; i < RENDERER_NUM_MAX_BUFFER; i++)
    {
        v->backbuffer_memory[i] = malloc(h * strd);
        v->backbuffer[i] = cairo_image_surface_create_for_data(v->backbuffer_memory[i], fmt, w, h, strd);
    }

    *PInst_AspectRatio(s) = (float)w / h;

    return v;
}

void Internal_PInst_DeinitFB(struct ProgramInstance *Inst, void *hFB)
{
    program_cairo_wrapper_t *v = hFB;
    // Erase screen
    void *d = cairo_image_surface_get_data(v->screen);
    uint32_t strd = cairo_image_surface_get_stride(v->screen);
    uint32_t y = cairo_image_surface_get_height(v->screen);
    memset(d, 0, strd * y);

    // Release memory
    for (size_t i = 0; i < RENDERER_NUM_MAX_BUFFER; i++)
    {
        cairo_surface_destroy(v->backbuffer[i]);
        free(v->backbuffer_memory[i]);
    }
    cairo_surface_destroy(v->screen);
    lvlog(LOGLEVEL_INFO, "Frame buffer has successfully deinitialized.\n");
}

void *Internal_PInst_LoadImgInternal(struct ProgramInstance *Inst, char const *Path)
{
    // Check if file exists.
    FILE *fp = fopen(Path, "r");
    if (fp == NULL)
        return NULL;

    fclose(fp);
    return cairo_image_surface_create_from_png(Path);
}

void *Internal_PInst_LoadFont(struct ProgramInstance *Inst, char const *Path, LOADRESOURCE_FLAG_T Flag)
{
    cairo_font_slant_t slant = (Flag & LOADRESOURCE_FLAG_FONT_ITALIC) ? CAIRO_FONT_SLANT_ITALIC : CAIRO_FONT_SLANT_NORMAL;
    cairo_font_weight_t weight = (Flag & LOADRESOURCE_FLAG_FONT_BOLD) ? CAIRO_FONT_WEIGHT_BOLD : CAIRO_FONT_WEIGHT_NORMAL;

    cairo_font_face_t *f = cairo_toy_font_face_create(Path, slant, weight);
    return f;
}

typedef struct _cairo_linuxfb_device
{
    int fb_fd;
    char *fb_data;
    long fb_screensize;
    struct fb_var_screeninfo fb_vinfo;
    struct fb_fix_screeninfo fb_finfo;
} cairo_linuxfb_device_t;

static void cairo_linuxfb_surface_destroy(void *device)
{
    cairo_linuxfb_device_t *dev = (cairo_linuxfb_device_t *)device;

    if (dev == NULL)
    {
        return;
    }
    munmap(dev->fb_data, dev->fb_screensize);
    close(dev->fb_fd);
    free(dev);
}

static cairo_surface_t *cairo_linuxfb_surface_create(const char *fb_name)
{
    cairo_linuxfb_device_t *device;
    cairo_surface_t *surface;

    if (fb_name == NULL)
    {
        fb_name = "/dev/fb0";
    }

    device = malloc(sizeof(*device));

    // Open the file for reading and writing
    device->fb_fd = open(fb_name, O_RDWR);
    if (device->fb_fd == -1)
    {
        perror("Error: cannot open framebuffer device");
        exit(1);
    }

    // Get variable screen information
    if (ioctl(device->fb_fd, FBIOGET_VSCREENINFO, &device->fb_vinfo) == -1)
    {
        perror("Error reading variable information");
        exit(3);
    }

    // Figure out the size of the screen in bytes
    device->fb_screensize = device->fb_vinfo.xres * device->fb_vinfo.yres * device->fb_vinfo.bits_per_pixel / 8;

    // Map the device to memory
    device->fb_data = (char *)mmap(0, device->fb_screensize,
                                   PROT_READ | PROT_WRITE, MAP_SHARED,
                                   device->fb_fd, 0);
    memset(device->fb_data, 0, device->fb_screensize);

    if ((int)device->fb_data == -1)
    {
        perror("Error: failed to map framebuffer device to memory");
        exit(4);
    }

    // Get fixed screen information
    if (ioctl(device->fb_fd, FBIOGET_FSCREENINFO, &device->fb_finfo) == -1)
    {
        perror("Error reading fixed information");
        exit(2);
    }

    surface = cairo_image_surface_create_for_data(device->fb_data,
                                                  CAIRO_FORMAT_ARGB32,
                                                  device->fb_vinfo.xres,
                                                  device->fb_vinfo.yres,
                                                  cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32,
                                                                                device->fb_vinfo.xres));

    logprintf("xres: %u, yres: %u, bpp: %d\n",
              device->fb_vinfo.xres,
              device->fb_vinfo.yres,
              device->fb_vinfo.bits_per_pixel);
    cairo_surface_set_user_data(surface, NULL, device,
                                &cairo_linuxfb_surface_destroy);

    return surface;
}

void Internal_PInst_Predraw(void *hFB, int ActiveBuffer)
{
    program_cairo_wrapper_t *fb = hFB;
    cairo_surface_t *surf_bck = fb->backbuffer[ActiveBuffer];

    // Clear back buffer
    uint32_t *d = cairo_image_surface_get_data(surf_bck);
    size_t strd = cairo_image_surface_get_stride(surf_bck);
    size_t h = cairo_image_surface_get_height(surf_bck);

    // Create context for buff
    fb->context = cairo_create(surf_bck);

    extern cairo_surface_t *gBackgroundSurface;
    if (gBackgroundSurface)
    {
        cairo_set_source_surface(fb->context, gBackgroundSurface, 0, 0);
        cairo_paint(fb->context);
    }
    else
    {
        memset(d, 0xff, strd * h);
    }
}

void Internal_PInst_Draw(void *hFB, struct RenderEventArg const *Arg, int ActiveBuffer)
{
    program_cairo_wrapper_t *fb = hFB;
    cairo_t *cr = fb->context;
    cairo_save(cr);

    // Translate location
    FTransform2 tr = Arg->Transform;
    tr.P.x *= fb->h;
    tr.P.y *= fb->h;

    switch (Arg->Type)
    {
    case ERET_IMAGE:
    {
        cairo_translate(cr, tr.P.x, tr.P.y);

        cairo_surface_t *rsrc = Arg->Data.Image.Image->data;
        int w = cairo_image_surface_get_width(rsrc);
        int h = cairo_image_surface_get_height(rsrc);

        // @todo. Scale

#if defined(PINST_RENDER_ALLOW_ROTATION)
        cairo_rotate(cr, tr.R);
#endif
        cairo_set_source_surface(cr, rsrc, -w / 2, -h / 2);
        cairo_paint(cr);
    }
    break;

    case ERET_TEXT:
    {
        // Select font
        struct RenderEventData_Text p = Arg->Data.Text;
        cairo_font_face_t *font = p.Font->data;
        cairo_set_font_face(cr, font);
        FColor c = p.rgba;
        cairo_set_source_rgba(cr, c.R, c.G, c.B, c.A);
        cairo_set_font_size(cr, (tr.S.x + tr.S.y) * .5f);

#if defined(PINST_RENDER_ALLOW_ROTATION)
        cairo_translate(...); // @todo.
        cairo_rotate(cr, tr.R);
#else
        cairo_text_extents_t ext;
        cairo_text_extents(cr, p.Str, &ext);
        const bool bHC = ((bool)p.Flags & PINST_TEXTFLAG_HALIGN_CENTER);
        const bool bHR = ((bool)p.Flags & PINST_TEXTFLAG_HALIGN_RIGHT);
        const bool bVC = ((bool)p.Flags & PINST_TEXTFLAG_VALIGN_CENTER);
        const bool bVR = ((bool)p.Flags & PINST_TEXTFLAG_VALIGN_DOWN);

        float xadd = (ext.width * 0.5 + ext.x_bearing) * ((int)bHR - (int)bHC);
        float yadd = (ext.height * 0.5 + ext.y_bearing) * ((int)bVR - (int)bVC);

        tr.P.x += xadd;
        tr.P.y += yadd;
        cairo_move_to(cr, tr.P.x, tr.P.y);
#endif

        cairo_show_text(cr, Arg->Data.Text.Str);
    }
    break;

    default:
        break;
    }

    cairo_restore(cr);
}

void Internal_PInst_Flush(void *hFB, int ActiveBuffer)
{
    program_cairo_wrapper_t *fb = hFB;

    // Finalize buffer context
    cairo_destroy(fb->context);

    // Copy value to frame buffer
    cairo_surface_t *surf_bck = fb->backbuffer[ActiveBuffer];
    //     cairo_t *frame = cairo_create(fb->screen);
    //     cairo_set_source_surface(frame, surf_bck, 0, 0);
    //     cairo_paint(frame);
    //     cairo_destroy(frame);

    uint32_t *dst = cairo_image_surface_get_data(fb->screen);
    uint32_t *src = cairo_image_surface_get_data(surf_bck);
    int x = cairo_image_surface_get_width(fb->screen);
    int h = cairo_image_surface_get_height(fb->screen);
    int sz = x * h;

    // For each pxls ...
    for (char *p, *s; sz--; ++dst, ++src)
    {
        p = dst;
        s = src;
        p[2] = s[0];
        p[1] = s[1];
        p[0] = s[2];
        p[3] = s[3];
    }
}

FVec2float PInst_ScreenToWorld(struct ProgramInstance *s, int x, int y)
{
    program_cairo_wrapper_t *fb = s->hFB;
    float aspect = s->AspectRatio;
    float xf = x / fb->h - 0.5 * aspect;
    float yf = y / fb->h - 0.5f;

    return (FVec2float){.x = xf, .y = yf};
}

FVec2int PInst_WorldToScreen(struct ProgramInstance *s, FVec2float v)
{
    program_cairo_wrapper_t *fb = s->hFB;
    float aspect = s->AspectRatio;
    FVec2int r;
    r.x = (v.x + aspect * 0.5f) * fb->h;
    r.y = (v.y + 0.5f) * fb->h;

    return r;
}