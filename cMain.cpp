#define _CRT_SECURE_NO_WARNINGS
#define NOMINMAX

#include <Windows.h>
#undef small

#include "cMain.h"
#include "wx/image.h"
#include "image_utils.h"
#include "mouse_utils.h"

#include <wx/utils.h> 
#include <wx/textfile.h>

#include <fstream>
#include <sstream>
#include <string>

#include <boost/filesystem.hpp>

#include <map>

#include <wx/dcbuffer.h>

#include <iostream>
#include <cstdlib>

#include <wx/wx.h>

#include <wx/dcgraph.h>
#include <wx/graphics.h>

#include "GCSvg.h"

#include "boost/lexical_cast.hpp"

#include <PDF/PDFNet.h>
#include <PDF/PDFDoc.h>
#include <PDF/Print.h> // new Print API
#include <iostream>

#include <utf8.h>
//#include <svgpp/svgpp.hpp>
#include <vector>

#define NANOSVG_IMPLEMENTATION	// Expands implementation
#include "nanosvg.h"

#include <wx/dir.h>
#include <wx/filename.h>

struct page {
	wxString filePath;
	int x;
	int y;
	int w;
	int h;
	int nw;
	int nh;
};

enum
{
	ID_Open = 1,
	ID_Cache=2,
	ID_Settings=3,
	ID_Darken=4,
	ID_PaneOpen=5
};

wxBEGIN_EVENT_TABLE(cMain, wxFrame)
EVT_MENU(ID_Open, cMain::Open)
EVT_MENU(wxID_EXIT, cMain::OnExit)
EVT_MENU(ID_Cache,cMain::OnCache)
EVT_MENU(ID_Darken,cMain::OnDarken)
EVT_BUTTON(ID_PaneOpen, cMain::Open)

EVT_BUTTON(100, cMain::OpenOne)
EVT_BUTTON(101, cMain::OpenTwo)
EVT_BUTTON(102, cMain::OpenThree)
EVT_BUTTON(103, cMain::OpenFour)
EVT_BUTTON(104, cMain::OpenFive)

EVT_CLOSE(cMain::OnClose)
EVT_PAINT(cMain::OnPaint)
EVT_MOTION(cMain::OnMouseMotion)
EVT_LEFT_DOWN(cMain::OnLeftDown)
EVT_LEFT_UP(cMain::OnLeftUp)
EVT_SIZE(cMain::OnResize)
EVT_ERASE_BACKGROUND(cMain::OnEraseBackground)
EVT_MOUSEWHEEL(cMain::OnMouseWheel)
EVT_MAXIMIZE(cMain::OnMaximize)
wxEND_EVENT_TABLE()

//start from 1
int currentPage=1;
wxString currentFilename = "";
int currentPageScroll=0;
int currentPageZoom=1;
wxString currentFileKey="";
bool currentCache = false;
bool currentDarken = true;
wxString currentImageDirPath="";
int currentPageCount=0;
int currentPageStart = 1;
int currentPageEnd = 1;

//pixel for scrolling
double currentScrollValue=0;
double currentPageHeight=0;
double currentScrollPercent = 0.0;

int originalHeight;
int originalWidth;

wxString currentMouseName="";
wxString currentMouseDown="";
wxString currentMouseDrag="";

int distMx = 0;
int distMy = 0;

int oldMx = 0;
int oldMy = 0;

wxBitmap* pageBitmaps[1200];

double distScroll = 0;
int contentWidth = 0;
int clientw;
int clienth;
int scrollSize = 20;
UINT32 scrollbar_arrow_color = 0xe8e8ec;
UINT32 scrollbar_color = 0xc2c3c9;
UINT32 scrollbar_over_color = 0x767676;
int currentTotalHeight=0;
double currentScrollDelta=0.0;
bool currentLoadFinish = false;
int currentLastLoaded = 0;
wxString currentFileTitle="";
//pages <bookname and page range, page bulk>
std::map<wxString, struct page[100]> bulkPages;

//current book open bulkPages key
std::map<wxString, wxString> currentKeys;

int currentSpacing = 10;

bool opened = false;
bool resize_mark;

std::map<int, wxString> recents;

//wxMenu* dictonary 
std::map<int, wxMenu*> menuItems;

void cMain::OpenOne(wxCommandEvent& event) {
	openPDF(recents[0]);
	SetStatusText(recents[0]);
}

void cMain::OpenTwo(wxCommandEvent& event) {
	openPDF(recents[1]);
}

void cMain::OpenThree(wxCommandEvent& event) {
	openPDF(recents[2]);
}

void cMain::OpenFour(wxCommandEvent& event) {
	openPDF(recents[3]);
}

void cMain::OpenFive(wxCommandEvent& event) {
	openPDF(recents[4]);
}

void cMain::output(std::string str) {
	std::ofstream outfile;
	outfile.open("output.txt", 'a');

	if (outfile.is_open())
	{
		outfile << str;

		outfile.flush();
		outfile.close();
	}
}

void saveToFileVar(std::string var,std::string value) {
	std::string lineCon = currentFileKey.ToStdString() + " " + var;
	std::string lineCon2 = lineCon + " " + value+"\n";

	std::ifstream infile("var-list.txt");

	std::string line;
	std::string filestr;

	filestr += lineCon2;
	while (std::getline(infile, line))
	{
		if (line.find(lineCon) == std::string::npos) {
			filestr += line + '\n';
		}
	}
	infile.close();

	std::ofstream outfile;
	outfile.open("var-list.txt");
	outfile << filestr;
	outfile.close();

}


