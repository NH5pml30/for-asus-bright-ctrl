#include "IndicatorWnd.h"

#define LOG_MODULE "IndicatorWnd"

IndicatorWnd::IndicatorWnd() {
  // Load the brightess icon bitmap
  CBitmap BrightIcon;
  if (BrightIcon.LoadBitmap(IDB_BRIGHTNESS_ICON)) {
    // Get icon size
    BITMAP bm{};
    BrightIcon.GetBitmap(&bm);
    IconSize = bm.bmWidth;
    // Create DC to draw the icon later
    BitmapDC.CreateCompatibleDC(NULL);
    BitmapDCObj.Attach(BitmapDC.SelectObject(BrightIcon.Detach()));
  } else {
    LOGE_V_LN("cannot load the brightness icon: ", GetLastError());
  }

  // Create DC which contains the progress bar mask
  BarDC.CreateCompatibleDC(NULL);
  BarDC.AssertValid();
  CBitmap BarBuf;
  BarBuf.CreateCompatibleBitmap(&BarDC, IconSize * 6, IconSize);
  BarBuf.AssertValid();
  BarDCObj.Attach(BarDC.SelectObject(BarBuf.Detach()));

  // Draw the progress bar mask
  {
    // Construct progress bar mask from params
    int height = int(PROGRESS_BAR_HEIGHT_FAC * IconSize);
    int width = int(PROGRESS_BAR_WIDTH_FAC * IconSize);
    int padding = int(PROGRESS_BAR_PADDING * IconSize);
    int half = IconSize / 2;

    CRgn Rgn;
    Rgn.CreateRoundRectRgn(padding, half - height / 2, padding + width,
                           half + height / 2, height, height);
    Rgn.AssertValid();
    BarDC.SetDCBrushColor(RGB(255, 255, 255));
    CBrush DCBrush;
    DCBrush.CreateStockObject(DC_BRUSH);
    DCBrush.AssertValid();
    BarDC.FillRgn(&Rgn, &DCBrush);
  }

  const wchar_t CLASS_NAME[] = L"`win` window class";

  CreateEx(WS_EX_TRANSPARENT | WS_EX_TOPMOST | WS_EX_TOOLWINDOW |
               WS_EX_NOACTIVATE | WS_EX_LAYERED,
           AfxRegisterWndClass(NULL), TEXT("Flicker-Free Dimming Indicator"),
           WS_POPUP, CFrameWnd::rectDefault, NULL, 0);
}

IndicatorWnd::~IndicatorWnd() {
  CBitmap BrightIcon;
  BrightIcon.Attach(BitmapDC.SelectObject(BitmapDCObj));
  CBitmap BarBuf;
  BarBuf.Attach(BarDC.SelectObject(BarDCObj));
}

void IndicatorWnd::setBrightnessCallback(std::function<void(int)> Callback) {
  BrightnessCallback = std::move(Callback);
}

void IndicatorWnd::resetBrightness(float NewFac) {
  LOGI_V_LN("reset brightness from ", Fac, " to ", NewFac);
  Fac = NewFac;
}

BEGIN_MESSAGE_MAP(IndicatorWnd, CWnd)
  ON_WM_CREATE()
  ON_WM_TIMER()
  ON_WM_PAINT()
  ON_WM_HOTKEY()
  ON_WM_ENDSESSION()
  ON_MESSAGE(WM_USER_RESET_BRIGHTNESS, OnResetBrightness)
END_MESSAGE_MAP()

INT IndicatorWnd::OnCreate(LPCREATESTRUCT lpCreateStruct) {
  if (CWnd::OnCreate(lpCreateStruct) == -1)
    return -1;

  // Set actual window size and hide for now
  int width_with_padding = int(PROGRESS_BAR_WIDTH_WITH_PADDING_FAC * IconSize);
  SetWindowPos(NULL, 0, 0, IconSize + width_with_padding, IconSize,
               SWP_NOZORDER | SWP_HIDEWINDOW);

  // Register hot keys for brightness control
  if (RegisterHotKey(m_hWnd, static_cast<int>(HotKeyType::BRIGHTNESS_DOWN),
                     MOD_CONTROL | MOD_WIN | MOD_SHIFT, VK_OEM_COMMA) == 0 ||
      RegisterHotKey(m_hWnd, static_cast<int>(HotKeyType::BRIGHTNESS_UP),
                     MOD_CONTROL | MOD_WIN | MOD_SHIFT, VK_OEM_PERIOD) == 0 ||
      RegisterHotKey(m_hWnd, static_cast<int>(HotKeyType::BRIGHTNESS_SYNC),
                     MOD_CONTROL | MOD_WIN | MOD_SHIFT, VK_OEM_2) == 0) {
    LOGE_V_LN("cannot register hotkeys: ", GetLastError());
  }

  // Make the window click-through
  SetLayeredWindowAttributes(0, BYTE(255 * OPACITY), LWA_ALPHA);
  return 0;
}

