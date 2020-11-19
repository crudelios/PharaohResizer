// PharaohResizer - A simple app that creates a Pharaoh executable with the requested resolution
// Created by crudelios

#include "MyForm.h"
#include "resource.h"
#include <cstdint>
#include <cstdio>
#include <wchar.h>
#include <msclr\marshal.h>
#include <msclr\marshal_cppstd.h>

extern "C"
{
#include <windows.h>
#include <lz4.h>
}

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;

char * compData;
char * memData;
u32 compSize;
const u32 exeSize = 2142208;

using namespace std;
using namespace PharaohResizer;

[STAThreadAttribute]

// Main function, prepares everything
int main()
{
	compData = nullptr;
	memData  = nullptr;
	compSize = 0;
	MyForm fm;
	fm.ShowDialog();

	delete[] memData;

	return 0;
}

// Loads the resource to a pointer
bool LoadResource()
{
	HRSRC hrsrc = nullptr;
	HGLOBAL hGlbl = nullptr;

	hrsrc = FindResource(nullptr, MAKEINTRESOURCE(IDR_EXECUTABLE1), L"EXECUTABLE");

	if (hrsrc == nullptr)
		return false;

	compSize = SizeofResource(nullptr, hrsrc);
	if (!compSize)
		return false;

	hGlbl = LoadResource(nullptr, hrsrc);
	if (hGlbl == nullptr)
		return false;

	compData = (char *)LockResource(hGlbl);

	if (compData == nullptr)
		return false;

	return true;
}

// Shows a message with an error (duh)
void ShowError(const char * error)
{
	String^ msg = gcnew String(error);

	MessageBox::Show(msg, "Error", MessageBoxButtons::OK, MessageBoxIcon::Error);
}

// Saves the file
System::Void MyForm::btn_Save_Click(System::Object^  sender, System::EventArgs^  e)
{
	// Only load the resource if it wasn't already loaded
	if (!memData)
	{
		if (!LoadResource())
		{
			char text[200];

			sprintf(text, "Error loading resource!\nError number: 0x%x", GetLastError());

			ShowError(text);

			return;
		}
	}

	// Get the selected width and height
	u32 myWidth  = Convert::ToUInt32(this->width->Value);
	u32 myHeight = Convert::ToUInt32(this->height->Value);

	// Check for invalid input
	if (myWidth & 3)
	{
		ShowError("The inserted width is not a multiple of 4.\n\nA multiple of 4 must be used for the width.");
		return;
	}

	if (myWidth < 1024 || myWidth > 7680)
	{
		ShowError("The inserted width is outside of the allowed range.\n\nPlease insert a number between 1024 and 7680.");
		return;
	}

	if (myHeight & 3)
	{
		ShowError("The inserted height is not a multiple of 4.\n\nA multiple of 4 must be used for the height.");
		return;
	}

	if (myHeight < 720 || myHeight > 4320)
	{
		ShowError("The inserted height is outside of the allowed range.\n\nPlease insert a number between 720 and 4320.");
		return;
	}

	// Good, no input problems.
	// Let's generate an automatic file name to save
	char fileName[25];

	sprintf(fileName, "Pharaoh - %ux%u.exe", myWidth, myHeight);
	String^ fname = gcnew String(fileName);

	// Show the save file dialog
	SaveFileDialog^ saveFileDialog1 = gcnew SaveFileDialog;
	saveFileDialog1->Filter = "Executable (*.exe)|*.exe";
	saveFileDialog1->Title = "Save Executable As...";
	saveFileDialog1->FileName = fname;
	saveFileDialog1->FilterIndex = 2;
	saveFileDialog1->RestoreDirectory = true;
	if (saveFileDialog1->ShowDialog() != ::DialogResult::OK)
	{
		return;
	}

	// Copy data from the resource to the memory location
	if (!memData)
	{
		memData = new char[exeSize];

		if(LZ4_uncompress(compData, memData, exeSize) < 0)
		{
			ShowError("There was a problem creating the file.\n\nIf the problem persists, try redownloading this program.");
			return;
		}
	}

	// Change resolution in the proper offsets
	memcpy(memData + 0xcfa0a, &myWidth,  sizeof(u32));
	memcpy(memData + 0xcfa0f, &myHeight, sizeof(u32));
	memcpy(memData + 0xcfda3, &myWidth,  sizeof(u32));
	memcpy(memData + 0xcfdad, &myHeight, sizeof(u32));

	// Get the save file location string
	std::wstring floc = msclr::interop::marshal_as<std::wstring>(saveFileDialog1->FileName);
	
	// Save the file
	FILE * myFile = _wfopen(floc.c_str(), L"wb");

	// Check for errors
	if (!myFile)
	{
		ShowError("The file could not be saved.\n\nYou may need administrator privileges to save the file to the selected folder.");
		return;
	}

	// Write the file
	if (fwrite(memData, sizeof(u8), exeSize, myFile) != exeSize)
	{
		ShowError("There was an error while saving the file. The saved file may be corrupt.\n\nPlease try saving the file again.");
		return;
	}

	// Close the file
	fclose(myFile);

	// Success!
	System::Windows::Forms::DialogResult doExit = MessageBox::Show("The file was successfuly saved.\n\nDo you want to save another file with a different resolution?", "Great Success!", MessageBoxButtons::YesNo, MessageBoxIcon::Information);

	// Exit the program if asked to
	if(doExit == System::Windows::Forms::DialogResult::No)
		Application::Exit();

	return;
}