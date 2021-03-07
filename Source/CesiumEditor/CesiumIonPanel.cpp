// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#include "CesiumIonPanel.h"
#include "IonLoginPanel.h"
#include "CesiumEditor.h"
#include "CesiumCommands.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Styling/SlateStyleRegistry.h"
#include "Widgets/Layout/SHeader.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Views/SListView.h"
#include "Widgets/Input/SButton.h"
#include "Editor.h"
#include "EditorModeManager.h"
#include "EngineUtils.h"
#include "ACesium3DTileset.h"
#include "UnrealConversions.h"
#include "IonQuickAddPanel.h"
#include "Cesium3DTiles/Tile.h"
#include "Cesium3DTiles/Tileset.h"
#include "CesiumIonRasterOverlay.h"

using namespace CesiumIonClient;

// Identifiers for the columns of the asset table view
static FName ColumnName_Name = "Name";
static FName ColumnName_Type = "Type";
static FName ColumnName_DateAdded = "DateAdded";
static FName ColumnName_Size = "Size";

CesiumIonPanel::CesiumIonPanel() :
    _connectionUpdatedDelegateHandle(),
    _assetsUpdatedDelegateHandle(),
    _pListView(nullptr),
    _assets(),
    _refreshInProgress(false),
    _refreshNeeded(false),
    _pDetails(nullptr),
    _pSelection(nullptr)
{
    this->_connectionUpdatedDelegateHandle = FCesiumEditorModule::ion().ConnectionUpdated.AddRaw(this, &CesiumIonPanel::Refresh);
    this->_assetsUpdatedDelegateHandle = FCesiumEditorModule::ion().AssetsUpdated.AddRaw(this, &CesiumIonPanel::Refresh);
}

CesiumIonPanel::~CesiumIonPanel() {
    FCesiumEditorModule::ion().AssetsUpdated.Remove(this->_assetsUpdatedDelegateHandle);
    FCesiumEditorModule::ion().ConnectionUpdated.Remove(this->_connectionUpdatedDelegateHandle);
}

void CesiumIonPanel::Construct(const FArguments& InArgs)
{
    // A function that returns the lambda that is used for rendering
    // the sort mode indicator of the header column: If sorting is
    // currently done based on the given name, then this will 
    // return the current _sortMode. Otherwise, it will return 
    // the 'None' sort mode.
    auto sortModeLambda = [this](const FName& columnName) {
        return [this, columnName]() {
            if (_sortColumnName != columnName) {
                return EColumnSortMode::None;
            }
            return _sortMode;
        };
    };

    this->_pListView = SNew(SListView<TSharedPtr<Asset>>)
        .ListItemsSource(&this->_assets)
        .OnMouseButtonDoubleClick(this, &CesiumIonPanel::AddAsset)
        .OnGenerateRow(this, &CesiumIonPanel::CreateAssetRow)
        .OnSelectionChanged(this, &CesiumIonPanel::AssetSelected)
        .HeaderRow( SNew(SHeaderRow)
            + SHeaderRow::Column(ColumnName_Name)
                .DefaultLabel(FText::FromString(TEXT("Name")))
                .SortMode_Lambda(sortModeLambda(ColumnName_Name))
                .OnSort(FOnSortModeChanged::CreateSP(this, &CesiumIonPanel::OnSortChange))
            + SHeaderRow::Column(ColumnName_Type)
                .DefaultLabel(FText::FromString(TEXT("Type")))
                .SortMode_Lambda(sortModeLambda(ColumnName_Type))
                .OnSort(FOnSortModeChanged::CreateSP(this, &CesiumIonPanel::OnSortChange))
            + SHeaderRow::Column(ColumnName_DateAdded)
                .DefaultLabel(FText::FromString(TEXT("Date added")))
                .SortMode_Lambda(sortModeLambda(ColumnName_DateAdded))
                .OnSort(FOnSortModeChanged::CreateSP(this, &CesiumIonPanel::OnSortChange))
            + SHeaderRow::Column(ColumnName_Size)
                .DefaultLabel(FText::FromString(TEXT("Size")))
                .SortMode_Lambda(sortModeLambda(ColumnName_Size))
                .OnSort(FOnSortModeChanged::CreateSP(this, &CesiumIonPanel::OnSortChange))
        );

    this->_pDetails = this->AssetDetails();

    ChildSlot
    [
        SNew(SSplitter)
            .Orientation(EOrientation::Orient_Horizontal)
        + SSplitter::Slot()
            .Value(0.66f)
        [
            this->_pListView.ToSharedRef()
        ]
        + SSplitter::Slot()
            .Value(0.34f)
        [
            SNew(SBorder).Padding(10)
            [
                SNew(SVerticalBox)
                + SVerticalBox::Slot()
                [
                    this->_pDetails.ToSharedRef()
                ]
                + SVerticalBox::Slot()
                [
                    SNew(STextBlock).Visibility_Lambda([this]() { return this->_pSelection ? EVisibility::Collapsed : EVisibility::Visible; })
                ]
            ]
        ]
    ];

    FCesiumEditorModule::ion().refreshAssets();
}

