/*

  Copyright (C) 2003  Razvan Cojocaru <razvanco@gmx.net>
 
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
  
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
  
  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

*/


#include <chmframe.h>
#include <chminputstream.h>
#include <contenttaghandler.h>
#include <chmfontdialog.h>
#include <wx/fontenum.h>
#include <wx/statbox.h>


namespace {

const wxChar *greeting = wxT(
	"<html><body>Hello, and welcome to "
	"<B>xCHM</B>, the UNIX CHM viewer.<br><br><B>"
	"xCHM</B> has been written by Razvan Cojocaru "
	"(razvanco@gmx.net). It is licensed under the "
	"<TT>GPL</TT>.<br><B>xCHM</B> is based on"
	" Jed Wing's <a href=\"http://66.93.236.84/~jedwin/projects"
	"/chmlib/\">CHMLIB</a> and <a href=\"http://www.wxwindows.org\">"
	"wxWindows</a>.<br><br>If you'd like to know more about CHM, you"
	" could check out <a href=\"http://www.speakeasy.org/~russotto"
	"/chm/\">Matthew Russoto's CHM page</a> or <a"
	" href=\"http://bonedaddy.net/pabs3/hhm/\">Pabs' CHM"
	" Specification page</a>.<br><br>If you'd like to use the code in"
	" your own stuff please figure <TT>GPL</TT> out first. Far too"
	" many people think <TT>GPL</TT>"
	" is bad out of utter ignorance.<br><br>Enjoy.</body></html>");


const wxChar *about_txt = wxT(
	"xCHM v. " VERSION "\nby Razvan Cojocaru (razvanco@gmx.net)\n\n"
	"Based on Jed Wing's CHMLIB (http://66.93.236.84/~jedwin/projects).\n"
	"Written with wxWindows (http://www.wxwindows.org).\n\n"
	"This program is (proudly) under the GPL.");

#include <htmbook.xpm>

} // namespace


#define CONTENTS_MARGIN 170


CHMFrame::CHMFrame(const wxString& title, const wxString& booksDir, 
		   const wxPoint& pos, const wxSize& size,
		   const wxString& normalFont, const wxString& fixedFont,
		   const int fontSize)
	: wxFrame((wxFrame *)NULL, -1, title, pos, size), _html(NULL),
	  _tcl(NULL), _sw(NULL), _menuFile(NULL), _tb(NULL), _ep(NULL),
	  _nb(NULL), _cb(NULL), _csp(NULL), _openPath(booksDir), 
	  _normalFonts(NULL), _fixedFonts(NULL), _normalFont(normalFont), 
	  _fixedFont(fixedFont), _fontSize(fontSize), _bookmarkSel(true),
	  _bookmarksDeleted(false)
{
	int sizes[7];

	for(int i = -3; i <= 3; ++i)
		sizes[i+3] = _fontSize + i * 2;

	SetIcon(wxIcon(htmbook_xpm));
	SetMenuBar(CreateMenu());

	_tb = CreateToolBar(wxTB_FLAT | wxTB_DOCKABLE
			    | wxTB_HORIZONTAL | wxTB_TEXT);
	InitToolBar(_tb);

	CreateStatusBar();
	SetStatusText(wxT("Ready."));

	_ep = new wxHtmlEasyPrinting(wxT("Printing"), this);

	_sw = new wxSplitterWindow(this);
	_sw->SetMinimumPaneSize(CONTENTS_MARGIN);

	_nb = new wxNotebook(_sw, -1);
	_nb->Show(FALSE);

	wxPanel* temp = CreateContentsPanel();

	_html = new CHMHtmlWindow(_sw, _tcl);
	_html->SetRelatedFrame(this, wxT("xCHM v. " VERSION));
	_html->SetRelatedStatusBar(0);

	_html->SetFonts(_normalFont, _fixedFont, sizes);
	_html->SetPage(greeting);

	_csp = new CHMSearchPanel(_nb, _tcl, _html);

	_nb->AddPage(temp, wxT("Contents"));
	_nb->AddPage(_csp, wxT("Search"));

	_sw->Initialize(_html);
}


CHMFrame::~CHMFrame()
{
	delete _ep;
	delete _fixedFonts;
	delete _normalFonts;
}


void CHMFrame::OnQuit(wxCommandEvent& WXUNUSED(event))
{
	Close(TRUE);
}


