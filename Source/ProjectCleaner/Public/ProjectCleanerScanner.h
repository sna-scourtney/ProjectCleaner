﻿// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProjectCleanerTypes.h"

class FAssetRegistryModule;
class UProjectCleanerScanSettings;
struct FProjectCleanerIndirectAsset;

// Responsible for scanning Content folder of project based on specified settings, all blueprint exposed functions should get data from here
struct FProjectCleanerScanner
{
	FProjectCleanerScanner();
	
	void Scan(const TWeakObjectPtr<UProjectCleanerScanSettings>& InScanSettings);
	void Reset();

	void GetSubFolders(const FString& InFolderPathAbs, TSet<FString>& SubFolders);

	int32 GetFoldersTotalNum(const FString& InFolderPathAbs) const;
	int32 GetFoldersEmptyNum(const FString& InFolderPathAbs) const;
	int32 GetAssetsTotalNum(const FString& InFolderPathAbs) const;
	int32 GetAssetsUnusedNum(const FString& InFolderPathAbs) const;
	
	int64 GetSizeTotal(const FString& InFolderPathAbs) const;
	int64 GetSizeUnused(const FString& InFolderPathAbs) const;

	bool IsEmptyFolder(const FString& InFolderPathAbs) const;
	bool IsExcludedFolder(const FString& InFolderPathAbs) const;

	
	const TMap<FString, FProjectCleanerFolderInfo>& GetFoldersTreeInfo();
	const TSet<FString>& GetForbiddenFoldersToScan();
private:
	void FindBlacklistAssets();
	void FindPrimaryAssets();
	void FindExcludedAssets();
	void FindIndirectAssets();
	void FindWithExternalRefsAssets();
	void CreateFoldersInfoTree();
	
	// All folders inside Content folder minus Forbidden folders (Engine Specific folders like Collections, WorldPartition folders that we must not scan)
	TSet<FString> FoldersAll;
	TSet<FString> FoldersEmpty;
	TSet<FString> FoldersForbiddenToScan;
	TSet<FString> FoldersForbiddenToDelete;
	TMap<FString, FProjectCleanerFolderInfo> FoldersTree;

	TSet<FString> FilesCorrupted;
	TSet<FString> FilesNonEngine;

	TArray<FAssetData> AssetsAll;
	TArray<FAssetData> AssetsUsed;
	TArray<FAssetData> AssetsUnused;
	TArray<FAssetData> AssetsPrimary;
	TArray<FAssetData> AssetsExcluded;
	TArray<FAssetData> AssetsIndirect;
	TArray<FProjectCleanerIndirectAsset> AssetsIndirectInfo;
	TArray<FAssetData> AssetsWithExternalRefs;
	// blacklist assets are considered used (like editor utility assets)
	TArray<FAssetData> AssetsBlacklist;
	
	bool bSilentMode = false;
	TWeakObjectPtr<UProjectCleanerScanSettings> ScanSettings;
	FAssetRegistryModule* ModuleAssetRegistry = nullptr;
};
