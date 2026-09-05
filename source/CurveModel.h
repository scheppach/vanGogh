#pragma once
#include <vector>
#include <cstddef>
#include <cmath>

struct CurvePoint
{
    float x;
    float y;
};

// Symmetrize, keep the positive part of the curve
inline std::vector<CurvePoint> symmetrizedPositiveHalf(
    const std::vector<CurvePoint>& source)
{
    std::vector<CurvePoint> output;
    output.reserve(source.size());

    // Find first index whose point is on the right half plane
    std::size_t first_positive_index = 0;

    while (first_positive_index < source.size() && source[first_positive_index].x < 0.0f) first_positive_index++;

    std::vector<CurvePoint> positive_points(source.begin() + first_positive_index, source.end());
    
    // If input is malformed or has two values at x = 0, return source
    if (positive_points.empty()) return source;
    if (positive_points.size() > 1
        && positive_points[0].x == 0.0f
        && positive_points[1].x == 0.0f)
    {
        return source;
    }

    for (std::size_t index = 0; index < positive_points.size(); index++)
    {
        output.push_back({ -positive_points[positive_points.size() - 1 - index].x,  -positive_points[positive_points.size() - 1 -index].y });
    }

    if (positive_points[0].x != 0 || positive_points[0].y != 0) output.push_back(positive_points[0]);

    for (std::size_t index = 1; index < positive_points.size(); index++)
    {
        output.push_back(positive_points[index]);
    }
    return output;
}

// Helper to check whether loaded curves are well formed
// Helper to check whether curve points is well formed
inline bool isCurveValid(const std::vector<CurvePoint>& points)
{
    if (points.size() < 2)
        return false;

    constexpr float tolerance = 0.000001f;

    if (std::abs(points.front().x + 1.0f) > tolerance)
        return false;

    if (std::abs(points.back().x - 1.0f) > tolerance)
        return false;

    for (std::size_t index = 0; index < points.size(); ++index)
    {
        const auto& point = points[index];

        if (!std::isfinite(point.x) || !std::isfinite(point.y))
            return false;

        if (point.x < -1.0f || point.x > 1.0f
            || point.y < -1.0f || point.y > 1.0f)
        {
            return false;
        }

        if (index > 0
            && point.x <= points[index - 1].x)
        {
            return false;
        }
    }

    return true;
}

// Magic number
inline constexpr std::size_t number_generated_points = 265;