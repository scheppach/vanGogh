#pragma once

#include "PluginProcessor.h"
#include "CurveEditor.h"

class VanGoghAudioProcessorEditor final
    : public juce::AudioProcessorEditor, public juce::ChangeListener
{
public:
    explicit VanGoghAudioProcessorEditor(
        VanGoghAudioProcessor&);
    ~VanGoghAudioProcessorEditor() override;


    void paint(juce::Graphics&) override;
    void resized() override;
    void changeListenerCallback(juce::ChangeBroadcaster* source) override;

private:
    // Plugin Audio Processor
    VanGoghAudioProcessor& processor;

    // Curve Editor for drawing points and curves
    CurveEditor curveEditor;


    // Declare GUI elements
    juce::ToggleButton dcFilterButton;
    juce::Slider inputGainKnob;
    juce::Slider outputGainKnob;
    juce::TextButton symmetrizeButton;
    juce::TextButton resetButton;
    juce::TextButton randomButton;
    juce::TextButton smoothRandomButton;
    juce::TextButton Save;
    juce::TextButton Load;

    // Declare juce attachments
    juce::AudioProcessorValueTreeState::ButtonAttachment
        dcFilterAttachment;

    juce::AudioProcessorValueTreeState::SliderAttachment
        inputGainAttachment;

    juce::AudioProcessorValueTreeState::SliderAttachment
        outputGainAttachment;

    void SaveDialog();
    void LoadDialog();

    std::unique_ptr<juce::FileChooser> fileChooser;


    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VanGoghAudioProcessorEditor)
};
