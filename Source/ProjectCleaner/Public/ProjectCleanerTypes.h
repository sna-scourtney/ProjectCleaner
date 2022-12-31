﻿// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProjectCleanerTypes.generated.h"

UENUM(BlueprintType)
enum class EProjectCleanerScanResult : uint8
{
	None,
	Success,
	AssetRegistryWorking,
	EditorInPlayMode,
	ScanningInProgress,
	CleaningInProgress,
	FailedToSaveAssets
};

USTRUCT(BlueprintType)
struct FProjectCleanerIndirectAssetInfo
{
	GENERATED_BODY()

	bool operator==(const FProjectCleanerIndirectAssetInfo& Other) const
	{
		return LineNum == Other.LineNum && FilePath.Equals(Other.FilePath);
	}

	bool operator!=(const FProjectCleanerIndirectAssetInfo& Other) const
	{
		return LineNum != Other.LineNum || !FilePath.Equals(Other.FilePath);
	}

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Indirect Asset", meta=(ToolTip="Indirect asset data"))
	FAssetData AssetData;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Indirect Asset", meta=(ToolTip="Line number where asset used in file"))
	int32 LineNum = 0;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Indirect Asset", meta=(ToolTip="File path where assets used"))
	FString FilePath;
};

struct FProjectCleanerTabNonEngineListItem
{
	FString FileName;
	FString FileExtension;
	FString FilePathAbs;
	int64 FileSize = 0;

	bool operator==(const FProjectCleanerTabNonEngineListItem& Other) const
	{
		return FilePathAbs.Equals(Other.FilePathAbs);
	}

	bool operator!=(const FProjectCleanerTabNonEngineListItem& Other) const
	{
		return !FilePathAbs.Equals(Other.FilePathAbs);
	}
};

struct FProjectCleanerTabCorruptedListItem
{
	FString FileName;
	FString FileExtension;
	FString FilePathAbs;
	int64 FileSize;

	bool operator==(const FProjectCleanerTabCorruptedListItem& Other) const
	{
		return FilePathAbs.Equals(Other.FilePathAbs);
	}

	bool operator!=(const FProjectCleanerTabCorruptedListItem& Other) const
	{
		return !FilePathAbs.Equals(Other.FilePathAbs);
	}
};

USTRUCT(BlueprintType)
struct FProjectCleanerScanSettings
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="ProjectCleaner|ScanSettings", meta=(ToolTip="Paths inside Content folder that must be excluded from scanning. Must be relative."))
	TArray<FString> ExcludedFolders;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="ProjectCleaner|ScanSettings", meta=(ToolTip="Assets classes that must be excluded from scanning."))
	TArray<UClass*> ExcludedClasses;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="ProjectCleaner|ScanSettings", meta=(ToolTip="Specific assets that must be excluded from scanning."))
	TArray<TSoftObjectPtr<UObject>> ExcludedAssets;
};

USTRUCT(BlueprintType)
struct FProjectCleanerScanData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category="ProjectCleaner|ScanData")
	EProjectCleanerScanResult ScanResult = EProjectCleanerScanResult::None;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category="ProjectCleaner|ScanData")
	FString ScanResultMsg;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category="ProjectCleaner|ScanData")
	TArray<FAssetData> AssetsAll;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category="ProjectCleaner|ScanData")
	TArray<FAssetData> AssetsUsed;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category="ProjectCleaner|ScanData")
	TArray<FAssetData> AssetsPrimary;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category="ProjectCleaner|ScanData")
	TArray<FAssetData> AssetsIndirect;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category="ProjectCleaner|ScanData")
	TArray<FProjectCleanerIndirectAssetInfo> AssetsIndirectInfo;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category="ProjectCleaner|ScanData")
	TArray<FAssetData> AssetsExcluded;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category="ProjectCleaner|ScanData")
	TArray<FAssetData> AssetsUnused;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category="ProjectCleaner|ScanData")
	TArray<FString> FoldersAll;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category="ProjectCleaner|ScanData")
	TArray<FString> FoldersEmpty;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category="ProjectCleaner|ScanData")
	TArray<FString> FilesCorrupted;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category="ProjectCleaner|ScanData")
	TArray<FString> FilesNonEngine;

	void Empty()
	{
		ScanResult = EProjectCleanerScanResult::None;
		ScanResultMsg.Empty();
		AssetsAll.Empty();
		AssetsUsed.Empty();
		AssetsPrimary.Empty();
		AssetsIndirect.Empty();
		AssetsIndirectInfo.Empty();
		AssetsExcluded.Empty();
		AssetsUnused.Empty();
		FoldersAll.Empty();
		FoldersEmpty.Empty();
		FilesCorrupted.Empty();
		FilesNonEngine.Empty();
	}
};

struct FProjectCleanerTreeViewItem
{
	FString FolderPathAbs;
	FString FolderPathRel;
	FString FolderName;
	int32 FoldersTotal = 0;
	int32 FoldersEmpty = 0;
	int64 SizeTotal = 0;
	int64 SizeUnused = 0;
	int32 AssetsTotal = 0;
	int32 AssetsUnused = 0;
	bool bVisible = true;
	bool bDevFolder = false;
	bool bExpanded = false;
	bool bExcluded = false;
	bool bEmpty = false;
	float PercentUnused = 0.0f; // 0 - 100 range
	float PercentUnusedNormalized = 0.0f; // 0 - 1 range

	bool IsValid() const
	{
		return FolderPathAbs.IsEmpty() == false;
	}

	// TSharedPtr<FProjectCleanerTreeViewItem> Parent;
	// TArray<FProjectCleanerTreeViewItem> SubItems;

	bool operator==(const FProjectCleanerTreeViewItem& Other) const
	{
		return FolderPathAbs.Equals(Other.FolderPathAbs);
	}

	bool operator!=(const FProjectCleanerTreeViewItem& Other) const
	{
		return !FolderPathAbs.Equals(Other.FolderPathAbs);
	}
};
