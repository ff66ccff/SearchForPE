#pragma once
#include "common.h"

struct ColorScheme {
    bool isDark;
    
    // Window & Backgrounds
    Gdiplus::Color bgWindow;
    Gdiplus::Color bgHeader;
    Gdiplus::Color bgControls;
    Gdiplus::Color bgCard;
    Gdiplus::Color bgCardHover;
    
    // Borders
    Gdiplus::Color borderCard;
    Gdiplus::Color borderCardHover;
    Gdiplus::Color borderInput;
    Gdiplus::Color borderInputFocus;
    
    // Typography
    Gdiplus::Color textPrimary;
    Gdiplus::Color textSecondary;
    Gdiplus::Color textMuted;
    
    // Badges
    Gdiplus::Color badgeJudgeBg;
    Gdiplus::Color badgeJudgeText;
    Gdiplus::Color badgeJudgeBorder;
    
    Gdiplus::Color badgeSingleBg;
    Gdiplus::Color badgeSingleText;
    Gdiplus::Color badgeSingleBorder;
    
    Gdiplus::Color badgeMultiBg;
    Gdiplus::Color badgeMultiText;
    Gdiplus::Color badgeMultiBorder;
    
    // Options & Answers
    Gdiplus::Color bgOptions;
    Gdiplus::Color borderOptions;
    Gdiplus::Color textOptions;
    
    Gdiplus::Color bgAnswer;
    Gdiplus::Color borderAnswer;
    Gdiplus::Color textAnswer;
    
    // Highlights & Actions
    Gdiplus::Color highlightBg;
    Gdiplus::Color highlightText;
    Gdiplus::Color btnActionBg;
    Gdiplus::Color btnActionHoverBg;
    Gdiplus::Color btnActionBorder;
    Gdiplus::Color btnActionText;
    
    // Accent
    Gdiplus::Color accentBlue;
    Gdiplus::Color accentBlueHover;
    
    // Win32 GDI Brushes/Colors
    COLORREF crBgWindow;
    COLORREF crBgControls;
    COLORREF crTextPrimary;
    COLORREF crSearchBg;
    COLORREF crSearchText;
};

inline ColorScheme GetLightTheme() {
    ColorScheme c;
    c.isDark = false;
    
    c.bgWindow = Gdiplus::Color(248, 250, 252);     // #f8fafc
    c.bgHeader = Gdiplus::Color(255, 255, 255);     // #ffffff
    c.bgControls = Gdiplus::Color(241, 245, 249);   // #f1f5f9
    c.bgCard = Gdiplus::Color(255, 255, 255);       // #ffffff
    c.bgCardHover = Gdiplus::Color(250, 250, 250);  // subtle hover
    
    c.borderCard = Gdiplus::Color(226, 232, 240);       // #e2e8f0
    c.borderCardHover = Gdiplus::Color(148, 163, 184);  // #94a3b8
    c.borderInput = Gdiplus::Color(203, 213, 225);      // #cbd5e1
    c.borderInputFocus = Gdiplus::Color(59, 130, 246);  // #3b82f6
    
    c.textPrimary = Gdiplus::Color(15, 23, 42);      // #0f172a
    c.textSecondary = Gdiplus::Color(71, 85, 105);   // #475569
    c.textMuted = Gdiplus::Color(148, 163, 184);     // #94a3b8
    
    // Judge (Green)
    c.badgeJudgeBg = Gdiplus::Color(220, 252, 231);     // #dcfce7
    c.badgeJudgeText = Gdiplus::Color(21, 128, 61);     // #15803d
    c.badgeJudgeBorder = Gdiplus::Color(134, 239, 172); // #86efac
    
    // Single (Blue)
    c.badgeSingleBg = Gdiplus::Color(219, 234, 254);     // #dbeafe
    c.badgeSingleText = Gdiplus::Color(29, 78, 216);     // #1d4ed8
    c.badgeSingleBorder = Gdiplus::Color(147, 197, 253); // #93c5fd
    
    // Multi (Purple)
    c.badgeMultiBg = Gdiplus::Color(243, 232, 255);      // #f3e8ff
    c.badgeMultiText = Gdiplus::Color(126, 34, 206);     // #7e22ce
    c.badgeMultiBorder = Gdiplus::Color(216, 180, 254);  // #d8b4fe
    
    // Options
    c.bgOptions = Gdiplus::Color(248, 250, 252);     // #f8fafc
    c.borderOptions = Gdiplus::Color(226, 232, 240); // #e2e8f0
    c.textOptions = Gdiplus::Color(30, 41, 59);      // #1e293b
    
    // Answer (Emerald)
    c.bgAnswer = Gdiplus::Color(236, 253, 245);      // #ecfdf5
    c.borderAnswer = Gdiplus::Color(167, 243, 208);  // #a7f3d0
    c.textAnswer = Gdiplus::Color(4, 120, 87);       // #047857
    
    // Highlight
    c.highlightBg = Gdiplus::Color(254, 240, 138);   // #fef08a
    c.highlightText = Gdiplus::Color(133, 77, 14);   // #854d0e
    
    // Action buttons
    c.btnActionBg = Gdiplus::Color(255, 255, 255);
    c.btnActionHoverBg = Gdiplus::Color(241, 245, 249);
    c.btnActionBorder = Gdiplus::Color(203, 213, 225);
    c.btnActionText = Gdiplus::Color(71, 85, 105);
    
    c.accentBlue = Gdiplus::Color(37, 99, 235);      // #2563eb
    c.accentBlueHover = Gdiplus::Color(29, 78, 216); // #1d4ed8
    
    c.crBgWindow = RGB(248, 250, 252);
    c.crBgControls = RGB(241, 245, 249);
    c.crTextPrimary = RGB(15, 23, 42);
    c.crSearchBg = RGB(255, 255, 255);
    c.crSearchText = RGB(15, 23, 42);
    
    return c;
}