void cMain::pushToRecent() {
	//construct a string
	//filename

	wxCharBuffer buffer = currentFilename.ToUTF8();
	char* c = buffer.data();
	std::string filename = c;

	//open read file
	std::ifstream infile;
	infile.open("recent-list.txt");
	std::string line;
	std::string fileStr;

	fileStr += (filename + '\n');
	//read all except the same string
	int count = 0;
	while (std::getline(infile, line, '\n')) {
		if (line!=filename && count<5) {
			fileStr += line + '\n';
			count++;
		}
	}

	//write all to file
	std::ofstream outfile;
	outfile.open("recent-list.txt");
	outfile << fileStr;
	outfile.close();

}

void changeCacheList(bool cache) {

	//read all lines untill hit the key file name and space in line
	std::ifstream infile("cache-list.txt");

	std::string filestr;
	std::string line;
	std::string filekey = currentFileKey.ToStdString() + " ";
	//get key file name size
	int filekeySize = filekey.size();
	std::string lineCon;

	//construct the line to change
	if (cache) {
		lineCon = filekey + "y\n";
	}
	else {
		lineCon = filekey + "n\n";
	}

	//read and get all other lines to string
	filestr += lineCon;
	while (std::getline(infile, line))
	{
		if (line.find(filekey) == std::string::npos) {
			filestr += line + '\n';
		}
	}
	infile.close();
	//write whole string to file

	std::ofstream outfile;
	outfile.open("cache-list.txt");
	outfile << filestr;
	outfile.close();

}

void getMenuCheck() {
	wxMenu* settings = menuItems[ID_Settings];
	changeCacheList(settings->IsChecked(ID_Cache));
	currentCache = settings->IsChecked(ID_Cache);
}

void cMain::OnCache(wxCommandEvent& event) {
	//save cache setting
	getMenuCheck();
}

void clearBitmaps() {
	for (int i = 0; i < 500; i++) {
		if (pageBitmaps[i]) {
			pageBitmaps[i]->FreeResource();
			pageBitmaps[i] = NULL;
		}
	}
}


void cMain::OnDarken(wxCommandEvent& event) {
	wxMenu* settings = menuItems[ID_Settings];
	currentDarken = settings->IsChecked(ID_Darken);

	clearBitmaps();
	Refresh();
	Update();

	//save to file
	if (currentDarken) {
		saveToFileVar("darken","y");
	}
	else {
		saveToFileVar("darken","n");
	}
}

wxString cMain::getBulkPagesKey(wxString filekey, int pageNumber) {

	int hpage = (int)(pageNumber/100)*100;
	int nhpage = hpage + 100;

	wxString key = filekey+wxT(" page:") + std::to_string(hpage) + wxT("~") + std::to_string(nhpage);

	return key;
}

std::string utf16to8(std::string str) {
	std::string str2;

	utf8::utf16to8(str.begin(), str.end(), std::back_inserter(str2));

	return str2;
}

std::string utf8to32(std::string str) {
	std::string str2;
	utf8::utf8to32(str.begin(), str.end(), std::back_inserter(str2));
	return str2;
}


bool cMain::checkCacheList() {

	//read all lines untill hit the key file name and space in line
	std::ifstream infile("cache-list.txt");

	std::string line;
	std::string filekey = currentFileKey.ToStdString()+" ";
	//get key file name size
	int filekeySize = filekey.size();
	bool cache = false;
	while (std::getline(infile, line))
	{
		if (line.find(filekey)!=std::string::npos) {
			//get y or n on that line
			
			if (line[filekeySize] == 'y') {
				cache = true;
			}
			break;
		}
	}
	if (cache) {
		output("cache");
	}
	
	currentCache = cache;
	return cache;
}


void cMain::checkMenuCB2() {

	wxMenu* settings = menuItems[ID_Settings];
	settings->Check(ID_Cache, currentCache);
}

void cMain::checkMenuCB() {

	bool cache=checkCacheList();

	wxMenu* settings = menuItems[ID_Settings];
	settings->Check(ID_Cache,cache);
	
}

//var list for this current filename
std::map<std::string, std::string> varlist;

void cMain::checkFileVars() {
	
	std::ifstream infile("var-list.txt");

	std::string line;
	std::string filekey = currentFileKey.ToStdString() + " ";
	
	int filekeySize = filekey.size();
	bool cache = false;
	while (std::getline(infile, line))
	{
		if (line.find(filekey) != std::string::npos) {
			//parse the line and add to varlist
			std::string subline = line.substr(filekeySize);
			int nspace=subline.find(" ");

			std::string varname = subline.substr(0, nspace);
			std::string varvalue = subline.substr((size_t)nspace + 1);

			varlist[varname] = varvalue;
		}
	}
}

void cMain::clearLayout() {
	this->GetSizer()->Clear(true);
}

