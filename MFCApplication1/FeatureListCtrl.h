#pragma once

#include "framework.h"

class CFeatureListCtrl : public CListCtrl
{
public:
    CFeatureListCtrl();

protected:
    DECLARE_MESSAGE_MAP()

    afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
    afx_msg void OnMouseMove(UINT nFlags, CPoint point);
    afx_msg void OnLButtonUp(UINT nFlags, CPoint point);

private:
    bool m_isDragging;
    int m_anchorIndex;
    int m_lastMin;
    int m_lastMax;

    int hitIndexFromPoint(CPoint pt) const;
    void selectRange(int a, int b);
};
