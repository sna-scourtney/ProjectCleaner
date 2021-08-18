﻿// Copyright 2021. Ashot Barkhudaryan. All Rights Reserved.

#include "Core/ProjectCleanerDataManager.h"
#include "ProjectCleaner.h"
#include "Core/ProjectCleanerUtility.h"
#include "UI/ProjectCleanerNotificationManager.h"
// Engine Headers
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetToolsModule.h"
#include "AssetViewUtils.h"
#include "ObjectTools.h"
#include "PackageTools.h"
#include "AssetRegistry/AssetData.h"
#include "Engine/AssetManager.h"
#include "Engine/AssetManagerSettings.h"
#include "Engine/MapBuildDataRegistry.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "Misc/ScopedSlowTask.h"
#include "GenericPlatform/GenericPlatformFile.h"
#include "HAL/PlatformFilemanager.h"
#include "Internationalization/Regex.h"
#include "Settings/ContentBrowserSettings.h"
#include "ShaderCompiler.h"
#include "Animation/AnimCompressionDerivedDataPublic.h"

FProjectCleanerDataManager::FProjectCleanerDataManager() :
	bSilentMode(false),
	bScanDeveloperContents(false),
	bAutomaticallyDeleteEmptyFolders(true),
	bCancelledByUser(false),
	AssetRegistry(nullptr),
	AssetTools(nullptr),
	PlatformFile(nullptr),
	RelativeRoot(TEXT("/Game"))
{
	AssetRegistry = &FModuleManager::LoadModuleChecked<FAssetRegistryModule>(AssetRegistryConstants::ModuleName);
	AssetTools = &FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools"));
	PlatformFile = &FPlatformFileManager::Get().GetPlatformFile();

	ensure(AssetRegistry && AssetTools && PlatformFile);
}

FProjectCleanerDataManager::~FProjectCleanerDataManager()
{
	AssetRegistry = nullptr;
}

void FProjectCleanerDataManager::AnalyzeProject()
{
	if (IsLoadingAssets(false)) return;
	
	FixupRedirectors();
	ProjectCleanerUtility::SaveAllAssets(!bSilentMode);
	FindAllAssets();
	FindInvalidFilesAndAssets();
	FindIndirectAssets();
	FindEmptyFolders(bScanDeveloperContents);
	FindPrimaryAssetClasses();
	FindAssetsWithExternalReferencers();
	FindUnusedAssets();
}

void FProjectCleanerDataManager::PrintInfo()
{
	UE_LOG(LogProjectCleaner, Display, TEXT("All Assets - %d"), AllAssets.Num());
	UE_LOG(LogProjectCleaner, Display, TEXT("Unused Assets - %d"), UnusedAssets.Num());
	UE_LOG(LogProjectCleaner, Display, TEXT("Corrupted Assets - %d"), CorruptedAssets.Num());
	UE_LOG(LogProjectCleaner, Display, TEXT("Non Engine Files - %d"), NonEngineFiles.Num());
	UE_LOG(LogProjectCleaner, Display, TEXT("IndirectAssets - %d"), IndirectAssets.Num());
	UE_LOG(LogProjectCleaner, Display, TEXT("Empty Folders - %d"), EmptyFolders.Num());
	UE_LOG(LogProjectCleaner, Display, TEXT("Excluded Assets - %d"), ExcludedAssets.Num());
}

void FProjectCleanerDataManager::SetExcludeClasses(const TArray<FString>& Classes)
{
	for (const auto& ClassName : Classes)
	{
		ExcludedClasses.Add(FName{*ClassName});
	}
}

void FProjectCleanerDataManager::SetExcludePaths(const TArray<FString>& Paths)
{
	for (const auto& Path : Paths)
	{
		ExcludedPaths.Add(FName{*Path});
	}
}

void FProjectCleanerDataManager::SetUserExcludedAssets(const TArray<FString>& Assets)
{
	for (const auto& Asset : Assets)
	{
		const FString ObjectPath = Asset + TEXT(".") + FPaths::GetBaseFilename(*Asset);
		const auto& AssetData = AssetRegistry->Get().GetAssetByObjectPath(FName{*ObjectPath});
		if (!AssetData.IsValid()) continue;

		UserExcludedAssets.AddUnique(AssetData);
	}
}

void FProjectCleanerDataManager::ExcludeSelectedAssets(const TArray<FAssetData>& Assets)
{
	if (Assets.Num() == 0) return;
	
	for (const auto& Asset : Assets)
	{
		UserExcludedAssets.AddUnique(Asset);
	}
}

void FProjectCleanerDataManager::ExcludeSelectedAssetsByType(const TArray<FAssetData>& Assets)
{
	if (Assets.Num() == 0) return;

	for (const auto& Asset : Assets)
	{
		ExcludedClasses.Add(ProjectCleanerUtility::GetClassName(Asset));
	}
}

bool FProjectCleanerDataManager::IncludeSelectedAssets(const TArray<FAssetData>& Assets)
{
	if (Assets.Num() == 0)
	{
		return false;
	}
	
	bool bHasConflictWithFilters = false;
	for (const auto& Asset : Assets)
	{
		if (IsExcludedByClass(Asset) || IsExcludedByPath(Asset))
		{
			bHasConflictWithFilters = true;
		}
	}

	if (bHasConflictWithFilters)
	{
		return false;
	}

	UserExcludedAssets.RemoveAllSwap([&] (const FAssetData& Asset)
	{
		return Assets.Contains(Asset);
	}, false);
	UserExcludedAssets.Shrink();

	return true;
}

void FProjectCleanerDataManager::IncludeAllAssets()
{
	ExcludedPaths.Empty();
	ExcludedClasses.Empty();
	UserExcludedAssets.Empty();
	ExcludedAssets.Empty();
}

