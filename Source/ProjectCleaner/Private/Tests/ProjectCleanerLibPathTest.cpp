﻿// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "ProjectCleaner.h"
#include "ProjectCleanerTypes.h"
#include "Libs/ProjectCleanerLibPath.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FProjectCleanerLibPathConvertTest,
	"Plugins.ProjectCleaner.Library.Path.Convert",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::EngineFilter
)

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FProjectCleanerLibPathNormalizeTest,
	"Plugins.ProjectCleaner.Library.Path.Normalize",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::EngineFilter
)

bool FProjectCleanerLibPathConvertTest::RunTest(const FString& Parameters)
{
	struct FTestCase
	{
		FString Input;
		FString Expected;
		EProjectCleanerPathType PathType;
	};

	const FString ProjectDir = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir());

	TArray<FTestCase> TestCases;

	// absolute to relative cases
	TestCases.Add(FTestCase{TEXT(""), TEXT(""), EProjectCleanerPathType::Absolute});
	// invalid paths must return empty string
	TestCases.Add(FTestCase{TEXT("aaa"), TEXT(""), EProjectCleanerPathType::Absolute});
	TestCases.Add(FTestCase{TEXT("./"), TEXT(""), EProjectCleanerPathType::Absolute});
	TestCases.Add(FTestCase{TEXT("."), TEXT(""), EProjectCleanerPathType::Absolute});
	TestCases.Add(FTestCase{TEXT("//"), TEXT(""), EProjectCleanerPathType::Absolute});
	// path must not contain duplicate or trailing slashes
	TestCases.Add(FTestCase{TEXT("/Game"), UProjectCleanerLibPath::FolderContent(EProjectCleanerPathType::Absolute), EProjectCleanerPathType::Absolute});
	TestCases.Add(FTestCase{TEXT("/Game/"), UProjectCleanerLibPath::FolderContent(EProjectCleanerPathType::Absolute), EProjectCleanerPathType::Absolute});
	TestCases.Add(FTestCase{TEXT("/Game//"), UProjectCleanerLibPath::FolderContent(EProjectCleanerPathType::Absolute), EProjectCleanerPathType::Absolute});
	TestCases.Add(FTestCase{TEXT("//Game//"), UProjectCleanerLibPath::FolderContent(EProjectCleanerPathType::Absolute), EProjectCleanerPathType::Absolute});
	TestCases.Add(FTestCase{TEXT("/Game///"), UProjectCleanerLibPath::FolderContent(EProjectCleanerPathType::Absolute), EProjectCleanerPathType::Absolute});
	TestCases.Add(FTestCase{TEXT("/Game/MyFolder"), UProjectCleanerLibPath::FolderContent(EProjectCleanerPathType::Absolute) / TEXT("MyFolder"), EProjectCleanerPathType::Absolute});
	TestCases.Add(FTestCase{TEXT("/Game/MyFolder/"), UProjectCleanerLibPath::FolderContent(EProjectCleanerPathType::Absolute) / TEXT("MyFolder"), EProjectCleanerPathType::Absolute});
	TestCases.Add(FTestCase{TEXT("/Game/MyFolder///"), UProjectCleanerLibPath::FolderContent(EProjectCleanerPathType::Absolute) / TEXT("MyFolder"), EProjectCleanerPathType::Absolute});
	TestCases.Add(FTestCase{TEXT("/Game///MyFolder///"), UProjectCleanerLibPath::FolderContent(EProjectCleanerPathType::Absolute) / TEXT("MyFolder"), EProjectCleanerPathType::Absolute});
	TestCases.Add(FTestCase{TEXT("/Game//MyFolder///"), UProjectCleanerLibPath::FolderContent(EProjectCleanerPathType::Absolute) / TEXT("MyFolder"), EProjectCleanerPathType::Absolute});
	TestCases.Add(FTestCase{
		TEXT("/Game/StarterContent/Materials"), UProjectCleanerLibPath::FolderContent(EProjectCleanerPathType::Absolute) / TEXT("StarterContent/Materials"), EProjectCleanerPathType::Absolute
	});
	TestCases.Add(FTestCase{
		TEXT("/Game//StarterContent//Materials/"), UProjectCleanerLibPath::FolderContent(EProjectCleanerPathType::Absolute) / TEXT("StarterContent/Materials"), EProjectCleanerPathType::Absolute
	});
	TestCases.Add(FTestCase{
		TEXT("/Game/StarterContent/Materials//"), UProjectCleanerLibPath::FolderContent(EProjectCleanerPathType::Absolute) / TEXT("StarterContent/Materials"), EProjectCleanerPathType::Absolute
	});
	TestCases.Add(FTestCase{TEXT("/Game/M_Car_Blue.uasset"), UProjectCleanerLibPath::FolderContent(EProjectCleanerPathType::Absolute) / TEXT("M_Car_Blue.uasset"), EProjectCleanerPathType::Absolute});
	TestCases.Add(FTestCase{TEXT("/Game/M_Car_Blue.uasset/"), UProjectCleanerLibPath::FolderContent(EProjectCleanerPathType::Absolute) / TEXT("M_Car_Blue.uasset"), EProjectCleanerPathType::Absolute});
	TestCases.Add(FTestCase{
		TEXT("/Game//M_Car_Blue.uasset/"), UProjectCleanerLibPath::FolderContent(EProjectCleanerPathType::Absolute) / TEXT("M_Car_Blue.uasset"), EProjectCleanerPathType::Absolute
	});
	TestCases.Add(FTestCase{
		TEXT("/Game/Folder/M_Car_Blue.uasset"), UProjectCleanerLibPath::FolderContent(EProjectCleanerPathType::Absolute) / TEXT("Folder/M_Car_Blue.uasset"), EProjectCleanerPathType::Absolute
	});
	TestCases.Add(FTestCase{
		TEXT("/Game/Folder/M_Car_Blue.uasset/"), UProjectCleanerLibPath::FolderContent(EProjectCleanerPathType::Absolute) / TEXT("Folder/M_Car_Blue.uasset"), EProjectCleanerPathType::Absolute
	});
	TestCases.Add(FTestCase{
		TEXT("/Game/Folder/M_Car_Blue.uasset//"), UProjectCleanerLibPath::FolderContent(EProjectCleanerPathType::Absolute) / TEXT("Folder/M_Car_Blue.uasset"), EProjectCleanerPathType::Absolute
	});
	TestCases.Add(FTestCase{
		TEXT("/Game/Folder//M_Car_Blue.uasset//"), UProjectCleanerLibPath::FolderContent(EProjectCleanerPathType::Absolute) / TEXT("Folder/M_Car_Blue.uasset"), EProjectCleanerPathType::Absolute
	});
	TestCases.Add(FTestCase{
		TEXT("/Game//Folder//M_Car_Blue.uasset//"), UProjectCleanerLibPath::FolderContent(EProjectCleanerPathType::Absolute) / TEXT("Folder/M_Car_Blue.uasset"), EProjectCleanerPathType::Absolute
	});

	// relative to absolute cases
	TestCases.Add(FTestCase{TEXT(""), TEXT(""), EProjectCleanerPathType::Relative});
	TestCases.Add(FTestCase{TEXT("aaa"), TEXT(""), EProjectCleanerPathType::Relative});
	TestCases.Add(FTestCase{TEXT("./"), TEXT(""), EProjectCleanerPathType::Relative});
	TestCases.Add(FTestCase{TEXT("."), TEXT(""), EProjectCleanerPathType::Relative});
	TestCases.Add(FTestCase{TEXT("//"), TEXT(""), EProjectCleanerPathType::Relative});
	TestCases.Add(FTestCase{UProjectCleanerLibPath::FolderContent(EProjectCleanerPathType::Absolute), TEXT("/Game"), EProjectCleanerPathType::Relative});
	TestCases.Add(FTestCase{UProjectCleanerLibPath::FolderContent(EProjectCleanerPathType::Absolute) / TEXT("/"), TEXT("/Game"), EProjectCleanerPathType::Relative});
	TestCases.Add(FTestCase{UProjectCleanerLibPath::FolderContent(EProjectCleanerPathType::Absolute) / TEXT("//"), TEXT("/Game"), EProjectCleanerPathType::Relative});
	TestCases.Add(FTestCase{UProjectCleanerLibPath::FolderContent(EProjectCleanerPathType::Absolute) / TEXT("MyFolder"), TEXT("/Game/MyFolder"), EProjectCleanerPathType::Relative});
	TestCases.Add(FTestCase{
		UProjectCleanerLibPath::FolderContent(EProjectCleanerPathType::Absolute) / TEXT("StarterContent/Materials"), TEXT("/Game/StarterContent/Materials"), EProjectCleanerPathType::Relative
	});
	TestCases.Add(FTestCase{
		UProjectCleanerLibPath::FolderContent(EProjectCleanerPathType::Absolute) / TEXT("StarterContent/Materials/"), TEXT("/Game/StarterContent/Materials"), EProjectCleanerPathType::Relative
	});
	TestCases.Add(FTestCase{
		UProjectCleanerLibPath::FolderContent(EProjectCleanerPathType::Absolute) / TEXT("StarterContent/Materials//"), TEXT("/Game/StarterContent/Materials"), EProjectCleanerPathType::Relative
	});
	TestCases.Add(FTestCase{
		UProjectCleanerLibPath::FolderContent(EProjectCleanerPathType::Absolute) / TEXT("StarterContent//Materials//"), TEXT("/Game/StarterContent/Materials"), EProjectCleanerPathType::Relative
	});
	TestCases.Add(FTestCase{UProjectCleanerLibPath::FolderContent(EProjectCleanerPathType::Absolute) / TEXT("material.uasset"), TEXT("/Game/material.uasset"), EProjectCleanerPathType::Relative});
	TestCases.Add(FTestCase{
		UProjectCleanerLibPath::FolderContent(EProjectCleanerPathType::Absolute) / TEXT("MyFolder/M_Car_Blue.uasset"), TEXT("/Game/MyFolder/M_Car_Blue.uasset"), EProjectCleanerPathType::Relative
	});


	for (const auto& TestCase : TestCases)
	{
		const FString Actual = UProjectCleanerLibPath::Convert(TestCase.Input, TestCase.PathType);
		if (Actual.Equals(TestCase.Expected, ESearchCase::CaseSensitive) == false)
		{
			UE_LOG(LogProjectCleaner, Error, TEXT("Expected %s, got %s, input %s"), *TestCase.Expected, *Actual, *TestCase.Input);
			return false;
		}
	}


	return true;
}

