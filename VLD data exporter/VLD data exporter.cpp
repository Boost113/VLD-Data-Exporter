// VLD data exporter.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "InputProcessor.h"
#include "CommandProcessor.h"

using namespace VLDDexporter;

int _tmain(int argc, _TCHAR* argv[]){

	int Programresult = 0;

	{
		// let's start by processing arguments //
		if(argc != 3){
			// invalid command line //
			cout << "wrong number of arguments, expected 2 files,\n input file (containing \"report\") and command file (anything else)" << endl;
			cout << "got " << argc << " arguments" << endl;
			Programresult = -1;
			goto programfinishlabel;
		}

		// get both arguments and check which file is which //
		wstring Arg1(argv[1]);
		wstring Arg2(argv[2]);
		

		wstring* selectedreport = NULL;
		wstring* selectedsettings = NULL;

		// check which file contains the word "report" //
		if(Arg1.find(L"report") != wstring::npos){
			// argument 1 is the report file //
			selectedreport = &Arg1;
			selectedsettings = &Arg2;
			
		} else {
			// second argument is the report file //
			selectedreport = &Arg2;
			selectedsettings = &Arg1;
		}

		cout << "processing report file ";
		wcout << *selectedreport << endl;

		// create processing
		InputProcessor FileProcessing(*selectedreport);

		// process the report file //
		if(!FileProcessing.ProcessFile()){
			// couldn't read the file //

			cout << "couldn't open report file, ";
			wcout << *selectedreport << L" for reading" << endl;
			goto programfinishlabel;
		}

		// create commands object from the other file //
		CommandProcessor Commands(*selectedsettings);
		if(!Commands.ProcessCommands()){

			cout << "invalid command file format (make sure that it's name doesn't contain \"report\"" << endl;
			goto programfinishlabel;
		}

		// run commands object on the report file //
		string outputfile = Commands.RunCommands(FileProcessing);


		// finished all actions //
		cout << endl << "finished exporting wanted data from the report, wrote "+outputfile << endl << endl;
	}
programfinishlabel:

	// wait a while and then quit //
	cout << endl << endl << "Quitting in 10 seconds..." << endl;
	Sleep(10000);

	return Programresult;
}

