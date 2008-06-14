/*
 * iconbox.c - icon widget
 *
 * Copyright © 2007-2008 Julien Danjou <julien@danjou.info>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 */

#include "widget.h"
#include "common/util.h"

typedef struct
{
    draw_image_t *image;
    bool resize;
} iconbox_data_t;

static int
iconbox_draw(draw_context_t *ctx, int screen __attribute__ ((unused)),
             widget_node_t *w,
             int offset,
             int used __attribute__ ((unused)),
             void *p __attribute__ ((unused)))
{
    iconbox_data_t *d = w->widget->data;
    draw_image_t *image = d->image;

    /* image not valid */
    if(!image)
        return (w->area.width = 0);

    if(d->resize)
        w->area.width = ((double) ctx->height / image->height) * image->width;
    else
        w->area.width = image->width;

    if(w->area.width > ctx->width - used)
        return (w->area.width = 0);

    w->area.height = ctx->height;

    w->area.x = widget_calculate_offset(ctx->width,
                                        w->area.width,
                                        offset,
                                        w->widget->align);

    w->area.y = 0;

    draw_image(ctx, w->area.x, w->area.y,
               d->resize ? ctx->height : 0, image);

    return w->area.width;
}

static widget_tell_status_t
iconbox_tell(widget_t *widget, const char *property, const char *new_value)
{
    iconbox_data_t *d = widget->data;

    if(!new_value)
        return WIDGET_ERROR_NOVALUE;

    if(!a_strcmp(property, "image"))
    {
        draw_image_delete(&d->image);
        d->image = draw_image_new(new_value);
    }
    else if(!a_strcmp(property, "resize"))
        d->resize = a_strtobool(new_value);
    else
       return WIDGET_ERROR;

    return WIDGET_NOERROR;
}

static void
iconbox_destructor(widget_t *widget)
{
    iconbox_data_t *d = widget->data;

    draw_image_delete(&d->image);
    p_delete(&d);
}

widget_t *
iconbox_new(alignment_t align)
{
    widget_t *w;
    iconbox_data_t *d;

    w = p_new(widget_t, 1);
    widget_common_new(w);
    w->align = align;
    w->draw = iconbox_draw;
    w->tell = iconbox_tell;
    w->destructor = iconbox_destructor;
    w->data = d = p_new(iconbox_data_t, 1);
    d->resize = true;

    return w;
}
// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
