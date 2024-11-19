#include "OptMan.h"

#define LOG_MODULE "OptMan"

void OptMan::setRPCCLientModule(HMODULE hClientModule) {
  if (!hClientModule) {
    LOGW_V_LN("setting NULL RPC Client module handle");
  } else {
    LOGI_V_LN("setting RPC Client module handle");
    OptMan::hClientModule = hClientModule;
  }
}

OptMan &OptMan::get() {
  if (!instance) {
    instance = std::make_unique<OptMan>();
    int iters = 0;
    while (!instance->InitRPCConnection() && iters++ < 5) {
      LOGW_V_LN("could not init RPC connection, backing off...");
      Sleep(100);
    }
    if (instance->isRpcConnectOk) {
      LOGI_V_LN("RPC connect ok");
      return *instance;
    }
    LOGE_V_LN("could not init RPC connection.");
  }
  return *instance;
}

OptMan &OptMan::reset() {
  LOGI_V_LN("resetting OptMan instance");
  instance.reset();
  return get();
}

OptMan::OptMan() {
  if (!hClientModule) {
    LOGE_V_LN("cannot operate without the RPCClient module");
    return;
  }
  loadedFuncs = true;
#define IMPORT_FUNC(hClientModule, func)                                       \
  do {                                                                         \
    func = (decltype(func))GetProcAddress(hClientModule, #func);               \
    if (!func) {                                                               \
      loadedFuncs = false;                                                     \
      LOGE_V_LN("cannot load the ", #func,                                     \
                " function from the RPCClient module");                        \
      return;                                                                  \
    }                                                                          \
  } while (false)

  IMPORT_FUNC(hClientModule, MyOptRpcClientInitialize);
  IMPORT_FUNC(hClientModule, MyOptRpcClientClose);

  IMPORT_FUNC(hClientModule, MyOptSetSplendidDimmingFunc);
  IMPORT_FUNC(hClientModule, MyOptGetSplendidColorModeFunc);

  IMPORT_FUNC(hClientModule, SetCallbackForReturnOptimizationResult);
  IMPORT_FUNC(hClientModule, SetCallbackForReturnOptimizationResultEx);
#undef IMPORT_FUNC
}

OptMan::~OptMan() { LOGI_V_LN("closing RPC connection"); }

bool OptMan::CheckValid() {
  if (!(loadedFuncs && rpcClient && isRpcConnectOk)) {
    LOGE_V_LN("RPC connection is not valid.");
    return false;
  } else {
    return true;
  }
}

void OptMan::SetSplendidDimming(int nMode) {
  if (!CheckValid())
    return;
  LOGI_V_LN("MyOptSetSplendidDimmingFunc(", nMode, ")");
  updateRpcConnectStatus(
      MyOptSetSplendidDimmingFunc(nMode, "", rpcClient.get()),
      "MyOptSetSplendidDimmingFunc");
  if (isRpcConnectOk)
    nSplendidDcScale = nMode;
}

void OptMan::GetOptimizationData() {
  if (!CheckValid())
    return;
  LOGI_V_LN("MyOptGetSplendidColorModeFunc()");
  updateRpcConnectStatus(MyOptGetSplendidColorModeFunc(rpcClient.get()),
                         "MyOptGetSplendidColorModeFunc");
}

void OptMan::CallbackForOptimization(int nFunc, int nData, LPCSTR lpstrData) {
  LOGI_V_LN("nFunc", nFunc, ", nData: ", nData, ", lpstrData: ", lpstrData);
  CallbackForOptimizationEx(nFunc, nData, CA2W(lpstrData));
}

void OptMan::CallbackForOptimizationEx(int nFunc, int nData,
                                       LPCWSTR lpstrData) {
  LOGI_V_LN("nFunc ", nFunc, ", nData: ", nData,
            ", lpstrData: ", CW2A(lpstrData));

  switch (nFunc) {
  case 279:
    LOGI_V_LN("MyOptSetSplendidDimmingFunc returned ", nData);
    break;
  case 18: {
    std::wstring_view wstr(lpstrData);
    size_t comma = wstr.find(',');
    if (comma == std::wstring_view::npos) {
      LOGE_V_LN("could not parse MyOptGetSplendidColorModeFunc output");
      return;
    }
    wstr.remove_prefix(comma + 1);
    int result6{};
    auto ss = std::wstringstream(std::wstring(wstr));
    if (!(ss >> result6)) {
      LOGE_V_LN("could not parse MyOptGetSplendidColorModeFunc "
                "output");
      return;
    }
    if (result6 < 40 || result6 > 100) {
      LOGI_V_LN("invalid splendidDcScale ", result6, ", setting to -1");
      nSplendidDcScale = -1;
    } else {
      LOGI_V_LN("parsed splendidDcScale to be ", result6, " in splendid units");
      nSplendidDcScale = result6;
    }
  }
  }
}

void OptMan::updateRpcConnectStatus(int64_t rpcStatus,
                                    std::string_view funcName) {
  switch (rpcStatus) {
  case RPC_X_BAD_STUB_DATA:
    LOGW_V_LN("stub received bad data for ", funcName, ", but ok");
    return;
  case ERROR_SUCCESS:
    LOGI_V_LN("RPC connect status still ok for ", funcName);
    return;
  }
  LOGE_V_LN("RPC connect failed for ", funcName, ": ", rpcStatus);
  isRpcConnectOk = false;
}

bool OptMan::InitRPCConnection() {
  rpcClient.reset();
  if (!loadedFuncs) {
    LOGE_V_LN("cannot operate without loaded RPC client functions");
    return isRpcConnectOk = false;
  }
  void *rpcClientData{};
  auto ret = MyOptRpcClientInitialize(&rpcClientData);
  if (rpcClientData) {
    LOGI_V_LN("initialized RPC client");
    rpcClient = std::unique_ptr<void, decltype(MyOptRpcClientClose)>(
        rpcClientData, MyOptRpcClientClose);
    nSplendidDcScale = -1;
    SetCallbackForReturnOptimizationResult(
        [](int nFunc, int nData, LPCSTR lpstrData) {
          return instance->CallbackForOptimization(nFunc, nData, lpstrData);
        },
        rpcClient.get());
    SetCallbackForReturnOptimizationResultEx(
        [](int nFunc, int nData, LPCWSTR lpstrData) {
          return instance->CallbackForOptimizationEx(nFunc, nData, lpstrData);
        },
        rpcClient.get());
    return isRpcConnectOk = true;
  } else {
    LOGE_V_LN("could not initialize RPC client: ", ret);
    return isRpcConnectOk = false;
  }
}
