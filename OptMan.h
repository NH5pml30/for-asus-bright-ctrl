#pragma once

#include <afxwin.h>

#include "Logger.h"

class OptMan {
  typedef void (*CallbackForOptimizationTy)(int nFunc, int nData,
                                            LPCSTR lpstrData);
  typedef void (*CallbackForOptimizationExTy)(int nFunc, int nData,
                                              LPCWSTR lpstrData);

  int64_t (*MyOptRpcClientInitialize)(void **rpcClientPtr){};
  int64_t (*MyOptRpcClientClose)(void *rpcClient){};
  int64_t (*MyOptSetSplendidDimmingFunc)(int nValue, void *rpcClient){};
  int64_t (*MyOptGetSplendidColorModeFunc)(void *rpcClient){};

  void (*SetCallbackForReturnOptimizationResult)(CallbackForOptimizationTy fn,
                                                 void *rpcClient){};
  void (*SetCallbackForReturnOptimizationResultEx)(
      CallbackForOptimizationExTy fn, void *rpcClient){};

public:
  static void setRPCCLientModule(HMODULE hClientModule);
  static OptMan &get();
  static OptMan &reset();

  OptMan();
  ~OptMan();
  bool CheckValid();
  void SetSplendidDimming(int nMode);
  void GetOptimizationData();

  int nSplendidDcScale = -1;
  bool isRpcConnectOk = false;

private:
  void CallbackForOptimization(int nFunc, int nData, LPCSTR lpstrData);
  void CallbackForOptimizationEx(int nFunc, int nData, LPCWSTR lpstrData);
  void updateRpcConnectStatus(int64_t rpcStatus, std::string_view funcName);
  bool InitRPCConnection();

  std::unique_ptr<void, decltype(MyOptRpcClientClose)> rpcClient =
      std::unique_ptr<void, decltype(MyOptRpcClientClose)>(
          nullptr, [](void *) -> int64_t { return 0; });
  bool loadedFuncs = false;

  static inline HMODULE hClientModule{};
  static inline std::unique_ptr<OptMan> instance;
};