void IndicatorWnd::OnTimer(UINT_PTR nIdEvent) {
  assert(nIdEvent == TIMER_ID);
  // Hide window after some time
  KillTimer(TIMER_ID);
  ShowWindow(FALSE);

  CWnd::OnTimer(nIdEvent);
}

void IndicatorWnd::OnHotKey(UINT nHotKeyId, UINT nKey1, UINT nKey2) {
  // Change brightness with 10% increments
  LOGI_V_LN("detected a hotkey press");
  float NewFac = Fac;
  switch (static_cast<HotKeyType>(nHotKeyId)) {
  case HotKeyType::BRIGHTNESS_DOWN:
    NewFac = std::clamp(Fac - 0.1f, 0.f, 1.f);
    break;
  case HotKeyType::BRIGHTNESS_UP:
    NewFac = std::clamp(Fac + 0.1f, 0.f, 1.f);
    break;
  case HotKeyType::BRIGHTNESS_SYNC:
    NewFac = -1;
    break;
  default:
    return CWnd::OnHotKey(nHotKeyId, nKey1, nKey2);
  }

  setBrightness(NewFac);
  CWnd::OnHotKey(nHotKeyId, nKey1, nKey2);
}

void IndicatorWnd::setBrightness(float NewFac) {
  if (NewFac < 0) {
    LOGI_V_LN("invalidated brightness");
    BrightnessCallback(-1);
  } else {
    LOGI_V_LN("changed brightness: ", Fac, "->", NewFac);
    Fac = NewFac;
    BrightnessCallback(int(round(Fac * 100.0f)));

    // Show the indicator now for some time
    InvalidateRect(NULL, TRUE);
    SetWindowPos(&CWnd::wndTopMost, 0, 0, 0, 0,
                 SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
    SetTimer(TIMER_ID, VISIBLE_US, NULL);
  }
}

void IndicatorWnd::OnPaint() {
  CPaintDC PaintDC(this);
  CBrush DCBrush;
  DCBrush.CreateStockObject(DC_BRUSH);

  // Draw the background
  {
    RECT PaintRect{};
    GetWindowRect(&PaintRect);
    PaintDC.SetDCBrushColor(PROGRESS_BAR_BG_COLOR);
    PaintDC.FillRect(&PaintRect, &DCBrush);
  }

  // Draw the brightness icon
  PaintDC.BitBlt(0, 0, IconSize, IconSize, &BitmapDC, 0, 0, SRCCOPY);

  // Draw the progress bar
  {
    // First, draw a rectangle of brightness level
    LONG padding = LONG(PROGRESS_BAR_PADDING * IconSize);
    RECT rc = {.left = IconSize + padding,
               .top = 0,
               .right = IconSize + padding +
                        LONG(PROGRESS_BAR_WIDTH_FAC * Fac * IconSize),
               .bottom = IconSize};

    PaintDC.SetDCBrushColor(PROGRESS_BAR_COLOR);
    PaintDC.FillRect(&rc, &DCBrush);
  }
  // Mask with the rounded corners mask
  PaintDC.BitBlt(IconSize, 0,
                 LONG(PROGRESS_BAR_WIDTH_WITH_PADDING_FAC * IconSize), IconSize,
                 &BarDC, 0, 0, SRCAND);
}

LRESULT IndicatorWnd::OnResetBrightness(WPARAM wParam, LPARAM lParam) {
  bool ShowIndicator = lParam;
  float NewFac = (int)wParam / 100.0f;

  if (ShowIndicator) {
    LOGI_V_LN("showing indicator");
    setBrightness(NewFac);
  } else {
    LOGI_V_LN("not showing indicator");
    resetBrightness(NewFac);
  }
  return 0;
}