bool FProjectCleanerDataManager::ExcludePath(const FString& InPath)
{
	if (InPath.IsEmpty()) return false;
	
	ExcludedPaths.Add(FName{*InPath});

	return true;
}

bool FProjectCleanerDataManager::IncludePath(const FString& InPath)
{
	if (InPath.IsEmpty()) return false;

	for (const auto& ExcludedPath : ExcludedPaths)
	{
		if (ExcludedPath.ToString().Equals(InPath)) continue;
		
		if (InPath.StartsWith(ExcludedPath.ToString()))
		{
			return false;
		}
	}
	
	ExcludedPaths.Remove(FName{*InPath});

	return true;
}

int32 FProjectCleanerDataManager::DeleteSelectedAssets(const TArray<FAssetData>& Assets)
{
	return ObjectTools::DeleteAssets(Assets);
}

int32 FProjectCleanerDataManager::DeleteAllUnusedAssets()
{
	constexpr int32 BucketSize = 500;
	int32 Deleted = 0;
	const int32 Total = UnusedAssets.Num();
	
	TArray<FAssetData> Bucket;
	TArray<UObject*> LoadedAssets;
	LoadedAssets.Reserve(BucketSize);
	Bucket.Reserve(BucketSize);

	FScopedSlowTask DeleteSlowTask(
		UnusedAssets.Num(),
		FText::FromString(FStandardCleanerText::DeletingUnusedAssets)
	);
	DeleteSlowTask.MakeDialog(true);
	
	while (UnusedAssets.Num() > 0)
	{
		if (DeleteSlowTask.ShouldCancel())
		{
			bCancelledByUser = true;
			break;
		}
		
		FillBucketWithAssets(Bucket,BucketSize);

		if (Bucket.Num() == 0)
		{
			break;
		}

		if (!PrepareBucketForDeletion(Bucket, LoadedAssets))
		{
			UE_LOG(LogProjectCleaner, Error, TEXT("Failed to load some assets. Aborting."))
			break;
		}

		Deleted += DeleteBucket(LoadedAssets);
		DeleteSlowTask.EnterProgressFrame(Bucket.Num(),ProjectCleanerUtility::GetDeletionProgressText(Deleted, Total, false));

		Bucket.Reset();
		LoadedAssets.Reset();
	}
	
	// Cleaning empty packages
	const TSet<FName> EmptyPackages = AssetRegistry->Get().GetCachedEmptyPackages();
	TArray<UPackage*> AssetPackages;
	for (const auto& EmptyPackage : EmptyPackages)
	{
		UPackage* Package = FindPackage(nullptr, *EmptyPackage.ToString());
		if (Package && Package->IsValidLowLevel())
		{
			AssetPackages.Add(Package);
		}
	}
	
	if (AssetPackages.Num() > 0)
	{
		ObjectTools::CleanupAfterSuccessfulDelete(AssetPackages);
	}
	
	CleanupAfterDelete();

	return Deleted;

	
	// if (AssetData.IsValid())
	// {
	// 	
	// 	// get asset
	// 	// while (Refs.Num() != 0)
	// 	// {
	// 	// 	AssetRegistry->Get().GetReferencers(AssetData.PackageName, Refs);
	// 	//
	// 	// 	for (const auto& Ref : Refs)
	// 	// 	{
	// 	// 		
	// 	// 	}
	// 	//
	// 	// 	Refs.Reset();
	// 	// }
	//
	// 	
	//
	// 	UE_LOG(LogProjectCleaner, Warning, TEXT("A"));
	// }
	// TArray<FString> ObjectPaths;
	// ObjectPaths.Reserve(UnusedAssets.Num());
	//
	// TArray<UObject*> LoadedAssets;
	// LoadedAssets.Reserve(UnusedAssets.Num());
	//
	// for (const auto& Asset : UnusedAssets)
	// {
	// 	ObjectPaths.Add(Asset.ObjectPath.ToString());
	// }
	//
	// if (AssetViewUtils::LoadAssetsIfNeeded(ObjectPaths, LoadedAssets, false, true))
	// {
	// 	return ObjectTools::DeleteObjects(LoadedAssets, false);		
	// }
	//
	// UE_LOG(LogProjectCleaner, Error, TEXT("Failed to load some assets"));
	// return 0;
	
	// FScopedSlowTask DeleteSlowTask(
	// 	UnusedAssets.Num(),
	// 	FText::FromString(FStandardCleanerText::DeletingUnusedAssets)
	// );
	// DeleteSlowTask.MakeDialog(true);

	// bool bCompileStopped = false;
	// while (UnusedAssets.Num() > 0)
	// {
	// 	if (DeleteSlowTask.ShouldCancel())
	// 	{
	// 		bCancelledByUser = true;
	// 		break;
	// 	}
	// 	
	// 	FillBucketWithAssets(Bucket, BucketSize);
	//
	// 	if (PrepareBucketForDeletion(Bucket,LoadedAssets))
	// 	{
	// 		const int32 Deleted = DeleteBucket(LoadedAssets);
	// 		if (Deleted != LoadedAssets.Num())
	// 		{
	// 			UE_LOG(LogProjectCleaner, Error, TEXT("Failed to delete some assets"));
	// 			DeleteSlowTask.EnterProgressFrame(UnusedAssets.Num());
	// 			break;
	// 		}
	// 		
	// 		DeletedNum += Deleted;
	// 		DeleteSlowTask.EnterProgressFrame(
	// 			Deleted,
	// 			ProjectCleanerUtility::GetDeletionProgressText(DeletedNum, Total, false)
	// 		);
	// 	}
	// 	else
	// 	{
	// 		UE_LOG(LogProjectCleaner, Error, TEXT("Failed to load some assets"));
	// 		break;
	// 	}
	// 	
	// 	LoadedAssets.Reset();
	// 	Bucket.Reset();
	// }
	
	// {
	// 	FScopedSlowTask AssetPrepareTask(
	// 		UnusedAssets.Num(),
	// 		FText::FromString(FStandardCleanerText::PreparingAssetsForDeletion)
	// 	);
	// 	AssetPrepareTask.MakeDialog(true);
	// 	for (const auto& Asset : UnusedAssets)
	// 	{
	// 		if (AssetPrepareTask.ShouldCancel())
	// 		{
	// 			bCancelledByUser = true;
	// 			break;
	// 		}
	//
	// 		const FString ProgressText = FString::Printf(TEXT("%s[%s]"),
	// 			FStandardCleanerText::PreparingAssetsForDeletion, *Asset.AssetName.ToString());
	// 		AssetPrepareTask.EnterProgressFrame(1.0, FText::FromString(ProgressText));
	// 		
	// 		if (Asset.AssetClass.IsEqual(UAnimSequence::StaticClass()->GetFName()))
	// 		{
	// 			UObject* AssetObject = Asset.GetAsset();
	// 			UAnimSequence* Seq = Cast<UAnimSequence>(AssetObject);
	// 			if (Seq && Seq->DoesNeedRecompress())
	// 			{
	// 				Seq->RequestAsyncAnimRecompression();
	// 				if (GAsyncCompressedAnimationsTracker)
	// 				{
	// 					GAsyncCompressedAnimationsTracker->WaitOnExistingCompression(Seq, true);
	// 				}
	// 			}
	// 		}
	// 	}
	// }
	//
	// constexpr int32 BucketSize = 20; // how many assets delete at a time
	// int32 DeletedAssetsNum = 0;
	// const int32 TotalNum = UnusedAssets.Num();
	//
	// if (IsRunningCommandlet())
	// {
	// 	TArray<FAssetData> Bucket;
	// 	Bucket.Reserve(BucketSize);
	//
	// 	while (UnusedAssets.Num() > 0)
	// 	{
	// 		FillBucketWithAssets(Bucket,BucketSize);
	// 	
	// 		TArray<UObject*> LoadedAssets;
	// 		LoadedAssets.Reserve(Bucket.Num());
	// 		if (PrepareBucketForDeletion(Bucket, LoadedAssets))
	// 		{
	// 			DeletedAssetsNum += DeleteBucket(LoadedAssets);
	// 		
	// 			UnusedAssets.RemoveAllSwap([&] (const FAssetData& Asset)
	// 			{
	// 				return Bucket.Contains(Asset); 
	// 			}, false);
	// 		
	// 			Bucket.Reset();
	// 		}
	// 		else
	// 		{
	// 			break;
	// 		}
	// 	}
	// 	
	// 	return DeletedAssetsNum;
	// }
	//
	// if (bCancelledByUser)
	// {
	// 	AnalyzeProject();
	// }
	//
	// TArray<FAssetData> Bucket;
	// Bucket.Reserve(BucketSize);
	//
	// FScopedSlowTask DeleteSlowTask(
	// 	UnusedAssets.Num(),
	// 	FText::FromString(FStandardCleanerText::DeletingUnusedAssets)
	// );
	// DeleteSlowTask.MakeDialog(true);
	//
	// while (UnusedAssets.Num() > 0)
	// {
	// 	if (DeleteSlowTask.ShouldCancel())
	// 	{
	// 		bCancelledByUser = true;
	// 		break;
	// 	}
	// 	
	// 	FillBucketWithAssets(Bucket,BucketSize);
	// 	
	// 	TArray<UObject*> LoadedAssets;
	// 	LoadedAssets.Reserve(Bucket.Num());
	// 	if (PrepareBucketForDeletion(Bucket, LoadedAssets))
	// 	{
	// 		DeletedAssetsNum += DeleteBucket(LoadedAssets);
	// 		DeleteSlowTask.EnterProgressFrame(
	// 			Bucket.Num(),
	// 			ProjectCleanerUtility::GetDeletionProgressText(DeletedAssetsNum, TotalNum, false)
	// 		);
	// 		
	// 		UnusedAssets.RemoveAllSwap([&] (const FAssetData& Asset)
	// 		{
	// 			return Bucket.Contains(Asset); 
	// 		}, false);
	// 		
	// 		Bucket.Reset();
	// 	}
	// 	else
	// 	{
	// 		DeleteSlowTask.EnterProgressFrame(UnusedAssets.Num());
	// 		break;
	// 		// todo:ashe23
	// 	}
	// }
	//
	// // Cleaning empty packages
	// const TSet<FName> EmptyPackages = AssetRegistry->Get().GetCachedEmptyPackages();
	// TArray<UPackage*> AssetPackages;
	// for (const auto& EmptyPackage : EmptyPackages)
	// {
	// 	UPackage* Package = FindPackage(nullptr, *EmptyPackage.ToString());
	// 	if (Package && Package->IsValidLowLevel())
	// 	{
	// 		AssetPackages.Add(Package);
	// 	}
	// }
	//
	// if (AssetPackages.Num() > 0)
	// {
	// 	ObjectTools::CleanupAfterSuccessfulDelete(AssetPackages);
	// }
	//
	// CleanupAfterDelete();
	//
	// return DeletedAssetsNum;
}

