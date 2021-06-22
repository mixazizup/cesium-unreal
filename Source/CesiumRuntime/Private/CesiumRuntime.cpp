// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#include "CesiumRuntime.h"
#include "Cesium3DTiles/registerAllTileContentTypes.h"
#include "CesiumUtility/Tracing.h"
#include "SpdlogUnrealLoggerSink.h"
#include <Modules/ModuleManager.h>
#include <spdlog/spdlog.h>

#define LOCTEXT_NAMESPACE "FCesiumRuntimeModule"

DEFINE_LOG_CATEGORY(LogCesium);

void FCesiumRuntimeModule::StartupModule() {
  Cesium3DTiles::registerAllTileContentTypes();

  std::shared_ptr<spdlog::logger> pLogger = spdlog::default_logger();
  pLogger->sinks() = {std::make_shared<SpdlogUnrealLoggerSink>()};

  FModuleManager::Get().LoadModuleChecked(TEXT("HTTP"));

  CESIUM_TRACE_INIT("tracer.json");
}

void FCesiumRuntimeModule::ShutdownModule() { CESIUM_TRACE_SHUTDOWN(); }

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FCesiumRuntimeModule, CesiumRuntime)