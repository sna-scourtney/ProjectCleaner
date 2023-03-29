﻿// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "Slate/SPjcTabScanSettings.h"
#include "PjcSubsystem.h"
#include "PjcSettings.h"
#include "PjcStyles.h"
#include "PjcConstants.h"
// Engine Headers
#include "Widgets/Layout/SSeparator.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/SCanvas.h"

void SPjcStatItem::Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InTable)
{
	StatItem = InArgs._StatItem;

	SMultiColumnTableRow::Construct(SMultiColumnTableRow::FArguments(), InTable);
}

TSharedRef<SWidget> SPjcStatItem::GenerateWidgetForColumn(const FName& InColumnName)
{
	if (InColumnName.IsEqual(TEXT("Category")))
	{
		return
			SNew(STextBlock)
			.Text(FText::FromString(StatItem->Category))
			.Font(FPjcStyles::GetFont("Bold", 13))
			.ToolTipText(FText::FromString(StatItem->ToolTip));
	}

	if (InColumnName.IsEqual(TEXT("Num")))
	{
		FNumberFormattingOptions NumberFormattingOptions;
		NumberFormattingOptions.SetUseGrouping(true);
		NumberFormattingOptions.SetMinimumFractionalDigits(0);

		const FString StrNum = StatItem->Num > 0 ? FText::AsNumber(StatItem->Num, &NumberFormattingOptions).ToString() : TEXT("");

		return
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().Padding(FMargin{10.0f, 0.0f})
			[
				SNew(STextBlock).Text(FText::FromString(StrNum))
			];
	}

	if (InColumnName.IsEqual(TEXT("Size")))
	{
		const FString StrSize = StatItem->Size > 0 ? FText::AsMemory(StatItem->Size, IEC).ToString() : TEXT("");

		return SNew(STextBlock).Text(FText::FromString(StrSize));
	}

	return SNew(STextBlock);
}

void SPjcTabScanSettings::Construct(const FArguments& InArgs)
{
	FPropertyEditorModule& PropertyEditor = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
	FDetailsViewArgs DetailsViewArgs;
	DetailsViewArgs.bUpdatesFromSelection = false;
	DetailsViewArgs.bLockable = false;
	DetailsViewArgs.bAllowSearch = false;
	DetailsViewArgs.bShowOptions = true;
	DetailsViewArgs.bAllowFavoriteSystem = false;
	DetailsViewArgs.NameAreaSettings = FDetailsViewArgs::HideNameArea;
	DetailsViewArgs.ViewIdentifier = "ProjectCleanerSettings";

	const auto SettingsProperty = PropertyEditor.CreateDetailView(DetailsViewArgs);
	SettingsProperty->SetObject(GetMutableDefault<UPjcSettings>());

	StatsUpdate();

	ChildSlot
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().Padding(FMargin{5.0f})
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight().Padding(FMargin{5.0f})
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().Padding(FMargin{0.0f, 0.0f, 5.0f, 0.0f})
				[
					SNew(SButton)
					.ContentPadding(FMargin{5.0f})
					.OnClicked_Raw(this, &SPjcTabScanSettings::OnBtnScanProjectClick)
					.ButtonColorAndOpacity(FPjcStyles::Get().GetColor("ProjectCleaner.Color.Blue"))
					[
						SNew(STextBlock)
						.Justification(ETextJustify::Center)
						.ToolTipText(FText::FromString(TEXT("Scan project for unused assets, empty folders and other files")))
						.ColorAndOpacity(FPjcStyles::Get().GetColor("ProjectCleaner.Color.White"))
						.ShadowOffset(FVector2D{1.5f, 1.5f})
						.ShadowColorAndOpacity(FLinearColor::Black)
						.Font(FPjcStyles::GetFont("Bold", 10))
						.Text(FText::FromString(TEXT("Scan Project")))
					]
				]
				+ SHorizontalBox::Slot().AutoWidth().Padding(FMargin{0.0f, 0.0f, 5.0f, 0.0f})
				[
					SNew(SButton)
					.ContentPadding(FMargin{5.0f})
					.OnClicked_Raw(this, &SPjcTabScanSettings::OnBtnCleanProjectClick)
					.IsEnabled_Raw(this, &SPjcTabScanSettings::BtnCleanProjectEnabled)
					.ButtonColorAndOpacity(FPjcStyles::Get().GetColor("ProjectCleaner.Color.Red"))
					[
						SNew(STextBlock)
						.Justification(ETextJustify::Center)
						.ToolTipText(FText::FromString(TEXT("Clean project based on specified CleanupMethod.")))
						.ColorAndOpacity(FPjcStyles::Get().GetColor("ProjectCleaner.Color.White"))
						.ShadowOffset(FVector2D{1.5f, 1.5f})
						.ShadowColorAndOpacity(FLinearColor::Black)
						.Font(FPjcStyles::GetFont("Bold", 10))
						.Text(FText::FromString(TEXT("Clean Project")))
					]
				]
			]
			+ SVerticalBox::Slot().Padding(FMargin{5.0f}).AutoHeight()
			[
				SNew(SSeparator)
				.Thickness(5.0f)
			]
			+ SVerticalBox::Slot().Padding(FMargin{5.0f}).AutoHeight()
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().Padding(FMargin{5.0f}).AutoHeight().HAlign(HAlign_Center).VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Font(FPjcStyles::GetFont("Bold", 15))
					.ColorAndOpacity(FPjcStyles::Get().GetColor("ProjectCleaner.Color.White"))
					.Text(FText::FromString(TEXT("Project Content Statistics")))
				]
				+ SVerticalBox::Slot().AutoHeight()
				[
					StatView.ToSharedRef()
				]
			]
			+ SVerticalBox::Slot().Padding(FMargin{5.0f}).AutoHeight()
			[
				SNew(SSeparator)
				.Thickness(5.0f)
			]
			+ SVerticalBox::Slot().Padding(FMargin{5.0f}).FillHeight(1.0f)
			[
				SNew(SBox)
				[
					SNew(SScrollBox)
					.ScrollWhenFocusChanges(EScrollWhenFocusChanges::NoScroll)
					.AnimateWheelScrolling(true)
					.AllowOverscroll(EAllowOverscroll::No)
					+ SScrollBox::Slot()
					[
						SettingsProperty
					]
				]
			]
		]
	];
}