int32 FProjectCleanerDataManager::DeleteEmptyFolders()
{
	FindEmptyFolders(bScanDeveloperContents);
	
	if (EmptyFolders.Num() == 0)
	{
		return 0;
	}

	int32 DeletedFoldersNum = 0;
	FScopedSlowTask DeleteSlowTask(
		EmptyFolders.Num(),
		FText::FromString(FStandardCleanerText::DeletingEmptyFolders)
	);
	DeleteSlowTask.MakeDialog(true);
	
	for (auto& EmptyFolder : EmptyFolders)
	{
		const FString EmptyFolderStr = EmptyFolder.ToString();
		const FString DeletionText = FString::Printf(
			TEXT("Deleted %d of %d empty folders [%s]"),DeletedFoldersNum,EmptyFolders.Num(),*EmptyFolderStr);
		DeleteSlowTask.EnterProgressFrame(1.0, FText::FromString(DeletionText));
		
		if (!IFileManager::Get().DirectoryExists(*EmptyFolderStr)) continue;

		if (!IFileManager::Get().DeleteDirectory(*EmptyFolderStr, false, true))
		{
			UE_LOG(LogProjectCleaner, Error, TEXT("Failed to delete %s folder."), *EmptyFolderStr);
			continue;
		}
		
		++DeletedFoldersNum;
		// removing folder path from asset registry
		AssetRegistry->Get().RemovePath(ProjectCleanerUtility::ConvertAbsolutePathToInternal(EmptyFolderStr));
	}

	CleanupAfterDelete();

	return DeletedFoldersNum;
}