cMain::cMain(int open,wxString filename) :wxFrame(nullptr,wxID_ANY,"Fast PDF Reader",wxDefaultPosition,wxSize(700,500)) {
	
	wxMenu* menuFile = new wxMenu();
	menuFile->Append(ID_Open, "&Open\tCtrl-O","Open PDF file");
	menuFile->AppendSeparator();
	menuFile->Append(wxID_EXIT);

	wxMenu* menuSettings = new wxMenu();
	menuItems[ID_Settings] = menuSettings;

	menuSettings->AppendCheckItem(ID_Cache, "&Cache", "Set cache for this PDF");
	menuSettings->AppendCheckItem(ID_Darken, "&Darken", "Darken PDF content");

	menuSettings->Check(ID_Darken, true);

	wxMenuBar* menuBar = new wxMenuBar();
	menuBar->Append(menuFile, "&File");
	menuBar->Append(menuSettings, "&Settings");

	SetMenuBar(menuBar);

	CreateStatusBar();
	SetStatusText("Ready");

	Centre();

	if (open) {
		
		SetStatusText(wxT("Opening ") + filename);
		std::string filename_str=utf16to8(filename.ToStdString());
		wxString mystring(filename_str.c_str(), wxConvUTF8);

		openPDF(mystring);
	}
	else {
		//draw open and open recent



		wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);

		sizer->AddSpacer(100);

		sizer->Add(new wxButton(this, ID_PaneOpen, "Open"), 0,wxALIGN_CENTER, 0);


		sizer->AddSpacer(20);
		
		std::ifstream infile;
		infile.open("recent-list.txt");
		std::string line;
		int count = 0;

		int btnID = 100;

		sizer->Add(new wxStaticText(this, -1, wxT("Open Recent Files:"), wxDefaultPosition, wxSize(150, 25), wxBU_LEFT),0, wxALIGN_CENTER,0);


		while (std::getline(infile, line, '\n')) {
			if (count < 5) {

				boost::filesystem::path f(line);
				
				//wxString fn = wxT("" + f.filename());

				wxString fn(f.filename().c_str(), wxConvUTF8);
				wxString fn2(line.c_str(), wxConvUTF8);

				recents[count] = fn2;
				
				sizer->Add(new wxButton(this,btnID+count,fn, wxDefaultPosition, wxSize(150, 25), wxBU_LEFT), 0, wxALIGN_CENTER, 0);
				count++;
			}
		}


		//sizer->SetSizeHints(this);
		SetSizer(sizer);
	}



	
}


void cMain::OnResize(wxSizeEvent& event) {
	clearBitmaps();

	GetClientSize(&clientw, &clienth);
	//change scroll value when page height change

	if (opened) {
		double percent = currentScrollValue / currentTotalHeight;

		int imgw = originalWidth;
		int imgh = originalHeight;

		double w = clientw - scrollSize;
		double h = ((double)originalHeight * (double)w) / (double)originalWidth;

		currentPageHeight = h + currentSpacing;

		currentTotalHeight = currentPageHeight * currentPageCount;

		currentScrollValue = percent * currentTotalHeight;
	}

	int winw, winh;
	GetSize(&winw, &winh);

	if (!opened) {
		this->ClearBackground();
	}

	Layout();
	RefreshRect(wxRect(0,0,winw,winh));

}

wxImage cMain::processImage(wxImage img) {
	unsigned char* data2 = img.GetData();

	int w = img.GetWidth();
	int h = img.GetHeight();

	unsigned char* data = (unsigned char*)malloc(w * h * 3);
	wxImage* img2 = new wxImage(w, h, data);

	for (int i = 0; i < w * h * 3; i++) {
		if (data2[i] <200 && data2[i]>50) {
			data[i] = data2[i]-50;
		}
		else if (data2[i]<50) {
			data[i] = 0;
		}
		else {
			data[i] = data2[i];
		}
	}
	
	//img2->SetData(data);

	return *img2;
}

int c = 0;
bool painting = false;