inline ColorScheme GetDarkTheme() {
    ColorScheme c;
    c.isDark = true;
    
    c.bgWindow = Gdiplus::Color(15, 23, 42);         // #0f172a
    c.bgHeader = Gdiplus::Color(30, 41, 59);         // #1e293b
    c.bgControls = Gdiplus::Color(11, 17, 32);       // darker
    c.bgCard = Gdiplus::Color(30, 41, 59);           // #1e293b
    c.bgCardHover = Gdiplus::Color(38, 51, 74);      // slightly lighter
    
    c.borderCard = Gdiplus::Color(51, 65, 85);       // #334155
    c.borderCardHover = Gdiplus::Color(100, 116, 139);// #64748b
    c.borderInput = Gdiplus::Color(51, 65, 85);      // #334155
    c.borderInputFocus = Gdiplus::Color(96, 165, 250); // #60a5fa
    
    c.textPrimary = Gdiplus::Color(248, 250, 252);   // #f8fafc
    c.textSecondary = Gdiplus::Color(203, 213, 225); // #cbd5e1
    c.textMuted = Gdiplus::Color(100, 116, 139);     // #64748b
    
    // Judge (Green Dark)
    c.badgeJudgeBg = Gdiplus::Color(6, 78, 59);         // #064e3b
    c.badgeJudgeText = Gdiplus::Color(110, 231, 183);   // #6ee7b7
    c.badgeJudgeBorder = Gdiplus::Color(4, 120, 87);    // #047857
    
    // Single (Blue Dark)
    c.badgeSingleBg = Gdiplus::Color(30, 58, 138);       // #1e3a8a
    c.badgeSingleText = Gdiplus::Color(147, 197, 253);   // #93c5fd
    c.badgeSingleBorder = Gdiplus::Color(29, 78, 216);   // #1d4ed8
    
    // Multi (Purple Dark)
    c.badgeMultiBg = Gdiplus::Color(88, 28, 135);        // #581c87
    c.badgeMultiText = Gdiplus::Color(216, 180, 254);    // #d8b4fe
    c.badgeMultiBorder = Gdiplus::Color(126, 34, 206);   // #7e22ce
    
    // Options
    c.bgOptions = Gdiplus::Color(15, 23, 42);         // #0f172a
    c.borderOptions = Gdiplus::Color(51, 65, 85);     // #334155
    c.textOptions = Gdiplus::Color(226, 232, 240);    // #e2e8f0
    
    // Answer (Emerald Dark)
    c.bgAnswer = Gdiplus::Color(6, 78, 59);           // #064e3b
    c.borderAnswer = Gdiplus::Color(5, 150, 105);     // #059669
    c.textAnswer = Gdiplus::Color(52, 211, 153);      // #34d399
    
    // Highlight (Warm Amber Dark)
    c.highlightBg = Gdiplus::Color(113, 63, 18);      // #713f12
    c.highlightText = Gdiplus::Color(253, 224, 71);   // #fde047
    
    // Action buttons
    c.btnActionBg = Gdiplus::Color(30, 41, 59);
    c.btnActionHoverBg = Gdiplus::Color(51, 65, 85);
    c.btnActionBorder = Gdiplus::Color(71, 85, 105);
    c.btnActionText = Gdiplus::Color(203, 213, 225);
    
    c.accentBlue = Gdiplus::Color(59, 130, 246);      // #3b82f6
    c.accentBlueHover = Gdiplus::Color(96, 165, 250); // #60a5fa
    
    c.crBgWindow = RGB(15, 23, 42);
    c.crBgControls = RGB(11, 17, 32);
    c.crTextPrimary = RGB(248, 250, 252);
    c.crSearchBg = RGB(30, 41, 59);
    c.crSearchText = RGB(248, 250, 252);
    
    return c;
}
