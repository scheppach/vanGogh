#include "CurveEditor.h"
#include <algorithm>
#include "CurveModel.h"
#include <limits>
#include <random>


// Constructor
CurveEditor::CurveEditor(VanGoghAudioProcessor& processorIn)
    : processor(processorIn),
    points(processor.getCurvePoints())
{
    setWantsKeyboardFocus(true);
    setMouseClickGrabsKeyboardFocus(true);
}


// Paint function
void CurveEditor::paint(juce::Graphics& graphics)
{
    // Fill the background
    graphics.fillAll(juce::Colours::black);

    // get bounds
    const auto full_bounds = getLocalBounds().toFloat();

    //sets 20 pixels margin
    const auto bounds = full_bounds.reduced(20.0f);

    // Draw visual bounds
    graphics.setColour(juce::Colours::grey);
    graphics.drawRect(bounds, axes_thickness);


    

    // Draw horizontal axis
    const float x_left = bounds.getX();
    const float y_centre = bounds.getCentreY();
    const float x_right = x_left + bounds.getWidth();
    
    graphics.drawLine(x_left, y_centre, x_right, y_centre, axes_thickness);

    // Draw vertical axis
    const float y_high = bounds.getY();
    const float y_low = y_high + bounds.getHeight();
    const float x_centre = bounds.getCentreX();

    graphics.drawLine(x_centre, y_low, x_centre, y_high, axes_thickness);

    // Draw axes labels, Rectangle order: x, y, width height

    graphics.drawText("In", juce::Rectangle<float>(x_right - 3.0f * axes_label_width, y_centre, 3.0f * axes_label_width, axes_label_width),
        juce::Justification::centred);
    graphics.drawText("Out", juce::Rectangle<float>(x_centre, y_high, 3.0f * axes_label_width, axes_label_width),
        juce::Justification::centred);


    // Draw curve and points
    if (points.size() > 1)
    {
        juce::Path curvePath;

        // Loop 1: build the curve path
        for (std::size_t index = 0; index + 1 < points.size(); ++index)
        {
            const CurvePoint first =
                convert_to_screen_coords(points[index], bounds);

            const CurvePoint second =
                convert_to_screen_coords(points[index + 1], bounds);

            if (index == 0)
                curvePath.startNewSubPath(first.x, first.y);

            curvePath.lineTo(second.x, second.y);
        }

        // Draw Curve
        graphics.setColour(juce::Colours::white);
        graphics.strokePath(
            curvePath,
            juce::PathStrokeType(line_thickness));


        // Loop 2: Draw points
        for (std::size_t index = 0; index < points.size(); ++index)
        {
            const CurvePoint screenPoint =
                convert_to_screen_coords(points[index], bounds);

            const bool isSelected =
                std::find(
                    selected_points.begin(),
                    selected_points.end(),
                    index) != selected_points.end();

            const float radius =
                isSelected ? 1.25f * point_radius : point_radius;

            graphics.setColour(
                isSelected ? juce::Colours::red
                : juce::Colours::darkgrey);

            graphics.fillEllipse(
                screenPoint.x - radius,
                screenPoint.y - radius,
                radius * 2.0f,
                radius * 2.0f);
        }
        // Draw last point
        const std::size_t last_index = points.size() - 1;
        const bool isSelected =
            std::find(
                selected_points.begin(),
                selected_points.end(),
                last_index) != selected_points.end();

        const float radius =
            isSelected ? 1.25f * point_radius : point_radius;

        graphics.setColour(
            isSelected ? juce::Colours::red
            : juce::Colours::darkgrey);

        const CurvePoint last_screen_point = convert_to_screen_coords(points[last_index], bounds);
        graphics.fillEllipse(
            last_screen_point.x - radius,
            last_screen_point.y - radius,
            radius * 2.0f,
            radius * 2.0f);
    }
}

// setCurvePoints from processor side. The input is trusted
void CurveEditor::setCurvePoints(std::vector<CurvePoint> curve)
{
    points = std::move(curve);
    selected_points.clear();
    drag_mode = DragMode::none;

    repaint();
}