FReply SPjcTabScanSettings::OnBtnScanProjectClick() const
{
	GEditor->GetEditorSubsystem<UPjcSubsystem>()->ProjectScan();

	return FReply::Handled();
}

FReply SPjcTabScanSettings::OnBtnCleanProjectClick() const
{
	// const FText Title = FText::FromString(TEXT("Confirm project cleaning"));
	// const FText Msg = GetCleanupText(GetDefault<UPjcSettings>()->CleanupMethod);
	// const EAppReturnType::Type Result = FMessageDialog::Open(EAppMsgType::YesNo, Msg, &Title);
	//
	// if (Result == EAppReturnType::No || Result == EAppReturnType::Cancel)
	// {
	// 	return FReply::Handled();
	// }
	//
	// GEditor->GetEditorSubsystem<UPjcSubsystem>()->ProjectClean();

	return FReply::Handled();
}

bool SPjcTabScanSettings::BtnCleanProjectEnabled() const
{
	return false;
}

void SPjcTabScanSettings::StatsUpdate()
{
	if (!StatView.IsValid())
	{
		SAssignNew(StatView, SListView<TSharedPtr<FPjcStatItem>>)
		.ListItemsSource(&StatItems)
		.OnGenerateRow_Raw(this, &SPjcTabScanSettings::OnStatsGenerateRow)
		.SelectionMode(ESelectionMode::None)
		.HeaderRow(GetStatsHeaderRow());
	}

	StatItems.Empty();

	StatItems.Emplace(MakeShareable(new FPjcStatItem{123213, 12312, TEXT("Files Total"), TEXT("Total number of files inside Content folder")}));
	StatItems.Emplace(MakeShareable(new FPjcStatItem{123213, 12312, TEXT("Files NonEngine"), TEXT("Files that dont have .umap or .uaaset extension, but are inside Content folder")}));
	StatItems.Emplace(MakeShareable(new FPjcStatItem{123213, 12312, TEXT("Files Corrupted"), TEXT("Files that have .umap or .uasset extension, but are not loaded by AssetRegistry")}));
	StatItems.Emplace(MakeShareable(new FPjcStatItem{-1, -1, TEXT(""), TEXT("")}));
	StatItems.Emplace(MakeShareable(new FPjcStatItem{-1, 12312, TEXT("Folders Total"), TEXT("Total number of folders inside Content folder")}));
	StatItems.Emplace(MakeShareable(new FPjcStatItem{-1, 12312, TEXT("Folders Empty"), TEXT("Total number of empty folders inside Content folder")}));
	StatItems.Emplace(MakeShareable(new FPjcStatItem{-1, -1, TEXT(""), TEXT("")}));
	StatItems.Emplace(MakeShareable(new FPjcStatItem{123213, 12312, TEXT("Assets Total"), TEXT("Total number of assets in project")}));
	StatItems.Emplace(MakeShareable(new FPjcStatItem{123213, 12312, TEXT("Assets Used"), TEXT("Total number of used assets in project")}));
	StatItems.Emplace(MakeShareable(new FPjcStatItem{123213, 12312, TEXT("		Primary"), TEXT("Total number of primary assets in project")}));
	StatItems.Emplace(MakeShareable(
		new FPjcStatItem{123213, 12312, TEXT("		Indirect"), TEXT("Total number of indirect assets in project. Assets that used in source code or config files")}));
	StatItems.Emplace(MakeShareable(new FPjcStatItem{
		123213, 12312, TEXT("		Editor"), TEXT("Total number of editor specific assets in project. Assets like EditorUtilityBlueprint, EditorUtilityWidgets or EditorTutorial")
	}));
	StatItems.Emplace(MakeShareable(new FPjcStatItem{123213, 12312, TEXT("		ExtRefs"), TEXT("Total number of assets that have external referencers outside Content folder")}));
	StatItems.Emplace(MakeShareable(new FPjcStatItem{123213, 12312, TEXT("		Excluded"), TEXT("Total number of exluded assets by settings")}));
	StatItems.Emplace(MakeShareable(new FPjcStatItem{123213, 12312, TEXT("Assets Unused"), TEXT("Total number of unused assets in project")}));

	StatView->RebuildList();
}

