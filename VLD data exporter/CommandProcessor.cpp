#include "stdafx.h"
// ------------------------------------ //
#ifndef VLDDEXPORTER_COMMANDPROCESSOR
#include "CommandProcessor.h"
#endif
#include "ExcelFormat.h"
#include "BasicExcel.hpp"
#include "InputProcessor.h"
using namespace VLDDexporter;
using namespace YExcel;
using namespace ExcelFormat;
// ------------------------------------ //


VLDDexporter::CommandProcessor::CommandProcessor(const wstring &file){
	// open read //
	Reader.open(file, ios::in);

	// get path from the input file //
	wstring tmpstr = file.substr(0, 1+file.find_last_of('\\'));
	OutputFilePath = "";
	OutputFilePath.assign(tmpstr.begin(), tmpstr.end());


	// set default settings //
	OutputFile = DEFAULT_OUTPUTFILE;

}

VLDDexporter::CommandProcessor::~CommandProcessor(){
	// close file //
	Reader.close();
}

bool VLDDexporter::CommandProcessor::ProcessCommands(){
	// function fails if we have failed to open the file //
	if(!Reader.is_open()){
		return false;
	}

	// load settings from file //

	// lines cannot be longer than this or the rest is lost //
	unique_ptr<char> tmpbuf(new char[500]); 

	bool RemoveFilters = true;

	// read the file line by line and store the data //
	while(Reader.good()){
		// read new buffer of data //
		Reader.getline(tmpbuf.get(), 500);

		// force ending null terminator //
		tmpbuf.get()[499] = '\0';

		// convert to string and check length //
		string curline = tmpbuf.get();
		if(curline.length() < 2)
			// skip empty //
			continue;

		// process command //
		if(RemoveFilters){
			// check for end //
			size_t i = curline.find("OutputFile");
			if(i == 0){
				// end found 
				RemoveFilters = false;
				continue;
			}
			// add to filters //
			this->RemoveFilters.push_back(unique_ptr<basic_regex<char>>(new basic_regex<char>(curline)));

			continue;
		}

		size_t i = curline.find("OutputFile");
		if(i != string::npos){
			// set new output file //
			size_t start;

			for(size_t a = string("OutputFile").size(); a < curline.size(); a++){
				if(curline[a] != ' '){
					start = a;
					break;
				}
			}


			OutputFile = curline.substr(start, curline.size()-start);
			continue;
		}
		i = curline.find("RemoveFilters:");
		if(i != string::npos){
			// entered remove filters //
			RemoveFilters = true;
			// clear possible default filters //
			this->RemoveFilters.clear();
			continue;
		}


	}

	return true;
}

