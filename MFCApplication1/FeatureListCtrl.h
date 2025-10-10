#pragma once

#include "framework.h"

class CFeatureListCtrl : public CListCtrl
{
public:
    CFeatureListCtrl();
    int m_sortCol = -1;         // 当前排序列，-1表示无排序
    bool m_sortAsc = true;      // 当前排序方向，true为升序，false为降序

protected:
    DECLARE_MESSAGE_MAP()

    afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
    afx_msg void OnMouseMove(UINT nFlags, CPoint point);
    afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
    afx_msg void OnHeaderClick(NMHDR* pNMHDR, LRESULT* pResult);
    static int CALLBACK CompareFunc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort);
    void updateHeaderSortArrow();

private:
    bool m_isDragging;
    int m_anchorIndex;
    int m_lastMin;
    int m_lastMax;

    int hitIndexFromPoint(CPoint pt) const;
    void selectRange(int a, int b);
};
