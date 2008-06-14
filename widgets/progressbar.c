/*
 * progressbar.c - progressbar widget
 *
 * Copyright © 2007-2008 Julien Danjou <julien@danjou.info>
 * Copyright © 2007-2008 Marco Candrian <mac@calmar.ws>
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
#include "screen.h"

extern awesome_t globalconf;

typedef struct bar_t bar_t;

struct bar_t
{
    /** Title of the data/bar */
    char *title;
    /** These or lower values won't fill the bar at all*/
    float min_value;
    /** These or higher values fill the bar fully */
    float max_value;
    /** Pointer to value */
    float value;
    /** Reverse filling */
    bool reverse;
    /** Foreground color */
    xcolor_t fg;
    /** Foreground color of turned-off ticks */
    xcolor_t fg_off;
    /** Foreground color when bar is half-full */
    xcolor_t *pfg_center;
    /** Foreground color when bar is full */
    xcolor_t *pfg_end;
    /** Background color */
    xcolor_t bg;
    /** Border color */
    xcolor_t bordercolor;
    /** The next and previous bar in the list */
    bar_t *next, *prev;
};

static void
bar_delete(bar_t **bar)
{
    p_delete(&(*bar)->title);
    p_delete(&(*bar)->pfg_center);
    p_delete(&(*bar)->pfg_end);
    p_delete(bar);
}

DO_SLIST(bar_t, bar, bar_delete)

typedef struct
{
    /** Width of the data_items */
    int width;
    /** Pixel between data items (bars) */
    int gap;
    /** Border width in pixels */
    int border_width;
    /** Padding between border and ticks/bar */
    int border_padding;
    /** Gap/distance between the individual ticks */
    int ticks_gap;
    /** Total number of ticks */
    int ticks_count;
    /** 90 Degree's turned */
    bool vertical;
    /** Height 0-1, where 1.0 is height of bar */
    float height;
    /** The bars */
    bar_t *bars;
} progressbar_data_t;

static void
progressbar_pcolor_set(xcolor_t **ppcolor, char *new_color)
{
    bool flag = false;
    if(!*ppcolor)
    {
        flag = true; /* p_delete && restore to NULL, if xcolor_new unsuccessful */
        *ppcolor = p_new(xcolor_t, 1);
    }
    if(!(xcolor_new(globalconf.connection,
                    globalconf.default_screen,
                    new_color, *ppcolor))
       && flag)
        p_delete(ppcolor);
}

static bar_t *
progressbar_data_add(progressbar_data_t *d, const char *new_data_title)
{
    bar_t *bar = p_new(bar_t, 1);

    bar->title = a_strdup(new_data_title);

    bar->fg = globalconf.colors.fg;
    bar->fg_off = globalconf.colors.bg;
    bar->bg = globalconf.colors.bg;
    bar->bordercolor = globalconf.colors.fg;
    bar->max_value = 100.0;

    /* append the bar in the list */
    bar_list_append(&d->bars, bar);

    return bar;
}