const FAssetRegistryModule* FProjectCleanerDataManager::GetAssetRegistry() const
{
	return AssetRegistry;
}

const TArray<FAssetData>& FProjectCleanerDataManager::GetAllAssets() const
{
	return AllAssets;
}

const TArray<FAssetData>& FProjectCleanerDataManager::GetUnusedAssets() const
{
	return UnusedAssets;
}

const TSet<FName>& FProjectCleanerDataManager::GetExcludedAssets() const
{
	return ExcludedAssets;
}

const TSet<FName>& FProjectCleanerDataManager::GetCorruptedAssets() const
{
	return CorruptedAssets;
}

const TSet<FName>& FProjectCleanerDataManager::GetNonEngineFiles() const
{
	return NonEngineFiles;
}

const TMap<FAssetData, FIndirectAsset>& FProjectCleanerDataManager::GetIndirectAssets() const
{
	return IndirectAssets;
}

const TSet<FName>& FProjectCleanerDataManager::GetEmptyFolders() const
{
	return EmptyFolders;
}

const TSet<FName>& FProjectCleanerDataManager::GetPrimaryAssetClasses() const
{
	return PrimaryAssetClasses;
}

void FProjectCleanerDataManager::SetCleanerConfigs(const UCleanerConfigs* CleanerConfigs)
{
	if (!CleanerConfigs) return;

	bScanDeveloperContents = CleanerConfigs->bScanDeveloperContents;
	
	const auto Settings = GetMutableDefault<UContentBrowserSettings>();
	Settings->SetDisplayDevelopersFolder(bScanDeveloperContents);
	Settings->PostEditChange();
	
	bAutomaticallyDeleteEmptyFolders = CleanerConfigs->bAutomaticallyDeleteEmptyFolders;

	ExcludedPaths.Empty();
	ExcludedClasses.Empty();
	ExcludedPaths.Reserve(CleanerConfigs->Paths.Num());
	ExcludedClasses.Reserve(CleanerConfigs->Classes.Num());
	
	for (const auto& DirectoryPath : CleanerConfigs->Paths)
	{
		if (DirectoryPath.Path.IsEmpty()) continue;
		
		ExcludedPaths.Add(FName{*DirectoryPath.Path});
	}
	for (const auto& ExcludedClass : CleanerConfigs->Classes)
	{
		if (!ExcludedClass) continue;
		ExcludedClasses.Add(ExcludedClass->GetFName());
	}
}

void FProjectCleanerDataManager::SetSilentMode(const bool SilentMode)
{
	bSilentMode = SilentMode;
}

void FProjectCleanerDataManager::SetScanDeveloperContents(const bool bScan)
{
	bScanDeveloperContents = bScan;
}

// PRIVATE Functions
void FProjectCleanerDataManager::FixupRedirectors() const
{
	FScopedSlowTask FixRedirectorsTask{
		1.0f,
		FText::FromString(FStandardCleanerText::FixingUpRedirectors)
	};
	FixRedirectorsTask.MakeDialog();
	
	FARFilter Filter;
	Filter.bRecursivePaths = true;
	Filter.PackagePaths.Emplace(RelativeRoot);
	Filter.ClassNames.Emplace(UObjectRedirector::StaticClass()->GetFName());

	// Getting all redirectors in project
	TArray<FAssetData> AssetList;
	AssetRegistry->Get().GetAssets(Filter, AssetList);
	
	if (AssetList.Num() > 0)
	{
		FScopedSlowTask FixRedirectorsLoadingTask(
			AssetList.Num(),
			FText::FromString(FStandardCleanerText::LoadingAssets)
		);
		FixRedirectorsLoadingTask.MakeDialog();

		TArray<UObjectRedirector*> Redirectors;
		Redirectors.Reserve(AssetList.Num());
		
		for (const auto& Asset : AssetList)
		{
			FixRedirectorsLoadingTask.EnterProgressFrame();
			
			UObject* AssetObj = Asset.GetAsset();
			if (!AssetObj) continue;

			const auto Redirector = CastChecked<UObjectRedirector>(AssetObj);
			if (!Redirector) continue;

			Redirectors.Add(Redirector);
		}

		Redirectors.Shrink();

		// Fix up all founded redirectors
		AssetTools->Get().FixupReferencers(Redirectors);
	}

	FixRedirectorsTask.EnterProgressFrame(1.0f);
}

void FProjectCleanerDataManager::FindAllAssets()
{
	AllAssets.Empty();
	AllAssets.Reserve(AssetRegistry->Get().GetAllocatedSize());
	AssetRegistry->Get().GetAssetsByPath(RelativeRoot, AllAssets, true);
}

