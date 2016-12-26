//
// MainPage.xaml.h
// Declaration of the MainPage class.
//

#pragma once

#include "MainPage.g.h"
#include "Trie.h"
#include <string>

namespace MakeDawg
{
	/// <summary>
	/// An empty page that can be used on its own or navigated to within a Frame.
	/// </summary>
	public ref class MainPage sealed
	{
	public:
		MainPage();

	private:
		// UI related methods
		void IOFolderButtonClick(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
		void LexiconFileButtonClick(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
		void MakeDawgButtonClick(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);

		// Trie related
		void AddWordsToTrie(LxpStd::Trie& trie, const std::string& filePath);
		std::string ConstructSummary(LxpStd::Trie& trie);

		// convenience functions
		static std::string ConvertPlatformStringToString(Platform::String^ platformString);
		static Platform::String^ ConvertStringToPlatformString(const std::string& inputString);
	};
}