void CesiumIonPanel::OnSortChange(const EColumnSortPriority::Type SortPriority, const FName& ColumnName, const EColumnSortMode::Type Mode)
{
	if (_sortColumnName == ColumnName)
	{
        if (_sortMode == EColumnSortMode::Type::None) {
            _sortMode = EColumnSortMode::Type::Ascending;
        } else if (_sortMode == EColumnSortMode::Type::Ascending) {
            _sortMode = EColumnSortMode::Type::Descending;
        } else {
            _sortMode = EColumnSortMode::Type::None;
        }
    } else {
        _sortColumnName = ColumnName;
        _sortMode = EColumnSortMode::Type::Ascending;
    } 
    Refresh();
}

static bool isSupportedTileset(const TSharedPtr<Asset>& pAsset) {
    return
        pAsset &&
        (
            pAsset->type == "3DTILES" ||
            pAsset->type == "TERRAIN"
        );
}

static bool isSupportedImagery(const TSharedPtr<Asset>& pAsset) {
    return
        pAsset &&
        pAsset->type == "IMAGERY";
}

TSharedRef<SWidget> CesiumIonPanel::AssetDetails()
{
    return SNew(SScrollBox)
        .Visibility_Lambda([this]() { return this->_pSelection ? EVisibility::Visible : EVisibility::Collapsed; })
        + SScrollBox::Slot()
        .Padding(10, 10, 10, 0)
        [
            SNew(STextBlock)
            .AutoWrapText(true)
            .TextStyle(FCesiumEditorModule::GetStyle(), "Heading")
            .Text_Lambda([this]() { return FText::FromString(utf8_to_wstr(this->_pSelection->name)); })
        ]
        + SScrollBox::Slot()
        .Padding(10, 5, 10, 10)
        .HAlign(EHorizontalAlignment::HAlign_Fill)
        [
            SNew(STextBlock)
            .Text_Lambda([this]() { return FText::FromString(utf8_to_wstr(std::string("(ID: ") + std::to_string(this->_pSelection->id) + ")")); })
        ]
        + SScrollBox::Slot()
        .Padding(10)
        .HAlign(EHorizontalAlignment::HAlign_Fill)
        [
            SNew(SButton)
            .Visibility_Lambda([this]() { return isSupportedTileset(this->_pSelection) ? EVisibility::Visible : EVisibility::Collapsed; })
            .HAlign(EHorizontalAlignment::HAlign_Center)
            .Text(FText::FromString(TEXT("Add to Level")))
            .OnClicked_Lambda([this]() { this->AddAssetToLevel(this->_pSelection); return FReply::Handled(); })
        ]
        + SScrollBox::Slot()
            .Padding(10)
            .HAlign(EHorizontalAlignment::HAlign_Fill)
        [
            SNew(SButton)
            .Visibility_Lambda([this]() { return isSupportedImagery(this->_pSelection) ? EVisibility::Visible : EVisibility::Collapsed; })
            .HAlign(EHorizontalAlignment::HAlign_Center)
            .Text(FText::FromString(TEXT("Drape Over Terrain Tileset")))
            .OnClicked_Lambda([this]() { this->AddOverlayToTerrain(this->_pSelection); return FReply::Handled(); })
        ]
        + SScrollBox::Slot()
        .Padding(10)
        .HAlign(EHorizontalAlignment::HAlign_Fill)
        [
            SNew(SButton)
            .Visibility_Lambda([this]() { return !isSupportedTileset(this->_pSelection) && !isSupportedImagery(this->_pSelection) ? EVisibility::Visible : EVisibility::Collapsed; })
            .HAlign(EHorizontalAlignment::HAlign_Center)
            .Text(FText::FromString(TEXT("This type of asset is not currently supported")))
            .IsEnabled(false)
        ]
        + SScrollBox::Slot()
        .Padding(10)
        .HAlign(EHorizontalAlignment::HAlign_Fill)
        [
            SNew(STextBlock)
            .TextStyle(FCesiumEditorModule::GetStyle(), "AssetDetailsFieldHeader")
            .Text(FText::FromString(TEXT("Description")))
        ]
        + SScrollBox::Slot()
        .Padding(10, 0)
        [
            SNew(STextBlock)
            .AutoWrapText(true)
            .TextStyle(FCesiumEditorModule::GetStyle(), "AssetDetailsFieldValue")
            .Text_Lambda([this]() { return FText::FromString(utf8_to_wstr(this->_pSelection->description)); })
        ]
        + SScrollBox::Slot()
        .Padding(10)
        .HAlign(EHorizontalAlignment::HAlign_Fill)
        [
            SNew(STextBlock)
            .TextStyle(FCesiumEditorModule::GetStyle(), "AssetDetailsFieldHeader")
            .Text(FText::FromString(TEXT("Attribution")))
        ]
        + SScrollBox::Slot()
        .Padding(10, 0)
        [
            SNew(STextBlock)
            .AutoWrapText(true)
            .TextStyle(FCesiumEditorModule::GetStyle(), "AssetDetailsFieldValue")
            .Text_Lambda([this]() { return FText::FromString(utf8_to_wstr(this->_pSelection->attribution)); })
        ];
}