void FProjectCleanerDataManager::FindInvalidFilesAndAssets()
{
	CorruptedAssets.Empty();
	NonEngineFiles.Empty();

	struct ProjectCleanerDirVisitor : IPlatformFile::FDirectoryVisitor
	{
		ProjectCleanerDirVisitor(
			const TArray<FAssetData>& Assets,
			TSet<FName>& NewCorruptedAssets,
			TSet<FName>& NewNonEngineFiles
		) :
		AllAssets(Assets),
		CorruptedAssets(NewCorruptedAssets),
		NonEngineFiles(NewNonEngineFiles) {}
		
		virtual bool Visit(const TCHAR* FilenameOrDirectory, bool bIsDirectory) override
		{
			const FString FullPath = FPaths::ConvertRelativePathToFull(FilenameOrDirectory);
			if (!bIsDirectory)
			{
				if (ProjectCleanerUtility::IsEngineExtension(FPaths::GetExtension(FullPath, false)))
				{
					// here we got absolute path "C:/MyProject/Content/material.uasset"
					// we must first convert that path to In Engine Internal Path like "/Game/material.uasset"
					const FString InternalFilePath = ProjectCleanerUtility::ConvertAbsolutePathToInternal(FullPath);
					// Converting file path to object path (This is for searching in AssetRegistry)
					// example "/Game/Name.uasset" => "/Game/Name.Name"
					FString ObjectPath = InternalFilePath;
					ObjectPath.RemoveFromEnd(FPaths::GetExtension(InternalFilePath, true));
					ObjectPath.Append(TEXT(".") + FPaths::GetBaseFilename(InternalFilePath));

					const FName ObjectPathName = FName{*ObjectPath};
					const bool IsInAssetRegistry = AllAssets.ContainsByPredicate([&] (const FAssetData& Elem)
					{
						return Elem.ObjectPath.IsEqual(ObjectPathName);
					});
					if (!IsInAssetRegistry)
					{
						CorruptedAssets.Add(ObjectPathName);
					}
				}
				else
				{
					NonEngineFiles.Add(FName{FullPath});
				}
			}

			return true;
		}
		const TArray<FAssetData>& AllAssets;
		TSet<FName>& CorruptedAssets;
		TSet<FName>& NonEngineFiles;
	};

	ProjectCleanerDirVisitor Visitor{AllAssets, CorruptedAssets, NonEngineFiles};
	FPlatformFileManager::Get().GetPlatformFile().IterateDirectoryRecursively(*FPaths::ProjectContentDir(), Visitor);
}

void FProjectCleanerDataManager::FindIndirectAssets()
{
	IndirectAssets.Empty();
	
	const FString SourceDir = FPaths::ProjectDir() + TEXT("Source/");
	const FString ConfigDir = FPaths::ProjectDir() + TEXT("Config/");
	const FString PluginsDir = FPaths::ProjectDir() + TEXT("Plugins/");
	
	TSet<FString> Files;
	Files.Reserve(200); // reserving some space
	
	// 1) finding all source files in main project "Source" directory (<yourproject>/Source/*)
	TArray<FString> FilesToScan;
	PlatformFile->FindFilesRecursively(FilesToScan, *SourceDir, TEXT(".cs"));
	PlatformFile->FindFilesRecursively(FilesToScan, *SourceDir, TEXT(".cpp"));
	PlatformFile->FindFilesRecursively(FilesToScan, *SourceDir, TEXT(".h"));
	PlatformFile->FindFilesRecursively(FilesToScan, *ConfigDir, TEXT(".ini"));
	Files.Append(FilesToScan);
	
	// 2) we should find all source files in plugins folder (<yourproject>/Plugins/*)
	TArray<FString> ProjectPluginsFiles;
	// finding all installed plugins in "Plugins" directory
	struct DirectoryVisitor : public IPlatformFile::FDirectoryVisitor
	{
		virtual bool Visit(const TCHAR* FilenameOrDirectory, bool bIsDirectory) override
		{
			if (bIsDirectory)
			{
				InstalledPlugins.Add(FilenameOrDirectory);
			}
	
			return true;
		}
	
		TArray<FString> InstalledPlugins;
	};
	
	DirectoryVisitor Visitor;
	PlatformFile->IterateDirectory(*PluginsDir, Visitor);
	
	// 3) for every installed plugin we scanning only "Source" and "Config" folders
	for (const auto& Dir : Visitor.InstalledPlugins)
	{
		const FString PluginSourcePathDir = Dir + "/Source";
		const FString PluginConfigPathDir = Dir + "/Config";
	
		PlatformFile->FindFilesRecursively(ProjectPluginsFiles, *PluginSourcePathDir, TEXT(".cs"));
		PlatformFile->FindFilesRecursively(ProjectPluginsFiles, *PluginSourcePathDir, TEXT(".cpp"));
		PlatformFile->FindFilesRecursively(ProjectPluginsFiles, *PluginSourcePathDir, TEXT(".h"));
		PlatformFile->FindFilesRecursively(ProjectPluginsFiles, *PluginConfigPathDir, TEXT(".ini"));
	}
	
	Files.Append(ProjectPluginsFiles);
	Files.Shrink();

	for (const auto& File : Files)
	{
		if (!PlatformFile->FileExists(*File)) continue;
	
		FString FileContent;
		FFileHelper::LoadFileToString(FileContent, *File);
		
		if (!ProjectCleanerUtility::HasIndirectlyUsedAssets(FileContent)) continue;

		static FRegexPattern Pattern(TEXT(R"(\/Game(.*)\b)"));
		FRegexMatcher Matcher(Pattern, FileContent);
		while (Matcher.FindNext())
		{
			const FName FoundedAssetObjectPath =  FName{Matcher.GetCaptureGroup(0)};
			if (!FoundedAssetObjectPath.IsValid()) continue;

			const FAssetData* AssetData = AllAssets.FindByPredicate([&] (const FAssetData& Elem)
			{
				return
					Elem.ObjectPath.IsEqual(FoundedAssetObjectPath) ||
					Elem.PackageName.IsEqual(FoundedAssetObjectPath);
			});

			if (!AssetData) continue;
			
			// if founded asset is ok, we loading file by lines to determine on what line its used
			TArray<FString> Lines;
			FFileHelper::LoadFileToStringArray(Lines, *File);
			for (int32 i = 0; i < Lines.Num(); ++i)
			{
				if (!Lines.IsValidIndex(i)) continue;
				if (!Lines[i].Contains(FoundedAssetObjectPath.ToString())) continue;
			
				FIndirectAsset IndirectAsset;
				IndirectAsset.File = FPaths::ConvertRelativePathToFull(File);
				IndirectAsset.RelativePath = AssetData->PackagePath;
				IndirectAsset.Line = i + 1;
				IndirectAssets.Add(*AssetData, IndirectAsset);
			}
		}
	}
}