void cMain::OnPaint(wxPaintEvent& event)
{
	if(!opened){
		this->ClearBackground();
	}

	painting = true;
	if (currentFileKey != "") {

		currentPage = 1;

		wxString bulkkey = getBulkPagesKey(currentFileKey, currentPage);
		int index = currentPage % 100;
		struct page p = bulkPages[bulkkey][index];

		//load first page
		
		int imgw = originalWidth;
		int imgh = originalHeight;

		double w = clientw - scrollSize;
		double h = ((double)originalHeight * (double)w) / (double)originalWidth;
		
		currentPageHeight = h+currentSpacing;


		currentTotalHeight =currentPageHeight * currentPageCount;
		currentPageZoom = w / imgw;

		
		//dist scroll

		if (currentMouseDrag == "scrollbar") {
			distScroll += distMy;
		}

		double totalBarLength = clienth - (scrollSize * 2);
		//visible percent
		double percent = (double)clienth / (double)currentTotalHeight;
		double barLen = percent * totalBarLength;

		if (barLen < 20) {
			barLen = 20;
		}


		double scrollPercent = (double)currentScrollValue / ((double)currentTotalHeight - (double)clienth);

		double barPos = scrollPercent * (totalBarLength - barLen);
		double distScrollPercent = 0;
		int distScrollValue = 0;

		if (barPos + distScroll <= 0 && distScroll < 0) {
			barPos = 0;
			scrollPercent = 0;
			currentScrollValue = 0;
		}
		else if (barPos + distScroll >= (totalBarLength - barLen) && distScroll > 0) {
			barPos = totalBarLength - barLen;
			scrollPercent = 1;
			currentScrollValue = (double)currentTotalHeight - (double)clienth;
		}
		else {
			distScrollPercent = distScroll / (totalBarLength - barLen);
			distScrollValue = distScrollPercent * ((double)currentTotalHeight - (double)clienth);
			currentScrollValue += distScrollValue;
		}
		distScroll = 0;

		//scrollable percent
		scrollPercent = (double)(currentScrollValue) / ((double)currentTotalHeight - (double)clienth);
		barPos = scrollPercent * (totalBarLength - barLen);

		//dist scroll

		//calculate start and end page
		currentPageStart = (currentScrollValue / currentPageHeight) + 1;
		currentPageEnd = ((currentScrollValue + clienth) / currentPageHeight) + 1;

		//output("currentScrollValue:" + std::to_string(currentScrollValue));

		if (currentPageStart > currentPageCount) {
			currentPageStart = currentPageCount;
		}
		if (currentPageEnd > currentPageCount) {
			currentPageEnd = currentPageCount;
		}

		if (currentPageStart <= 0) {
			currentPageStart = 1;
		}
		if (currentPageEnd <= 0) {
			currentPageStart = 1;
		}

		
		//scale first page
		wxBufferedPaintDC dc(this);
		dc.Clear();

		int x = 0;
		int y = 0;
		int index2 = 0;
		struct page p2;
		wxImage img2;
		wxBitmap bmp2;
		wxString bulkkey2;


		//output(std::to_string(currentPageStart));

		for (int i = currentPageStart; i <= currentPageEnd; i++) {
			y = (currentScrollValue * -1) + (i - 1) * (currentPageHeight);


			bulkkey2 = getBulkPagesKey(currentFileKey, i);
			index2 = i % 100;

			if (bulkPages[bulkkey2] == NULL) {
				return;
			}
			p2 = bulkPages[bulkkey2][index2];
			
			
			if (pageBitmaps[i]==NULL) {
				img2.LoadFile(p2.filePath);

				if (img2.IsOk()) {

					if (currentDarken) {
						img2 = processImage(img2);
					}

					wxBitmap* bmp=new wxBitmap(img2.Scale(w, h, wxIMAGE_QUALITY_BILINEAR));

					pageBitmaps[i] = bmp;
					
					
					dc.DrawBitmap(*bmp, x, y, false);
				}
			}
			else {
				dc.DrawBitmap(*pageBitmaps[i], x, y, false);
			}
			//SetStatusText(wxString::Format(wxT("%i"), c++));
		}
		
		
		//draw scrollbar
		wxBrush brush(wxColour(scrollbar_arrow_color), wxBRUSHSTYLE_SOLID);
		dc.SetPen(*wxTRANSPARENT_PEN);
		dc.SetBrush(brush);

		dc.DrawRectangle(wxPoint(clientw - scrollSize, 0), wxSize(scrollSize, clienth));

		wxBitmap icon_top;
		wxString cwd = wxGetCwd();
		wxString icon_path1;

		if (currentMouseName == wxT("topScrollButton")) {
			icon_path1 = cwd + "\\images\\arrow-top-over.png";
		}
		else {
			icon_path1 = cwd + "\\images\\arrow-top.png";
		}

		wxString icon_path2;
		if (currentMouseName == wxT("bottomScrollButton")) {
			icon_path2 = cwd + "\\images\\arrow-bottom-over.png";
		}
		else {
			icon_path2 = cwd + "\\images\\arrow-bottom.png";
		}

		
		//output(icon_path1.ToStdString());

		wxImageHandler* pngLoader = new wxPNGHandler();
		wxImage::AddHandler(pngLoader);

		wxImage img_icon1, img_icon2;
		img_icon1.LoadFile(icon_path1, wxBITMAP_TYPE_PNG);
		img_icon2.LoadFile(icon_path2, wxBITMAP_TYPE_PNG);
		wxBitmap icon_bmp1(img_icon1.Scale(scrollSize, scrollSize));
		wxBitmap icon_bmp2(img_icon2.Scale(scrollSize, scrollSize));

		pushMouseRect(clientw - scrollSize, 0, scrollSize, scrollSize, wxT("topScrollButton"));
		pushMouseRect(clientw - scrollSize, clienth - scrollSize, scrollSize, scrollSize, wxT("bottomScrollButton"));

		dc.DrawBitmap(icon_bmp1, clientw - scrollSize, 0, false);
		dc.DrawBitmap(icon_bmp2, clientw - scrollSize, clienth - scrollSize, false);

		

		UINT32 barColor;
		if (currentMouseDrag == "scrollbar" || currentMouseName == "scrollbar") {
			barColor = scrollbar_over_color;
		}
		else {
			barColor = scrollbar_color;
		}

		

		wxBrush brush2(wxColour(barColor), wxBRUSHSTYLE_SOLID);
		dc.SetPen(*wxTRANSPARENT_PEN);
		dc.SetBrush(brush2);
		dc.DrawRectangle(wxPoint(clientw - scrollSize + 5, barPos + scrollSize), wxSize(scrollSize - 10, barLen));

		pushMouseRect(clientw - scrollSize, barPos + scrollSize, scrollSize, barLen, wxT("scrollbar"));
		pushMouseRect(clientw - scrollSize, scrollSize, scrollSize, barPos, "upScroll");
		pushMouseRect(clientw - scrollSize, barPos + scrollSize + barLen, scrollSize, clienth - scrollSize - barPos - scrollSize - barLen, "downScroll");

		pushMouseRect(0, 0, clientw - scrollSize, clienth, "viewArea");

		updateLoadPercentage();

		

	}
else {



}
	event.Skip();
	painting = false;

}