static int
progressbar_draw(draw_context_t *ctx,
                 int screen __attribute__ ((unused)),
                 widget_node_t *w,
                 int offset,
                 int used __attribute__ ((unused)),
                 void *p __attribute__ ((unused)))
{
    /* pb_.. values points to the widget inside a potential border */
    int values_ticks, pb_x, pb_y, pb_height, pb_width, pb_progress, pb_offset;
    int unit = 0, nbbars = 0; /* tick + gap */
    area_t rectangle, pattern_rect;
    progressbar_data_t *d = w->widget->data;
    bar_t *bar;

    if(!d->bars)
        return 0;

    for(bar = d->bars; bar; bar = bar->next)
        nbbars++;

    if(d->vertical)
    {
        pb_width = (int) ((d->width - 2 * (d->border_width + d->border_padding) * nbbars
                   - d->gap * (nbbars - 1)) / nbbars);
        w->area.width = nbbars
                        * (pb_width + 2 * (d->border_width + d->border_padding)
                        + d->gap) - d->gap;
    }
    else
    {
        pb_width = d->width - 2 * (d->border_width + d->border_padding);
        if(d->ticks_count && d->ticks_gap)
        {
            unit = (pb_width + d->ticks_gap) / d->ticks_count;
            pb_width = unit * d->ticks_count - d->ticks_gap; /* rounded to match ticks... */
        }
        w->area.width = pb_width + 2 * (d->border_width + d->border_padding);
    }

    w->area.x = widget_calculate_offset(ctx->width,
                                        w->area.width,
                                        offset,
                                        w->widget->align);
    w->area.y = 0;

    /* for a 'reversed' progressbar:
     * basic progressbar:
     * 1. the full space gets the size of the formerly empty one
     * 2. the pattern must be mirrored
     * 3. the formerly 'empty' side gets drawed with fg colors, the 'full' with bg-color
     *
     * ticks:
     * 1. round the values to a full tick accordingly
     * 2. finally draw the gaps
     */

    pb_x = w->area.x + d->border_width + d->border_padding;
    pb_offset = 0;

    if(d->vertical)
    {
        /** \todo maybe prevent to calculate that stuff below over and over again
         * (->use static-values) */
        pb_height = (int) (ctx->height * d->height + 0.5)
                    - 2 * (d->border_width + d->border_padding);
        if(d->ticks_count && d->ticks_gap)
        {
            /* '+ d->ticks_gap' because a unit includes a ticks + ticks_gap */
            unit = (pb_height + d->ticks_gap) / d->ticks_count;
            pb_height = unit * d->ticks_count - d->ticks_gap;
        }

        pb_y = w->area.y + ((int) (ctx->height * (1 - d->height)) / 2)
               + d->border_width + d->border_padding;

        for(bar = d->bars; bar; bar = bar->next)
        {
            if(d->ticks_count && d->ticks_gap)
            {
                values_ticks = (int)(d->ticks_count * (bar->value - bar->min_value)
                               / (bar->max_value - bar->min_value) + 0.5);
                if(values_ticks)
                    pb_progress = values_ticks * unit - d->ticks_gap;
                else
                    pb_progress = 0;
            }
            else
                /* e.g.: min = 50; max = 56; 53 should show 50% graph
                 * (53(val) - 50(min) / (56(max) - 50(min) = 3 / 5 = 0.5 = 50%
                 * round that ( + 0.5 and (int)) and finally multiply with height
                 */
                pb_progress = (int) (pb_height * (bar->value - bar->min_value)
                                     / (bar->max_value - bar->min_value) + 0.5);

            if(d->border_width)
            {
                /* border rectangle */
                rectangle.x = pb_x + pb_offset - d->border_width - d->border_padding;
                rectangle.y = pb_y - d->border_width - d->border_padding;
                rectangle.width = pb_width + 2 * (d->border_padding + d->border_width);
                rectangle.height = pb_height + 2 * (d->border_padding + d->border_width);

                if(d->border_padding)
                    draw_rectangle(ctx, rectangle, 1.0, true, bar->bg);
                draw_rectangle(ctx, rectangle, d->border_width, false, bar->bordercolor);
            }

            pattern_rect.x = pb_x;
            pattern_rect.width =  0;
            pattern_rect.y = pb_y;

            /* new value/progress in px + pattern setup */
            if(bar->reverse)
            {
                /* invert: top with bottom part */
                pb_progress = pb_height - pb_progress;
                pattern_rect.height = pb_height;
            }
            else
            {
                /* bottom to top */
                pattern_rect.y += pb_height;
                pattern_rect.height = - pb_height;
            }

            /* bottom part */
            if(pb_progress > 0)
            {
                rectangle.x = pb_x + pb_offset;
                rectangle.y = pb_y + pb_height - pb_progress;
                rectangle.width = pb_width;
                rectangle.height = pb_progress;

                /* fg color */
                if(bar->reverse)
                    draw_rectangle(ctx, rectangle, 1.0, true, bar->fg_off);
                else
                    draw_rectangle_gradient(ctx, rectangle, 1.0, true, pattern_rect,
                                            &bar->fg, bar->pfg_center, bar->pfg_end);
            }

            /* top part */
            if(pb_height - pb_progress > 0) /* not filled area */
            {
                rectangle.x = pb_x + pb_offset;
                rectangle.y = pb_y;
                rectangle.width = pb_width;
                rectangle.height = pb_height - pb_progress;

                /* bg color */
                if(bar->reverse)
                    draw_rectangle_gradient(ctx, rectangle, 1.0, true, pattern_rect,
                                            &bar->fg, bar->pfg_center, bar->pfg_end);
                else
                    draw_rectangle(ctx, rectangle, 1.0, true, bar->fg_off);
            }
            /* draw gaps TODO: improve e.g all in one */
            if(d->ticks_count && d->ticks_gap)
            {
                rectangle.width = pb_width;
                rectangle.height = d->ticks_gap;
                rectangle.x = pb_x + pb_offset;
                for(rectangle.y = pb_y + (unit - d->ticks_gap);
                        pb_y + pb_height - d->ticks_gap >= rectangle.y;
                        rectangle.y += unit)
                    draw_rectangle(ctx, rectangle, 1.0, true, bar->bg);
            }
            pb_offset += pb_width + d->gap + 2 * (d->border_width + d->border_padding);
        }
    }
    else /* a horizontal progressbar */
    {
        pb_height = (int) ((ctx->height * d->height
                            - nbbars * 2 * (d->border_width + d->border_padding)
                            - (d->gap * (nbbars - 1))) / nbbars + 0.5);
        pb_y = w->area.y + ((int) (ctx->height * (1 - d->height)) / 2)
               + d->border_width + d->border_padding;

        for(bar = d->bars; bar; bar = bar->next)
        {
            if(d->ticks_count && d->ticks_gap)
            {
                /* +0.5 rounds up ticks -> turn on a tick when half of it is reached */
                values_ticks = (int)(d->ticks_count * (bar->value - bar->min_value)
                                     / (bar->max_value - bar->min_value) + 0.5);
                if(values_ticks)
                    pb_progress = values_ticks * unit - d->ticks_gap;
                else
                    pb_progress = 0;
            }
            else
                pb_progress = (int) (pb_width * (bar->value - bar->min_value)
                                     / (bar->max_value - bar->min_value) + 0.5);

            if(d->border_width)
            {
                /* border rectangle */
                rectangle.x = pb_x - d->border_width - d->border_padding;
                rectangle.y = pb_y + pb_offset - d->border_width - d->border_padding;
                rectangle.width = pb_width + 2 * (d->border_padding + d->border_width);
                rectangle.height = pb_height + 2 * (d->border_padding + d->border_width);

                if(d->border_padding)
                    draw_rectangle(ctx, rectangle, 1.0, true, bar->bg);
                draw_rectangle(ctx, rectangle, d->border_width, false, bar->bordercolor);
            }

            pattern_rect.y = pb_y;
            pattern_rect.height = 0;
            pattern_rect.x = pb_x;

            /* new value/progress in px + pattern setup */
            if(bar->reverse)
            {
                /* reverse: right to left */
                pb_progress = pb_width - pb_progress;
                pattern_rect.x += pb_width;
                pattern_rect.width = - pb_width;
            }
            else
                /* left to right */
                pattern_rect.width = pb_width;

            /* left part */
            if(pb_progress > 0)
            {
                rectangle.x = pb_x;
                rectangle.y = pb_y + pb_offset;
                rectangle.width = pb_progress;
                rectangle.height = pb_height;

                /* fg color */
                if(bar->reverse)
                    draw_rectangle(ctx, rectangle, 1.0, true, bar->fg_off);
                else
                    draw_rectangle_gradient(ctx, rectangle, 1.0, true, pattern_rect,
                                            &bar->fg, bar->pfg_center, bar->pfg_end);
            }

            /* right part */
            if(pb_width - pb_progress > 0)
            {
                rectangle.x = pb_x + pb_progress;
                rectangle.y = pb_y +  pb_offset;
                rectangle.width = pb_width - pb_progress;
                rectangle.height = pb_height;

                /* bg color */
                if(bar->reverse)
                    draw_rectangle_gradient(ctx, rectangle, 1.0, true, pattern_rect,
                                            &bar->fg, bar->pfg_center, bar->pfg_end);
                else
                    draw_rectangle(ctx, rectangle, 1.0, true, bar->fg_off);
            }
            /* draw gaps TODO: improve e.g all in one */
            if(d->ticks_count && d->ticks_gap)
            {
                rectangle.width = d->ticks_gap;
                rectangle.height = pb_height;
                rectangle.y = pb_y + pb_offset;
                for(rectangle.x = pb_x + (unit - d->ticks_gap);
                        pb_x + pb_width - d->ticks_gap >= rectangle.x;
                        rectangle.x += unit)
                    draw_rectangle(ctx, rectangle, 1.0, true, bar->bg);
            }

            pb_offset += pb_height + d->gap + 2 * (d->border_width + d->border_padding);
        }
    }

    w->area.height = ctx->height;
    return w->area.width;
}

