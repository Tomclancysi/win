#include "pch.h"
#include "framework.h"
#include <vector>
#include <set>
#include "Resource.h"
#include "FeatureSelectorDlg.h"
#include "FeatureListCtrl.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

CFeatureSelectorDlg::CFeatureSelectorDlg()
    : CDialogEx(IDD_FEATURE_SELECTOR)
{
}

void CFeatureSelectorDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_LIST_FEATURES, m_listFeatures);
    DDX_Control(pDX, IDC_LIST_FILTERS, m_listFilters);
}

BEGIN_MESSAGE_MAP(CFeatureSelectorDlg, CDialogEx)
    ON_WM_CONTEXTMENU()
    ON_COMMAND(ID_SELECT_ALL, &CFeatureSelectorDlg::OnSelectAll)
    ON_COMMAND(ID_SELECT_NONE, &CFeatureSelectorDlg::OnSelectNone)
    ON_COMMAND(ID_SELECT_INVERT, &CFeatureSelectorDlg::OnSelectInvert)
    ON_NOTIFY(LVN_ITEMCHANGED, IDC_LIST_FILTERS, &CFeatureSelectorDlg::OnFilterItemChanged)
END_MESSAGE_MAP()

BOOL CFeatureSelectorDlg::OnInitDialog()
{
    CDialogEx::OnInitDialog();

    // Extended styles（开启双缓冲减少闪烁）
    m_listFeatures.SetExtendedStyle(m_listFeatures.GetExtendedStyle() | LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_CHECKBOXES | LVS_EX_DOUBLEBUFFER);
    m_listFilters.SetExtendedStyle(m_listFilters.GetExtendedStyle() | LVS_EX_FULLROWSELECT | LVS_EX_CHECKBOXES | LVS_EX_DOUBLEBUFFER);

    setupFeaturesList();
    setupFiltersList();
    initializeData();
    refreshFeaturesView();

    // Build context menu
    m_ctxMenu.CreatePopupMenu();
    m_ctxMenu.AppendMenu(MF_STRING, ID_SELECT_ALL, _T("全选"));
    m_ctxMenu.AppendMenu(MF_STRING, ID_SELECT_NONE, _T("全部选"));
    m_ctxMenu.AppendMenu(MF_STRING, ID_SELECT_INVERT, _T("反选"));

    return TRUE;
}

void CFeatureSelectorDlg::setupFeaturesList()
{
    m_listFeatures.InsertColumn(0, _T("特性"), LVCFMT_LEFT, 120);
    m_listFeatures.InsertColumn(1, _T("显示名称"), LVCFMT_LEFT, 120);
    m_listFeatures.InsertColumn(2, _T("类别"), LVCFMT_LEFT, 80);
}

void CFeatureSelectorDlg::setupFiltersList()
{
    m_listFilters.InsertColumn(0, _T("类别筛选器"), LVCFMT_LEFT, 110);

    struct Pair { FeatureCategory c; LPCTSTR t; } pairs[] = {
        { FeatureCategory::General, _T("常规") },
        { FeatureCategory::Geometry, _T("几何图形") },
        { FeatureCategory::Other, _T("其它") },
        { FeatureCategory::Effect3D, _T("三维效果") },
        { FeatureCategory::Graphic, _T("图形") }
    };

    for (int i = 0; i < (int)_countof(pairs); ++i)
    {
        int idx = m_listFilters.InsertItem(i, pairs[i].t);
        m_listFilters.SetCheck(idx, TRUE);
        m_enabledCategories.insert(pairs[i].c);
    }
}

void CFeatureSelectorDlg::initializeData()
{
    // 示例数据，可替换为实际数据来源
    m_allFeatures.clear();
    m_allFeatures.push_back({ _T("EdgeStyleId"), _T("EdgeStyleId"), FeatureCategory::Effect3D, true });
    m_allFeatures.push_back({ _T("FaceStyleId"), _T("FaceStyleId"), FeatureCategory::Effect3D, true });
    m_allFeatures.push_back({ _T("VisualStyleId"), _T("VisualStyleId"), FeatureCategory::Effect3D, true });
    m_allFeatures.push_back({ _T("半径"), _T("半径"), FeatureCategory::Geometry, true });
    m_allFeatures.push_back({ _T("布尔"), _T("布尔"), FeatureCategory::Other, true });
    m_allFeatures.push_back({ _T("材质"), _T("材质"), FeatureCategory::Effect3D, true });
    m_allFeatures.push_back({ _T("超链接地址"), _T("超链接地址"), FeatureCategory::Graphic, true });
    m_allFeatures.push_back({ _T("关键字"), _T("关键字"), FeatureCategory::General, true });
    m_allFeatures.push_back({ _T("全局属性"), _T("全局属性"), FeatureCategory::General, true });
}

CString CFeatureSelectorDlg::categoryToText(FeatureCategory c) const
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

void CFeatureSelectorDlg::refreshFeaturesView()
{
    m_listFeatures.DeleteAllItems();
    int row = 0;
    for (const auto& f : m_allFeatures)
    {
        if (m_enabledCategories.find(f.category) == m_enabledCategories.end())
            continue;

        int idx = m_listFeatures.InsertItem(row, f.attribute);
        m_listFeatures.SetItemText(idx, 1, f.displayName);
        m_listFeatures.SetItemText(idx, 2, categoryToText(f.category));
        m_listFeatures.SetCheck(idx, f.selected);
        ++row;
    }
}

void CFeatureSelectorDlg::OnContextMenu(CWnd* pWnd, CPoint point)
{
    if (pWnd->GetSafeHwnd() == m_listFeatures.GetSafeHwnd())
    {
        m_ctxMenu.TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y, this);
    }
}

void CFeatureSelectorDlg::OnSelectAll()
{
    for (int i = 0; i < m_listFeatures.GetItemCount(); ++i)
        m_listFeatures.SetCheck(i, TRUE);
}

void CFeatureSelectorDlg::OnSelectNone()
{
    for (int i = 0; i < m_listFeatures.GetItemCount(); ++i)
        m_listFeatures.SetCheck(i, FALSE);
}

void CFeatureSelectorDlg::OnSelectInvert()
{
    for (int i = 0; i < m_listFeatures.GetItemCount(); ++i)
        m_listFeatures.SetCheck(i, !m_listFeatures.GetCheck(i));
}

void CFeatureSelectorDlg::OnFilterItemChanged(NMHDR* pNMHDR, LRESULT* pResult)
{
    LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
    if ((pNMLV->uChanged & LVIF_STATE) != 0)
    {
        // rebuild enabled set
        m_enabledCategories.clear();
        for (int i = 0; i < m_listFilters.GetItemCount(); ++i)
        {
            bool checked = m_listFilters.GetCheck(i) != FALSE;
            FeatureCategory cat = FeatureCategory::General;
            switch (i)
            {
            case 0: cat = FeatureCategory::General; break;
            case 1: cat = FeatureCategory::Geometry; break;
            case 2: cat = FeatureCategory::Other; break;
            case 3: cat = FeatureCategory::Effect3D; break;
            case 4: cat = FeatureCategory::Graphic; break;
            }
            if (checked) m_enabledCategories.insert(cat);
        }
        refreshFeaturesView();
    }
    *pResult = 0;
}