void cMain::updateLoadPercentage() {

	if (currentLoadFinish == false) {
		//check loaded percentage
		wxString imagePath_r;
		wxString keyName_r;
		int index_r;
		struct page pItem;
		wxString pItem_path;
		bool e;
		for (int i = 1; i <= currentPageCount; i++) {
			imagePath_r = currentImageDirPath + getImageFilename(currentImageDirPath, i);
			keyName_r = getBulkPagesKey(currentFileKey, i);
			index_r = i % 100;
			pItem = bulkPages[keyName_r][index_r];

			pItem_path = pItem.filePath;
			boost::filesystem::path pItem_fpath(pItem_path);

			//SetStatusText("path:"+pItem_path);

			e = boost::filesystem::exists(pItem_fpath);
			if (e && i > currentLastLoaded) {
				currentLastLoaded = i;
				
				int percent = ((double)currentLastLoaded / (double)currentPageCount) * 100.00;
				wxString percent_str(std::to_string(percent).c_str(), wxConvUTF8); ;

				SetStatusText(wxT("Loading PDF... ") + percent_str + wxT("%"));

			}
			else if (e == false) {
				break;
			}
		}

		if (currentLastLoaded == currentPageCount) {
			currentLoadFinish = true;
			SetStatusText("Ready");
		}
		
	}
}

size_t ExecuteProcess(std::wstring FullPathToExe, std::wstring Parameters, size_t SecondsToWait)
{
	size_t iMyCounter = 0, iReturnVal = 0, iPos = 0;
	DWORD dwExitCode = 0;
	std::wstring sTempStr = L"";

	/* - NOTE - You should check here to see if the exe even exists */

	/* Add a space to the beginning of the Parameters */
	if (Parameters.size() != 0)
	{
		if (Parameters[0] != L' ')
		{
			Parameters.insert(0, L" ");
		}
	}

	/* The first parameter needs to be the exe itself */
	sTempStr = FullPathToExe;
	iPos = sTempStr.find_last_of(L"\\");
	sTempStr.erase(0, iPos + 1);
	Parameters = sTempStr.append(Parameters);

	/* CreateProcessW can modify Parameters thus we allocate needed memory */
	wchar_t* pwszParam = new wchar_t[Parameters.size() + 1];
	if (pwszParam == 0)
	{
		return 1;
	}
	const wchar_t* pchrTemp = Parameters.c_str();
	wcscpy_s(pwszParam, Parameters.size() + 1, pchrTemp);

	/* CreateProcess API initialization */
	STARTUPINFOW siStartupInfo;
	PROCESS_INFORMATION piProcessInfo;
	memset(&siStartupInfo, 0, sizeof(siStartupInfo));
	memset(&piProcessInfo, 0, sizeof(piProcessInfo));
	siStartupInfo.cb = sizeof(siStartupInfo);
	siStartupInfo.dwFlags = STARTF_USESHOWWINDOW;
	siStartupInfo.wShowWindow = SW_HIDE;

	if (CreateProcessW(const_cast<LPCWSTR>(FullPathToExe.c_str()),
		pwszParam, 0, 0, false,
		CREATE_DEFAULT_ERROR_MODE, 0, 0,
		&siStartupInfo, &piProcessInfo) != false)
	{
		/* Watch the process. */
		dwExitCode = WaitForSingleObject(piProcessInfo.hProcess, (SecondsToWait * 1000));
	}
	else
	{
		/* CreateProcess failed */
		iReturnVal = GetLastError();
	}

	/* Free memory */
	delete[]pwszParam;
	pwszParam = 0;

	/* Release handles */
	CloseHandle(piProcessInfo.hProcess);
	CloseHandle(piProcessInfo.hThread);

	return iReturnVal;
}

DWORD WINAPI mythread(__in LPVOID lpParameter)
{
	wxString* cmd = (wxString*)lpParameter;
	wxArrayString strs;
	ExecuteProcess(L"pdftopng.exe",cmd->ToStdWstring(), 5);
	return 0;
}

void cMain::gotoPage(int page) {
	currentScrollValue = (page - 1) * (currentPageHeight);
}


void cMain::getCurrentScroll() {

	wxString filePath = currentImageDirPath + wxT("data.txt");

	if (wxFileExists(filePath) == false || currentImageDirPath == "") {
		return;
	}


	std::ifstream infile;
	infile.open(filePath.ToStdString());
	std::string line;

	std::string ss= "scrollPercent:";

	while (std::getline(infile, line, '\n')) {
		if (line.find(ss) != std::string::npos) {
			std::string v_str = line.substr(ss.size());
			double v  = boost::lexical_cast<double>(v_str);
			currentScrollValue = v * currentTotalHeight;
			break;
		}
	}

	infile.close();

}


