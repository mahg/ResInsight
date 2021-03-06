/////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2011-     Statoil ASA
//  Copyright (C) 2013-     Ceetron Solutions AS
//  Copyright (C) 2011-2012 Ceetron AS
// 
//  ResInsight is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
// 
//  ResInsight is distributed in the hope that it will be useful, but WITHOUT ANY
//  WARRANTY; without even the implied warranty of MERCHANTABILITY or
//  FITNESS FOR A PARTICULAR PURPOSE.
// 
//  See the GNU General Public License at <http://www.gnu.org/licenses/gpl.html> 
//  for more details.
//
/////////////////////////////////////////////////////////////////////////////////

#include "Rim3dOverlayInfoConfig.h"

#include "RicGridStatisticsDialog.h"

#include "RigCaseCellResultsData.h"
#include "RigEclipseCaseData.h"
#include "RigEclipseNativeVisibleCellsStatCalc.h"
#include "RigFemNativeVisibleCellsStatCalc.h"
#include "RigFemPartCollection.h"
#include "RigFemPartResultsCollection.h"
#include "RigFemResultAddress.h"
#include "RigFlowDiagResults.h"
#include "RigFlowDiagVisibleCellsStatCalc.h"
#include "RigGeoMechCaseData.h"
#include "RigMainGrid.h"
#include "RigStatisticsDataCache.h"

#include "RimCellEdgeColors.h"
#include "RimEclipseCase.h"
#include "RimEclipseCellColors.h"
#include "RimEclipseFaultColors.h"
#include "RimEclipsePropertyFilterCollection.h"
#include "RimEclipseView.h"
#include "RimFaultInViewCollection.h"
#include "RimGeoMechCase.h"
#include "RimGeoMechResultDefinition.h"
#include "RimGeoMechView.h"
#include "RimReservoirCellResultsStorage.h"
#include "RimSimWellInViewCollection.h"
#include "RimView.h"

#include "RiuViewer.h"

#include <QMessageBox>

CAF_PDM_SOURCE_INIT(Rim3dOverlayInfoConfig, "View3dOverlayInfoConfig");
//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------

namespace caf
{
    template<>
    void caf::AppEnum<Rim3dOverlayInfoConfig::StatisticsTimeRangeType>::setUp()
    {
        addItem(Rim3dOverlayInfoConfig::ALL_TIMESTEPS, "ALL_TIMESTEPS", "All Time Steps");
        addItem(Rim3dOverlayInfoConfig::CURRENT_TIMESTEP, "CURRENT_TIMESTEP", "Current Time Step");
        setDefault(Rim3dOverlayInfoConfig::ALL_TIMESTEPS);
    }
}
//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------

