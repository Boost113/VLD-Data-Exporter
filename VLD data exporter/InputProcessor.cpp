#include "stdafx.h"
// ------------------------------------ //
#ifndef VLDDEXPORTER_INPUTPROCESSOR
#include "InputProcessor.h"
#endif
using namespace VLDDexporter;
// ------------------------------------ //

// comparison functions for sorting //

bool CompareSmartProcessedBlocks(const shared_ptr<ProcessedBlock> &first, const shared_ptr<ProcessedBlock> &second){

	return first->LeakedBytes > second->LeakedBytes;
}

bool CompareSmartCallstackFile(const shared_ptr<CallstackFileEntry> &first, const shared_ptr<CallstackFileEntry> &second){

	return first->BlameCount > second->BlameCount;
}

bool CompareSmartCallstackFunction(const shared_ptr<CallstackFunction> &first, const shared_ptr<CallstackFunction> &second){

	return first->BlameCount > second->BlameCount;
}

bool ComparePtrCallstackFunction(const CallstackFunction* first, const CallstackFunction* second){

	return first->BlameCount > second->BlameCount;
}

// ------------------ InputProcessor ------------------ //
VLDDexporter::InputProcessor::InputProcessor(const wstring &file){
	// open file for reading //
	Reader.open(file, ios::in);
}

VLDDexporter::InputProcessor::~InputProcessor(){
	// close file //
	Reader.close();
}

bool VLDDexporter::InputProcessor::ProcessFile(){
	// we'll return false if we cannot read the file //
	if(!Reader.is_open()){

		return false;
	}
	// lines cannot be longer than this or the rest is lost //
	unique_ptr<char> tmpbuf(new char[500]); 

	shared_ptr<ProcessedBlock> ProcessingBlock(NULL);
	bool ProcessingCallstack = false;
	int LineNumber = 0;

	// read the file line by line and store the data //
	while(Reader.good()){
		// read new buffer of data //
		Reader.getline(tmpbuf.get(), 500);
		LineNumber++;
		// force ending null terminator //
		tmpbuf.get()[499] = '\0';
		// convert to string and check length //
		string curline = tmpbuf.get();
		if(curline.length() < 2)
			// skip empty //
			continue;

		// process line //
		if(curline[0] == '-'){
			ProcessingCallstack = false;
			if(!_ProcessBlockStartLine(curline, &ProcessingBlock)){

				cout << "failed to process line: " << curline << endl;
				return false;
			}
			continue;
		}
		if(ProcessingCallstack){
			// check did the callstack end //
			if(curline.find("Data:") != string::npos){
				// ended //
				ProcessingCallstack = false;
				ProcessingBlock.reset();
				continue;
			}
			if(!_ProcessCallstackFunctionLine(curline, &ProcessingBlock)){

				cout << "failed to process callstack entry, line (" << LineNumber << "): " << curline << endl;
				ProcessingCallstack = false;
				ProcessingBlock.reset();
				continue;
			}
		}
		// check have we reached callstack line //
		if(curline.find("Call Stack:") != string::npos){
			ProcessingCallstack = true;
			continue;
		}
		// we'll just skip this line //


	}
	// sorts imported data //
	_FinalizeDataImport();


	return true;
}

bool VLDDexporter::InputProcessor::_ProcessBlockStartLine(const string &line, shared_ptr<ProcessedBlock>* blockptr){
	// we assume that this line is like this:
	// ---------- Block 327 at 0x00E89880: 8 bytes ----------

	// can only be line with new block start //
	size_t i = line.find_first_of('k');
	if(i == string::npos){
		cout << "error invalid file, block line is invalid";
		return false;
	}
	// we need to parse an integer from the line //
	string number;
	// -1 when converted to unsigned will be very large number (easy to find errors) //
	UINT blocknumber = -1;
	int leakedamount = -1;
	i++;
	for(i; i < line.size(); i++){
		// if this is a part of number add it //
		if(line[i] >= '0' && line[i] <= '9'){
			// this could be quite performance poison, but 4 digit numbers won't affect that much (*shouldn't*) //
			number += line[i];
			continue;
		}
		if(number.size() > 0){
			// we got the number //
			break;
		}
	}
	// parse number //
	stringstream converter;
	converter.str(number);
	converter >> blocknumber;

	number.clear();

	// byte count //
	bool stoplooking = false;
	i++;
	for(i; i < line.size(); i++){
		if(!stoplooking){
			if(line[i] == ':'){
				stoplooking = true;
			}
		}
		// we have found the number //
		if(line[i] >= '0' && line[i] <= '9'){
			// this could be quite performance poison, but 4 digit numbers won't affect that much (*shouldn't*) //
			number += line[i];
			continue;
		}
		if(number.size() > 0){
			// we got the number //
			break;
		}
	}
	converter.str(number);
	converter >> leakedamount;

	// create new block //
	(*blockptr) = shared_ptr<ProcessedBlock>(new ProcessedBlock(blocknumber, leakedamount));
	// add to data vector //
	LoadedBlocks.push_back((*blockptr));

	return true;
}

