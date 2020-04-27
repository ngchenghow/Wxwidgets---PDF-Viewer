#pragma once

#include "wx/wx.h"

class cMain:public wxFrame
{
public:
	cMain(int argc,wxString filename);
	void Open(wxCommandEvent& event);
	void OnExit(wxCommandEvent& event);
	void OnCache(wxCommandEvent& event);
	void OnDarken(wxCommandEvent& event);
	void OnClose(wxCloseEvent& event);
	void OnPaint(wxPaintEvent& WXUNUSED(evt));
	void OnMouseMotion(wxMouseEvent& event);
	void OnLeftDown(wxMouseEvent& event);
	void OnLeftUp(wxMouseEvent& event);
	void OnResize(wxSizeEvent& event);
	void OnEraseBackground(wxEraseEvent& event);
	void OnMouseClick(wxString name);
	void leftUpHandler();
	void OnMouseWheel(wxMouseEvent& event);
	int getPageCount(wxString filename);
	void openPDF(wxString filename);
	void gotoPage(int page);
	void getCurrentScroll();
	void updateLoadPercentage();
	void OnMaximize(wxMaximizeEvent& event);
	void checkFileVars();
	void render();
	void output(std::string str);
	void executeCN(wxString cmd, wxString filename, wxString newFilename);
	wxImage processImage(wxImage img);

	std::string getFileVar(std::string var);
	void checkDarken();
	void clearLayout();
	void pushToRecent();

	void OpenOne(wxCommandEvent& event);
	void OpenTwo(wxCommandEvent& event);
	void OpenThree(wxCommandEvent& event);
	void OpenFour(wxCommandEvent& event);
	void OpenFive(wxCommandEvent& event);

	wxString getFileKey(wxString filename);
	wxString getFileTitle(wxString filename);
	bool checkCacheList();
	void checkMenuCB();
	void checkMenuCB2();

	wxString getBulkPagesKey(wxString filekey, int pageNumber);
	wxString getImageFilename(wxString dir_path, int page);
	void setFullWidth(int* w, int* h, wxImage img);

	void closePDF();
	void redraw();

	~cMain();

	wxDECLARE_EVENT_TABLE();
};

