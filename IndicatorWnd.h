#pragma once

#include <afxwin.h>

#include <algorithm>
#include <cassert>
#include <functional>

#include "resource.h"
#include "Logger.h"

#define WM_USER_RESET_BRIGHTNESS (WM_USER + 2)

class IndicatorWnd : public CWnd {
public:
  IndicatorWnd();
  ~IndicatorWnd();
  void setBrightnessCallback(std::function<void(int)> Callback);
  void setBrightness(float NewFac);
  void resetBrightness(float NewFac);

protected:
  DECLARE_MESSAGE_MAP()

  afx_msg INT OnCreate(LPCREATESTRUCT lpCreateStruct);
  afx_msg void OnTimer(UINT_PTR nIdEvent);
  afx_msg void OnHotKey(UINT nHotKeyId, UINT nKey1, UINT nKey2);
  afx_msg void OnPaint();

  afx_msg LRESULT OnResetBrightness(WPARAM wParam, LPARAM lParam);

private:
  LONG IconSize{};
  CDC BitmapDC, BarDC;
  CGdiObject BitmapDCObj, BarDCObj;
  float Fac = 1;
  std::function<void(int)> BrightnessCallback;

  enum class HotKeyType { BRIGHTNESS_DOWN, BRIGHTNESS_UP, BRIGHTNESS_SYNC };

  static inline constexpr UINT_PTR TIMER_ID = 1;
  static inline constexpr UINT VISIBLE_US = 2000;
  static inline constexpr float OPACITY = 0.7f;
  static inline constexpr float PROGRESS_BAR_HEIGHT_FAC = 1.0f / 3;
  static inline constexpr float PROGRESS_BAR_WIDTH_FAC = 5;
  static inline constexpr float PROGRESS_BAR_WIDTH_WITH_PADDING_FAC = 6;
  static inline constexpr float PROGRESS_BAR_PADDING =
      (PROGRESS_BAR_WIDTH_WITH_PADDING_FAC - PROGRESS_BAR_WIDTH_FAC) / 2;
  static inline constexpr COLORREF PROGRESS_BAR_COLOR = RGB(50, 50, 255);
  static inline constexpr COLORREF PROGRESS_BAR_BG_COLOR = RGB(100, 100, 100);
};
