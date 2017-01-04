#include "stdafx.h"
#include "helpers.h"

pELHost dll_host = NULL;
pELLayout dll_layout = NULL;
const ELGenericFunctionTable *dll_gen = NULL;
const ELThemeColors *dll_theme = NULL;
const ELLogInfo *dll_log = NULL;
pELUnitConverter dll_uc = NULL;
const ELHostInfo *dll_host_info = NULL;
pELDialogs dll_dialogs = NULL;
pELCANSniffer dll_cana = NULL;
pELCANSniffer dll_canb = NULL;
pELModeler dll_model = NULL;
pELTarget dll_target = NULL;
pEL3DData dll_3d = NULL;

// Custom ostream manipulator for displaying hex numbers as uppercase and zero-padded
hex_upper_manip hex_upper(int width) {
    hex_upper_manip h = { width };
    return h;
}
ostream &operator<<(ostream &out, const hex_upper_manip& h) { return out << std::hex << std::uppercase << std::setfill(_T('0')) << std::setw(h.w); }

float brightness(COLORREF bgr) {
    // "Perceived" brightness calculation: 0.299 * R + 0.587 * G + 0.114 * B
    // Note that the coefficients are scaled by 1, 256, and 65536 to avoid doing an extra right-shift on each color component
    return (bgr & 0x0000FF) * 1.17255e-3f + (bgr & 0x00FF00) * 8.99203e-6f + (bgr & 0xFF0000) * 6.82158e-9f;
}

COLORREF color_at(const ELChannelFormattingInfo &i, const ELColorGradient &cg, float f) {
    COLORREF clr = 0;
    return (dll_gen->ColorAt((const pELColorGradient)&cg, (const pELColorTriple)&dll_theme->gradient_colors[i.color_triple_index], f, &clr), clr);
}

bool flags(pELChannelInfo i, CHANNELFLAGS flag) { return (i->flags & flag) == flag; }

bool type(pELChannelInfo i, CHANNELTYPE type) { return (i->type & type) == i->type; }

bool logged(pELChannelInfo i) { return i->log.size > 0 && i->log.min_array != NULL && i->log.max_array != NULL; }

bool in_log(ELChannelInfo &i) { return (i.flags & CHANNELFLAGS_LOGGED) != 0 || logged(&i); }

bool live(const ELLogInfo *i) { return i->display_cursor == LOG_MAX_TIMESTAMP; }

void adjust_name(ELChannelInfo &info, INFOSELECT which, pELString &string, std::string &out, bool append) {
    if (dll_host->AdjustUnitInName(&info, which, &string) == DS_SUCCESS) {
        if (append) { out += string->str; }
        else { out = string->str; }
    }
    else {
        switch (which) {
        case INFOSELECT_PRIMARY:
            if (append) { out += info.primary.name.str; }
            else { out = info.primary.name.str; }
            return;

        case INFOSELECT_X_AXIS:
            if (append) { out += info.x_axis.name.str; }
            else { out = info.x_axis.name.str; }
            return;

        case INFOSELECT_Y_AXIS:
            if (append) { out += info.y_axis.name.str; }
            else { out = info.y_axis.name.str; }
            return;
        }
    }
}

void move(const ELChannelCommonInfo &src, ELColorGradient &dest) {
    dest.low_start = dll_uc->EvaluateConversion(src.base_unit, src.unit, src.info.color_gradient.low_start);
    dest.low_end = dll_uc->EvaluateConversion(src.base_unit, src.unit, src.info.color_gradient.low_end);
    dest.high_start = dll_uc->EvaluateConversion(src.base_unit, src.unit, src.info.color_gradient.high_start);
    dest.high_end = dll_uc->EvaluateConversion(src.base_unit, src.unit, src.info.color_gradient.high_end);
}

void move(const ELColorGradient &src, ELChannelCommonInfo &dest) {
    dest.info.color_gradient.low_start = dll_uc->EvaluateConversion(dest.unit, dest.base_unit, src.low_start);
    dest.info.color_gradient.low_end = dll_uc->EvaluateConversion(dest.unit, dest.base_unit, src.low_end);
    dest.info.color_gradient.high_start = dll_uc->EvaluateConversion(dest.unit, dest.base_unit, src.high_start);
    dest.info.color_gradient.high_end = dll_uc->EvaluateConversion(dest.unit, dest.base_unit, src.high_end);
}