void FProjectCleanerDataManager::FindEmptyFolders(const bool bScanDevelopersContent)
{
	EmptyFolders.Empty();
	
	ProjectCleanerUtility::FindEmptyFoldersInPath(FPaths::ProjectContentDir() / TEXT("*"), EmptyFolders);

	const FString CollectionsFolder = FPaths::ProjectContentDir() + TEXT("Collections/");
	const FString DevelopersFolder = FPaths::ProjectContentDir() + TEXT("Developers/");
	const FString UserDir = DevelopersFolder + FPaths::GameUserDeveloperFolderName() + TEXT("/");
	const FString UserCollectionsDir = UserDir + TEXT("Collections/");
	
	EmptyFolders.Remove(FName{*CollectionsFolder});
	EmptyFolders.Remove(FName{*DevelopersFolder});
	EmptyFolders.Remove(FName{*UserDir});
	EmptyFolders.Remove(FName{*UserCollectionsDir});

	if (!bScanDevelopersContent)
	{
		// find all folders that are under developer folders
		TSet<FName> FilteredFolders;
		FilteredFolders.Reserve(EmptyFolders.Num());

		for (const auto& Folder : EmptyFolders)
		{
			if (
				FPaths::IsUnderDirectory(Folder.ToString(), CollectionsFolder) ||
				FPaths::IsUnderDirectory(Folder.ToString(), DevelopersFolder)
			)
			{
				FilteredFolders.Add(Folder);
			}
		}

		for (const auto& Folder : FilteredFolders)
		{
			EmptyFolders.Remove(Folder);
		}
	}
}

void FProjectCleanerDataManager::FindPrimaryAssetClasses()
{
	PrimaryAssetClasses.Empty();
	
	const auto& AssetManager = UAssetManager::Get();
	if (!AssetManager.IsValid()) return;
	
	TArray<FPrimaryAssetTypeInfo> AssetTypeInfos;
	AssetManager.Get().GetPrimaryAssetTypeInfoList(AssetTypeInfos);

	for (const auto& AssetTypeInfo : AssetTypeInfos)
	{
		UClass* AssetTypeCLass = AssetTypeInfo.AssetBaseClassLoaded;
		if (!AssetTypeCLass) continue;
		FName ClassName = AssetTypeCLass->GetFName();
		PrimaryAssetClasses.Add(ClassName);
	}
}

void FProjectCleanerDataManager::FindAssetsWithExternalReferencers()
{
	AssetsWithExternalRefs.Empty();
	TArray<FName> Refs;
	for (const auto& Asset : AllAssets)
	{
		AssetRegistry->Get().GetReferencers(Asset.PackageName, Refs);

		const bool HasExternalRefs = Refs.ContainsByPredicate([](const FName& Ref)
		{
			return !Ref.ToString().StartsWith(TEXT("/Game"));
		});

		if (HasExternalRefs)
		{
			AssetsWithExternalRefs.AddUnique(Asset);
		}

		Refs.Reset();
	}
}

void FProjectCleanerDataManager::FindUnusedAssets()
{
	UnusedAssets.Empty();
	UnusedAssets.Reserve(AllAssets.Num());
	ExcludedAssets.Empty();
	ExcludedAssets.Reserve(AllAssets.Num());

	TSet<FName> UsedAssets;
	UsedAssets.Reserve(AllAssets.Num());
	FindUsedAssets(UsedAssets);
	UsedAssets.Shrink();

	FindExcludedAssets(UsedAssets);

	TSet<FName> UsedAssetsDependencies;
	UsedAssetsDependencies.Reserve(AllAssets.Num());
	FindUsedAssetsDependencies(UsedAssets, UsedAssetsDependencies);

	const bool IsMegascansLoaded = FModuleManager::Get().IsModuleLoaded("MegascansPlugin");
	for (const auto& Asset : AllAssets)
	{
		if (UsedAssetsDependencies.Contains(Asset.PackageName)) continue;
		if (PrimaryAssets.Contains(Asset)) continue;
		if (IsMegascansLoaded && ProjectCleanerUtility::IsUnderMegascansFolder(Asset)) continue;
		
		UnusedAssets.Add(Asset);
	}
	UnusedAssets.Shrink();
}

