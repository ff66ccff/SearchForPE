#pragma once
#include "common.h"

// Semantic design tokens, mirrored 1:1 from design/mock.css so the native
// Direct2D painter and the approved mockup cannot drift apart.
struct Theme {
    bool dark = false;

    Color window;          // page background
    Color surface;         // panel background
    Color surfaceHover;    // row hover
    Color surfaceSunken;   // search field resting background
    Color surfaceRaised;   // floating button / selected chip

    Color border;
    Color borderStrong;
    Color divider;
    Color hairline;

    Color textPrimary;
    Color textSecondary;
    Color textMuted;
    Color textOnAccent;

    Color accent;
    Color accentSoft;

    Color success;
    Color successSoft;
    Color danger;
    Color dangerSoft;

    Color dotJudge;
    Color dotSingle;
    Color dotMulti;

    Color switchTrackOff;
    Color switchTrackOn;
    Color switchThumb;

    Color scrollThumb;
    Color scrollThumbHot;

    Color shadowSoft;
    Color controlHover;    // ghost button / chip hover wash

    Color chipText;
    Color chipBg;
    Color chipSelectedBg;
    Color windowBorder;

    // Native window chrome + edit control
    COLORREF captionColor;
    COLORREF captionText;
    COLORREF editBg;
    COLORREF editText;
};

inline Theme MakeLightTheme() {
    Theme t;
    t.dark = false;

    t.window        = Hex(0xF5F6F8);
    t.surface       = Hex(0xFFFFFF);
    t.surfaceHover  = Hex(0xF7F8FA);
    t.surfaceSunken = Hex(0xEFF1F4);
    t.surfaceRaised = Hex(0xFFFFFF);

    t.border        = Hex(0xE4E7EC);
    t.borderStrong  = Hex(0xDCE0E6);
    t.divider       = Hex(0xEEF0F3);
    t.hairline      = Hex(0xE4E7EC);

    t.textPrimary   = Hex(0x14161A);
    t.textSecondary = Hex(0x5B6270);
    t.textMuted     = Hex(0x98A0AD);
    t.textOnAccent  = Hex(0xFFFFFF);

    t.accent        = Hex(0x3D5BD9);
    t.accentSoft    = Hex(0xEDF0FC);

    t.success       = Hex(0x0C7A55);
    t.successSoft   = Hex(0xE9F7F1);
    t.danger        = Hex(0xB92B2B);
    t.dangerSoft    = Hex(0xFBEDED);

    t.dotJudge      = Hex(0x12A06C);
    t.dotSingle     = Hex(0x4E86E6);
    t.dotMulti      = Hex(0x8F76DD);

    t.switchTrackOff = Hex(0xDCE0E6);
    t.switchTrackOn  = Hex(0x3D5BD9);
    t.switchThumb    = Hex(0xFFFFFF);

    t.scrollThumb    = Hex(0xC9CDD4);
    t.scrollThumbHot = Hex(0xB2B8C2);

    t.shadowSoft     = Hex(0x101828, 0.10f);
    t.controlHover   = Hex(0xE9EBEF);

    t.chipText       = Hex(0x3D5BD9);
    t.chipBg         = Hex(0xEDF0FC);
    t.chipSelectedBg = Hex(0xFFFFFF);
    t.windowBorder   = Hex(0xD9DDE3);

    t.captionColor   = RGB(0xF7, 0xF8, 0xFA);
    t.captionText    = RGB(0x14, 0x16, 0x1A);
    t.editBg         = RGB(0xEF, 0xF1, 0xF4);
    t.editText       = RGB(0x14, 0x16, 0x1A);
    return t;
}

inline Theme MakeDarkTheme() {
    Theme t;
    t.dark = true;

    t.window        = Hex(0x111317);
    t.surface       = Hex(0x191C21);
    t.surfaceHover  = Hex(0x1E2228);
    t.surfaceSunken = Hex(0x1D2127);
    t.surfaceRaised = Hex(0x191C21);

    t.border        = Hex(0x282C33);
    t.borderStrong  = Hex(0x343941);
    t.divider       = Hex(0x24282F);
    t.hairline      = Hex(0x282C33);

    t.textPrimary   = Hex(0xE8EAEE);
    t.textSecondary = Hex(0xA0A7B3);
    t.textMuted     = Hex(0x6C7480);
    t.textOnAccent  = Hex(0x11151F);

    t.accent        = Hex(0x95A6FF);
    t.accentSoft    = Hex(0x242B45);

    t.success       = Hex(0x5FD8A4);
    t.successSoft   = Hex(0x153328);
    t.danger        = Hex(0xF08A8A);
    t.dangerSoft    = Hex(0x3A1D1E);

    t.dotJudge      = Hex(0x35C08C);
    t.dotSingle     = Hex(0x7EA6F0);
    t.dotMulti      = Hex(0xB394F0);

    t.switchTrackOff = Hex(0x343941);
    t.switchTrackOn  = Hex(0x95A6FF);
    t.switchThumb    = Hex(0xFFFFFF);

    t.scrollThumb    = Hex(0x3A3F48);
    t.scrollThumbHot = Hex(0x4C525C);

    t.shadowSoft     = Hex(0x000000, 0.45f);
    t.controlHover   = Hex(0x262B33);

    t.chipText       = Hex(0x95A6FF);
    t.chipBg         = Hex(0x242B45);
    t.chipSelectedBg = Hex(0x22262D);
    t.windowBorder   = Hex(0x2C3138);

    t.captionColor   = RGB(0x19, 0x1C, 0x21);
    t.captionText    = RGB(0xE8, 0xEA, 0xEE);
    t.editBg         = RGB(0x1D, 0x21, 0x27);
    t.editText       = RGB(0xE8, 0xEA, 0xEE);
    return t;
}