//remove directory
bool RemDirRF(wxString rmDir) {

	if (rmDir.length() <= 3) {
		wxLogError("You asked RemDirRF() to delete a possible root directory.\
  This is disabled by default.  If you really need to delete \"" +
			rmDir + "\" please alter the RemDirRF() definition.");

		return false;
	}
	//*/
	// *************************************************************
	// END SAFETY CHECK
	// *************************************************************

	// first make sure that the dir exists
	if (!wxDir::Exists(rmDir)) {
		wxLogError(rmDir + " does not exist.  Could not remove directory.");
		return false;
	}

	// append a slash if we don't have one
	if (rmDir[rmDir.length() - 1] != wxFILE_SEP_PATH) {
		rmDir += wxFILE_SEP_PATH;
	}

	// define our directory object.  When we begin traversing it, the
	// os will not let go until the object goes out of scope.
	wxDir* dir = new wxDir(rmDir);

	// check for allocation failure
	if (dir == NULL) {
		wxLogError("Could not allocate new memory on the heap!");
		return false;
	}

	// our file name temp var
	wxString filename;
	// get the first filename
	bool cont = dir->GetFirst(&filename);

	// if there are files to process
	if (cont) {
		do {
			// if the next filename is actually a directory
			if (wxDirExists(rmDir + filename)) {
				// delete this directory
				RemDirRF(rmDir + filename);
			}
			else {
				// otherwise attempt to delete this file
				if (!wxRemoveFile(rmDir + filename)) {
					// error if we couldn't
					wxLogError("Could not remove file \"" + rmDir + filename + "\"");
				}
			}
		}
		// get the next file name
		while (dir->GetNext(&filename));
	}

	// Remove our directory object, so the OS will let go of it and
	// allow us to delete it
	delete dir;

	// now actually try to delete it
	if (!wxFileName::Rmdir(rmDir)) {
		// error if we couldn't
		wxLogError("Could not remove directory " + rmDir);
		// return accordingly
		return false;
	}
	// otherwise
	else {
		// return that we were successfull.
		return true;
	}

}



void saveCurrentScroll() {

	std::fstream outfile;
	wxString filePath = currentImageDirPath + wxT("data.txt");
	outfile.open(filePath.ToStdString(), std::ios::out);

	//int savePage = (currentScrollValue / currentPageHeight)+1;
	double percent = currentScrollValue / (double)currentTotalHeight;

	std::string str = "scrollPercent:" + std::to_string(percent) + "\n";

	outfile << str;

	outfile.flush();
	outfile.close();

}

std::string cMain::getFileVar(std::string var) {
	if (varlist.find(var) != varlist.end()) {
		return varlist[var];
	}
	else {
		return "";
	}
}

void cMain::checkDarken() {
	std::string darken = getFileVar("darken");
	wxMenu* settings = menuItems[ID_Settings];


	if (darken == "n") {
		currentDarken = false;
	}
	else {
		currentDarken = true;
	}

	settings->Check(ID_Darken, currentDarken);
}

//create first page and startup file

void cMain::closePDF() {

	if (currentFilename == "") {
		return;
	}

	//save scroll value
	saveCurrentScroll();

	if (currentCache == false) {
		wxString cwd = wxGetCwd();
		//remove folder
		wxString dir_path = cwd + wxT("\\pdf-images\\") + currentFileKey;
		RemDirRF(dir_path);
	}

	//reset vars
	currentLoadFinish = false;
	currentLastLoaded = 0;
	currentFilename = wxT("");
	currentPageScroll = 0;
	currentScrollPercent = 0.0;
	currentImageDirPath = "";
	currentScrollValue = 0;
	currentCache = false;
	opened = false;
}



void cMain::openPDF(wxString filename) {

	clearLayout();


	if (currentFilename!="") {
		closePDF();
	}

	int page = 1;
	currentPage = page;
	currentFilename = filename;

	
	//create page 1 png
	wxString filekey = getFileKey(filename);
	currentFileTitle = getFileTitle(filename);
	SetLabel(currentFileTitle+wxT(".pdf"));

	currentFileKey = filekey;
	wxString dir_path = wxT("pdf-images\\") + filekey + wxT("\\");
	currentImageDirPath = dir_path;

	//create directory if not exists
	boost::filesystem::path dir(dir_path);
	if (boost::filesystem::exists(dir) == false)
	{
		boost::filesystem::create_directory(dir);
	}

	//create first page image
	dir_path.Replace(wxT(" "), wxT(""), true);
	//wxString newFilename = wxT("pdf-copy\\") + filekey + wxT(".pdf");
	wxString newFilename = filename;

	wxString cmd = wxT("pdftopng -r 100 -f 1 -l 1 \"") + newFilename + wxT("\" ")+ dir_path+wxT("");
	
	//wxString cmd3 = wxT("pdf2svg \"")+newFilename + wxT("\" ")+ dir_path+wxT("-000001.svg 1");
	
	wxCharBuffer buffer = cmd.ToUTF8();
	char* c = buffer.data();
	std::string cmd_str = c;

	//output(str);

	wxArrayString strs;
	
	//wxExecute(cmd, strs, wxEXEC_HIDE_CONSOLE);
	//executeCN(,filename,newFilename);
	
	wxExecute(cmd,strs,wxEXEC_HIDE_CONSOLE);

	//page count
	int pageCount = getPageCount(newFilename);
	currentPageCount = pageCount;


	wxString cmd2 = wxT(" -r 100 -f 1 -l ")+ wxString::Format(wxT("%i"), pageCount) +wxT(" \"") + newFilename + wxT("\" ") + dir_path + wxT("");
	//wxString cmd2 = wxT(" -o \"")+ dir_path +wxT("\" \"") + newFilename + wxT("\" ");
	//wxString cmd4 = wxT(" ")+ newFilename + wxT(" ") + dir_path+wxT("-%06d.svg all");
	//xString cmd5 = cmd2 + cmd4;

	wxCharBuffer buffer2 = cmd2.ToUTF8();
	char* c2 = buffer2.data();
	std::string cmd2_str = c2;

	//output(cmd2_str);

	//init first page item
	wxString imageFilename_path = dir_path + getImageFilename(dir_path, page);
	
	wxArrayString strs2;
	//wxExecute(wxT("" + cmd_str2), strs2, wxEXEC_HIDE_CONSOLE);
	
	wxImageHandler* pngLoader = new wxPNGHandler();
	wxImage::AddHandler(pngLoader);

	wxImage img;
	img.LoadFile(imageFilename_path, wxBITMAP_TYPE_PNG);
	
	//prepare bulkpages
	wxString keyName = getBulkPagesKey(filekey, page);
	int index = page % 100;

	//prepare x,y,w,h
	int x = 0;
	int y = 0;
	
	int w, h, imgw, imgh;
	setFullWidth(&w, &h, img);
	imgw = img.GetWidth();
	imgh = img.GetHeight();

	originalHeight = imgh;
	originalWidth = imgw;

	currentPageHeight = (double)h+(double)currentSpacing;

	currentTotalHeight = currentPageHeight * pageCount;
	currentScrollDelta = (1.0 / ((double)currentTotalHeight / 10000.0))*2;

	bulkPages[keyName][index] = { imageFilename_path,x,y,w,h,imgw,imgh };
	
	//setup image paths
	wxString imagePath_r;
	wxString keyName_r;
	int index_r;
	struct page p;

	getCurrentScroll();

	for (int i = 1; i <= pageCount; i++) {
		imagePath_r = dir_path + getImageFilename(dir_path, i);
		keyName_r = getBulkPagesKey(filekey, i);
		index_r = i % 100;
		
		bulkPages[keyName_r][index_r] = {imagePath_r};
	}

	updateLoadPercentage();

	if (currentLoadFinish == false) {
		//create other pages
		HANDLE myhandle;
		DWORD mythreadid;
		myhandle = CreateThread(0, 0, mythread, (void*)&cmd2, 0, &mythreadid);
	}


	redraw();
	int winw, winh;
	GetSize(&winw, &winh);
	RefreshRect(wxRect(0, 0, winw, winh));
	
	opened = true;

	checkMenuCB();
	checkFileVars();

	checkDarken();
	pushToRecent();

	while (!currentLoadFinish) {
		int winw, winh;
		GetSize(&winw, &winh);
		RefreshRect(wxRect(0, 0, winw, winh));
		wxYield();
	}
	
	
}


