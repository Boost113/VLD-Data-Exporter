#ifndef VLDDEXPORTER_COMMANDPROCESSOR
#define VLDDEXPORTER_COMMANDPROCESSOR
// ------------------------------------ //
#ifndef VLDDEXPORTER_STDAFX
#include "stdafx.h"
#endif
#include "InputProcessor.h"
#include "ExcelFormat.h"
// ------------------------------------ //

namespace VLDDexporter{

#define DEFAULT_OUTPUTFILE		"exported.xls"



	class CommandProcessor{
	public:
		CommandProcessor(const wstring &file);
		~CommandProcessor();

		bool ProcessCommands();

		// runs the commands loaded from the file and returns the generated file name //
		string RunCommands(InputProcessor& data);

	protected:
		void _FilterInputData(InputProcessor& data);
		// ------------------------------------ //
		// file reader //
		ifstream Reader;

		// variables processed from the file //
		string OutputFile;
		string OutputFilePath;

		vector<unique_ptr<basic_regex<char>>> RemoveFilters;

	};

}













#endif