bool FProjectCleanerLibPathNormalizeTest::RunTest(const FString& Parameters)
{
	// path must not contain any duplicate slashes
	// path must not end with trailing slashes

	struct FTestCase
	{
		FString Input;
		FString Expected;
	};

	TArray<FTestCase> TestCases;
	TestCases.Add(FTestCase{TEXT(""), TEXT("")});
	TestCases.Add(FTestCase{TEXT("/"), TEXT("")});
	TestCases.Add(FTestCase{TEXT("/Game"), TEXT("/Game")});
	TestCases.Add(FTestCase{TEXT("/Game/"), TEXT("/Game")});
	TestCases.Add(FTestCase{TEXT("/MyFolder//"), TEXT("/MyFolder")});
	TestCases.Add(FTestCase{TEXT("//MyFolder///"), TEXT("/MyFolder")});
	TestCases.Add(FTestCase{TEXT("/MyFolder//my_file.txt"), TEXT("/MyFolder/my_file.txt")});
	TestCases.Add(FTestCase{FPaths::ProjectContentDir(), FPaths::ProjectDir() / TEXT("Content")});

	for (const auto& TestCase : TestCases)
	{
		const FString Actual = UProjectCleanerLibPath::Normalize(TestCase.Input);

		if (!Actual.Equals(TestCase.Expected))
		{
			UE_LOG(LogProjectCleaner, Warning, TEXT("Expected: %s, got: %s, input: %s"), *TestCase.Expected, *Actual, *TestCase.Input);
			return false;
		}
	}

	return true;
}
