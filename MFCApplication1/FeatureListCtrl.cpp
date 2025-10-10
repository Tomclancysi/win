#include "pch.h"
#include "framework.h"
#include "FeatureListCtrl.h"
#include <algorithm>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

CFeatureListCtrl::CFeatureListCtrl()
    : m_isDragging(false), m_anchorIndex(-1), m_lastMin(-1), m_lastMax(-1)
{
}

BEGIN_MESSAGE_MAP(CFeatureListCtrl, CListCtrl)
    ON_WM_LBUTTONDOWN()
    ON_WM_MOUSEMOVE()
    ON_WM_LBUTTONUP()
    ON_NOTIFY_REFLECT(LVN_COLUMNCLICK, OnHeaderClick)
END_MESSAGE_MAP()

int CFeatureListCtrl::hitIndexFromPoint(CPoint pt) const
{
    LVHITTESTINFO ht{};
    ht.pt = pt;
    const_cast<CFeatureListCtrl*>(this)->SubItemHitTest(&ht);
    if (ht.iItem >= 0 && (ht.flags & (LVHT_ONITEMICON | LVHT_ONITEMLABEL)))
        return ht.iItem;
    return -1;
}

void CFeatureListCtrl::selectRange(int a, int b)
{
    if (a > b) std::swap(a, b);

    int count = GetItemCount();
    a = max(0, a);
    b = min(count - 1, b);

    // 只对增量差异进行更新，避免全量闪烁
    if (m_lastMin == -1 && m_lastMax == -1)
    {
        SetRedraw(FALSE);
        for (int i = 0; i < count; ++i)
            SetItemState(i, 0, LVIS_SELECTED);
        for (int i = a; i <= b; ++i)
            SetItemState(i, LVIS_SELECTED, LVIS_SELECTED);
        SetRedraw(TRUE);
        Invalidate(FALSE);
    }
    else
    {
        SetRedraw(FALSE);
        // 取消不再包含的部分
        for (int i = m_lastMin; i <= m_lastMax; ++i)
        {
            if (i < a || i > b)
                SetItemState(i, 0, LVIS_SELECTED);
        }
        // 选中新包含的部分
        for (int i = a; i <= b; ++i)
        {
            if (i < m_lastMin || i > m_lastMax)
                SetItemState(i, LVIS_SELECTED, LVIS_SELECTED);
        }
        SetRedraw(TRUE);
        Invalidate(FALSE);
    }

    m_lastMin = a;
    m_lastMax = b;
    EnsureVisible(b, FALSE);
}

void CFeatureListCtrl::OnLButtonDown(UINT nFlags, CPoint point)
{
    SetFocus();
    int idx = hitIndexFromPoint(point);
    if (idx >= 0)
    {
        m_isDragging = true;
        m_anchorIndex = idx;
        m_lastMin = m_lastMax = -1;
        selectRange(idx, idx);
        SetCapture();
    }
    else
    {
        CListCtrl::OnLButtonDown(nFlags, point);
    }
}

void CFeatureListCtrl::OnMouseMove(UINT nFlags, CPoint point)
{
    if (m_isDragging)
    {
        int idx = hitIndexFromPoint(point);
        if (idx >= 0 && m_anchorIndex >= 0)
            selectRange(m_anchorIndex, idx);
    }
    CListCtrl::OnMouseMove(nFlags, point);
}

void CFeatureListCtrl::OnLButtonUp(UINT nFlags, CPoint point)
{
    if (m_isDragging)
    {
        ReleaseCapture();
        m_isDragging = false;
        m_lastMin = m_lastMax = -1; // 完成一次拖选
    }
    CListCtrl::OnLButtonUp(nFlags, point);
}

void CFeatureListCtrl::OnHeaderClick(NMHDR* pNMHDR, LRESULT* pResult)
{
	NMLISTVIEW* pNMListView = (NMLISTVIEW*)pNMHDR;
	int col = pNMListView->iSubItem; // 点击的列索引

    if (m_sortCol == col)
        m_sortAsc = !m_sortAsc;
    else {
        m_sortCol = col;
        m_sortAsc = true;
    }
    // 排序
    SortItems(CompareFunc, (LPARAM)this);
    // 更新header箭头
    updateHeaderSortArrow();
    *pResult = 0;
}

int CALLBACK CFeatureListCtrl::CompareFunc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
    CFeatureListCtrl* pThis = (CFeatureListCtrl*)lParamSort;
    int col = pThis->m_sortCol;
    bool asc = pThis->m_sortAsc;

	FeatureItem* feat1 = (FeatureItem*)lParam1;
	FeatureItem* feat2 = (FeatureItem*)lParam2;

    int cmp = 0;
    switch (col)
    {
    case 0:
        cmp = feat1->attribute.Compare(feat2->attribute);
        break;
    case 1:
        cmp = feat1->displayName.Compare(feat2->displayName);
        break;
    case 2:
        cmp = categoryToText(feat1->category).Compare(categoryToText(feat2->category));
        break;
    }
    return asc ? cmp : -cmp;
}

void CFeatureListCtrl::updateHeaderSortArrow()
{
    CHeaderCtrl* pHeader = GetHeaderCtrl();
    if (!pHeader) return;
    int nColCount = pHeader->GetItemCount();
    for (int i = 0; i < nColCount; ++i)
    {
        HDITEM hdi = { 0 };
        hdi.mask = HDI_FORMAT;
        pHeader->GetItem(i, &hdi);
        hdi.fmt &= ~(HDF_SORTUP | HDF_SORTDOWN);
        if (i == m_sortCol)
            hdi.fmt |= m_sortAsc ? HDF_SORTUP : HDF_SORTDOWN;
        pHeader->SetItem(i, &hdi);
    }
}


CString categoryToText(FeatureCategory c)
{
	switch (c)
	{
	case FeatureCategory::General: return _T("常规");
	case FeatureCategory::Geometry: return _T("几何图形");
	case FeatureCategory::Other: return _T("其它");
	case FeatureCategory::Effect3D: return _T("三维效果");
	case FeatureCategory::Graphic: return _T("图形");
	}
	return _T("");
}
