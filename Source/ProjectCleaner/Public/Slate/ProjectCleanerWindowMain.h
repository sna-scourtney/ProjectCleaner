﻿// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class SProjectCleanerWindowMain : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SProjectCleanerWindowMain)
		{
		}

	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
private:
	FText GetNumAllAssets() const;
	FText GetNumUsedAssets() const;
	FText GetNumUnusedAssets() const;
	FText GetNumExcludedAssets() const;
	FText GetNumIndirectAssets() const;
	FText GetNumAllFolders() const;
	FText GetNumEmptyFolders() const;
	FText GetNumCorruptedAssets() const;
	FText GetNumExternalFiles() const;
	FText GetSizeTotal() const;
	FText GetSizeUsed() const;
	FText GetSizeUnused() const;
	FText GetSizeExcluded() const;
	FText GetSizeIndirect() const;
	static void OnNavigateWiki();
	static bool IsWidgetEnabled();
	static int32 GetWidgetIndex();
};