void CHMFrame::OnAbout(wxCommandEvent& WXUNUSED(event))
{
	::wxMessageBox(about_txt, wxT("About xCHM"),
		       wxOK | wxICON_INFORMATION, this );
}


void CHMFrame::OnOpen(wxCommandEvent& WXUNUSED(event))
{
	wxString selection =
		::wxFileSelector(wxT("Choose a file.."), _openPath, 
				 wxEmptyString, wxT("chm"),
#ifndef __WXMOTIF__
				 // they say Motif can't handle the following.
				 wxT("CHM files (*.chm)|*.chm;*.CHM|"
				 "All files (*.*)|*.*"), 
#else
				 wxT("All files (*.*)|*.*"),
#endif
				 wxOPEN | wxFILE_MUST_EXIST, this);

	if(selection.IsEmpty() || !_tcl)
		return;

	_openPath = selection.BeforeLast(wxT('/'));
	LoadCHM(selection);
}


void CHMFrame::OnChangeFonts(wxCommandEvent& WXUNUSED(event))
{
	// First time initialization only.
	if(_normalFonts == NULL) {
		wxFontEnumerator enu;
		enu.EnumerateFacenames();
		_normalFonts = new wxArrayString;
		*_normalFonts = *enu.GetFacenames();
		_normalFonts->Sort();
	}

	if(_fixedFonts == NULL) {
		wxFontEnumerator enu;
		enu.EnumerateFacenames(wxFONTENCODING_SYSTEM, TRUE);
		_fixedFonts = new wxArrayString;
		*_fixedFonts = *enu.GetFacenames();
		_fixedFonts->Sort();
	}


	assert(_normalFonts != NULL);
	assert(_fixedFonts != NULL);

	CHMFontDialog cfd(this, _normalFonts, _fixedFonts, _normalFont,
			  _fixedFont, _fontSize);

	if(cfd.ShowModal() == wxID_OK) {
		
		wxBusyCursor bc;

		_html->SetFonts(_normalFont = cfd.NormalFont(), 
				_fixedFont = cfd.FixedFont(),
				cfd.Sizes());

		_fontSize = *(cfd.Sizes() + 3);

		wxString page = _html->GetOpenedPage();

		if(page.IsEmpty())
			_html->SetPage(greeting); 
	}
}


void CHMFrame::OnHome(wxCommandEvent& WXUNUSED(event))
{
	CHMFile *chmf = CHMInputStream::GetCache();

	if(!chmf)
		return;

	_html->LoadPage(wxString(wxT("file:")) + chmf->ArchiveName()
		+ wxT("#chm:") + chmf->HomePage());
}


void CHMFrame::OnHistoryForward(wxCommandEvent& WXUNUSED(event))
{
	_html->HistoryForward();
}


void CHMFrame::OnHistoryBack(wxCommandEvent& WXUNUSED(event))
{
	_html->HistoryBack();
}


void CHMFrame::OnShowContents(wxCommandEvent& WXUNUSED(event))
{
	if(_sw->IsSplit()) {
		_tb->ToggleTool(ID_Contents, FALSE);
		_menuFile->Check(ID_Contents, FALSE);
		
		_sw->Unsplit(_nb);
		_nb->Show(FALSE);
	} else {		
			
		if(_tcl->GetCount() > 1) {
			
			_tb->ToggleTool(ID_Contents, TRUE);
			_menuFile->Check(ID_Contents, TRUE);

			_nb->Show(TRUE);
			_sw->SplitVertically(_nb, _html, CONTENTS_MARGIN);

		} else {
			_tb->ToggleTool(ID_Contents, FALSE);
			_menuFile->Check(ID_Contents, FALSE);
			
			::wxMessageBox(wxT("Couldn't extract the book"
				       " contents tree."), 
				       wxT("No contents.."), 
				       wxOK | wxICON_WARNING, this );
		}
	}
}


void CHMFrame::OnPrint(wxCommandEvent& WXUNUSED(event))
{
	wxString page = _html->GetOpenedPage();

	if(page.IsEmpty())
		_ep->PrintText(greeting);
	else
		_ep->PrintFile(page);
}


