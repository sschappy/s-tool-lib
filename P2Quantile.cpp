#include "P2Quantile.hpp"

#include <algorithm>
#include <limits>


P2QuantileEstimator::P2QuantileEstimator(double quantile) : m_p(quantile)
{
    if (m_p <= 0.0 || m_p >= 1.0)
    {
        m_p = 0.5;
    }

    m_dn[0] = 0.0;
    m_dn[1] = m_p / 2.0;
    m_dn[2] = m_p;
    m_dn[3] = (1.0 + m_p) / 2.0;
    m_dn[4] = 1.0;
}

void P2QuantileEstimator::update(double x)
{
    ++m_count;

    if (m_count <= P2_NUM_POINTS)
    {
        m_qval[m_count - 1] = x;

        if (m_count == P2_NUM_POINTS)
        {
            std::sort(m_qval.begin(), m_qval.end());
            for (int i = 0; i < P2_NUM_POINTS; ++i)
            {
                m_npos[i] = i + 1;
            }
        }
        return;
    }

    int k = 0;
    while (k < P2_NUM_POINTS && m_qval[k] <= x)
    {
        ++k;
    }

    for (int i = k; i < P2_NUM_POINTS; ++i)
    {
        ++m_npos[i];
    }

    if (k == 0)
    {
        m_qval[0] = x;
        m_npos[0] = 1;
    }
    else if (k == P2_NUM_POINTS)
    {
        m_qval[P2_NUM_POINTS - 1] = x;
    }

    for (int i = 1; i < P2_NUM_POINTS - 1; ++i)
    {
        double ni = m_npos[i];
        double ni_expect = 1.0 + (m_count - 1) * m_dn[i];

        if (ni_expect - ni <= -1.0 && m_npos[i - 1] - m_npos[i] < -1)
        {
            m_qval[i] -= (m_qval[i - 1] - m_qval[i]) / (m_npos[i - 1] - m_npos[i]);
            --m_npos[i];
        }
        else if (ni_expect - ni >= 1.0 && m_npos[i + 1] - m_npos[i] > 1)
        {
            m_qval[i] += (m_qval[i + 1] - m_qval[i]) / (m_npos[i + 1] - m_npos[i]);
            ++m_npos[i];
        }
    }
}

void P2QuantileEstimator::reset()
{
    m_count = 0;
    std::fill(m_npos.begin(), m_npos.end(), 0);
    std::fill(m_qval.begin(), m_qval.end(), 0.0);
}

P2BoundingBoxFilter::P2BoundingBoxFilter()
{
    m_lowerEstimator.resize(BOX_DIMENSION, P2QuantileEstimator(m_lowerp));
    m_upperEstimator.resize(BOX_DIMENSION, P2QuantileEstimator(m_upperp));

    m_bboxMin.resize(BOX_DIMENSION, std::numeric_limits<double>::max());
    m_bboxMax.resize(BOX_DIMENSION, std::numeric_limits<double>::lowest());

    m_filterMin.resize(BOX_DIMENSION, std::numeric_limits<double>::lowest());
    m_filterMax.resize(BOX_DIMENSION, std::numeric_limits<double>::max());

    m_oriBboxMin.resize(BOX_DIMENSION, std::numeric_limits<double>::max());
    m_oriBboxMax.resize(BOX_DIMENSION, std::numeric_limits<double>::lowest());
}

void P2BoundingBoxFilter::process(double x, double y, double z)
{
    ++m_totalPoints;

    bool accepted = x >= m_filterMin[0] && x <= m_filterMax[0] && y >= m_filterMin[1] &&
                    y <= m_filterMax[1] && z >= m_filterMin[2] && z <= m_filterMax[2];

    std::vector<double> input{ x, y, z };

    for (int i = 0; i < BOX_DIMENSION; ++i)
    {
        m_oriBboxMin[i] = std::min(m_oriBboxMin[i], input[i]);
        m_oriBboxMax[i] = std::max(m_oriBboxMax[i], input[i]);
    }

    if (accepted)
    {
        ++m_acceptedPoints;

        for (int i = 0; i < BOX_DIMENSION; ++i)
        {
            m_lowerEstimator[i].update(input[i]);
            m_upperEstimator[i].update(input[i]);
        }

        if (m_totalPoints >= MIN_FILTER_POINTS * 2)
        {
            for (int i = 0; i < BOX_DIMENSION; ++i)
            {
                m_filterMin[i] = m_lowerEstimator[i].getQuantileValue();
                m_filterMax[i] = m_upperEstimator[i].getQuantileValue();
            }
        }
        else if (m_totalPoints >= MIN_FILTER_POINTS)
        {
            for (int i = 0; i < BOX_DIMENSION; ++i)
            {
                double data_range = m_bboxMax[i] - m_bboxMin[i];
                double factor = std::max(0.1, 1.0 - static_cast<double>(m_totalPoints) / (MIN_FILTER_POINTS * 2));
                double extension = data_range * factor;
                m_filterMin[i] = std::max(m_filterMin[i], m_lowerEstimator[i].getQuantileValue() - extension);
                m_filterMax[i] = std::min(m_filterMax[i], m_upperEstimator[i].getQuantileValue() + extension);
            }
        }

        for (int i = 0; i < BOX_DIMENSION; ++i)
        {
            m_bboxMin[i] = std::min(m_bboxMin[i], input[i]);
            m_bboxMax[i] = std::max(m_bboxMax[i], input[i]);
        }
    }
    else
    {
        ++m_filteredPoints;
    }
}

void P2BoundingBoxFilter::reset()
{
    m_totalPoints = 0;
    m_filteredPoints = 0;
    m_acceptedPoints = 0;

    for (auto& estimator : m_lowerEstimator)
    {
        estimator.reset();
    }
    for (auto& estimator : m_upperEstimator)
    {
        estimator.reset();
    }

    std::fill(m_bboxMin.begin(), m_bboxMin.end(), std::numeric_limits<double>::max());
    std::fill(m_bboxMax.begin(), m_bboxMax.end(), std::numeric_limits<double>::lowest());
    std::fill(m_filterMin.begin(), m_filterMin.end(), std::numeric_limits<double>::lowest());
    std::fill(m_filterMax.begin(), m_filterMax.end(), std::numeric_limits<double>::max());
    std::fill(m_oriBboxMin.begin(), m_oriBboxMin.end(), std::numeric_limits<double>::max());
    std::fill(m_oriBboxMax.begin(), m_oriBboxMax.end(), std::numeric_limits<double>::lowest());
}