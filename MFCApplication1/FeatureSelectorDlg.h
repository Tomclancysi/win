#pragma once

#include "framework.h"
#include <vector>
#include <set>
#include "Resource.h"
#include "FeatureListCtrl.h"

class CFeatureSelectorDlg : public CDialogEx
{
public:
    CFeatureSelectorDlg();

    enum { IDD = IDD_FEATURE_SELECTOR };

protected:
    virtual void DoDataExchange(CDataExchange* pDX) override;
    virtual BOOL OnInitDialog() override;

    DECLARE_MESSAGE_MAP()

private:
    // 自定义列表，支持拖动选择
    CListCtrl m_listFilters;
    CFeatureListCtrl m_listFeatures;
    CImageList m_imageList; // for checkboxes style consistency if needed

    CMenu m_ctxMenu;

    std::vector<FeatureItem> m_allFeatures;
    std::set<FeatureCategory> m_enabledCategories;

    void initializeData();
    void setupFeaturesList();
    void setupFiltersList();
    void refreshFeaturesView();

    afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
    afx_msg void OnSelectAll();
    afx_msg void OnSelectNone();
    afx_msg void OnSelectInvert();
    afx_msg void OnFilterItemChanged(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnFeatureItemChanged(NMHDR* pNMHDR, LRESULT* pResult);
};


