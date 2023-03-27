﻿// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Libs/PjcLibPath.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPjcLibPathDefaults,
	"Plugins.ProjectCleaner.Libs.Path.DefaultPaths",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter
)

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPjcLibPathNormalize,
	"Plugins.ProjectCleaner.Libs.Path.Normalize",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter
)

bool FPjcLibPathDefaults::RunTest(const FString& Parameters)
{
	// default paths must not end with slash and must start with drive letter

	TArray<FString> TestCases;
	TestCases.Emplace(FPjcLibPath::ProjectDir());
	TestCases.Emplace(FPjcLibPath::ContentDir());
	TestCases.Emplace(FPjcLibPath::SourceDir());
	TestCases.Emplace(FPjcLibPath::ConfigDir());
	TestCases.Emplace(FPjcLibPath::PluginsDir());
	TestCases.Emplace(FPjcLibPath::SavedDir());
	TestCases.Emplace(FPjcLibPath::DevelopersDir());
	TestCases.Emplace(FPjcLibPath::CollectionsDir());
	TestCases.Emplace(FPjcLibPath::CurrentUserDevelopersDir());
	TestCases.Emplace(FPjcLibPath::CurrentUserCollectionsDir());

	for (const auto& TestCase : TestCases)
	{
		TestFalse(FString::Printf(TEXT("DefaultPaths - Input: \"%s\" must not be empty"), *TestCase), TestCase.IsEmpty());
		TestFalse(FString::Printf(TEXT("DefaultPaths - Input: \"%s\" must not contain any trailing slash"), *TestCase), TestCase.EndsWith(TEXT("/")) || TestCase.EndsWith(TEXT("\\")));
		TestFalse(FString::Printf(TEXT("DefaultPaths - Input: \"%s\" must be absolute"), *TestCase), !(TestCase.Len() > 2 && TestCase[1] == ':'));
		TestFalse(FString::Printf(TEXT("DefaultPaths - Input: \"%s\" must be collapsed"), *TestCase), TestCase.Contains(TEXT("..")) || TestCase.Contains(TEXT(".")));
	}

	return true;
}


bool FPjcLibPathNormalize::RunTest(const FString& Parameters)
{
	// normalized version for paths must have following contracts
	// 1. must start with slash or Disk drive letter (example C:/)
	// 2. must not contain any duplicate slashes
	// 3. must not end with trailing slash
	// 4. all consecutive or mixed separators must be collapsed
	// 5. must return empty string if path is outside project directory

	// for any invalid path must TOptional

	TArray<TPair<FString, FString>> TestCases = {

		// empty cases
		TPair<FString, FString>(TEXT(""), TEXT("")),
		TPair<FString, FString>(TEXT("C:/"), TEXT("")),
		TPair<FString, FString>(TEXT("C://"), TEXT("")),
		TPair<FString, FString>(TEXT("C:\\"), TEXT("")),
		TPair<FString, FString>(TEXT(":\\"), TEXT("")),
		TPair<FString, FString>(TEXT(":/"), TEXT("")),
		TPair<FString, FString>(TEXT(":"), TEXT("")),
		TPair<FString, FString>(TEXT("."), TEXT("")),
		TPair<FString, FString>(TEXT("Some/Invalid/PAath"), TEXT("")),
		TPair<FString, FString>(TEXT("/Game//..//"), TEXT("")),
		TPair<FString, FString>(FPaths::ConvertRelativePathToFull(FPaths::EngineDir()), TEXT("")),
		TPair<FString, FString>(FPaths::ConvertRelativePathToFull(FPaths::EngineContentDir()), TEXT("")),

		// absolute paths
		TPair<FString, FString>(FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir()), FPaths::ConvertRelativePathToFull(FPaths::ProjectDir() / TEXT("Content"))),
		TPair<FString, FString>(FPaths::ConvertRelativePathToFull(FPaths::ProjectPluginsDir()), FPaths::ConvertRelativePathToFull(FPaths::ProjectDir() / TEXT("Plugins"))),
		TPair<FString, FString>(FPaths::ConvertRelativePathToFull(FPaths::ProjectConfigDir()), FPaths::ConvertRelativePathToFull(FPaths::ProjectDir() / TEXT("Config"))),
		TPair<FString, FString>(FPaths::ConvertRelativePathToFull(FPaths::GameSourceDir()), FPaths::ConvertRelativePathToFull(FPaths::ProjectDir() / TEXT("Source"))),
		TPair<FString, FString>(FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir() / TEXT("MyFolder/")), FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir() / TEXT("MyFolder"))),
		TPair<FString, FString>(FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir() / TEXT("//MyFolder/")),
		                        FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir() / TEXT("MyFolder"))),
		TPair<FString, FString>(FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir() / TEXT("//MyFolder/Test")),
		                        FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir() / TEXT("MyFolder/Test"))),
		TPair<FString, FString>(FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir() / TEXT("//MyFolder/Test/")),
		                        FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir() / TEXT("MyFolder/Test"))),
		TPair<FString, FString>(FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir() / TEXT("//MyFolder/Test//")),
		                        FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir() / TEXT("MyFolder/Test"))),
		TPair<FString, FString>(FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir() / TEXT("//MyFolder/./Test//")),
		                        FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir() / TEXT("MyFolder/Test"))),
		TPair<FString, FString>(FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir() / TEXT("//MyFolder//..//Test//")),
		                        FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir() / TEXT("Test"))),

		// relative asset paths
		TPair<FString, FString>(TEXT("/Game"), TEXT("/Game")),
		TPair<FString, FString>(TEXT("/Game/"), TEXT("/Game")),
		TPair<FString, FString>(TEXT("/Game//"), TEXT("/Game")),
		TPair<FString, FString>(TEXT("/Game/MyFolder"), TEXT("/Game/MyFolder")),
		TPair<FString, FString>(TEXT("/Game/MyFolder/"), TEXT("/Game/MyFolder")),
		TPair<FString, FString>(TEXT("/Game/MyFolder//"), TEXT("/Game/MyFolder")),
		TPair<FString, FString>(TEXT("/Game//MyFolder/"), TEXT("/Game/MyFolder")),
		TPair<FString, FString>(TEXT("/Game//MyFolder/"), TEXT("/Game/MyFolder")),
		TPair<FString, FString>(TEXT("//Game//MyFolder//"), TEXT("/Game/MyFolder")),

	};

	// Run the test cases
	for (const TPair<FString, FString>& TestCase : TestCases)
	{
		const FString Input = TestCase.Key;
		const FString Expected = TestCase.Value;
		const FString Actual = FPjcLibPath::Normalize(Input);

		TestEqual(FString::Printf(TEXT("PathNormalize - Input: \"%s\""), *Input), Actual, Expected);
	}

	return true;
}