void FProjectCleanerDataManager::FindUsedAssets(TSet<FName>& UsedAssets)
{
	FARFilter Filter;
	Filter.bRecursiveClasses = true;
	Filter.bRecursivePaths = true;
	Filter.PackagePaths.Add(TEXT("/Game"));
	Filter.ClassNames.Append(PrimaryAssetClasses.Array());
	Filter.ClassNames.Add(UMapBuildDataRegistry::StaticClass()->GetFName());

	AssetRegistry->Get().GetAssets(Filter, PrimaryAssets);

	for (const auto& Asset : PrimaryAssets)
	{
		UsedAssets.Add(Asset.PackageName);
	}

	for (const auto& Asset : IndirectAssets)
	{
		UsedAssets.Add(Asset.Key.PackageName);
	}

	for (const auto& Asset : AssetsWithExternalRefs)
	{
		UsedAssets.Add(Asset.PackageName);
	}

	if (!bScanDeveloperContents)
	{
		TArray<FAssetData> AssetsInDeveloperFolder;
		AssetRegistry->Get().GetAssetsByPath(TEXT("/Game/Developers"), AssetsInDeveloperFolder, true);

		for (const auto& Asset : AssetsInDeveloperFolder)
		{
			UsedAssets.Add(Asset.PackageName);
		}
	}
}

void FProjectCleanerDataManager::FindUsedAssetsDependencies(const TSet<FName>& UsedAssets, TSet<FName>& UsedAssetsDeps) const
{
	TArray<FName> Stack;
	for (const auto& Asset : UsedAssets)
	{
		UsedAssetsDeps.Add(Asset);
		Stack.Add(Asset);

		TArray<FName> Deps;
		while (Stack.Num() > 0)
		{
			const auto CurrentPackageName = Stack.Pop(false);
			Deps.Reset();

			AssetRegistry->Get().GetDependencies(CurrentPackageName, Deps);

			Deps.RemoveAllSwap([&] (const FName& Dep)
			{
				return !Dep.ToString().StartsWith(*RelativeRoot.ToString());
			}, false);

			for (const auto& Dep : Deps)
			{
				bool bIsAlreadyInSet = false;
				UsedAssetsDeps.Add(Dep, &bIsAlreadyInSet);
				if (!bIsAlreadyInSet)
				{
					Stack.Add(Dep);
				}
			}
		}
	}
}

void FProjectCleanerDataManager::FindExcludedAssets(TSet<FName>& UsedAssets)
{
	// excluded by user
	for (const auto& Asset : UserExcludedAssets)
	{
		UsedAssets.Add(Asset.PackageName);

		if (!PrimaryAssets.Contains(Asset))
		{
			ExcludedAssets.Add(Asset.PackageName);
		}
	}

	// excluded by path
	{
		TArray<FAssetData> AllExcludedAssets;
	
		FARFilter Filter;		
		Filter.bRecursivePaths = true;
	
		for (const auto& ExcludedPath : ExcludedPaths)
		{
			Filter.PackagePaths.Add(ExcludedPath);
		}

		AssetRegistry->Get().GetAssets(Filter, AllExcludedAssets);

		for (const auto& Asset : AllExcludedAssets)
		{
			UsedAssets.Add(Asset.PackageName);
			if (!PrimaryAssets.Contains(Asset))
			{
				ExcludedAssets.Add(Asset.PackageName);
			}
		}
	}

	// excluded by class
	for (const auto& Asset : AllAssets)
	{
		if (IsExcludedByClass(Asset))
		{
			UsedAssets.Add(Asset.PackageName);
			if (!PrimaryAssets.Contains(Asset))
			{
				ExcludedAssets.Add(Asset.PackageName);
			}
		}
	}
}

void FProjectCleanerDataManager::AnalyzeUnusedAssets()
{
	UnusedAssetsInfos.Empty();
	UnusedAssetsInfos.Reserve(UnusedAssets.Num());
	
	TArray<FName> Refs;
	TArray<FName> Deps;

	FScopedSlowTask AnalyzeTask(
		UnusedAssets.Num(),
		FText::FromString(FStandardCleanerText::AnalyzingAssets)
	);
	
	for (const auto& UnusedAsset : UnusedAssets)
	{
		AnalyzeTask.EnterProgressFrame();
		
		FUnusedAssetInfo UnusedAssetInfo;

		AssetRegistry->Get().GetReferencers(UnusedAsset.PackageName, Refs);
		AssetRegistry->Get().GetDependencies(UnusedAsset.PackageName, Deps);
		
		Refs.RemoveAllSwap([&] (const FName& Ref)
		{
			return !Ref.ToString().StartsWith(RelativeRoot.ToString()) || Ref.IsEqual(UnusedAsset.PackageName);
		}, false);

		Deps.RemoveAllSwap([&] (const FName& Dep)
		{
			return !Dep.ToString().StartsWith(RelativeRoot.ToString()) || Dep.IsEqual(UnusedAsset.PackageName);
		}, false);

		Refs.Shrink();
		Deps.Shrink();

		for (const auto& Ref : Refs)
		{
			UnusedAssetInfo.Refs.Add(Ref);
		}
		for (const auto& Dep : Deps)
		{
			UnusedAssetInfo.Deps.Add(Dep);
		}

		TSet<FName> CommonAssets = UnusedAssetInfo.Refs.Intersect(UnusedAssetInfo.Deps);
		if (CommonAssets.Num() > 0)
		{
			UnusedAssetInfo.UnusedAssetType = EUnusedAssetType::CircularAsset;
			
			for (const auto& CommonAsset : CommonAssets)
			{
				const FString ObjectPath = CommonAsset.ToString() + TEXT(".") + FPaths::GetBaseFilename(*CommonAsset.ToString());
				const FAssetData AssetData = AssetRegistry->Get().GetAssetByObjectPath(FName{*ObjectPath});
				if (AssetData.IsValid())
				{
					UnusedAssetInfo.CommonAssets.Add(AssetData);
				}
			}
		}

		if (Refs.Num() == 0)
		{
			UnusedAssetInfo.UnusedAssetType = EUnusedAssetType::RootAsset;
		}

		UnusedAssetsInfos.Add(UnusedAsset, UnusedAssetInfo);
		
		Refs.Reset();
		Deps.Reset();
	}
}