TSharedRef<SHeaderRow> SPjcTabScanSettings::GetStatsHeaderRow() const
{
	return
		SNew(SHeaderRow)
		+ SHeaderRow::Column("Category")
		  .FillWidth(0.6f)
		  .HAlignCell(HAlign_Left)
		  .VAlignCell(VAlign_Center)
		  .HAlignHeader(HAlign_Center)
		  .HeaderContentPadding(PjcConstants::HeaderRowMargin)
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("Category")))
			.ColorAndOpacity(FPjcStyles::Get().GetSlateColor("ProjectCleaner.Color.Green"))
			.Font(FPjcStyles::GetFont("Light", PjcConstants::HeaderRowFontSize))
		]
		+ SHeaderRow::Column("Num")
		  .FillWidth(0.2f)
		  .HAlignCell(HAlign_Center)
		  .VAlignCell(VAlign_Center)
		  .HAlignHeader(HAlign_Center)
		  .HeaderContentPadding(PjcConstants::HeaderRowMargin)
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("Count")))
			.ColorAndOpacity(FPjcStyles::Get().GetSlateColor("ProjectCleaner.Color.Green"))
			.Font(FPjcStyles::GetFont("Light", PjcConstants::HeaderRowFontSize))
		]
		+ SHeaderRow::Column("Size")
		  .FillWidth(0.2f)
		  .HAlignCell(HAlign_Center)
		  .VAlignCell(VAlign_Center)
		  .HAlignHeader(HAlign_Center)
		  .HeaderContentPadding(PjcConstants::HeaderRowMargin)
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("Size")))
			.ColorAndOpacity(FPjcStyles::Get().GetSlateColor("ProjectCleaner.Color.Green"))
			.Font(FPjcStyles::GetFont("Light", PjcConstants::HeaderRowFontSize))
		];
}

TSharedRef<ITableRow> SPjcTabScanSettings::OnStatsGenerateRow(TSharedPtr<FPjcStatItem> Item, const TSharedRef<STableViewBase>& OwnerTable) const
{
	return SNew(SPjcStatItem, OwnerTable).StatItem(Item);
}


