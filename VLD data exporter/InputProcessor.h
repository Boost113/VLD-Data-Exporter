#ifndef VLDDEXPORTER_INPUTPROCESSOR
#define VLDDEXPORTER_INPUTPROCESSOR
// ------------------------------------ //
#ifndef VLDDEXPORTER_STDAFX
#include "stdafx.h"
#endif
// ------------------------------------ //


namespace VLDDexporter{

	// forward declaration for friend access //
	class CommandProcessor;
	struct CallstackFunction;


	// file that is mentioned in the report //
	struct CallstackFileEntry{
		// sets initial blame count to zero, so that adding functions will have right blame count //
		CallstackFileEntry(const string &filename);

		void AddFunctionToFile(CallstackFunction* func);

		string PathOrName;
		// how many leaks are in this file //
		int BlameCount;
		vector<int> LeakLines;
		// references to CallStackFunctions that are from this file //
		vector<CallstackFunction*> FunctionsInfile;
	};

	// function that has appeared in 1 or more callstack in the leaks //
	struct CallstackFunction{
		CallstackFunction(const string &functionname, const int &line);

		string Name;
		// how many leaks does this function cause //
		int BlameCount;

		int Line;
		// owning file //
		CallstackFileEntry* ContainedInFile;
	};


	// block of leaked memory loaded from the file //
	struct ProcessedBlock{
		ProcessedBlock(const UINT &number, const int &bytecount);

		UINT AllocationNumber;
		int LeakedBytes;

		// functions that are responsible for this leak //
		vector<CallstackFunction*> FunctionsInBlock;
	};




	class InputProcessor{
		friend CommandProcessor;
	public:
		InputProcessor(const wstring &file);
		~InputProcessor();

		bool ProcessFile();

		void FileAndFunctionFound(shared_ptr<ProcessedBlock> block, const int &line, const string &file, const string &function);

		// finding functions //
		shared_ptr<CallstackFunction> FindFunctionEntry(const string &function);
		shared_ptr<CallstackFileEntry> FindFileEntry(const string &file);


		void DeleteEntirely(ProcessedBlock* block);
		void DeleteEntirely(CallstackFileEntry* file);
		void DeleteEntirely(CallstackFunction* func);
	protected:
		
		bool _ProcessBlockStartLine(const string &line, shared_ptr<ProcessedBlock>* blockptr);
		bool _ProcessCallstackFunctionLine(const string &line, shared_ptr<ProcessedBlock>* blockptr);
		void _FinalizeDataImport();
		// ------------------------------------ //

		// file reader //
		ifstream Reader;

		// list of all the data blocks that are in the file (these vectors are responsible for cleaning everything up) //
		vector<shared_ptr<ProcessedBlock>> LoadedBlocks;
		vector<shared_ptr<CallstackFileEntry>> LeakingFiles;
		vector<shared_ptr<CallstackFunction>> LeakingFunctions;
	};

}
#endif