bool VLDDexporter::InputProcessor::_ProcessCallstackFunctionLine(const string &line, shared_ptr<ProcessedBlock>* blockptr){
	// we assume that this line is like this:
	// c:\program files (x86)\microsoft visual studio 10.0\vc\include\xmemory (36): EngineD.dll!std::_Allocate<std::_Container_proxy> + 0x15 bytes

	// let's try to find the line number //
	size_t i = line.find_first_of(":", 10);
	if(i == string::npos){

		cout << "couldn't find second ':' (if path is shorter than 10 characters this might happen) " << endl;
		return false;
	}
	// at this point let's copy the function name //
	size_t funcstart = i+2;
	size_t funcend = line.find_last_of('+')-2;


	// first non space character //
	size_t filenamestart = line.find_first_not_of(" ");
	size_t filenameend = 0;

	size_t numstart;
	size_t numend;


	for(i; i > filenamestart; i--){
		if(line[i] == '('){

			numstart = i+1;
			filenameend = i-2;
			break;
		}
		if(line[i] == ')'){

			numend = i-1;
			continue;
		}
	}

	int fileline = -1;

	stringstream converter;
	converter.str(line.substr(numstart, numend-numstart+1));
	converter >> fileline;

	string filename = line.substr(filenamestart, filenameend-filenamestart+1);
	string funcname = line.substr(funcstart, funcend-funcstart+1);

	// this will automatically add new function to the tree and file if they don't exist or increase existing ones //
	FileAndFunctionFound(*blockptr, fileline, filename, funcname);

	return true;
}

void VLDDexporter::InputProcessor::FileAndFunctionFound(shared_ptr<ProcessedBlock> block, const int &line, const string &file, const string &function){
	// check do we already have a file found //
	shared_ptr<CallstackFileEntry> tmpfile = FindFileEntry(file);
	if(tmpfile.get() == NULL){

		tmpfile = shared_ptr<CallstackFileEntry>(new CallstackFileEntry(file));
		LeakingFiles.push_back(tmpfile);
	}
	

	// get function //
	shared_ptr<CallstackFunction> tmpfunc = FindFunctionEntry(function);
	if(tmpfunc.get() == NULL){

		tmpfunc = shared_ptr<CallstackFunction>(new CallstackFunction(function, line));
		LeakingFunctions.push_back(tmpfunc);
	} else {
		// add blame //
		tmpfunc->BlameCount++;
	}
	// always add to block //
	block->FunctionsInBlock.push_back(tmpfunc.get());

	bool found = false;
	for(size_t i = 0; i < tmpfile->FunctionsInfile.size(); i++){
		if(tmpfile->FunctionsInfile[i]->Name == function){
			// the function is already added //
			found = true;
			break;
		}
	}

	if(found){
		// let's just add a blame for another leak of the same function //
		tmpfile->BlameCount++;
		return;
	}
	// we need to add the function //
	tmpfile->AddFunctionToFile(tmpfunc.get());
}

shared_ptr<CallstackFunction> VLDDexporter::InputProcessor::FindFunctionEntry(const string &function){
	// loop and return matching entry, or NULL //
	for(size_t i = 0; i < LeakingFunctions.size(); i++){
		if(LeakingFunctions[i]->Name == function)
			return LeakingFunctions[i];
	}
	return NULL;
}

shared_ptr<CallstackFileEntry> VLDDexporter::InputProcessor::FindFileEntry(const string &file){
	// loop and return matching entry, or NULL //
	for(size_t i = 0; i < LeakingFiles.size(); i++){
		if(LeakingFiles[i]->PathOrName == file)
			return LeakingFiles[i];
	}
	return NULL;
}

