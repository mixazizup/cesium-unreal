// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#include "CesiumCreditSystem.h"
#include "Cesium3DTiles/CreditSystem.h"
#include "CesiumCreditSystemBPLoader.h"
#include "CesiumRuntime.h"
#include "Engine/World.h"
#include <string>
#include <vector>

/*static*/ UClass* ACesiumCreditSystem::CesiumCreditSystemBP = nullptr;

/*static*/ ACesiumCreditSystem*
ACesiumCreditSystem::GetDefaultForActor(AActor* Actor) {
  // Blueprint loading can only happen in a constructor, so we instantiate a
  // loader object that retrieves the blueprint class in its constructor. We can
  // destroy the loader immediately once it's done since it will have already
  // set CesiumCreditSystemBP.
  if (!CesiumCreditSystemBP) {
    UCesiumCreditSystemBPLoader* bpLoader =
        NewObject<UCesiumCreditSystemBPLoader>();
    CesiumCreditSystemBP = bpLoader->CesiumCreditSystemBP;
    bpLoader->ConditionalBeginDestroy();
  }

  ACesiumCreditSystem* pACreditSystem = FindObject<ACesiumCreditSystem>(
      Actor->GetLevel(),
      TEXT("CesiumCreditSystemDefault"));
  if (!pACreditSystem) {
    if (!CesiumCreditSystemBP) {
      UE_LOG(
          LogCesium,
          Warning,
          TEXT(
              "Blueprint not found, unable to retrieve default ACesiumCreditSystem"));
      return nullptr;
    }

    FActorSpawnParameters spawnParameters;
    spawnParameters.Name = TEXT("CesiumCreditSystemDefault");
    spawnParameters.OverrideLevel = Actor->GetLevel();
    pACreditSystem = Actor->GetWorld()->SpawnActor<ACesiumCreditSystem>(
        CesiumCreditSystemBP,
        spawnParameters);
  }

  return pACreditSystem;
}

ACesiumCreditSystem::ACesiumCreditSystem()
    : _pCreditSystem(std::make_shared<Cesium3DTiles::CreditSystem>()),
      _lastCreditsCount(0) {
  PrimaryActorTick.bCanEverTick = true;
}

bool ACesiumCreditSystem::ShouldTickIfViewportsOnly() const { return true; }

void ACesiumCreditSystem::Tick(float DeltaTime) {
  Super::Tick(DeltaTime);

  if (!_pCreditSystem) {
    return;
  }

  const std::vector<Cesium3DTiles::Credit>& creditsToShowThisFrame =
      _pCreditSystem->getCreditsToShowThisFrame();

  // if the credit list has changed, we want to reformat the credits
  CreditsUpdated =
      creditsToShowThisFrame.size() != _lastCreditsCount ||
      _pCreditSystem->getCreditsToNoLongerShowThisFrame().size() > 0;
  if (CreditsUpdated) {
    std::string creditString =
        "<head>\n<meta charset=\"utf-16\"/>\n</head>\n<body style=\"color:white\"><ul>";
    for (size_t i = 0; i < creditsToShowThisFrame.size(); ++i) {
      creditString +=
          "<li>" + _pCreditSystem->getHtml(creditsToShowThisFrame[i]) + "</li>";
    }
    creditString += "</ul></body>";
    Credits = UTF8_TO_TCHAR(creditString.c_str());

    _lastCreditsCount = creditsToShowThisFrame.size();
  }

  _pCreditSystem->startNextFrame();
}