// Mouse Click functions
void CurveEditor::mouseDown(const juce::MouseEvent& event)
{
    const auto bounds = getLocalBounds().toFloat().reduced(20.0f);
    if (!bounds.expanded(click_radius).contains(event.position))
        return;

    const CurvePoint eventScreenPosition =
        convert_to_CurvePoint(event.position);

    std::vector<std::size_t> pointsInRange;
    std::vector<float> distances;

    for (std::size_t index = 0; index < points.size(); ++index)
    {
        const float distance =
            distance_sq(
                eventScreenPosition,
                convert_to_screen_coords(points[index], bounds));

        if (distance < click_radius * click_radius)
        {
            distances.push_back(distance);
            pointsInRange.push_back(index);
        }
    }

    if (!distances.empty())
    {
        const auto minimumIterator =
            std::min_element(distances.begin(), distances.end());
        const std::size_t minimumIndex =
            static_cast<std::size_t>(
                std::distance(distances.begin(), minimumIterator));
        const std::size_t pointIndex = pointsInRange[minimumIndex];

        if (event.mods.isRightButtonDown())
        {
            if (pointIndex != 0 && pointIndex != points.size() - 1)
            {
                points.erase(points.begin() + pointIndex);
                selected_points.clear();
                drag_mode = DragMode::none;
                processor.submitCurvePoints(points);
                repaint();
            }

            return;
        }

        // Check whether selected point is already selected
        const bool pointIsAlreadySelected =
            std::find(
                selected_points.begin(),
                selected_points.end(),
                pointIndex) != selected_points.end();

        if (!pointIsAlreadySelected)
        {
            selected_points.clear();
            selected_points.push_back(pointIndex);
        }

        drag_mode = DragMode::movingPoints;
        drag_start_position = event.position;
        points_at_drag_start = points;
        repaint();
        return;
    }

    if (event.mods.isRightButtonDown())
    {
        drag_mode = DragMode::none;
        return;
    }

    if (distance_to_curve_sq(eventScreenPosition, bounds)
        < curve_click_radius * curve_click_radius)
    {
        const CurvePoint eventAudioPosition =
            convert_to_audio_coords(eventScreenPosition, bounds);
        point_insertion(eventAudioPosition);
        drag_mode = DragMode::movingPoints;
        drag_start_position = event.position;
        points_at_drag_start = points;
        return;
    }

    drag_start_position = event.position;
    points_at_drag_start = points;
    drag_mode = DragMode::selectingRectangle;
}
void CurveEditor::mouseDrag(const juce::MouseEvent& event)
{
    const auto bounds = getLocalBounds().toFloat().reduced(20.0f);

    switch (drag_mode)
    {
    case DragMode::movingPoints:
    {
        const juce::Point<float> screenOffset{
            event.position.x - drag_start_position.x,
            event.position.y - drag_start_position.y};

        const CurvePoint audioOffset{
            2.0f * screenOffset.x / bounds.getWidth(),
            -2.0f * screenOffset.y / bounds.getHeight()};

        if (selected_points.empty()
            || points_at_drag_start.size() != points.size())
            break;

        const std::size_t leftmostSelected =
            *std::min_element(selected_points.begin(), selected_points.end());
        const std::size_t rightmostSelected =
            *std::max_element(selected_points.begin(), selected_points.end());

        const bool endpointIsSelected =
            leftmostSelected == 0
            || rightmostSelected == points.size() - 1;

        if (!endpointIsSelected)
        {
            float minimumOffsetX = -1.0f;
            float maximumOffsetX = 1.0f;

            if (leftmostSelected > 0)
            {
                minimumOffsetX =
                    points_at_drag_start[leftmostSelected - 1].x
                    - points_at_drag_start[leftmostSelected].x;
            }

            if (rightmostSelected + 1 < points.size())
            {
                maximumOffsetX =
                    points_at_drag_start[rightmostSelected + 1].x
                    - points_at_drag_start[rightmostSelected].x;
            }

            const float allowedOffsetX = juce::jlimit(
                minimumOffsetX,
                maximumOffsetX,
                audioOffset.x);

            move_selected_points_x({ allowedOffsetX, 0.0f });
        }

        move_selected_points_y(audioOffset);
        processor.submitCurvePoints(points);
        repaint();
        break;
    }

    case DragMode::selectingRectangle:
    {
        selected_points.clear();

        const CurvePoint point1 =
            convert_to_audio_coords(
                convert_to_CurvePoint(drag_start_position), bounds);
        const CurvePoint point2 =
            convert_to_audio_coords(
                convert_to_CurvePoint(event.position), bounds);

        for (std::size_t index = 0; index < points.size(); ++index)
        {
            const bool isWithinX =
                points[index].x >= std::min(point1.x, point2.x)
                && points[index].x <= std::max(point1.x, point2.x);
            const bool isWithinY =
                points[index].y >= std::min(point1.y, point2.y)
                && points[index].y <= std::max(point1.y, point2.y);

            if (isWithinX && isWithinY)
                selected_points.push_back(index);
        }

        repaint();
        break;
    }

    case DragMode::none:
        break;
    }
}
void CurveEditor::mouseUp(const juce::MouseEvent& event)
{
    juce::ignoreUnused(event);

    // reset temporary copy
    drag_mode = DragMode::none;
    points_at_drag_start.clear();
}