void CHMFrame::OnAddBookmark(wxCommandEvent& WXUNUSED(event))
{
	wxTreeItemId id = _tcl->GetSelection();

	if(!id.IsOk())
		return;

	wxString title = _tcl->GetItemText(id);

 	if(title.IsEmpty())
		return;
       
	URLTreeItem *data = reinterpret_cast<URLTreeItem *>(
		_tcl->GetItemData(id));

	if(!data || (data->_url).IsEmpty())
		return;

	_cb->Append(title, new wxString(data->_url));

	_bookmarkSel = false;
	_cb->SetSelection(_cb->GetCount() - 1);
	_bookmarkSel = true;		
}


void CHMFrame::OnRemoveBookmark(wxCommandEvent& WXUNUSED(event))
{
	if(!_cb->GetCount())
		return;

	_cb->Delete(_cb->GetSelection());
	_bookmarksDeleted = true;

	if(_cb->GetCount()) {
		_bookmarkSel = false;
		_cb->SetSelection(0);
		_bookmarkSel = true;
	} else {
		_cb->SetValue(wxEmptyString);
	}
}


void CHMFrame::OnBookmarkSel(wxCommandEvent& WXUNUSED(event))
{
	if(!_bookmarkSel)
		return;

	wxString *url = reinterpret_cast<wxString *>(
		_cb->GetClientData(_cb->GetSelection()));

	if(!url || url->IsEmpty())
		return;

	CHMFile *chmf = CHMInputStream::GetCache();

	if(!chmf)
		return;

	_html->LoadPage(wxString(wxT("file:")) + chmf->ArchiveName() +
			wxT("#chm:/") + *url);
}


void CHMFrame::OnSelectionChanged(wxTreeEvent& event)
{
	wxTreeItemId id = event.GetItem();
	CHMFile *chmf = CHMInputStream::GetCache();

	if(id == _tcl->GetRootItem() || !chmf)
		return;

	URLTreeItem *data = reinterpret_cast<URLTreeItem *>(
		_tcl->GetItemData(id));

	if(!data || data->_url.IsEmpty())
		return;

	if(!_html->IsCaller()) {
		_html->SetSync(false);
		_html->LoadPage(wxString(wxT("file:")) + chmf->ArchiveName() +
				wxT("#chm:/") + data->_url);
		_html->SetSync(true);
	}
}


void CHMFrame::OnCloseWindow(wxCloseEvent& WXUNUSED(event))
{
	int xorig, yorig, width, height;

	GetPosition(&xorig, &yorig);
	GetSize(&width, &height);

	wxConfig config(wxT("xchm"));
	
	config.Write(wxT("/Position/xOrig"), xorig);
	config.Write(wxT("/Position/yOrig"), yorig);
	config.Write(wxT("/Position/width"), width);
	config.Write(wxT("/Position/height"), height);
	config.Write(wxT("/Paths/lastOpenedDir"), _openPath);
	config.Write(wxT("/Fonts/normalFontFace"), _normalFont);
	config.Write(wxT("/Fonts/fixedFontFace"), _fixedFont);
	config.Write(wxT("/Fonts/size"), _fontSize);

	SaveBookmarks();
	Destroy();
}


void CHMFrame::LoadCHM(const wxString& archive)
{
	wxBusyCursor bc;

	SaveBookmarks();

	_html->HistoryClear();
	_html->LoadPage(wxString(wxT("file:")) + archive +
		      wxT("#chm:/"));

	CHMFile *chmf = CHMInputStream::GetCache();

	if(chmf) {
		wxString title = chmf->Title();

		if(_tcl->GetCount())
			_tcl->DeleteAllItems();
		chmf->GetTopicsTree(_tcl);

		if(!title.IsEmpty()) {
			wxString titleBarText = 
				wxString(wxT("xCHM v. " VERSION ": ")) + title;

			SetTitle(titleBarText);
			_html->SetRelatedFrame(this, titleBarText);
		}
	}

	// if we have contents..
	if(_tcl->GetCount() > 1) {		
		if(!_sw->IsSplit()) {
			_nb->Show(TRUE);
			_sw->SplitVertically(_nb, _html, CONTENTS_MARGIN);
			_menuFile->Check(ID_Contents, TRUE);
			_tb->ToggleTool(ID_Contents, TRUE);
		}
	} else {

		if(_sw->IsSplit()) {
			_sw->Unsplit(_nb);
			_nb->Show(FALSE);
		}
		_menuFile->Check(ID_Contents, FALSE);
		_tb->ToggleTool(ID_Contents, FALSE);
	}

	// select Contents
	_nb->SetSelection(0);
	_csp->Reset();
	
	LoadBookmarks();
}


