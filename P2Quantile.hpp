#ifndef _P2_QUANTILE_H_
#define _P2_QUANTILE_H_
#include <array>
#include <cstdint>
#include <limits>
#include <vector>

class P2QuantileEstimator
{
public:
    P2QuantileEstimator(double quantile = 0.5);
    void update(double x);

    inline double getQuantile() const { return m_p; }
    inline double getQuantileValue() const { return m_count >= P2_NUM_POINTS ? m_qval[2] : std::numeric_limits<double>::quiet_NaN(); }
    inline std::array<double, 5> getMarkers() const { return m_qval; }
    inline uint32_t getCount() const { return m_count; }
    void reset();
private:
    static constexpr uint32_t P2_NUM_POINTS = 5;

    uint32_t m_count{ 0 };
    std::array<uint32_t, P2_NUM_POINTS> m_npos;
    double m_p{ 0.5 };
    std::array<double, P2_NUM_POINTS> m_qval;
    std::array<double, P2_NUM_POINTS> m_dn;
};

class P2BoundingBoxFilter
{
public:
    P2BoundingBoxFilter();

    void process(double x, double y, double z);

    inline std::vector<double> getBboxMin() const { return m_bboxMin; }
    inline std::vector<double> getBboxMax() const { return m_bboxMax; }
    inline std::vector<double> getFilterMin() const { return m_filterMin; }
    inline std::vector<double> getFilterMax() const { return m_filterMax; }
    inline std::vector<double> getOriginBboxMin() const { return m_oriBboxMin; }
    inline std::vector<double> getOriginBboxMax() const { return m_oriBboxMax; }
    void reset();

private:
    static constexpr uint32_t BOX_DIMENSION = 3;
    static constexpr uint32_t MIN_FILTER_POINTS = 100;

    double m_lowerp{ 0.05 };
    double m_upperp{ 0.95 };
    uint32_t m_totalPoints{ 0 };
    uint32_t m_filteredPoints{ 0 };
    uint32_t m_acceptedPoints{ 0 };
    std::vector<P2QuantileEstimator> m_lowerEstimator;
    std::vector<P2QuantileEstimator> m_upperEstimator;
    std::vector<double> m_bboxMin;
    std::vector<double> m_bboxMax;
    std::vector<double> m_filterMin;
    std::vector<double> m_filterMax;
    std::vector<double> m_oriBboxMin;
    std::vector<double> m_oriBboxMax;
};
#endif