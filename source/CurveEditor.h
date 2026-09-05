#pragma once

#include "PluginProcessor.h"
#include <optional>
#include "CurveModel.h"

enum class DragMode
{
    none,
    movingPoints,
    selectingRectangle
};

class CurveEditor final : public juce::Component
{
public:
    explicit CurveEditor(VanGoghAudioProcessor& processor);

    void paint(juce::Graphics& graphics) override;
    void mouseDown(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;
    void mouseUp(const juce::MouseEvent& event) override;
    bool keyPressed(const juce::KeyPress& key) override;

    // Utility methods
    void reset_to_linear();
    void symmetrize();
    void generate_random();
    void generate_smooth_random();

    void setCurvePoints(std::vector<CurvePoint> curve);

private:
    VanGoghAudioProcessor& processor;

    // In normalized audio coordinates
    std::vector<CurvePoint> points;

    // Indices of selected points
    std::vector<std::size_t> selected_points;


    // Helper conversion functions
    CurvePoint convert_to_screen_coords(CurvePoint point, juce::Rectangle<float> bounds);
    CurvePoint convert_to_audio_coords(CurvePoint point, juce::Rectangle<float> bounds);

    // Helper function to convert juce::Point<float> to CurvePoint
    CurvePoint convert_to_CurvePoint(juce::Point<float>);

    // Distance helper
    float distance_sq(CurvePoint p1, CurvePoint p2);
    float distance_to_curve_sq(
        CurvePoint mousePosition,
        juce::Rectangle<float> bounds);

    // Point insertion helper
    void point_insertion(CurvePoint point);

    // Unprotected Mover helper, expects audio curve points
    void move_selected_points_x(CurvePoint offset);
    void move_selected_points_y(CurvePoint offset);
    void move_selected_points_offset_x(float epsilon);


    // Mouse drag behaviour
    DragMode drag_mode = DragMode::none;
    juce::Point<float> drag_start_position;
    std::vector<CurvePoint> points_at_drag_start;


    // Magic numbers
    const float axes_thickness = 2.0f;
    const float axes_label_width = 20.0f;
    const float line_thickness = 2.0f;
    const float point_radius = 3.0f;
    const float click_radius = 9.0f;
    const float curve_click_radius = 6.0f;
    const float epsilon = 0.0001f;
    const float std_dev = 0.02f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CurveEditor);
};
