﻿// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "Slate/SPjcTabAssetsUnused.h"
#include "Slate/SPjcTreeView.h"
#include "Slate/SPjcStatAssets.h"
#include "PjcCmds.h"
#include "PjcTypes.h"
#include "PjcSubsystem.h"
// Engine Headers
#include "ObjectTools.h"
#include "Settings/ContentBrowserSettings.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SSeparator.h"

void SPjcTabAssetsUnused::Construct(const FArguments& InArgs)
{
	UContentBrowserSettings* ContentBrowserSettings = GetMutableDefault<UContentBrowserSettings>();
	if (ContentBrowserSettings)
	{
		ContentBrowserSettings->SetDisplayDevelopersFolder(true);
		ContentBrowserSettings->SetDisplayEngineFolder(false);
		ContentBrowserSettings->SetDisplayCppFolders(false);
		ContentBrowserSettings->SetDisplayPluginFolders(false);
		ContentBrowserSettings->PostEditChange();
	}

	Cmds = MakeShareable(new FUICommandList);

	Cmds->MapAction(
		FPjcCmds::Get().TabAssetsUnusedBtnScan,
		FExecuteAction::CreateRaw(this, &SPjcTabAssetsUnused::OnScanProjectAssets)
	);

	Cmds->MapAction(
		FPjcCmds::Get().TabAssetsUnusedBtnClean,
		FExecuteAction::CreateRaw(this, &SPjcTabAssetsUnused::OnCleanProject),
		FCanExecuteAction::CreateRaw(this, &SPjcTabAssetsUnused::CanCleanProject)
	);

	FPropertyEditorModule& PropertyEditor = UPjcSubsystem::GetModulePropertyEditor();
	FDetailsViewArgs DetailsViewArgs;
	DetailsViewArgs.bUpdatesFromSelection = false;
	DetailsViewArgs.bLockable = false;
	DetailsViewArgs.bAllowSearch = false;
	DetailsViewArgs.bShowOptions = true;
	DetailsViewArgs.bAllowFavoriteSystem = false;
	DetailsViewArgs.bShowPropertyMatrixButton = false;
	DetailsViewArgs.NameAreaSettings = FDetailsViewArgs::HideNameArea;
	DetailsViewArgs.ViewIdentifier = "PjcEditorAssetExcludeSettings";

	const auto SettingsProperty = PropertyEditor.CreateDetailView(DetailsViewArgs);
	SettingsProperty->SetObject(GetMutableDefault<UPjcAssetExcludeSettings>());

	UPjcSubsystem::AssetCategoryMappingInit(AssetsCategoryMapping);

	ChildSlot
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().FillHeight(1.0f).Padding(5.0f)
		[
			SNew(SSplitter)
			.PhysicalSplitterHandleSize(3.0f)
			.Style(FEditorStyle::Get(), "DetailsView.Splitter")
			+ SSplitter::Slot().Value(0.2f)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight().Padding(3.0f)
				[
					CreateToolbar()
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(3.0f)
				[
					SNew(SSeparator).Thickness(5.0f)
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(3.0f)
				[
					SAssignNew(StatAssetsPtr, SPjcStatAssets)
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(3.0f)
				[
					SNew(SSeparator).Thickness(5.0f)
				]
				+ SVerticalBox::Slot().FillHeight(1.0f).Padding(3.0f)
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
			+ SSplitter::Slot().Value(0.35f)
			[
				SNew(STextBlock)
				// TreeViewPtr.ToSharedRef()
			]
			+ SSplitter::Slot().Value(0.45f)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().FillHeight(1.0f).Padding(5.0f)
				[
					SNew(STextBlock)
					// ContentBrowserModule.Get().CreateAssetPicker(AssetPickerConfig)
				]
			]
		]
	];
}

void SPjcTabAssetsUnused::OnScanProjectAssets()
{
	FString ErrMsg;
	UPjcSubsystem::ScanProjectAssets(AssetsCategoryMapping, ErrMsg);

	if (ErrMsg.IsEmpty())
	{
		if (StatAssetsPtr.IsValid())
		{
			StatAssetsPtr->StatsUpdateData(AssetsCategoryMapping);
		}
	}
	else
	{
		UPjcSubsystem::ShowNotificationWithOutputLog(ErrMsg, SNotificationItem::CS_Fail, 5.0f);
	}
}

void SPjcTabAssetsUnused::OnCleanProject()
{
	ObjectTools::DeleteAssets(AssetsCategoryMapping[EPjcAssetCategory::Unused].Array(), true);
	
	// todo:ashe23 cleanup setting window here
	// const TSharedRef<SWindow> Window = SNew(SWindow).Title(FText::FromString(TEXT("Some Title"))).ClientSize(FVector2D{600, 400});
	// const TSharedRef<SWidget> Content =
	// 	SNew(SVerticalBox)
	// 	+ SVerticalBox::Slot()
	// 	[
	// 		SNew(STextBlock).Text(FText::FromString(TEXT("Some Content")))
	// 	];
	//
	// Window->SetContent(Content);
	//
	// if (GEditor)
	// {
	// 	GEditor->EditorAddModalWindow(Window);
	// }
}

bool SPjcTabAssetsUnused::CanCleanProject()
{
	return AssetsCategoryMapping[EPjcAssetCategory::Unused].Num() > 0;
}

TSharedRef<SWidget> SPjcTabAssetsUnused::CreateToolbar() const
{
	FToolBarBuilder ToolBarBuilder{Cmds, FMultiBoxCustomization::None};
	ToolBarBuilder.BeginSection("PjcSectionTabAssetUnusedActions");
	{
		ToolBarBuilder.AddToolBarButton(FPjcCmds::Get().TabAssetsUnusedBtnScan);
		ToolBarBuilder.AddToolBarButton(FPjcCmds::Get().TabAssetsUnusedBtnClean);
	}
	ToolBarBuilder.EndSection();

	return ToolBarBuilder.MakeWidget();
}

// TSharedPtr<SWidget> SPjcTabAssetsUnused::GetContentBrowserContextMenu(const TArray<FAssetData>& Assets) const
// {
// 	FMenuBuilder MenuBuilder{true, Cmds};
//
// 	MenuBuilder.BeginSection(TEXT("PjcAssetsInfo"), FText::FromString(TEXT("Info")));
// 	{
// 		MenuBuilder.AddMenuEntry(FPjcCmds::Get().OpenViewerSizeMap);
// 		MenuBuilder.AddMenuEntry(FPjcCmds::Get().OpenViewerReference);
// 		MenuBuilder.AddMenuEntry(FPjcCmds::Get().OpenViewerAudit);
// 	}
// 	MenuBuilder.EndSection();
//
// 	MenuBuilder.BeginSection(TEXT("PjcAssetActions"), FText::FromString(TEXT("Actions")));
// 	{
// 		MenuBuilder.AddMenuEntry(FPjcCmds::Get().AssetsExclude);
// 		MenuBuilder.AddMenuEntry(FPjcCmds::Get().AssetsExcludeByClass);
// 		MenuBuilder.AddMenuEntry(FPjcCmds::Get().AssetsInclude);
// 		MenuBuilder.AddMenuEntry(FPjcCmds::Get().AssetsIncludeByClass);
// 		MenuBuilder.AddMenuEntry(FPjcCmds::Get().AssetsDelete);
// 	}
// 	MenuBuilder.EndSection();
//
// 	return MenuBuilder.MakeWidget();
// }