void cMain::OnMouseClick(wxString name) {
	if (name == "topScrollButton") {
		distScroll -= currentScrollDelta;
		int winw, winh;
		GetSize(&winw, &winh);
		RefreshRect(wxRect(0, 0, winw, winh));
	}
	else if (name == "bottomScrollButton") {
		distScroll += currentScrollDelta;
		int winw, winh;
		GetSize(&winw, &winh);
		RefreshRect(wxRect(0, 0, winw, winh));
	}
	else if (name == "upScroll") {
		distScroll -= currentScrollDelta*2;
		int winw, winh;
		GetSize(&winw, &winh);
		RefreshRect(wxRect(0, 0, winw, winh));
	}
	else if (name == "downScroll") {
		distScroll += currentScrollDelta*2;
		int winw, winh;
		GetSize(&winw, &winh);
		RefreshRect(wxRect(0, 0, winw, winh));
	}
}

void cMain::OnMaximize(wxMaximizeEvent& event) {
	clearBitmaps();
	
	GetClientSize(&clientw, &clienth);

	this->ClearBackground();
	
	Layout();
	
	int winw, winh;
	GetSize(&winw, &winh);
	RefreshRect(wxRect(0, 0, winw, winh));
}

void cMain::OnMouseWheel(wxMouseEvent& event) {
	
	if (currentMouseName == "viewArea"||currentMouseName=="upScroll" || currentMouseName == "downScroll" || currentMouseName == "scrollbar" || currentMouseName == "topScrollButton" || currentMouseName == "bottomScrollButton") {
		int r = event.GetWheelRotation();
		if (r > 0) {
			distScroll -= currentScrollDelta;
			int winw, winh;
			GetSize(&winw, &winh);
			RefreshRect(wxRect(0, 0, clientw, clienth));
			
		}
		else if (r < 0) {
			distScroll += currentScrollDelta;
			int winw, winh;
			GetSize(&winw, &winh);
			RefreshRect(wxRect(0, 0, clientw, clienth));
			
		}
	}
}

void cMain::OnMouseMotion(wxMouseEvent& event) {
	const wxPoint pt = wxGetMousePosition();
	int mouseX = pt.x;
	int mouseY = pt.y;
	bool redraw = false;

	distMx = mouseX - oldMx;
	distMy = mouseY - oldMy;

	oldMx = mouseX;
	oldMy = mouseY;

	ScreenToClient(&mouseX, &mouseY);

	wxString name = getMouseName(mouseX, mouseY);
	if (currentMouseName != name) {
		currentMouseName = name;
		redraw = true;
	}
	else if (currentMouseDrag != "") {
		redraw = true;
	}

	if (name == "") {
		currentMouseName = "";
		currentMouseDown = "";
	}


	if (currentMouseDown != "") {
		currentMouseDrag = currentMouseDown;
	}

	wxMouseState s=wxGetMouseState();

	if (s.LeftIsDown() == false && currentMouseDown!="") {
		leftUpHandler();
	}

	if (currentMouseDrag == "viewArea") {
		currentScrollValue -= distMy;
	}

	if (redraw) {
		int winw, winh;
		GetSize(&winw, &winh);
		RefreshRect(wxRect(0, 0, winw, winh));
	}
	
}

void cMain::OnLeftDown(wxMouseEvent& event) {
	if (currentMouseName != "") {
		currentMouseDown = currentMouseName;
	}
}