bool CurveEditor::keyPressed(const juce::KeyPress& key)
{
    // Delete any selected keys
    if (key.getKeyCode() == juce::KeyPress::deleteKey)
    {
        std::sort(selected_points.rbegin(), selected_points.rend());

        for (const std::size_t index : selected_points)
        {
            if (index != 0 && index != points.size() - 1)
                points.erase(points.begin() + index);
        }

        selected_points.clear();
        drag_mode = DragMode::none;

        processor.submitCurvePoints(points);
        repaint();

        return true;
    }
    // Do nothing if any other key is pressed
    return false;
}



// Reset points to a linear curve spanned by two points at (-1,-1) and (+1, +1)
void CurveEditor::reset_to_linear()
{
    points.clear();
    selected_points.clear();
    points.push_back({ -1.0f, -1.0f });
    points.push_back({ 1.0f , 1.0f });
    processor.submitCurvePoints(points);
    repaint();
}

// Symmetrize, keep the positive part of the curve
void CurveEditor::symmetrize()
{
    points = symmetrizedPositiveHalf(points);

    // Clear up selection and resubmit
    selected_points.clear();
    processor.submitCurvePoints(points);
    repaint();
}


// Produce random curve
void CurveEditor::generate_random()
{
    points.clear();
    points.reserve(number_generated_points);

    juce::Random random;
   
    for (std::size_t index = 0; index < number_generated_points; index++)
    {
        const float x = (static_cast<float>(index) / static_cast<float>(number_generated_points - 1) - 0.5f) * 2.0f;
        const float y = random.nextFloat() * 2.0f - 1.0f;
        points.push_back({ x, y });
    }

    selected_points.clear();
    processor.submitCurvePoints(points);
    repaint();
}

// Generate smooth random
void CurveEditor::generate_smooth_random()
{
    points.clear();
    points.reserve(number_generated_points);

    std::mt19937 generator(std::random_device{}());
    std::normal_distribution<float> distribution(0.0f, std_dev);

    float previousY = -1.0f;

    // Generates a new point with y = previousY + linear rise + random drawn from a gaussian
    for (std::size_t index = 0;
        index < number_generated_points;
        ++index)
    {
        const float x =juce::jmap(static_cast<float>(index),0.0f, static_cast<float>(number_generated_points - 1), -1.0f, 1.0f);

        if (index > 0)
        {
            const float linearRise = 2.0f / static_cast<float>(number_generated_points - 1);

            previousY = juce::jlimit(-1.0f, 1.0f, previousY + linearRise + distribution(generator));
        }
        points.push_back({ x, previousY });
    }

    selected_points.clear();
    processor.submitCurvePoints(points);
    repaint();
}



// Converts from -1, +1 audio range to screen coordinates of the Rectangle object
CurvePoint CurveEditor::convert_to_screen_coords(CurvePoint point, juce::Rectangle<float> bounds)
{
    const float x_norm = point.x;
    const float y_norm = point.y;

    const float x_screen = bounds.getWidth() * 0.5f * (x_norm + 1.0f) + bounds.getX();
    const float y_screen = bounds.getHeight() * -0.5f * (y_norm - 1.0f) + bounds.getY();

    CurvePoint screen_point{ x_screen, y_screen };

    return screen_point;
}