// FText SPjcTabScanSettings::GetStatAssetsTotal() const
// {
// 	const FString StrSize = FText::AsMemory(SizeAssetsTotal, IEC).ToString();
// 	const FString StrNum = FText::AsNumber(NumAssetsTotal, &NumberFormattingOptions).ToString();
//
// 	return FText::FromString(FString::Printf(TEXT("Assets Total - %s ( %s )"), *StrNum, *StrSize));
// }
//
// FText SPjcTabScanSettings::GetStatAssetsUsed() const
// {
// 	const FString StrSize = FText::AsMemory(SizeAssetsUsed, IEC).ToString();
// 	const FString StrNum = FText::AsNumber(NumAssetsUsed, &NumberFormattingOptions).ToString();
//
// 	return FText::FromString(FString::Printf(TEXT("Assets Used - %s ( %s )"), *StrNum, *StrSize));
// }
//
// FText SPjcTabScanSettings::GetStatAssetsUnused() const
// {
// 	const FString StrSize = FText::AsMemory(SizeAssetsUnused, IEC).ToString();
// 	const FString StrNum = FText::AsNumber(NumAssetsUnused, &NumberFormattingOptions).ToString();
//
// 	return FText::FromString(FString::Printf(TEXT("Assets Unused - %s ( %s )"), *StrNum, *StrSize));
// }
//
// FText SPjcTabScanSettings::GetStatAssetsExcluded() const
// {
// 	const FString StrSize = FText::AsMemory(SizeAssetsExcluded, IEC).ToString();
// 	const FString StrNum = FText::AsNumber(NumAssetsExcluded, &NumberFormattingOptions).ToString();
//
// 	return FText::FromString(FString::Printf(TEXT("Excluded - %s ( %s )"), *StrNum, *StrSize));
// }
//
// FText SPjcTabScanSettings::GetStatAssetsIndirect() const
// {
// 	const FString StrSize = FText::AsMemory(SizeAssetsIndirect, IEC).ToString();
// 	const FString StrNum = FText::AsNumber(NumAssetsIndirect, &NumberFormattingOptions).ToString();
//
// 	return FText::FromString(FString::Printf(TEXT("Indirect - %s ( %s )"), *StrNum, *StrSize));
// }
//
// FText SPjcTabScanSettings::GetStatAssetsPrimary() const
// {
// 	const FString StrSize = FText::AsMemory(SizeAssetsPrimary, IEC).ToString();
// 	const FString StrNum = FText::AsNumber(NumAssetsPrimary, &NumberFormattingOptions).ToString();
//
// 	return FText::FromString(FString::Printf(TEXT("Primary - %s ( %s )"), *StrNum, *StrSize));
// }
//
// FText SPjcTabScanSettings::GetStatAssetsEditor() const
// {
// 	const FString StrSize = FText::AsMemory(SizeAssetsEditor, IEC).ToString();
// 	const FString StrNum = FText::AsNumber(NumAssetsEditor, &NumberFormattingOptions).ToString();
//
// 	return FText::FromString(FString::Printf(TEXT("Editor - %s ( %s )"), *StrNum, *StrSize));
// }
//
// FText SPjcTabScanSettings::GetStatAssetsExtReferenced() const
// {
// 	const FString StrSize = FText::AsMemory(SizeAssetsExtReferenced, IEC).ToString();
// 	const FString StrNum = FText::AsNumber(NumAssetsExtReferenced, &NumberFormattingOptions).ToString();
//
// 	return FText::FromString(FString::Printf(TEXT("ExtReferenced - %s ( %s )"), *StrNum, *StrSize));
// }
//
// FText SPjcTabScanSettings::GetStatFilesNonEngine() const
// {
// 	const FString StrSize = FText::AsMemory(SizeFilesNonEngine, IEC).ToString();
// 	const FString StrNum = FText::AsNumber(NumFilesNonEngine, &NumberFormattingOptions).ToString();
//
// 	return FText::FromString(FString::Printf(TEXT("Files NonEngine - %s ( %s )"), *StrNum, *StrSize));
// }
//
// FText SPjcTabScanSettings::GetStatFilesCorrupted() const
// {
// 	const FString StrSize = FText::AsMemory(SizeFilesCorrupted, IEC).ToString();
// 	const FString StrNum = FText::AsNumber(NumFilesCorrupted, &NumberFormattingOptions).ToString();
//
// 	return FText::FromString(FString::Printf(TEXT("Files Corrupted - %s ( %s )"), *StrNum, *StrSize));
// }
//
// FText SPjcTabScanSettings::GetStatFoldersEmpty() const
// {
// 	const FString StrNum = FText::AsNumber(NumFoldersEmpty, &NumberFormattingOptions).ToString();
//
// 	return FText::FromString(FString::Printf(TEXT("Folders Empty - %s"), *StrNum));
// }
//
// FText SPjcTabScanSettings::GetCleanupText(const EPjcCleanupMethod CleanupMethod) const
// {
// 	switch (CleanupMethod)
// 	{
// 		case EPjcCleanupMethod::None: return FText::FromString(TEXT(""));
// 		case EPjcCleanupMethod::Full: return FText::FromString(TEXT("Are you sure you want to delete all unused assets and empty folders in project?"));
// 		case EPjcCleanupMethod::UnusedAssetsOnly: return FText::FromString(TEXT("Are you sure you want to delete all unused assets in project?"));
// 		case EPjcCleanupMethod::EmptyFoldersOnly: return FText::FromString(TEXT("Are you sure you want to delete all empty folders in project?"));
// 		default:
// 			return FText::FromString(TEXT(""));
// 	}
// }