static widget_tell_status_t
progressbar_tell(widget_t *widget, const char *property, const char *new_value)
{
    progressbar_data_t *d = widget->data;
    int value;
    char *title, *setting;
    char *new_val;
    bar_t *bar;

    if(!new_value)
        return WIDGET_ERROR_NOVALUE;
    /* following properties need a datasection */
    else if(!a_strcmp(property, "fg")
            || !a_strcmp(property, "data")
            || !a_strcmp(property, "fg_off")
            || !a_strcmp(property, "bg")
            || !a_strcmp(property, "bordercolor")
            || !a_strcmp(property, "fg_center")
            || !a_strcmp(property, "fg_end")
            || !a_strcmp(property, "min_value")
            || !a_strcmp(property, "max_value")
            || !a_strcmp(property, "reverse"))
    {
        /* check if this section is defined already */
        new_val = a_strdup(new_value);
        title = strtok(new_val, " ");
        if(!(setting = strtok(NULL, " ")))
        {
            p_delete(&new_val);
            return WIDGET_ERROR_NOVALUE;
        }
        for(bar = d->bars; bar; bar = bar->next)
            if(!a_strcmp(title, bar->title))
                break;

        /* no section found -> create one */
        if(!bar)
            bar = progressbar_data_add(d, title);

        /* change values accordingly... */
        if(!a_strcmp(property, "data"))
        {
            value = atof(setting);
            bar->value = (value < bar->min_value ? bar->min_value :
                          (value > bar->max_value ? bar->max_value : value));
        }
        else if(!a_strcmp(property, "fg"))
            xcolor_new(globalconf.connection, globalconf.default_screen, setting, &bar->fg);
        else if(!a_strcmp(property, "bg"))
            xcolor_new(globalconf.connection, globalconf.default_screen, setting, &bar->bg);
        else if(!a_strcmp(property, "fg_off"))
            xcolor_new(globalconf.connection, globalconf.default_screen, setting, &bar->fg_off);
        else if(!a_strcmp(property, "bordercolor"))
            xcolor_new(globalconf.connection, globalconf.default_screen, setting, &bar->bordercolor);
        else if(!a_strcmp(property, "fg_center"))
            progressbar_pcolor_set(&bar->pfg_center, setting);
        else if(!a_strcmp(property, "fg_end"))
            progressbar_pcolor_set(&bar->pfg_end, setting);
        else if(!a_strcmp(property, "min_value"))
        {
            bar->min_value = atof(setting);
            /* hack to prevent max_value beeing less than min_value
             * and also preventing a division by zero when both are equal */
            if(bar->max_value <= bar->min_value)
                bar->max_value = bar->max_value + 0.0001;
            /* force a actual value into the newly possible range */
            if(bar->value < bar->min_value)
                bar->value = bar->min_value;
        }
        else if(!a_strcmp(property, "max_value"))
        {
            bar->max_value = atof(setting);
            if(bar->min_value >= bar->max_value)
                bar->min_value = bar->max_value - 0.0001;
            if(bar->value > bar->max_value)
                bar->value = bar->max_value;
        }
        else if(!a_strcmp(property, "reverse"))
            bar->reverse = a_strtobool(setting);

        p_delete(&new_val);
        return WIDGET_NOERROR;
    }
    else if(!a_strcmp(property, "gap"))
        d->gap = atoi(new_value);
    else if(!a_strcmp(property, "ticks_count"))
        d->ticks_count = atoi(new_value);
    else if(!a_strcmp(property, "ticks_gap"))
        d->ticks_gap = atoi(new_value);
    else if(!a_strcmp(property, "border_padding"))
        d->border_padding = atoi(new_value);
    else if(!a_strcmp(property, "border_width"))
        d->border_width = atoi(new_value);
    else if(!a_strcmp(property, "width"))
        d->width = atoi(new_value);
    else if(!a_strcmp(property, "height"))
        d->height = atof(new_value);
    else if(!a_strcmp(property, "vertical"))
        d->vertical = a_strtobool(new_value);
    else
        return WIDGET_ERROR;

    return WIDGET_NOERROR;
}

static void
progressbar_destructor(widget_t *widget)
{
    progressbar_data_t *d = widget->data;

    bar_list_wipe(&d->bars);
    p_delete(&d);
}

widget_t *
progressbar_new(alignment_t align)
{
    widget_t *w;
    progressbar_data_t *d;

    w = p_new(widget_t, 1);
    widget_common_new(w);
    w->align = align;
    w->draw = progressbar_draw;
    w->tell = progressbar_tell;
    w->destructor = progressbar_destructor;
    d = w->data = p_new(progressbar_data_t, 1);

    d->height = 0.80;
    d->width = 80;

    d->ticks_gap = 1;
    d->border_width = 1;
    d->gap = 2;

    return w;
}
// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