/**
 * @brief Returns a comparator for the property of an Asset that is
 * associated with the given column name.
 * 
 * @param columnName The column name
 * @return The comparator, comparing is ascending order (comparing by 
 * the asset->name by default, if the given column name was not known)
 */
static std::function<bool(const TSharedPtr<Asset>&, const TSharedPtr<Asset>&)> comparatorFor(const FName& columnName) 
{
    if (columnName == ColumnName_Type) {
        return [](const TSharedPtr<Asset>& a0, const TSharedPtr<Asset>& a1) {
            return a0->type < a1->type;
        };
    }
    if (columnName == ColumnName_DateAdded) {
        return [](const TSharedPtr<Asset>& a0, const TSharedPtr<Asset>& a1) {
            return a0->dateAdded < a1->dateAdded;
        };
    }
    if (columnName == ColumnName_Size) {
        return [](const TSharedPtr<Asset>& a0, const TSharedPtr<Asset>& a1) {
            return a0->bytes < a1->bytes;
        };
    }
    return [](const TSharedPtr<Asset>& a0, const TSharedPtr<Asset>& a1) {
        return a0->name < a1->name;
    };
}


void CesiumIonPanel::ApplySorting() {

    //UE_LOG(LogActor, Warning, TEXT("applySorting %s with %d"), *_sortColumnName.ToString(), _sortMode);

    if (_sortMode == EColumnSortMode::Type::None) {
        return;
    }
    auto baseComparator = comparatorFor(_sortColumnName);
    if (_sortMode == EColumnSortMode::Type::Ascending) {
        this->_assets.Sort(baseComparator);
    } else {
        this->_assets.Sort([&baseComparator](const TSharedPtr<Asset>& a0, const TSharedPtr<Asset>& a1) {
           return baseComparator(a1, a0);
        });
    }
}

