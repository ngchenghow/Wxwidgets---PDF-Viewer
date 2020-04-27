
#include "cApp.h"
#include "wx/cmdline.h"

#define _CRT_SECURE_NO_WARNINGS

wxIMPLEMENT_APP(cApp);

cApp::cApp() {

}

cApp::~cApp() {

}

bool cApp::OnInit() {

	if (wxGetApp().argc > 1) {
		m_frame1 = new cMain(1, wxGetApp().argv[1]);
	}
	else {
		m_frame1 = new cMain(0,wxT(""));
	}
	

	//m_frame1->output(wxGetApp().argv[0].ToStdString());
	
	
	//m_frame1->SetStatusText(wxGetApp().argv[1]);
	//m_frame1->openPDF(wxGetApp().argv[1]);
	
	
	m_frame1->Show();
	
	return true;
}