std::string VLDDexporter::CommandProcessor::RunCommands(InputProcessor& data){
	// let's run filters on the input data //
	_FilterInputData(data);

	// run the routine with the loaded parameters //
	// output spreadsheet //
	BasicExcel xls;

	// workbook sheets:
	// hot files, hot functions, summary, leaked blocks
	xls.New(4);

	// create hot files sheet //
	BasicExcelWorksheet* hotfilessheet = xls.GetWorksheet(0);
	hotfilessheet->Rename("Hot files");

	// hot functions sheet //
	BasicExcelWorksheet* hotfunctions = xls.GetWorksheet(1);
	hotfunctions->Rename("Hot functions");

	// summary sheet //
	BasicExcelWorksheet* summarysheet = xls.GetWorksheet(2);
	summarysheet->Rename("Summary");

	// leaked blocks //
	BasicExcelWorksheet* leakedblocks = xls.GetWorksheet(3);
	leakedblocks->Rename("Leaked blocks");


	XLSFormatManager fmt_mgr(xls);

	// create required fonts //
	ExcelFont fontheader;
	fontheader._weight = FW_BOLD;
	//fontheader._color_index = 
	ExcelFont fontdata;
	fontheader._weight = FW_NORMAL;

	CellFormat fmtheader(fmt_mgr);
	fmtheader.set_font(fontheader);

	CellFormat fmtdata(fmt_mgr);
	fmtdata.set_font(fontdata);

	int Row = 0;
	int Col = 0;

	// hot files //
	for(Col = 2; Col < 6; Col++){
		// get current cell //
		BasicExcelCell* cell = hotfilessheet->Cell(Row, Col);

		switch(Col){
		case 2: { cell->Set("File"); } break;
		case 3: { cell->Set("Blame count"); } break;
		case 4: { cell->Set("Lines"); } break;
		case 5: { cell->Set("Blame gathering function"); } break;
		}


		cell->SetFormat(fmtheader);
	}
	// set proper widths //
	hotfilessheet->SetColWidth(2, 24000);
	hotfilessheet->SetColWidth(3, 5000);
	hotfilessheet->SetColWidth(4, 6000);
	hotfilessheet->SetColWidth(5, 10000);

	// output data //
	for(size_t i = 0; i < data.LeakingFiles.size(); i++){
		// increase row //
		Row++;

		CallstackFileEntry* file = data.LeakingFiles[i].get();

		for(Col = 2; Col < 6; Col++){
			// get current cell //
			BasicExcelCell* cell = hotfilessheet->Cell(Row, Col);


			switch(Col){
			case 2: { cell->Set(file->PathOrName.c_str()); } break;
			case 3: { cell->SetInteger(file->BlameCount); } break;
			case 4: 
				{
					string lines;

					for(size_t a = 0; a < file->LeakLines.size(); a++){
						if(a != 0)
							lines += ", ";
						stringstream converter;
						converter << file->LeakLines[a];
						string tmpstr = "";
						converter >> tmpstr;
						lines += tmpstr;
					}
					cell->Set(lines.c_str()); 
				}
				break;
			case 5: { cell->Set(file->FunctionsInfile[0]->Name.c_str()); } break;
			}


			cell->SetFormat(fmtdata);
		}
	}

	Row = 0;
	Col = 0;
	// output hot functions //
	for(Col = 2; Col < 6; Col++){
		// get current cell //
		BasicExcelCell* cell = hotfunctions->Cell(Row, Col);

		switch(Col){
		case 2: { cell->Set("Function"); } break;
		case 3: { cell->Set("Blame count"); } break;
		case 4: { cell->Set("File"); } break;
		case 5: { cell->Set("Line"); } break;
		}


		cell->SetFormat(fmtheader);
	}

	// set proper widths //
	hotfunctions->SetColWidth(2, 32000);
	hotfunctions->SetColWidth(3, 5000);
	hotfunctions->SetColWidth(4, 24000);

	// output data //
	for(size_t i = 0; i < data.LeakingFunctions.size(); i++){
		// increase row //
		Row++;

		CallstackFunction* func = data.LeakingFunctions[i].get();

		for(Col = 2; Col < 6; Col++){
			// get current cell //
			BasicExcelCell* cell = hotfunctions->Cell(Row, Col);

			switch(Col){
			case 2: { cell->Set(func->Name.c_str()); } break;
			case 3: { cell->SetInteger(func->BlameCount); } break;
			case 4: { cell->Set(func->ContainedInFile->PathOrName.c_str()); } break;
			case 5: { cell->SetInteger(func->Line); } break;
			}


			cell->SetFormat(fmtdata);
		}

	}

	string finaloutput = OutputFilePath+OutputFile;

	// delete old first //
	wstringstream sstream;
	sstream << finaloutput.c_str();
	wstring path;
	sstream >> path;

	DeleteFile((L"\\\\?\\"+path).c_str());

	// save file //
	xls.SaveAs(finaloutput.c_str());

	// return the created output //
	return OutputFilePath+OutputFile;
}

void VLDDexporter::CommandProcessor::_FilterInputData(InputProcessor& data){
	// run filters as regex on all three vectors and completely erase all matching data //
	// we can return if we have empty filters //
	if(RemoveFilters.size() == 0)
		return;

	int DelCount = 0;

	for(size_t i = 0; i < data.LeakingFiles.size(); i++){

		bool del = false;
		// check for match with regex //
		for(size_t a = 0; a < RemoveFilters.size(); a++){
			// check for match // 
			if(regex_search(data.LeakingFiles[i]->PathOrName, *RemoveFilters[a])){

				del = true;
				break;
			}
		}



		if(del){
			cout << "del file " << data.LeakingFiles[i]->PathOrName << endl;
			data.DeleteEntirely(data.LeakingFiles[i].get());
			i--;
			DelCount++;
			continue;
		}
	}

	for(size_t i = 0; i < data.LeakingFunctions.size(); i++){

		bool del = false;
		// check for match with regex //
		for(size_t a = 0; a < RemoveFilters.size(); a++){
			// check for match // 
			if(regex_search(data.LeakingFunctions[i]->Name, *RemoveFilters[a])){

				del = true;
				break;
			}
		}



		if(del){
			cout << "del func " << data.LeakingFunctions[i]->Name << endl;
			data.DeleteEntirely(data.LeakingFunctions[i].get());
			i--;
			DelCount++;
			continue;
		}
	}

	cout << "deleted " << DelCount << " data entries due to remove filters" << endl;
}
