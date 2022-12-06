﻿// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "Slate/Tabs/SProjectCleanerTabScanSettings.h"
#include "ProjectCleanerStyles.h"
#include "ProjectCleanerScanner.h"
#include "ProjectCleanerLibrary.h"
#include "ProjectCleanerScanSettings.h"
// Engine Headers
#include "ProjectCleanerConstants.h"
#include "Widgets/Images/SThrobber.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SSeparator.h"

void SProjectCleanerTabScanSettings::Construct(const FArguments& InArgs)
{
	Scanner = InArgs._Scanner;
	if (!Scanner.IsValid()) return;

	ScanSettings = GetMutableDefault<UProjectCleanerScanSettings>();
	if (!ScanSettings.IsValid()) return;

	FPropertyEditorModule& PropertyEditor = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
	FDetailsViewArgs DetailsViewArgs;
	DetailsViewArgs.bUpdatesFromSelection = false;
	DetailsViewArgs.bLockable = false;
	DetailsViewArgs.bAllowSearch = false;
	DetailsViewArgs.bShowOptions = false;
	DetailsViewArgs.bAllowFavoriteSystem = false;
	DetailsViewArgs.NameAreaSettings = FDetailsViewArgs::HideNameArea;
	DetailsViewArgs.ViewIdentifier = "ProjectCleanerScanSettings";

	ScanSettingsProperty = PropertyEditor.CreateDetailView(DetailsViewArgs);
	ScanSettingsProperty->SetObject(ScanSettings.Get());

	ChildSlot
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		  .AutoHeight()
		  .Padding(FMargin{5.0f})
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			  .AutoWidth()
			  .Padding(FMargin{0.0f, 0.0f, 5.0f, 0.0f})
			[
				SNew(SButton)
				.ContentPadding(FMargin{5.0f})
				.OnClicked_Raw(this, &SProjectCleanerTabScanSettings::OnBtnScanProjectClick)
				.IsEnabled_Raw(this, &SProjectCleanerTabScanSettings::BtnScanProjectEnabled)
				.ButtonColorAndOpacity(FProjectCleanerStyles::Get().GetColor("ProjectCleaner.Color.Blue"))
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.AutoWidth()
					[
						SNew(STextBlock)
						.Justification(ETextJustify::Center)
						.ToolTipText(FText::FromString(TEXT("Scan the project with the specified scan settings.")))
						.ColorAndOpacity(FProjectCleanerStyles::Get().GetColor("ProjectCleaner.Color.White"))
						.ShadowOffset(FVector2D{1.5f, 1.5f})
						.ShadowColorAndOpacity(FLinearColor::Black)
						.Font(FProjectCleanerStyles::GetFont("Bold", 10))
						.Text(FText::FromString(TEXT("Scan Project")))
					]
					+ SHorizontalBox::Slot()
					.AutoWidth()
					[
						SNew(SBox)
						.WidthOverride(16.0f)
						.HeightOverride(16.0f)
						[
							SNew(SImage)
							.Visibility_Raw(this, &SProjectCleanerTabScanSettings::GetBtnScanProjectStatusVisibility)
							.ToolTipText_Raw(this, &SProjectCleanerTabScanSettings::GetBtnScanProjectToolTipText)
							.Image(FProjectCleanerStyles::Get().GetBrush(TEXT("ProjectCleaner.IconWarning16")))
						]
					]
				]
			]
			+ SHorizontalBox::Slot()
			  .AutoWidth()
			  .Padding(FMargin{0.0f, 0.0f, 5.0f, 0.0f})
			[
				SNew(SButton)
				.ContentPadding(FMargin{5.0f})
				.OnClicked_Raw(this, &SProjectCleanerTabScanSettings::OnBtnCleanProjectClick)
				.IsEnabled_Raw(this, &SProjectCleanerTabScanSettings::BtnCleanProjectEnabled)
				.ButtonColorAndOpacity(FProjectCleanerStyles::Get().GetColor("ProjectCleaner.Color.Red"))
				[
					SNew(STextBlock)
					.Justification(ETextJustify::Center)
					.ToolTipText(FText::FromString(TEXT("Remove all unused assets from the project. This won't delete any excluded assets.")))
					.ColorAndOpacity(FProjectCleanerStyles::Get().GetColor("ProjectCleaner.Color.White"))
					.ShadowOffset(FVector2D{1.5f, 1.5f})
					.ShadowColorAndOpacity(FLinearColor::Black)
					.Font(FProjectCleanerStyles::GetFont("Bold", 10))
					.Text(FText::FromString(TEXT("Clean Project")))
				]
			]
			+ SHorizontalBox::Slot()
			  .AutoWidth()
			  .Padding(FMargin{0.0f, 0.0f, 5.0f, 0.0f})
			[
				SNew(SButton)
				.ContentPadding(FMargin{5.0f})
				.OnClicked_Raw(this, &SProjectCleanerTabScanSettings::OnBtnDeleteEmptyFoldersClick)
				.IsEnabled_Raw(this, &SProjectCleanerTabScanSettings::BtnDeleteEmptyFoldersEnabled)
				.ButtonColorAndOpacity(FProjectCleanerStyles::Get().GetColor("ProjectCleaner.Color.Red"))
				[
					SNew(STextBlock)
					.Justification(ETextJustify::Center)
					.ToolTipText(FText::FromString(TEXT("Remove all empty folders in the project.")))
					.ColorAndOpacity(FProjectCleanerStyles::Get().GetColor("ProjectCleaner.Color.White"))
					.ShadowOffset(FVector2D{1.5f, 1.5f})
					.ShadowColorAndOpacity(FLinearColor::Black)
					.Font(FProjectCleanerStyles::GetFont("Bold", 10))
					.Text(FText::FromString(TEXT("Delete Empty Folders")))
				]
			]
		]
		+ SVerticalBox::Slot()
		  .Padding(FMargin{5.0f})
		  .AutoHeight()
		[
			SNew(SSeparator)
			.Thickness(5.0f)
		]
		+ SVerticalBox::Slot()
		  .Padding(FMargin{5.0f})
		  .FillHeight(1.0f)
		[
			SNew(SBox)
			[
				SNew(SScrollBox)
				.ScrollWhenFocusChanges(EScrollWhenFocusChanges::NoScroll)
				.AnimateWheelScrolling(true)
				.AllowOverscroll(EAllowOverscroll::No)
				+ SScrollBox::Slot()
				[
					ScanSettingsProperty.ToSharedRef()
				]
			]
		]
	];
}

