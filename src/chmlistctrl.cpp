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


#include <chmlistctrl.h>
#include <chmhtmlwindow.h>
#include <wx/settings.h>


CHMListCtrl::CHMListCtrl(wxWindow *parent, CHMHtmlWindow *html,
			 wxWindowID id)
	: wxListCtrl(parent, id, wxDefaultPosition, wxDefaultSize,
		     wxLC_REPORT | wxLC_NO_HEADER | wxLC_SINGLE_SEL | 
		     wxSUNKEN_BORDER), _html(html)
{
	InsertColumn(0, wxEmptyString);
}


void CHMListCtrl::Reset()
{
	DeleteAllItems();
	_urls.Clear();
	UpdateUI();
}


void CHMListCtrl::AddPairItem(const wxString& title, const wxString& url)
{ 
	InsertItem(GetItemCount(), title);
	_urls.Add(url);
}


void CHMListCtrl::LoadSelected()
{
	long item = -1;
        item = GetNextItem(item,
			   wxLIST_NEXT_ALL,
			   wxLIST_STATE_SELECTED);

        if(item == -1 || item > (long)_urls.GetCount() - 1)
		return;

	_html->LoadPage(_urls[(size_t)item]);	
}


void CHMListCtrl::UpdateUI()
{
	int newSize = GetClientSize().GetWidth() - 1;
	int cpp = GetCountPerPage();
	
	SetColumnWidth(0, wxLIST_AUTOSIZE);
	int currSize = GetColumnWidth(0);

	// If there's a scrollbar extract the width from the client area.
	if(cpp >= 0 && GetItemCount() > cpp)
		newSize -= wxSystemSettings::GetSystemMetric(wxSYS_VSCROLL_X);

	SetColumnWidth(0, newSize >= currSize ? newSize : currSize);
}



void CHMListCtrl::FindBestMatch(const wxString& title)
{
	wxListItem info;
	info.m_col = 0;

	long sz = GetItemCount();
	int tl = title.length();

	for(long i = 0; i < sz; ++i) {

		info.m_itemId = i;
		GetItem(info);

		if(!info.m_text.Left(tl).CmpNoCase(title)) {

			EnsureVisible(i);
			SetItemState(i, wxLIST_STATE_SELECTED,
				     wxLIST_STATE_SELECTED);
			break;
		}
	}

	Refresh();
	wxWindow::Update();
}



void CHMListCtrl::OnSize(wxSizeEvent& event)
{
	UpdateUI();
	event.Skip();
}



BEGIN_EVENT_TABLE(CHMListCtrl, wxListCtrl)
	EVT_SIZE(CHMListCtrl::OnSize)
END_EVENT_TABLE()