// Converts from screen coordinates of the Rectangle object to -1, +1 audio coordinates
CurvePoint CurveEditor::convert_to_audio_coords(CurvePoint point, juce::Rectangle<float> bounds)
{
    const float x_screen = point.x;
    const float y_screen = point.y;

    const float x_norm = 2.0f * (x_screen - bounds.getX()) / bounds.getWidth() - 1.0f;
    const float y_norm = -2.0f * (y_screen - bounds.getY()) / bounds.getHeight() + 1.0f;

    CurvePoint screen_point{ x_norm, y_norm };

    return screen_point;
}

//Converts juce::Point<float> to CurvePoint
CurvePoint CurveEditor::convert_to_CurvePoint(juce::Point<float> point)
{
    CurvePoint curve_point{ point.x, point.y };
    return curve_point;
}

// Distance helper
float CurveEditor::distance_sq(CurvePoint p1, CurvePoint p2)
{
    return (p1.x - p2.x) * (p1.x - p2.x) + (p1.y - p2.y) * (p1.y - p2.y);
}


// Inserts point at correct index, point is expected to be in normalised coords
void CurveEditor::point_insertion(CurvePoint newPoint)
{
    if (points.size() < 2
        || newPoint.x <= points.front().x
        || newPoint.x >= points.back().x)
    {
        return;
    }

    std::size_t index = 1;

    while (index < points.size()
        && points[index].x < newPoint.x)
    {
        ++index;
    }

    if (newPoint.x <= points[index - 1].x
        || newPoint.x >= points[index].x)
    {
        return;
    }

    // Insert new point on the current curve at the click x value
    const CurvePoint left = points[index - 1];
    const CurvePoint right = points[index];
    const CurvePoint point_insert{newPoint.x , left.y + (newPoint.x - left.x) / (right.x - left.x) * (right.y - left.y)};

    points.insert(points.begin() + index, point_insert);
    
    selected_points.clear();
    selected_points.push_back(index);
    processor.submitCurvePoints(points);
    repaint();
}


// Moves points in x and y direction respecively
void CurveEditor::move_selected_points_x(CurvePoint offset)
{
    const float offset_x = offset.x;

    for (const std::size_t index : selected_points)
    {
        if (points_at_drag_start[index].x + offset_x > 1.0f) points[index].x = 1.0f;
        else if (points_at_drag_start[index].x + offset_x < -1.0f) points[index].x = -1.0f;
        else points[index].x = points_at_drag_start[index].x + offset.x;
    }
}

void CurveEditor::move_selected_points_y(CurvePoint offset)
{
    const float offset_y = offset.y;

    for (const std::size_t index : selected_points)
    {
        if (points_at_drag_start[index].y + offset_y > 1.0f) points[index].y = 1.0f;
        else if (points_at_drag_start[index].y + offset_y < -1.0f) points[index].y = -1.0f;
        else points[index].y = points_at_drag_start[index].y + offset.y;
    }
}

void CurveEditor::move_selected_points_offset_x(float offsetX)
{
    for (const std::size_t index : selected_points)
    {
        points[index].x = juce::jlimit(
            -1.0f,
            1.0f,
            points_at_drag_start[index].x + offsetX);
    }
}


float CurveEditor::distance_to_curve_sq(
    CurvePoint mousePosition,
    juce::Rectangle<float> bounds)
{
    const juce::Point<float> mouseScreenPosition(
        mousePosition.x,
        mousePosition.y);

    float minimumDistanceSq =
        std::numeric_limits<float>::max();

    for (std::size_t index = 0;
        index + 1 < points.size();
        ++index)
    {
        const CurvePoint first =
            convert_to_screen_coords(points[index], bounds);

        const CurvePoint second =
            convert_to_screen_coords(points[index + 1], bounds);

        const juce::Line<float> segment(
            juce::Point<float>(first.x, first.y),
            juce::Point<float>(second.x, second.y));

        juce::Point<float> nearestPoint;

        const float distance =
            segment.getDistanceFromPoint(
                mouseScreenPosition,
                nearestPoint);

        minimumDistanceSq =
            std::min(minimumDistanceSq, distance * distance);
    }

    return minimumDistanceSq;
}