void CesiumIonPanel::Refresh() {
    const Assets& assets = FCesiumEditorModule::ion().getAssets();

    this->_assets.SetNum(assets.items.size());

    for (size_t i = 0; i < assets.items.size(); ++i) {
        this->_assets[i] = MakeShared<Asset>(assets.items[i]);
    }
    ApplySorting();
    this->_pListView->RequestListRefresh();
}

void CesiumIonPanel::AssetSelected(TSharedPtr<CesiumIonClient::Asset> item, ESelectInfo::Type selectionType)
{
    this->_pSelection = item;
}

void CesiumIonPanel::AddAsset(TSharedPtr<CesiumIonClient::Asset> item) {
    if (item->type == "IMAGERY") {
        this->AddOverlayToTerrain(item);
    } else {
        this->AddAssetToLevel(item);
    }
}

void CesiumIonPanel::AddAssetToLevel(TSharedPtr<CesiumIonClient::Asset> item)
{
    UWorld* pCurrentWorld = GEditor->GetEditorWorldContext().World();
    ULevel* pCurrentLevel = pCurrentWorld->GetCurrentLevel();

    AActor* pNewActor = GEditor->AddActor(pCurrentLevel, ACesium3DTileset::StaticClass(), FTransform(), false, RF_Public | RF_Transactional);
    ACesium3DTileset* pTileset = Cast<ACesium3DTileset>(pNewActor);
    pTileset->SetActorLabel(utf8_to_wstr(item->name));
    pTileset->IonAssetID = item->id;
    pTileset->IonAccessToken = utf8_to_wstr(FCesiumEditorModule::ion().getAssetAccessToken().token);

    pTileset->RerunConstructionScripts();
}

void CesiumIonPanel::AddOverlayToTerrain(TSharedPtr<CesiumIonClient::Asset> item) {
    UWorld* pCurrentWorld = GEditor->GetEditorWorldContext().World();
    ULevel* pCurrentLevel = pCurrentWorld->GetCurrentLevel();

    ACesium3DTileset* pTilesetActor = FCesiumEditorModule::FindFirstTilesetSupportingOverlays();
    if (!pTilesetActor) {
        pTilesetActor = FCesiumEditorModule::CreateTileset("Cesium World Terrain", 1);
    }

    UCesiumRasterOverlay* pOverlay = FCesiumEditorModule::AddOverlay(pTilesetActor, item->name, item->id);

    pTilesetActor->RerunConstructionScripts();

    GEditor->SelectNone(true, false);
    GEditor->SelectActor(pTilesetActor, true, true, true, true);
    GEditor->SelectComponent(pOverlay, true, true, true);
}

namespace {

    class AssetsTableRow : public SMultiColumnTableRow<TSharedPtr<Asset>> {
    public:
        void Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTableView, const TSharedPtr<Asset>& pItem) {
            this->_pItem = pItem;
            SMultiColumnTableRow<TSharedPtr<Asset>>::Construct(InArgs, InOwnerTableView);
        }

        virtual TSharedRef<SWidget> GenerateWidgetForColumn(const FName& InColumnName) override {
            if (InColumnName == "Name") {
                return SNew(STextBlock)
                    .Text(FText::FromString(utf8_to_wstr(_pItem->name)));
            } else if (InColumnName == "Type") {
                return SNew(STextBlock)
                    .Text(FText::FromString(utf8_to_wstr(_pItem->type)));
            } else if (InColumnName == "DateAdded") {
                return SNew(STextBlock)
                    .Text(FText::FromString(utf8_to_wstr(_pItem->dateAdded)));
            } else if (InColumnName == "Size") {
                return SNew(STextBlock)
                    .Text(FText::FromString(utf8_to_wstr(_pItem->bytes > 0 ? std::to_string(_pItem->bytes) : "-")));
            } else {
                return SNew(STextBlock);
            }
        }

    private:
        TSharedPtr<Asset> _pItem;
    };

}

TSharedRef<ITableRow> CesiumIonPanel::CreateAssetRow(TSharedPtr<Asset> item, const TSharedRef<STableViewBase>& list) {
    return SNew(AssetsTableRow, list, item);
}