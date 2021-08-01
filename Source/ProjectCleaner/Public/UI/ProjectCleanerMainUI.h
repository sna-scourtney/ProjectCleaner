﻿// Copyright 2021. Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Core/ProjectCleanerManager.h"
#include "Widgets/SCompoundWidget.h"

class SProjectCleanerMainUI : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SProjectCleanerMainUI) {}
    SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	virtual ~SProjectCleanerMainUI() override;

private:
	/* Initializers */
	void InitTabs();
	void OnCleanerManagerUpdated() const;
	
	/* Callbacks */
	TSharedRef<SDockTab> OnUnusedAssetTabSpawn(const FSpawnTabArgs& SpawnTabArgs);
	TSharedRef<SDockTab> OnExcludedAssetsTabSpawn(const FSpawnTabArgs& SpawnTabArgs);
	TSharedRef<SDockTab> OnNonEngineFilesTabSpawn(const FSpawnTabArgs& SpawnTabArgs);
	TSharedRef<SDockTab> OnCorruptedFilesTabSpawn(const FSpawnTabArgs& SpawnTabArgs);
	TSharedRef<SDockTab> OnIndirectAssetsTabSpawn(const FSpawnTabArgs& SpawnTabArgs);
	
	/* Btn Callbacks */
	FReply OnRefreshBtnClick();
	FReply OnDeleteUnusedAssetsBtnClick();
	FReply OnDeleteEmptyFolderClick() const;
	EAppReturnType::Type ShowConfirmationWindow(const FText& Title, const FText& ContentText) const;
	static bool IsConfirmationWindowCanceled(EAppReturnType::Type Status);
	
	/* UI Data */
	TWeakPtr<class SProjectCleanerStatisticsUI> StatisticsUI;
	TWeakPtr<class SProjectCleanerUnusedAssetsBrowserUI> UnusedAssetsBrowserUI;
	TWeakPtr<class SProjectCleanerNonEngineFilesUI> NonEngineFilesUI;
	TWeakPtr<class SProjectCleanerConfigsUI> CleanerConfigsUI;
	TWeakPtr<class SProjectCleanerCorruptedFilesUI> CorruptedFilesUI;
	TWeakPtr<class SProjectCleanerIndirectAssetsUI> IndirectAssetsUI;
	TWeakPtr<class SProjectCleanerExcludeOptionsUI> ExcludeOptionUI;
	TWeakPtr<class SProjectCleanerExcludedAssetsUI> ExcludedAssetsUI;
	TSharedPtr<class FTabManager> TabManager;
	TSharedPtr<FTabManager::FLayout> TabLayout;

	/* Data */
	ProjectCleanerManager CleanerManager;
};