void FProjectCleanerDataManager::FillBucketWithAssets(TArray<FAssetData>& Bucket, const int32 BucketSize)
{
	// AnalyzeUnusedAssets();
	
	// Searching Root assets
	int32 Index = 0;
	TArray<FName> Refs;
	while (Bucket.Num() < BucketSize && UnusedAssets.IsValidIndex(Index))
	{
		const FAssetData CurrentAsset = UnusedAssets[Index];
		AssetRegistry->Get().GetReferencers(CurrentAsset.PackageName, Refs);
		Refs.RemoveAllSwap([&] (const FName& Ref)
		{
			return !Ref.ToString().StartsWith(RelativeRoot.ToString()) || Ref.IsEqual(CurrentAsset.PackageName);
		}, false);
		Refs.Shrink();

		if (Refs.Num() == 0)
		{
			Bucket.AddUnique(CurrentAsset);
			UnusedAssets.RemoveAt(Index);
		}

		Refs.Reset();
		
		++Index;
	}
		
	if (Bucket.Num() > 0)
	{
		return;
	}

	if (UnusedAssets.Num() == 0)
	{
		return;
	}
	
	TArray<FAssetData> Stack;
	Stack.Add(UnusedAssets[0]);
	
	while (Stack.Num() > 0)
	{
		const FAssetData Current = Stack.Pop(false);
		Bucket.AddUnique(Current);
		UnusedAssets.Remove(Current);
		
		AssetRegistry->Get().GetReferencers(Current.PackageName, Refs);
		
		Refs.RemoveAllSwap([&] (const FName& Ref)
		{
			return !Ref.ToString().StartsWith(RelativeRoot.ToString()) || Ref.IsEqual(Current.PackageName);
		}, false);
		Refs.Shrink();
	
		for (const auto& Ref : Refs)
		{
			const FString ObjectPath = Ref.ToString() + TEXT(".") + FPaths::GetBaseFilename(*Ref.ToString());
			const FAssetData AssetData = AssetRegistry->Get().GetAssetByObjectPath(FName{*ObjectPath});
			if (AssetData.IsValid())
			{
				if (!Bucket.Contains(AssetData))
				{
					Stack.Add(AssetData);
				}

				Bucket.AddUnique(AssetData);
				UnusedAssets.Remove(AssetData);
			}
		}
		
		Refs.Reset();
	}
}

bool FProjectCleanerDataManager::PrepareBucketForDeletion(const TArray<FAssetData>& Bucket, TArray<UObject*>& LoadedAssets)
{
	// FScopedSlowTask LoadingSlowTask(
	// 	Bucket.Num(),
	// 	FText::FromString(FStandardCleanerText::PreparingAssetsForDeletion)
	// );
	// LoadingSlowTask.MakeDialog();
	//
	// for (const auto& Asset : Bucket)
	// {
	// 	LoadingSlowTask.EnterProgressFrame();
	//
	// 	UObject* Object = Asset.GetAsset();
	// 	if (!Object) continue;
	//
	// 	LoadedAssets.Add(Object);
	// 	
	// 	
	// }
	//
	// return LoadedAssets.Num() == Bucket.Num();
	//
	TArray<FString> ObjectPaths;
	ObjectPaths.Reserve(Bucket.Num());
	
	for (const auto& Asset : Bucket)
	{
		ObjectPaths.Add(Asset.ObjectPath.ToString());
	}
	
	return AssetViewUtils::LoadAssetsIfNeeded(ObjectPaths, LoadedAssets, false, true);
}

int32 FProjectCleanerDataManager::DeleteBucket(const TArray<UObject*>& LoadedAssets)
{
	int32 DeletedAssetsNum = ObjectTools::DeleteObjects(LoadedAssets, false);
	
	if (DeletedAssetsNum == 0)
	{
		DeletedAssetsNum = ObjectTools::ForceDeleteObjects(LoadedAssets, false);
	}
	
	if (GShaderCompilingManager && GShaderCompilingManager->GetNumRemainingJobs() > 0)
	{
		GShaderCompilingManager->Shutdown();
	}
	
	return DeletedAssetsNum;
}

void FProjectCleanerDataManager::CleanupAfterDelete()
{
	// ProjectCleanerUtility::UpdateAssetRegistry(true);
	
	AnalyzeProject();

	if (!IsRunningCommandlet())
	{
		ProjectCleanerUtility::FocusOnGameFolder();
	}
}

bool FProjectCleanerDataManager::IsExcludedByClass(const FAssetData& AssetData) const
{
	if (!AssetData.IsValid()) return false;
	
	return ExcludedClasses.Contains(ProjectCleanerUtility::GetClassName(AssetData));
}

bool FProjectCleanerDataManager::IsExcludedByPath(const FAssetData& AssetData) const
{
	if (!AssetData.IsValid()) return false;
	
	for (const auto& ExcludedPath : ExcludedPaths)
	{
		if (AssetData.PackagePath.ToString().StartsWith(ExcludedPath.ToString()))
		{
			return true;
		}
	}
	
	return false;
}

bool FProjectCleanerDataManager::IsLoadingAssets(const bool bShowNotification) const
{
	if (!AssetRegistry) return true;

	const bool bIsAssetRegistryWorking = AssetRegistry->Get().IsLoadingAssets();

	if (bShowNotification && bIsAssetRegistryWorking && !IsRunningCommandlet())
	{
		ProjectCleanerNotificationManager::AddTransient(
			FText::FromString(FStandardCleanerText::AssetRegistryStillWorking),
			SNotificationItem::CS_Fail,
			3.0f
		);
	}
	
	return bIsAssetRegistryWorking;
}