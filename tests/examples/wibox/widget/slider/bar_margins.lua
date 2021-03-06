--DOC_HIDE_ALL
local parent    = ...
local wibox     = require("wibox")
local beautiful = require("beautiful")

local function gen(val)
    return wibox.widget {
        bar_border_color    = beautiful.border_color,
        bar_border_width    = 2,
        bar_margins         = val,
        handle_color        = beautiful.bg_normal,
        handle_border_color = beautiful.border_color,
        handle_border_width = 2,
        widget              = wibox.widget.slider,
    }
end

local l = wibox.layout {
    gen(0), gen(3), gen(6), gen({
                                top    = 12,
                                bottom = 12,
                                left   = 12,
                                right  = 12,
                            }),
    forced_height = 30,
    forced_width  = 400,
    spacing       = 5,
    layout        = wibox.layout.flex.horizontal
}

parent:add(l)

--DOC_HIDE vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