void VLDDexporter::InputProcessor::_FinalizeDataImport(){
	// sort all the vectors according to blame count //
	sort(LoadedBlocks.begin(), LoadedBlocks.end(), CompareSmartProcessedBlocks);
	sort(LeakingFiles.begin(), LeakingFiles.end(), CompareSmartCallstackFile);
	sort(LeakingFunctions.begin(), LeakingFunctions.end(), CompareSmartCallstackFunction);

	// we need to sort functions in files //
	for(size_t i = 0; i < LeakingFiles.size(); i++){


		sort(LeakingFiles[i]->FunctionsInfile.begin(), LeakingFiles[i]->FunctionsInfile.end(), ComparePtrCallstackFunction);
	}

}

void VLDDexporter::InputProcessor::DeleteEntirely(ProcessedBlock* block){
	// find block and erase it //
	for(size_t i = 0; i < LoadedBlocks.size(); i++){
		if(LoadedBlocks[i].get() == block){

			LoadedBlocks.erase(LoadedBlocks.begin()+i);
			break;
		}
	}



}

void VLDDexporter::InputProcessor::DeleteEntirely(CallstackFileEntry* file){
	// find file and erase it //
	for(size_t i = 0; i < LeakingFiles.size(); i++){
		if(LeakingFiles[i].get() == file){

			LeakingFiles.erase(LeakingFiles.begin()+i);
			break;
		}
	}

	// find all references and delete them //
	for(size_t i = 0; i < LeakingFunctions.size(); i++){
		if(LeakingFunctions[i]->ContainedInFile == file){
			// delete this too //
			DeleteEntirely(LeakingFunctions[i].get());
			i--;
			continue;
		}
	}

}

void VLDDexporter::InputProcessor::DeleteEntirely(CallstackFunction* func){
	// find function and erase it //
	for(size_t i = 0; i < LeakingFunctions.size(); i++){
		if(LeakingFunctions[i].get() == func){

			LeakingFunctions.erase(LeakingFunctions.begin()+i);
			break;
		}
	}

	// find all references and delete them //
	for(size_t i = 0; i < LeakingFiles.size(); i++){
		for(size_t a = 0; a < LeakingFiles[i]->FunctionsInfile.size(); a++){
			if(LeakingFiles[i]->FunctionsInfile[a] == func){

				LeakingFiles[i]->FunctionsInfile.erase(LeakingFiles[i]->FunctionsInfile.begin()+a);
				break;
			}
		}
		if(LeakingFiles[i]->FunctionsInfile.size() == 0){
			// delete this too //
			DeleteEntirely(LeakingFiles[i].get());
			i--;
			continue;
		}
	}

	for(size_t i = 0; i < LoadedBlocks.size(); i++){
		for(size_t a = 0; a < LoadedBlocks[i]->FunctionsInBlock.size(); a++){
			if(LoadedBlocks[i]->FunctionsInBlock[a] == func){

				LoadedBlocks[i]->FunctionsInBlock.erase(LoadedBlocks[i]->FunctionsInBlock.begin()+a);
				break;
			}
		}
		if(LoadedBlocks[i]->FunctionsInBlock.size() == 0){
			// delete this too //
			DeleteEntirely(LoadedBlocks[i].get());
			i--;
			continue;
		}
	}

}

// ------------------ ClassStackFileEntry ------------------ //
VLDDexporter::CallstackFileEntry::CallstackFileEntry(const string &filename) : PathOrName(filename), BlameCount(0){

}

void VLDDexporter::CallstackFileEntry::AddFunctionToFile(CallstackFunction* func){
	// increase blame count //
	BlameCount++;

	LeakLines.push_back(func->Line);
	FunctionsInfile.push_back(func);
	// set function's file pointer to this //
	func->ContainedInFile = this;
}
// ------------------ CallStackFunction ------------------ //
VLDDexporter::CallstackFunction::CallstackFunction(const string &functionname, const int &line) : Name(functionname), Line(line){
	// one initial occurrence //
	BlameCount = 1;
}
// ------------------ ProcessedBlock ------------------ //
VLDDexporter::ProcessedBlock::ProcessedBlock(const UINT &number, const int &bytecount) : AllocationNumber(number) , LeakedBytes(bytecount){

}