namespace {

const wxChar *open_help = wxT("Open a CHM book.");
const wxChar *fonts_help = wxT("Change fonts.");
const wxChar *print_help = wxT("Print the page currently displayed");
const wxChar *contents_help = wxT("On or off?");
const wxChar *home_help = wxT("Go to the book's start page.");
const wxChar *forward_help = wxT("Go forward in history. Per book.");
const wxChar *back_help = wxT("Back to the last visited page. Per book.");
const wxChar *about_help = wxT("About the program.");

} // namespace


wxMenuBar* CHMFrame::CreateMenu()
{
	_menuFile = new wxMenu;

	_menuFile->Append(ID_Open, wxT("&Open .."), open_help);
	_menuFile->Append(ID_Print, wxT("&Print page.."), print_help);
	_menuFile->Append(ID_Fonts, wxT("&Fonts.."), fonts_help);
	_menuFile->AppendSeparator();
	_menuFile->AppendCheckItem(ID_Contents, wxT("&Show contents tree"),
				   contents_help);
	_menuFile->AppendSeparator();
	_menuFile->Append(ID_Quit, wxT("E&xit"), 
			  wxT("Quit the application."));

	wxMenu *menuHistory = new wxMenu;

	menuHistory->Append(ID_Home, wxT("&Home"), home_help);
	menuHistory->Append(ID_Forward, wxT("&Forward"), forward_help);
	menuHistory->Append(ID_Back, wxT("&Back"), back_help);
	
	wxMenu *menuHelp = new wxMenu;
	menuHelp->Append(ID_About, wxT("&About .."), about_help);

	wxMenuBar *menuBar = new wxMenuBar;
	menuBar->Append(_menuFile, wxT("&File"));
	menuBar->Append(menuHistory, wxT("Hi&story"));
	menuBar->Append(menuHelp, wxT("&Help"));

	return menuBar;
}


wxPanel* CHMFrame::CreateContentsPanel()
{
	wxPanel *temp = new wxPanel(_nb);
	wxSizer *sizer = new wxBoxSizer(wxVERTICAL);
	wxSizer *bmarks = new wxStaticBoxSizer(
		new wxStaticBox(temp, -1, wxT("Bookmarks")), wxVERTICAL);
	wxSizer *inner = new  wxBoxSizer(wxHORIZONTAL);

	temp->SetAutoLayout(TRUE);
        temp->SetSizer(sizer);

	_tcl = new wxTreeCtrl(temp, ID_TreeCtrl, wxDefaultPosition, 
			      wxDefaultSize, wxTR_HAS_BUTTONS |
			      wxSUNKEN_BORDER | wxTR_HIDE_ROOT |
			      wxTR_SINGLE | wxTR_LINES_AT_ROOT);

	_cb = new wxComboBox(temp, ID_Bookmarks, wxT(""), wxDefaultPosition,
			     wxDefaultSize, 0, NULL, wxCB_DROPDOWN 
			     | wxCB_READONLY);
	sizer->Add(_tcl, 1, wxEXPAND, 0);
	sizer->Add(bmarks, 0, wxEXPAND | wxALL, 0);

	bmarks->Add(_cb, 0, wxEXPAND | wxBOTTOM, 5);
	bmarks->Add(inner, 1, wxEXPAND, 0);

	wxButton *badd = new wxButton(temp, ID_Add, wxT("Add"));
	wxButton *bremove = new wxButton(temp, ID_Remove, wxT("Remove"));

#if wxUSE_TOOLTIPS
	badd->SetToolTip(wxT("Add displayed page to bookmarks."));
	bremove->SetToolTip(wxT("Remove selected bookmark."));
#endif

	inner->Add(badd, 1, wxEXPAND | wxRIGHT, 2);
	inner->Add(bremove, 1, wxEXPAND | wxLEFT, 2);

	return temp;
}


void CHMFrame::LoadBookmarks()
{
	_cb->Clear();
	_cb->SetValue(wxEmptyString);

	CHMFile *chmf = CHMInputStream::GetCache();

	if(!chmf)
		return;

	wxConfig config(wxT("xchm"));
	wxString bookname = chmf->ArchiveName();
	bookname.Replace(wxT("/"), wxT("|"), TRUE);

	bookname = wxString(wxT("/Bookmarks/")) + bookname;
	long noEntries;

	config.SetPath(bookname);
	wxString title, url;

	if(config.Read(wxT("noEntries"), &noEntries)) {
		
		const wxChar* format1 = wxT("bookmark_%ld_title");
		const wxChar* format2 = wxT("bookmark_%ld_url");
		
		for(long i = 0; i < noEntries; ++i) {
			config.Read(wxString::Format(format1, i), &title);
			config.Read(wxString::Format(format2, i), &url);

			_cb->Append(title, new wxString(url));
		}
	}
}


