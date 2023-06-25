#include <afxwin.h>
#include <afxmt.h>

#include <cassert>
#include <chrono>
#include <fstream>
#include <functional>
#include <iostream>
#include <optional>
#include <string>
#include <variant>
#include <vector>

#include <Msi.h>
#include <appmodel.h>
#include <fcntl.h>
#include <io.h>

#include "IndicatorWnd.h"
#include "OptMan.h"

#define LOG_MODULE "CRpcThread"

#define WM_USER_BRIGHTNESS_CHANGED (WM_USER + 1)

class CRpcThread : public CWinThread {
public:
  CRpcThread(CWinThread *Parent) : Parent(Parent) {}

protected:
  BOOL InitInstance() override {
    resetBrightness(false);
    return TRUE;
  }

  DECLARE_MESSAGE_MAP()

  afx_msg void OnBrightnessChanged(WPARAM wParam, LPARAM lParam) {
    int Percent = (int)wParam;
    LOGI_V_LN("got task to set brightness to ", Percent, "%");
    if (Percent < 0) {
      LOGI_V_LN("requested brightness sync");
      resetBrightness(true);
      return;
    }
    if (!checkConnection())
      return;
    OptMan::get().SetSplendidDimming(int(40 + Percent / 100.0f * 60));
  }

private:
  bool checkConnection() {
    if (!OptMan::get().CheckValid()) {
      LOGE_V_LN("worker thread cannot operate without a valid RPC connection");
      OptMan::reset();
      return OptMan::get().CheckValid();
    }
    return true;
  }

  void resetBrightness(bool ShowIndicator) {
    if (!checkConnection())
      return;
    auto &Man = OptMan::get();
    Man.GetOptimizationData();
    LOGI_V_LN("Sending reset brightness to ", Man.nSplendidDcScale,
              " splendid units to the UI thread");
    PostMessage(*Parent->m_pMainWnd, WM_USER_RESET_BRIGHTNESS,
                (WPARAM) int((Man.nSplendidDcScale - 40) / 60.0f * 100),
                ShowIndicator);
  }

  CWinThread *Parent{};
};

BEGIN_MESSAGE_MAP(CRpcThread, CWinThread)
  ON_THREAD_MESSAGE(WM_USER_BRIGHTNESS_CHANGED, OnBrightnessChanged)
END_MESSAGE_MAP()

#undef LOG_MODULE
#define LOG_MODULE "CRpcApp"

class CRpcApp : public CWinApp {
protected:
  BOOL InitInstance() override {
    Logger::initLog(std::ofstream(LOG_FILE_NAME, std::ios_base::app));

    m.emplace(TRUE, MUTEX_NAME);
    if (GetLastError() != ERROR_SUCCESS) {
      LOGE_V_LN("Program is already running, exiting...");
      return FALSE;
    }

    UINT32 Count = 0;
    UINT32 BufLen = 0;
    LONG Res = GetPackagesByPackageFamily(ASUS_ASSISTANT_FAMILY_NAME, &Count,
                                          NULL, &BufLen, NULL);
    if (Res != ERROR_INSUFFICIENT_BUFFER) {
      LOGE_V_LN("GetPackagesByPackageFamily failed: ", Res);
      return FALSE;
    }
    if (Count != 1) {
      LOGE_V_LN("found too many packages (", Count, ") instead of 1");
      return FALSE;
    }
    if (BufLen == 0) {
      LOGE_V_LN("invalid bufLen value: ", BufLen);
      return FALSE;
    }

    CStringW PackageFullName('\0', BufLen - 1);
    {
      std::vector<PWSTR> NamePtrs(Count);
      Res = GetPackagesByPackageFamily(ASUS_ASSISTANT_FAMILY_NAME, &Count,
                                       NamePtrs.data(), &BufLen,
                                       PackageFullName.GetBuffer());
    }
    if (Res != 0) {
      LOGE_V_LN("getPackagesByPackageFamily failed: ", Res);
      return FALSE;
    }

    LOGI_V_LN("obtained package full name: ", CW2A(PackageFullName));

    BufLen = 0;
    Res = GetPackagePathByFullName(PackageFullName.GetString(), &BufLen, NULL);
    if (Res != ERROR_INSUFFICIENT_BUFFER) {
      LOGE_V_LN("getPackagePathByFullName failed: ", Res);
      return FALSE;
    }
    if (BufLen == 0) {
      LOGE_V_LN("invalid bufLen value: ", BufLen);
      return FALSE;
    }
    CStringW RpcClientSrc('\0', BufLen - 1);
    Res = GetPackagePathByFullName(PackageFullName.GetString(), &BufLen,
                                   RpcClientSrc.GetBuffer());
    if (Res != 0) {
      LOGE_V_LN("GetPackagePathByFullName failed: ", Res);
      return FALSE;
    }

    LOGI_V_LN("obtained package path: ", CW2A(RpcClientSrc));
    RpcClientSrc += RPC_CLIENT_DLL_REL_DIR;
    RpcClientSrc += RPC_CLIENT_DLL_NAME;

    CStringW RpcClientDst = TEXT(".\\");
    RpcClientDst += RPC_CLIENT_DLL_NAME;

    Res = CopyFile(RpcClientSrc, RpcClientDst, FALSE);
    if (Res == 0) {
      LOGE_V_LN("CopyFile(", CW2A(RpcClientSrc), ", ", CW2A(RpcClientDst),
                " failed: ", Res);
      return FALSE;
    }

    HMODULE hClientModule = LoadLibrary(RPC_CLIENT_DLL_NAME);
    if (hClientModule == NULL) {
      LOGE_V_LN("cannot load RPC Client library: ", GetLastError());
      return FALSE;
    }

    OptMan::setRPCCLientModule(hClientModule);
    if (!OptMan::get().CheckValid())
      LOGE_V_LN(
          "worker thread will not operate without a valid RPC connection");

    m_pMainWnd = &TheWindow.emplace();

    s.emplace(this).CreateThread();
    TheWindow->setBrightnessCallback([&](int Percent) {
      LOGI_V_LN("sending brightness changed to ", Percent,
                "% to the RPC thread");
      s->PostThreadMessage(WM_USER_BRIGHTNESS_CHANGED, (WPARAM)Percent, 0);
    });

    return TRUE;
  }

  BOOL ExitInstance() override {
    m_pMainWnd = NULL;
    return TRUE;
  }

  std::optional<CMutex> m;
  std::optional<IndicatorWnd> TheWindow;
  std::optional<CRpcThread> s;

  std::ofstream log;

  static inline constexpr auto LOG_FILE_NAME = "log.txt";
  static inline constexpr auto MUTEX_NAME =
      TEXT("MyFlickerFreeDimmingHotkeysApp");
  static inline constexpr auto ASUS_ASSISTANT_FAMILY_NAME =
      TEXT("B9ECED6F.ASUSPCAssistant_qmba6cd70vzyy");
  static inline constexpr auto RPC_CLIENT_DLL_REL_DIR =
      TEXT("\\ModuleDll\\HWSettings\\");
  static inline constexpr auto RPC_CLIENT_DLL_NAME =
      TEXT("AsusCustomizationRpcClient.dll");
};

CRpcApp app;
