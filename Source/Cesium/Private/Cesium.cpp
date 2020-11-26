// Copyright Epic Games, Inc. All Rights Reserved.

#include "Cesium.h"
#include "Cesium3DTiles/registerAllTileContentTypes.h"
#include "Cesium3DTiles/Logging.h"
#include "UnrealCesiumLogger.h"
#include <sstream>
#include <iostream>

#define LOCTEXT_NAMESPACE "FCesiumModule"

class LStream : public std::stringbuf {
protected:
	int sync() {
		UE_LOG(LogTemp, Warning, TEXT("%s"), *FString(str().c_str()));
		str("");
		return std::stringbuf::sync();
	}
};

static LStream LogStream;

void FCesiumModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
	Cesium3DTiles::Logging::initializeLogging();

	//Cesium3DTiles::ILogger* logger = new UnrealCesiumLogger();
	//Cesium3DTiles::registerLogger(logger);

	Cesium3DTiles::registerAllTileContentTypes();

	std::cout.rdbuf(&LogStream);
}

void FCesiumModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FCesiumModule, Cesium)