void CHMFrame::SaveBookmarks()
{
	long noEntries = _cb->GetCount();

	if(!_bookmarksDeleted && noEntries == 0)
		return;

	CHMFile *chmf = CHMInputStream::GetCache();

	if(!chmf)
		return;

	wxConfig config(wxT("xchm"));
	wxString bookname = chmf->ArchiveName();
	bookname.Replace(wxT("/"), wxT("|"), TRUE);
	bookname = wxString(wxT("/Bookmarks/")) + bookname;

	if(_bookmarksDeleted) {
		config.DeleteGroup(bookname);
	}

	if(noEntries == 0)
		return;

	config.SetPath(bookname);
	config.Write(wxT("noEntries"), noEntries);

	const wxChar* format1 = wxT("bookmark_%ld_title");
	const wxChar* format2 = wxT("bookmark_%ld_url");

	for(long i = 0; i < noEntries; ++i) {
		wxString *url = reinterpret_cast<wxString *>(
			_cb->GetClientData((int)i));

		config.Write(wxString::Format(format1, i), 
			     _cb->GetString((int)i));

		if(url)
			config.Write(wxString::Format(format2, i), *url);
	}
}


namespace {

#include <fileopen.xpm>
#include <print.xpm>
#include <back.xpm>
#include <forward.xpm>
#include <home.xpm>
#include <helpicon.xpm>
#include <htmsidep.xpm>
#include <htmoptns.xpm>

} // namespace


bool CHMFrame::InitToolBar(wxToolBar *toolbar)
{
	toolbar->AddTool(ID_Open, wxT("Open .."), wxBitmap(fileopen_xpm),
			 open_help);
	toolbar->AddTool(ID_Print, wxT("Print .."), wxBitmap(print_xpm),
			 print_help);
	toolbar->AddTool(ID_Fonts, wxT("Fonts .."), wxBitmap(htmoptns_xpm),
			 fonts_help);
	toolbar->AddCheckTool(ID_Contents, wxT("Contents"),
			      wxBitmap(htmsidep_xpm),
			      wxBitmap(htmsidep_xpm), contents_help);
	toolbar->AddSeparator();
	toolbar->AddTool(ID_Back, wxT("Back"), wxBitmap(back_xpm), back_help);
	toolbar->AddTool(ID_Forward, wxT("Forward"), wxBitmap(forward_xpm), 
			 forward_help);
	toolbar->AddTool(ID_Home, wxT("Home"), wxBitmap(home_xpm), 
			 home_help);
	toolbar->AddSeparator();
	toolbar->AddTool(ID_About, wxT("About"), wxBitmap(helpicon_xpm), 
			 about_help);

	toolbar->Realize();

	return TRUE;
}


// They say this needs to be in the implementation file.
BEGIN_EVENT_TABLE(CHMFrame, wxFrame)
	EVT_MENU(ID_Quit,  CHMFrame::OnQuit)
	EVT_MENU(ID_About, CHMFrame::OnAbout)
	EVT_MENU(ID_Open, CHMFrame::OnOpen)
	EVT_MENU(ID_Fonts, CHMFrame::OnChangeFonts)
	EVT_MENU(ID_Home, CHMFrame::OnHome)
	EVT_MENU(ID_Forward, CHMFrame::OnHistoryForward)
	EVT_MENU(ID_Back, CHMFrame::OnHistoryBack)
	EVT_MENU(ID_Contents, CHMFrame::OnShowContents)
	EVT_MENU(ID_Print, CHMFrame::OnPrint)
	EVT_BUTTON(ID_Add, CHMFrame::OnAddBookmark)
	EVT_BUTTON(ID_Remove, CHMFrame::OnRemoveBookmark)
	EVT_TREE_SEL_CHANGED(ID_TreeCtrl, CHMFrame::OnSelectionChanged)
	EVT_COMBOBOX(ID_Bookmarks, CHMFrame::OnBookmarkSel)
	EVT_CLOSE(CHMFrame::OnCloseWindow)
END_EVENT_TABLE()

