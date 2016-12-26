//
// MainPage.xaml.cpp
// Implementation of the MainPage class.
//

#include "pch.h"
#include "MainPage.xaml.h"
#include "Trie.h"

#include <cvt/wstring>
#include <codecvt>
#include <fstream>
#include <iostream>
#include <ppltasks.h>
#include <sstream>
#include <string>

using namespace MakeDawg;

using namespace concurrency;
using namespace Platform;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Controls::Primitives;
using namespace Windows::UI::Xaml::Data;
using namespace Windows::UI::Xaml::Input;
using namespace Windows::UI::Xaml::Media;
using namespace Windows::UI::Xaml::Navigation;
using namespace Windows::Storage::Pickers;
using namespace Windows::Storage;

using namespace LxpStd;
using namespace std;

// The Blank Page item template is documented at http://go.microsoft.com/fwlink/?LinkId=402352&clcid=0x409

MainPage::MainPage()
{
	InitializeComponent();
}

void MakeDawg::MainPage::IOFolderButtonClick(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	// set the file picker
	FolderPicker^ picker = ref new FolderPicker();
	picker->SuggestedStartLocation = Pickers::PickerLocationId::DocumentsLibrary;
	picker->FileTypeFilter->Append(".txt");

	// need to use C++ tasks for async!
	auto pickerTask = create_task(picker->PickSingleFolderAsync());
	pickerTask.then([this](StorageFolder^ folder)
	{
		if (folder != nullptr)
		{
			// set permission for access
			AccessCache::StorageApplicationPermissions::FutureAccessList->AddOrReplace("PickedFolderToken", folder);

			// update the text field
			ioFolderInput->Text = folder->Name;
		}
	});
}

void MakeDawg::MainPage::LexiconFileButtonClick(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	// set the file picker
	FileOpenPicker^ picker = ref new FileOpenPicker();
	picker->SuggestedStartLocation = Pickers::PickerLocationId::DocumentsLibrary;
	picker->FileTypeFilter->Append(".txt");

	// need to use C++ tasks for async!
	auto pickerTask = create_task(picker->PickSingleFileAsync());
	pickerTask.then([this](StorageFile^ file)
	{
		if (file != nullptr)
		{
			// populate it in the text field
			lexiconFileInput->Text = file->Path;
		}
	});
}

void MakeDawg::MainPage::MakeDawgButtonClick(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	// add words to Trie
	Trie trie;
	statusOutput->Text = "Reading input file...";
	string fileName = ConvertPlatformStringToString(lexiconFileInput->Text);
	AddWordsToTrie(trie, fileName);

	// compress
	statusOutput->Text = "Compressing...";
	while (trie.Compress() == false)
	{
			// do nothing
	}

	// save file
	statusOutput->Text = "Saving DAWG file...";
	string lexiconName = ConvertPlatformStringToString(lexiconNameInput->Text);
	fileName = ConvertPlatformStringToString(dawgFileInput->Text);
	trie.SaveAsDawg(fileName, lexiconName);

	// show summary
	string summary = ConstructSummary(trie);
	statusOutput->Text = ConvertStringToPlatformString(summary);
}

void MakeDawg::MainPage::AddWordsToTrie(Trie& trie, const string& filePath)
{
	// open the file
	ifstream fileStream(filePath, ifstream::in);
	string line;

	// read until end of file is reached
	while (fileStream.good())
	{
		// read a line
		getline(fileStream, line);

		// add word to trie if it is not a comment
		if (line.length() > 0 && line[0] != '#')
		{
			// need to extract the first word (skipping whitespace)
			string word;
			stringstream(line);
			line >> word;
			if (word.length() > 0)
				trie.AddWord(word.c_str());
		}
	}

	fileStream.close();
}

string MakeDawg::MainPage::ConstructSummary(Trie& trie)
{
	ostringstream summaryStream;
	TrieDiagnostics diagnostics;
	trie.GetDiagnostics(diagnostics);

	summaryStream << "SUMMARY" << "\n\n"
		<< "BEFORE COMPRESSION" << '\n'
		<< "Number of Nodes: " << diagnostics.numNodes << '\n'
		<< "Number of First Children: " << diagnostics.numFirstChildrenBeforeCompression << "\n\n"
		<< "AFTER COMPRESSION" << '\n'
		<< "Number of Nodes: " << diagnostics.numNodesAfterCompression << '\n'
		<< "Number of First Children: " << diagnostics.numFirstChildrenAfterCompression << "\n\n"
		<< "NUMBER OF WORDS: " << diagnostics.numWords << '\n';

	return summaryStream.str();
}

string MakeDawg::MainPage::ConvertPlatformStringToString(Platform::String^ platformString)
{
	stdext::cvt::wstring_convert<std::codecvt_utf8<wchar_t>> convert;
	string returnString = convert.to_bytes(platformString->Data());
	return returnString;
}

Platform::String^ MakeDawg::MainPage::ConvertStringToPlatformString(const string& inputString)
{
	std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
	std::wstring inputStringW = converter.from_bytes(inputString);
	Platform::String^ returnString = ref new Platform::String(inputStringW.c_str());

	return returnString;
}