void cMain::leftUpHandler() {

	if (currentMouseName != "" || currentMouseDrag != "") {
		OnMouseClick(currentMouseName);
		currentMouseDrag = "";
		currentMouseDown = "";
		int winw, winh;
		GetSize(&winw, &winh);
		RefreshRect(wxRect(0, 0, winw, winh));
	}

	
}

void cMain::OnLeftUp(wxMouseEvent& event) {
	
	leftUpHandler();
	
}


void cMain::Open(wxCommandEvent& event) {
	
	wxString filename = wxFileSelector("Choose a file to open",wxEmptyString,wxEmptyString,wxEmptyString,"*.pdf");
	if (!filename.empty())
	{
		openPDF(filename);
	}
	
}

wxString cMain::getFileTitle(wxString filename) {
	int lastSlashIndex = 0;
	int lastDotIndex = 0;
	int count = 0;
	for (wxString::const_iterator it = filename.begin(); it != filename.end(); ++it) {
		wchar_t wch = *it;
		if (wch == '\\') {
			lastSlashIndex = count;
		}
		else if (wch == '.') {
			lastDotIndex = count;
		}
		count++;
	}

	wxString str = filename.SubString(lastSlashIndex + 1, lastDotIndex - 1);

	return str;
}

wxString cMain::getFileKey(wxString filename) {
	
	int lastSlashIndex=0;
	int lastDotIndex = 0;
	int count = 0;
	for (wxString::const_iterator it = filename.begin(); it != filename.end(); ++it) {
		wchar_t wch = *it;
		if (wch == '\\') {
			lastSlashIndex = count;
		}
		else if (wch == '.') {
			lastDotIndex = count;
		}
		count++;
	}

	wxString str = filename.SubString(lastSlashIndex+1,lastDotIndex-1);

	wxString str2=wxT("");
	str.Replace(wxT(" "), wxT(""), true);
	for (wxString::const_iterator it = str.begin(); it != str.end(); ++it) {
		wchar_t wch = *it;
		
		int d = (int)wch;
		std::string s = std::to_string(d);
		str2 += wxT("" + s);
	}

	if (str2.size() > 150) {
		str2 = str2.SubString(0, 100) + str2.SubString(str2.size() - 20, str2.size());
	}

	return str2;
}

wxString cMain::getImageFilename(wxString dir_path, int page) {
	wxString str = wxT(""+std::to_string(page));

	int dn=6-str.size();
	wxString num = "";
	for (int i = 0; i < dn; i++) {
		num += "0";
	}

	str =wxT("-")+ num + str+wxT(".png");

	return str;
}


void cMain::setFullWidth(int* w, int* h,wxImage img) {
	int winw, winh, imgw, imgh;
	GetClientSize(&winw, &winh);
	imgw = img.GetWidth();
	imgh = img.GetHeight();

	//set full width
	*w = winw-scrollSize;
	*h = (imgh * (*w)) / imgw;
}

void cMain::redraw() {
	int ow, oh;
	GetSize(&ow, &oh);
	SetSize(ow + 1, oh + 1);
	SetSize(ow, oh);
}


void cMain::executeCN(wxString cmd,wxString filename,wxString newFilename) {

	//wxCopyFile(filename, newFilename, true);
	/*
	std::ifstream in(filename.ToStdString(),
		std::ios_base::in | std::ios_base::binary);  // Use binary mode so we can
	std::ofstream out(newFilename.ToStdString(),            // handle all kinds of file
		std::ios_base::out | std::ios_base::binary); // content.

	 // Make sure the streams opened okay...
	const static int BUF_SIZE = 4096;
	char buf[BUF_SIZE];

	do {
		in.read(&buf[0], BUF_SIZE);      // Read at most n bytes into
		out.write(&buf[0], in.gcount()); // buf, then write the buf to
	} while (in.gcount() > 0);          // the output.

	// Check streams for problems...

	in.close();
	out.close();
	*/

	wxArrayString strs;
	wxExecute(cmd, strs, wxEXEC_HIDE_CONSOLE);

	/*
	wxString cwd = wxGetCwd();
	std::string cwd_str = cwd.ToStdString();
	std::string cmd_str = cmd.ToStdString();

	wxTextFile file(cwd+wxT("\\execn.bat"));

	file.Open();
	file.Clear();
	file.AddLine(wxT("cd ") + cwd);
	file.AddLine(wxT("chcp 936"));
	file.AddLine(wxT("copy \"")+filename+wxT("\" ")+newFilename);
	file.AddLine(cmd);

	file.Write();
	file.Close();


	wxArrayString strs;
	wxExecute(cwd+wxT("\\execn.bat"), strs, wxEXEC_HIDE_CONSOLE);
	*/
}

int cMain::getPageCount(wxString filename) {
	using namespace pdftron;
	using namespace Common;
	using namespace SDF;
	using namespace PDF;

	pdftron::PDFNet::Initialize();

	wxCharBuffer buffer = filename.ToUTF8();
	char* c = buffer.data();
	std::string filename_str = c;

	PDFDoc doc(filename_str);

	return doc.GetPageCount();
	
	
}


void cMain::OnExit(wxCommandEvent& event)
{

	Close();
}


void cMain::OnClose(wxCloseEvent& event)
{

	
	
	closePDF();

	Destroy();
}


void cMain::OnEraseBackground(wxEraseEvent& event) {

}

cMain::~cMain() {

}