namespace caf
{
    template<>
    void caf::AppEnum<Rim3dOverlayInfoConfig::StatisticsCellRangeType>::setUp()
    {
        addItem(Rim3dOverlayInfoConfig::ALL_CELLS, "ALL_CELLS", "All Active Cells");
        addItem(Rim3dOverlayInfoConfig::VISIBLE_CELLS, "VISIBLE_CELLS", "Visible Cells");
        setDefault(Rim3dOverlayInfoConfig::ALL_CELLS);
    }
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
Rim3dOverlayInfoConfig::Rim3dOverlayInfoConfig()
{
    CAF_PDM_InitObject("Info Box", ":/InfoBox16x16.png", "", "");

    CAF_PDM_InitField(&active, "Active", true, "Active", "", "", "");
    active.uiCapability()->setUiHidden(true);

    CAF_PDM_InitField(&showAnimProgress,       "ShowAnimProgress",       true, "Animation progress", "", "", "");
    CAF_PDM_InitField(&showCaseInfo,           "ShowInfoText",           true, "Case Info", "", "", "");
    CAF_PDM_InitField(&showResultInfo,         "ShowResultInfo",         true, "Result Info", "", "", "");
    CAF_PDM_InitField(&showHistogram,          "ShowHistogram",          true, "Histogram", "", "", "");
    CAF_PDM_InitField(&showVolumeWeightedMean, "ShowVolumeWeightedMean", true, "Mobile Volume Weighted Mean", "", "", "");

    CAF_PDM_InitFieldNoDefault(&m_statisticsTimeRange, "StatisticsTimeRange", "Statistics Time Range", "", "", "");
    CAF_PDM_InitFieldNoDefault(&m_statisticsCellRange, "StatisticsCellRange", "Statistics Cell Range", "", "", "");
    //m_statisticsCellRange.uiCapability()->setUiHidden(true);

    m_isVisCellStatUpToDate = false;

    m_gridStatisticsDialog = std::unique_ptr<RicGridStatisticsDialog>(new RicGridStatisticsDialog(nullptr));
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
Rim3dOverlayInfoConfig::~Rim3dOverlayInfoConfig()
{

}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void Rim3dOverlayInfoConfig::fieldChangedByUi(const caf::PdmFieldHandle* changedField, const QVariant& oldValue, const QVariant& newValue)
{

    if ( hasInvalidStatisticsCombination() )
    {
        displayPropertyFilteredStatisticsMessage(false);
        if ( changedField == &m_statisticsTimeRange ) m_statisticsTimeRange = CURRENT_TIMESTEP;
        if ( changedField == &m_statisticsCellRange ) m_statisticsCellRange = ALL_CELLS;
    }

    if (changedField == &showResultInfo)
    {
        if (!showResultInfo())
        {
            showVolumeWeightedMean = false;
            showVolumeWeightedMean.uiCapability()->setUiReadOnly(true);
        }
        else
        {
            showVolumeWeightedMean = true;
            showVolumeWeightedMean.uiCapability()->setUiReadOnly(false);
        }
    }

    this->update3DInfo();

    if (m_viewDef && m_viewDef->viewer())
    {
        m_viewDef->viewer()->update();
    }

}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void Rim3dOverlayInfoConfig::setPosition(cvf::Vec2ui position)
{
    m_position = position;
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
Rim3dOverlayInfoConfig::HistogramData Rim3dOverlayInfoConfig::histogramData()
{
    auto eclipseView = dynamic_cast<RimEclipseView*>(m_viewDef.p());
    auto geoMechView = dynamic_cast<RimGeoMechView*>(m_viewDef.p());
    
    if (eclipseView) return histogramData(eclipseView);
    if (geoMechView) return histogramData(geoMechView);
    return HistogramData();
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
QString Rim3dOverlayInfoConfig::caseInfoText()
{
    auto eclipseView = dynamic_cast<RimEclipseView*>(m_viewDef.p());
    auto geoMechView = dynamic_cast<RimGeoMechView*>(m_viewDef.p());

    if (eclipseView) return caseInfoText(eclipseView);
    if (geoMechView) return caseInfoText(geoMechView);
    return "";
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
QString Rim3dOverlayInfoConfig::resultInfoText(const HistogramData& histData)
{
    auto eclipseView = dynamic_cast<RimEclipseView*>(m_viewDef.p());
    auto geoMechView = dynamic_cast<RimGeoMechView*>(m_viewDef.p());

    if (eclipseView) return resultInfoText(histData, eclipseView);
    if (geoMechView) return resultInfoText(histData, geoMechView);
    return "";
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
Rim3dOverlayInfoConfig::HistogramData Rim3dOverlayInfoConfig::histogramData(RimEclipseView* eclipseView) 
{
    HistogramData histData;

    if (eclipseView)
    {
        bool isResultsInfoRelevant = eclipseView->hasUserRequestedAnimation() && eclipseView->cellResult()->hasResult();

        if (isResultsInfoRelevant)
        {
            size_t scalarIndex = eclipseView->cellResult()->scalarResultIndex();

            if (scalarIndex != cvf::UNDEFINED_SIZE_T)
            {
                if (m_statisticsCellRange == ALL_CELLS)
                {
                    if (m_statisticsTimeRange == ALL_TIMESTEPS)
                    {
                        eclipseView->currentGridCellResults()->minMaxCellScalarValues(scalarIndex, histData.min, histData.max);
                        eclipseView->currentGridCellResults()->p10p90CellScalarValues(scalarIndex, histData.p10, histData.p90);
                        eclipseView->currentGridCellResults()->meanCellScalarValues(scalarIndex, histData.mean);
                        eclipseView->currentGridCellResults()->sumCellScalarValues(scalarIndex, histData.sum);
                        histData.histogram = &(eclipseView->currentGridCellResults()->cellScalarValuesHistogram(scalarIndex));
                    }
                    else if (m_statisticsTimeRange == CURRENT_TIMESTEP)
                    {
                        int currentTimeStep = eclipseView->currentTimeStep();
                        if (eclipseView->cellResult()->hasStaticResult())
                        {
                            currentTimeStep = 0;
                        }

                        eclipseView->currentGridCellResults()->minMaxCellScalarValues(scalarIndex, currentTimeStep, histData.min, histData.max);
                        eclipseView->currentGridCellResults()->p10p90CellScalarValues(scalarIndex, currentTimeStep, histData.p10, histData.p90);
                        eclipseView->currentGridCellResults()->meanCellScalarValues(scalarIndex, currentTimeStep, histData.mean);
                        eclipseView->currentGridCellResults()->sumCellScalarValues(scalarIndex, currentTimeStep, histData.sum);
                        eclipseView->currentGridCellResults()->mobileVolumeWeightedMean(scalarIndex, currentTimeStep, histData.weightedMean);

                        histData.histogram = &(eclipseView->currentGridCellResults()->cellScalarValuesHistogram(scalarIndex, currentTimeStep));
                    }
                    else
                    {
                        CVF_ASSERT(false);
                    }
                }
                else if (m_statisticsCellRange == VISIBLE_CELLS)
                {
                    updateVisCellStatsIfNeeded();
                    if (m_statisticsTimeRange == ALL_TIMESTEPS)
                    {
                        // TODO: Only valid if we have no dynamic property filter
                        m_visibleCellStatistics->meanCellScalarValues(histData.mean);
                        m_visibleCellStatistics->minMaxCellScalarValues(histData.min, histData.max);
                        m_visibleCellStatistics->p10p90CellScalarValues(histData.p10, histData.p90);
                        m_visibleCellStatistics->sumCellScalarValues(histData.sum);

                        histData.histogram = &(m_visibleCellStatistics->cellScalarValuesHistogram());
                    }
                    else if (m_statisticsTimeRange == CURRENT_TIMESTEP)
                    {
                        int currentTimeStep = eclipseView->currentTimeStep();
                        if (eclipseView->cellResult()->hasStaticResult())
                        {
                            currentTimeStep = 0;
                        }

                        m_visibleCellStatistics->meanCellScalarValues(currentTimeStep, histData.mean);
                        m_visibleCellStatistics->minMaxCellScalarValues(currentTimeStep, histData.min, histData.max);
                        m_visibleCellStatistics->p10p90CellScalarValues(currentTimeStep, histData.p10, histData.p90);
                        m_visibleCellStatistics->sumCellScalarValues(currentTimeStep, histData.sum);
                        m_visibleCellStatistics->mobileVolumeWeightedMean(currentTimeStep, histData.weightedMean);

                        histData.histogram = &(m_visibleCellStatistics->cellScalarValuesHistogram(currentTimeStep));
                    }
                }
            }
            else if (eclipseView->cellResult()->isFlowDiagOrInjectionFlooding())
            {
                if (m_statisticsTimeRange == CURRENT_TIMESTEP || m_statisticsTimeRange == ALL_TIMESTEPS) // All timesteps is ignored
                {
                    int currentTimeStep = eclipseView->currentTimeStep();

                    if (m_statisticsCellRange == ALL_CELLS)
                    {
                        RigFlowDiagResults* fldResults = eclipseView->cellResult()->flowDiagSolution()->flowDiagResults();
                        RigFlowDiagResultAddress resAddr = eclipseView->cellResult()->flowDiagResAddress();

                        fldResults->minMaxScalarValues(resAddr, currentTimeStep, &histData.min, &histData.max);
                        fldResults->p10p90ScalarValues(resAddr, currentTimeStep, &histData.p10, &histData.p90);
                        fldResults->meanScalarValue(resAddr, currentTimeStep, &histData.mean);
                        fldResults->sumScalarValue(resAddr, currentTimeStep, &histData.sum);
                        fldResults->mobileVolumeWeightedMean(resAddr, currentTimeStep, &histData.weightedMean);

                        histData.histogram = &(fldResults->scalarValuesHistogram(resAddr, currentTimeStep));
                    }
                    else if (m_statisticsCellRange == VISIBLE_CELLS)
                    {
                        updateVisCellStatsIfNeeded();

                        m_visibleCellStatistics->meanCellScalarValues(currentTimeStep, histData.mean);
                        m_visibleCellStatistics->minMaxCellScalarValues(currentTimeStep, histData.min, histData.max);
                        m_visibleCellStatistics->p10p90CellScalarValues(currentTimeStep, histData.p10, histData.p90);
                        m_visibleCellStatistics->sumCellScalarValues(currentTimeStep, histData.sum);
                        m_visibleCellStatistics->mobileVolumeWeightedMean(currentTimeStep, histData.weightedMean);

                        histData.histogram = &(m_visibleCellStatistics->cellScalarValuesHistogram(currentTimeStep));
                    }
                }
            }
        }
    }
    return histData;
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
Rim3dOverlayInfoConfig::HistogramData Rim3dOverlayInfoConfig::histogramData(RimGeoMechView* geoMechView)
{
    HistogramData histData;

    if (geoMechView)
    {
        RimGeoMechCase* geoMechCase = geoMechView->geoMechCase();
        RigGeoMechCaseData* caseData = geoMechCase ? geoMechCase->geoMechData() : nullptr;
        bool isResultsInfoRelevant = caseData && geoMechView->hasUserRequestedAnimation() && geoMechView->cellResultResultDefinition()->hasResult();

        if (isResultsInfoRelevant)
        {
            RigFemResultAddress resAddress = geoMechView->cellResultResultDefinition()->resultAddress();
            if (m_statisticsCellRange == ALL_CELLS)
            {
                if (m_statisticsTimeRange == ALL_TIMESTEPS)
                {
                    caseData->femPartResults()->meanScalarValue(resAddress, &histData.mean);
                    caseData->femPartResults()->minMaxScalarValues(resAddress, &histData.min, &histData.max);
                    caseData->femPartResults()->p10p90ScalarValues(resAddress, &histData.p10, &histData.p90);
                    caseData->femPartResults()->sumScalarValue(resAddress, &histData.sum);

                    histData.histogram = &(caseData->femPartResults()->scalarValuesHistogram(resAddress));
                }
                else if (m_statisticsTimeRange == CURRENT_TIMESTEP)
                {
                    int timeStepIdx = geoMechView->currentTimeStep();
                    caseData->femPartResults()->meanScalarValue(resAddress, timeStepIdx, &histData.mean);
                    caseData->femPartResults()->minMaxScalarValues(resAddress, timeStepIdx, &histData.min, &histData.max);
                    caseData->femPartResults()->p10p90ScalarValues(resAddress, timeStepIdx, &histData.p10, &histData.p90);
                    caseData->femPartResults()->sumScalarValue(resAddress, timeStepIdx, &histData.sum);

                    histData.histogram = &(caseData->femPartResults()->scalarValuesHistogram(resAddress, timeStepIdx));
                }
            }
            else if (m_statisticsCellRange == VISIBLE_CELLS)
            {
                this->updateVisCellStatsIfNeeded();

                if (m_statisticsTimeRange == ALL_TIMESTEPS)
                {
                    // TODO: Only valid if we have no dynamic property filter
                    m_visibleCellStatistics->meanCellScalarValues(histData.mean);
                    m_visibleCellStatistics->minMaxCellScalarValues(histData.min, histData.max);
                    m_visibleCellStatistics->p10p90CellScalarValues(histData.p10, histData.p90);
                    m_visibleCellStatistics->sumCellScalarValues(histData.sum);

                    histData.histogram = &(m_visibleCellStatistics->cellScalarValuesHistogram());
                }
                else if (m_statisticsTimeRange == CURRENT_TIMESTEP)
                {
                    int timeStepIdx = geoMechView->currentTimeStep();
                    m_visibleCellStatistics->meanCellScalarValues(timeStepIdx, histData.mean);
                    m_visibleCellStatistics->minMaxCellScalarValues(timeStepIdx, histData.min, histData.max);
                    m_visibleCellStatistics->p10p90CellScalarValues(timeStepIdx, histData.p10, histData.p90);
                    m_visibleCellStatistics->sumCellScalarValues(timeStepIdx, histData.sum);

                    histData.histogram = &(m_visibleCellStatistics->cellScalarValuesHistogram(timeStepIdx));
                }
            }
        }
    }
    return histData;
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
QString Rim3dOverlayInfoConfig::caseInfoText(RimEclipseView* eclipseView)
{
    QString infoText;

    if (eclipseView)
    {
        QString caseName;
        QString totCellCount;
        QString activeCellCountText;
        QString fractureActiveCellCount;
        QString iSize, jSize, kSize;
        QString zScale;

        if (eclipseView->mainGrid())
        {
            caseName = eclipseView->eclipseCase()->caseUserDescription();
            totCellCount = QString::number(eclipseView->mainGrid()->globalCellArray().size());
            size_t mxActCellCount = eclipseView->eclipseCase()->eclipseCaseData()->activeCellInfo(RiaDefines::MATRIX_MODEL)->reservoirActiveCellCount();
            size_t frActCellCount = eclipseView->eclipseCase()->eclipseCaseData()->activeCellInfo(RiaDefines::FRACTURE_MODEL)->reservoirActiveCellCount();
            if (frActCellCount > 0)  activeCellCountText += "Matrix : ";
            activeCellCountText += QString::number(mxActCellCount);
            if (frActCellCount > 0)  activeCellCountText += " Fracture : " + QString::number(frActCellCount);

            iSize = QString::number(eclipseView->mainGrid()->cellCountI());
            jSize = QString::number(eclipseView->mainGrid()->cellCountJ());
            kSize = QString::number(eclipseView->mainGrid()->cellCountK());

            zScale = QString::number(eclipseView->scaleZ());

        }

        infoText += QString(
            "<p><b><center>-- %1 --</center></b><p>  "
            "<b>Cell count. Total:</b> %2 <b>Active:</b> %3 <br>"
            "<b>Main Grid I,J,K:</b> %4, %5, %6 <b>Z-Scale:</b> %7<br>").arg(caseName, totCellCount, activeCellCountText, iSize, jSize, kSize, zScale);
    }

    return infoText;
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
QString Rim3dOverlayInfoConfig::caseInfoText(RimGeoMechView* geoMechView)
{
    QString infoText;

    if (geoMechView)
    {
        RimGeoMechCase* geoMechCase = geoMechView->geoMechCase();
        RigGeoMechCaseData* caseData = geoMechCase ? geoMechCase->geoMechData() : nullptr;
        RigFemPartCollection* femParts = caseData ? caseData->femParts() : NULL;

        if (femParts)
        {
            QString caseName = geoMechCase->caseUserDescription();
            QString cellCount = QString("%1").arg(femParts->totalElementCount());
            QString zScale = QString::number(geoMechView->scaleZ());

            infoText = QString(
                "<p><b><center>-- %1 --</center></b><p>"
                "<b>Cell count:</b> %2 <b>Z-Scale:</b> %3<br>").arg(caseName, cellCount, zScale);
        }
    }
    return infoText;
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
QString Rim3dOverlayInfoConfig::resultInfoText(const HistogramData& histData, RimEclipseView* eclipseView)
{
    QString infoText;

    if (eclipseView)
    {
        bool isResultsInfoRelevant = eclipseView->hasUserRequestedAnimation() && eclipseView->cellResult()->hasResult();

        if (eclipseView->cellResult()->isTernarySaturationSelected())
        {
            QString propName = eclipseView->cellResult()->resultVariableUiShortName();
            infoText += QString("<b>Cell Property:</b> %1 ").arg(propName);
        }

        if (isResultsInfoRelevant)
        {
            QString propName = eclipseView->cellResult()->resultVariableUiShortName();
            QString timeRangeText = m_statisticsTimeRange().uiText();
            if (eclipseView->cellResult()->isFlowDiagOrInjectionFlooding())
            {
                timeRangeText = caf::AppEnum<StatisticsTimeRangeType>::uiText(CURRENT_TIMESTEP);
            }

            infoText += QString("<b>Cell Property:</b> %1 ").arg(propName);
            infoText += QString("<br><b>Statistics:</b> ") + timeRangeText + " and " + m_statisticsCellRange().uiText();
            infoText += QString("<table border=0 cellspacing=5 >"
                                "<tr> <td>Min</td> <td>P10</td> <td>Mean</td> <td>P90</td> <td>Max</td> <td>Sum</td> </tr>"
                                "<tr> <td>%1</td>  <td> %2</td> <td>  %3</td> <td> %4</td> <td> %5</td> <td> %6</td> </tr>"
                                "</table>").arg(histData.min).arg(histData.p10).arg(histData.mean).arg(histData.p90).arg(histData.max).arg(histData.sum);

            if (eclipseView->faultResultSettings()->hasValidCustomResult())
            {
                QString faultMapping;
                bool isShowingGrid = eclipseView->faultCollection()->isGridVisualizationMode();
                if (!isShowingGrid)
                {
                    if (eclipseView->faultCollection()->faultResult() == RimFaultInViewCollection::FAULT_BACK_FACE_CULLING)
                    {
                        faultMapping = "Cells behind fault";
                    }
                    else if (eclipseView->faultCollection()->faultResult() == RimFaultInViewCollection::FAULT_FRONT_FACE_CULLING)
                    {
                        faultMapping = "Cells in front of fault";
                    }
                    else
                    {
                        faultMapping = "Cells in front and behind fault";
                    }
                }
                else
                {
                    faultMapping = "Cells in front and behind fault";
                }

                infoText += QString("<b>Fault results: </b> %1<br>").arg(faultMapping);
                infoText += QString("<b>Fault Property:</b> %1 <br>").arg(eclipseView->faultResultSettings()->customFaultResult()->resultVariableUiShortName());
            }
        }

        if (eclipseView->hasUserRequestedAnimation() && eclipseView->cellEdgeResult()->hasResult())
        {
            double min, max;
            QString cellEdgeName = eclipseView->cellEdgeResult()->resultVariableUiShortName();
            eclipseView->cellEdgeResult()->minMaxCellEdgeValues(min, max);
            infoText += QString("<b>Cell Edge Property:</b> %1 ").arg(cellEdgeName);
            infoText += QString("<table border=0 cellspacing=5 >"
                                "<tr> <td>Min</td> <td></td> <td></td> <td></td> <td>Max</td> </tr>"
                                "<tr> <td>%1</td>  <td></td> <td></td> <td></td> <td> %2</td></tr>"
                                "</table>").arg(min).arg(max);
        }

        if (showVolumeWeightedMean() && histData.weightedMean != HUGE_VAL)
        {
            infoText += QString("<b>Mobile Volume Weighted Mean:</b> %1").arg(histData.weightedMean);
        }
    }
    return infoText;
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
QString Rim3dOverlayInfoConfig::resultInfoText(const HistogramData& histData, RimGeoMechView* geoMechView)
{
    QString infoText;

    if (geoMechView)
    {
        RimGeoMechCase* geoMechCase = geoMechView->geoMechCase();
        RigGeoMechCaseData* caseData = geoMechCase ? geoMechCase->geoMechData() : nullptr;
        bool isResultsInfoRelevant = caseData && geoMechView->hasUserRequestedAnimation() && geoMechView->cellResultResultDefinition()->hasResult();

        if (isResultsInfoRelevant)
        {
            QString resultPos;
            QString fieldName = geoMechView->cellResultResultDefinition()->resultFieldUiName();
            QString compName = geoMechView->cellResultResultDefinition()->resultComponentUiName();

            switch (geoMechView->cellResultResultDefinition()->resultPositionType())
            {
            case RIG_NODAL:
                resultPos = "Nodal";
                break;

            case RIG_ELEMENT_NODAL:
                resultPos = "Element nodal";
                break;

            case RIG_INTEGRATION_POINT:
                resultPos = "Integration point";
                break;

            default:
                break;
            }

            infoText += QString("<b>Cell result:</b> %1, %2, %3").arg(resultPos).arg(fieldName).arg(compName);

            infoText += QString("<br><b>Statistics:</b> ") + m_statisticsTimeRange().uiText() + " and " + m_statisticsCellRange().uiText();
            infoText += QString("<table border=0 cellspacing=5 >"
                                "<tr> <td>Min</td> <td>P10</td> <td>Mean</td> <td>P90</td> <td>Max</td> <td>Sum</td> </tr>"
                                "<tr> <td>%1</td>  <td> %2</td> <td> %3</td>  <td> %4</td> <td> %5</td> <td> %6</td> </tr>"
                                "</table>").arg(histData.min).arg(histData.p10).arg(histData.mean).arg(histData.p90).arg(histData.max).arg(histData.sum);
        }
    }
    return infoText;
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void Rim3dOverlayInfoConfig::showStatisticsInfoDialog()
{
    if (m_viewDef)
    {
        // Show dialog before setting data due to text edit auto height setting
        m_gridStatisticsDialog->resize(600, 800);
        m_gridStatisticsDialog->show();

        m_gridStatisticsDialog->setLabel("Grid statistics");
        m_gridStatisticsDialog->setInfoText(m_viewDef);
        m_gridStatisticsDialog->setHistogramData(m_viewDef);

        m_gridStatisticsDialog->raise();
    }
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void Rim3dOverlayInfoConfig::update3DInfo()
{
    this->updateUiIconFromToggleField();

    if (!m_viewDef) return;
    if (!m_viewDef->viewer()) return;

    if (!this->active())
    {
        m_viewDef->viewer()->showInfoText(false);
        m_viewDef->viewer()->showHistogram(false);
        m_viewDef->viewer()->showAnimationProgress(false);

        return;
    }

    m_viewDef->viewer()->showInfoText(showCaseInfo() || showResultInfo());
    m_viewDef->viewer()->showHistogram(false);
    m_viewDef->viewer()->showAnimationProgress(showAnimProgress());

    m_isVisCellStatUpToDate = false;

    if (hasInvalidStatisticsCombination())
    {
        displayPropertyFilteredStatisticsMessage(true);
        m_statisticsTimeRange = CURRENT_TIMESTEP;
    }

    RimEclipseView * reservoirView = dynamic_cast<RimEclipseView*>(m_viewDef.p());
    if (reservoirView)
    {
        updateEclipse3DInfo(reservoirView);

        // Update statistics dialog
        m_gridStatisticsDialog->setInfoText(reservoirView);
        m_gridStatisticsDialog->setHistogramData(reservoirView);
    }

    RimGeoMechView * geoMechView = dynamic_cast<RimGeoMechView*>(m_viewDef.p());
    if (geoMechView)
    {
        updateGeoMech3DInfo(geoMechView);

        // Update statistics dialog
        m_gridStatisticsDialog->setInfoText(geoMechView);
        m_gridStatisticsDialog->setHistogramData(geoMechView);
    }
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
caf::PdmFieldHandle* Rim3dOverlayInfoConfig::objectToggleField()
{
    return &active;
}


//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void Rim3dOverlayInfoConfig::defineUiOrdering(QString uiConfigName, caf::PdmUiOrdering& uiOrdering)
{
    caf::PdmUiGroup* visGroup = uiOrdering.addNewGroup("Visibility");

    visGroup->add(&showAnimProgress);
    visGroup->add(&showCaseInfo);
    visGroup->add(&showResultInfo);
    visGroup->add(&showVolumeWeightedMean);
    visGroup->add(&showHistogram);

    caf::PdmUiGroup* statGroup = uiOrdering.addNewGroup("Statistics Options");
    RimEclipseView * eclipseView = dynamic_cast<RimEclipseView*>(m_viewDef.p());

    if (!eclipseView || !eclipseView->cellResult()->isFlowDiagOrInjectionFlooding())
    {
        statGroup->add(&m_statisticsTimeRange);
    }
    statGroup->add(&m_statisticsCellRange);

    uiOrdering.skipRemainingFields(true);
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void Rim3dOverlayInfoConfig::setReservoirView(RimView* ownerReservoirView)
{
    m_viewDef = ownerReservoirView;
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void Rim3dOverlayInfoConfig::updateEclipse3DInfo(RimEclipseView * eclipseView)
{
    HistogramData histData;

    if (showHistogram() || showResultInfo())
    {
        histData = histogramData();
    }

    QString infoText;

    if (showCaseInfo())
    {
        infoText = caseInfoText();
    }

    if (showResultInfo())
    {
        infoText += resultInfoText(histData);
    }

    if (!infoText.isEmpty())
    {
        eclipseView->viewer()->setInfoText(infoText);
    }

    if (showHistogram())
    {
        bool isResultsInfoRelevant = eclipseView->hasUserRequestedAnimation() && eclipseView->cellResult()->hasResult();
        
        if (isResultsInfoRelevant && histData.histogram)
        {
            eclipseView->viewer()->showHistogram(true);
            eclipseView->viewer()->setHistogram(histData.min, histData.max, *histData.histogram);
            eclipseView->viewer()->setHistogramPercentiles(histData.p10, histData.p90, histData.mean);
        }
    }
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void Rim3dOverlayInfoConfig::updateGeoMech3DInfo(RimGeoMechView * geoMechView)
{
    HistogramData histData;

    if (showResultInfo() || showHistogram())
    {
        histData = histogramData(geoMechView);
    }

    // Compose text

    QString infoText;

    if (showCaseInfo())
    {
        infoText = caseInfoText(geoMechView);
    }

    if (showResultInfo())
    {
        infoText += resultInfoText(histData, geoMechView);
    }

    if (!infoText.isEmpty())
    {
        geoMechView->viewer()->setInfoText(infoText);
    }

    // Populate histogram

    if (showHistogram())
    {
        RimGeoMechCase* geoMechCase = geoMechView->geoMechCase();
        RigGeoMechCaseData* caseData = geoMechCase ? geoMechCase->geoMechData() : nullptr;
        bool isResultsInfoRelevant = caseData && geoMechView->hasUserRequestedAnimation() && geoMechView->cellResultResultDefinition()->hasResult();

        if (isResultsInfoRelevant)
        {
            geoMechView->viewer()->showHistogram(true);
            geoMechView->viewer()->setHistogram(histData.min, histData.max, *histData.histogram);
            geoMechView->viewer()->setHistogramPercentiles(histData.p10, histData.p90, histData.mean);
        }
    }
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void Rim3dOverlayInfoConfig::updateVisCellStatsIfNeeded()
{
    RimEclipseView * eclipseView = dynamic_cast<RimEclipseView*>(m_viewDef.p());
    RimGeoMechView * geoMechView = dynamic_cast<RimGeoMechView*>(m_viewDef.p());

    if (!m_isVisCellStatUpToDate)
    {
        cvf::ref<RigStatisticsCalculator> calc;
        if (geoMechView)
        {
            RigFemResultAddress resAddress = geoMechView->cellResultResultDefinition()->resultAddress();
            calc = new RigFemNativeVisibleCellsStatCalc(geoMechView->geoMechCase()->geoMechData(),
                resAddress,
                geoMechView->currentTotalCellVisibility().p());

        }
        else if (eclipseView)
        {
            if (eclipseView->cellResult()->isFlowDiagOrInjectionFlooding())
            {
                RigFlowDiagResultAddress resAddr = eclipseView->cellResult()->flowDiagResAddress();
                RigFlowDiagResults* fldResults = eclipseView->cellResult()->flowDiagSolution()->flowDiagResults();
                calc = new RigFlowDiagVisibleCellsStatCalc(fldResults,
                    resAddr,
                    eclipseView->currentTotalCellVisibility().p());

            }
            else
            {
                size_t scalarIndex = eclipseView->cellResult()->scalarResultIndex();
                calc = new RigEclipseNativeVisibleCellsStatCalc(eclipseView->currentGridCellResults(),
                    scalarIndex,
                    eclipseView->currentTotalCellVisibility().p());
            }
        }

        m_visibleCellStatistics = new RigStatisticsDataCache(calc.p());
        m_isVisCellStatUpToDate = true;
    }
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void Rim3dOverlayInfoConfig::displayPropertyFilteredStatisticsMessage(bool showSwitchToCurrentTimestep)
{
    static bool isShowing = false;

    QString switchString;
    if (showSwitchToCurrentTimestep)
    {
        switchString = QString("<br>"
            "Switching to statistics for <b>Current Time Step</b>");
    }

    if (!isShowing)
    {
        isShowing = true;
        QMessageBox::information(m_viewDef->viewer()->layoutWidget(),
            QString("ResInsight"),
            QString("Statistics not available<br>"
                "<br>"
                "Statistics calculations of <b>Visible Cells</b> for <b>All Time Steps</b> is not supported<br>"
                "when you have an active Property filter on a time varying result.<br>")
            + switchString);
        isShowing = false;
    }
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
bool Rim3dOverlayInfoConfig::hasInvalidStatisticsCombination()
{
    if (m_viewDef->propertyFilterCollection() 
        && m_viewDef->propertyFilterCollection()->hasActiveDynamicFilters() 
        && m_statisticsCellRange() == VISIBLE_CELLS 
        && m_statisticsTimeRange() == ALL_TIMESTEPS )
    {
        RimEclipseView * eclipseView = dynamic_cast<RimEclipseView*>(m_viewDef.p());
        if (!(eclipseView && eclipseView->cellResult()->isFlowDiagOrInjectionFlooding())) // If isFlowDiagOrInjFlooding then skip this check as ALL_TIMESTEPS is overridden to CURRENT behind the scenes
        {
            return true;
        }
    }

    return false;
}