bool SProjectCleanerTabScanSettings::BtnScanProjectEnabled() const
{
	return Scanner.IsValid();
}

bool SProjectCleanerTabScanSettings::BtnCleanProjectEnabled() const
{
	return Scanner.IsValid() && Scanner->GetAssetsUnused().Num() > 0 && Scanner->GetScannerDataState() == EProjectCleanerScannerDataState::Actual;
}

bool SProjectCleanerTabScanSettings::BtnDeleteEmptyFoldersEnabled() const
{
	return Scanner.IsValid() && Scanner->GetFoldersEmpty().Num() > 0 && Scanner->GetScannerDataState() == EProjectCleanerScannerDataState::Actual;
}

FReply SProjectCleanerTabScanSettings::OnBtnScanProjectClick() const
{
	Scanner->Scan();

	return FReply::Handled();
}

FReply SProjectCleanerTabScanSettings::OnBtnCleanProjectClick() const
{
	const auto ConfirmationStatus = UProjectCleanerLibrary::ConfirmationWindowShow(
		FText::FromString(ProjectCleanerConstants::MsgConfirmCleanProjectTitle),
		FText::FromString(ProjectCleanerConstants::MsgConfirmCleanProject)
	);
	if (UProjectCleanerLibrary::ConfirmationWindowCancelled(ConfirmationStatus))
	{
		return FReply::Handled();
	}

	Scanner->CleanProject();

	return FReply::Handled();
}

FReply SProjectCleanerTabScanSettings::OnBtnDeleteEmptyFoldersClick() const
{
	const auto ConfirmationStatus = UProjectCleanerLibrary::ConfirmationWindowShow(
		FText::FromString(ProjectCleanerConstants::MsgConfirmDeleteEmptyFoldersTitle),
		FText::FromString(ProjectCleanerConstants::MsgConfirmDeleteEmptyFolders)
	);
	if (UProjectCleanerLibrary::ConfirmationWindowCancelled(ConfirmationStatus))
	{
		return FReply::Handled();
	}

	Scanner->DeleteEmptyFolders();

	return FReply::Handled();
}

EVisibility SProjectCleanerTabScanSettings::GetBtnScanProjectStatusVisibility() const
{
	if (!Scanner.IsValid()) return EVisibility::Collapsed;

	return Scanner->GetScannerDataState() != EProjectCleanerScannerDataState::Actual ? EVisibility::Visible : EVisibility::Collapsed;
}

FText SProjectCleanerTabScanSettings::GetBtnScanProjectToolTipText() const
{
	if (!Scanner.IsValid()) return FText::FromString(TEXT(""));

	if (Scanner->GetScannerDataState() == EProjectCleanerScannerDataState::NotScanned)
	{
		return FText::FromString(TEXT("Project never scanned"));
	}

	if (Scanner->GetScannerDataState() == EProjectCleanerScannerDataState::Obsolete)
	{
		return FText::FromString(TEXT("Re-Scan required. Asset Registry has been updated."));
	}

	return FText::FromString(TEXT(""));
}
