#include "PluginEditor.h"

VanGoghAudioProcessorEditor::
VanGoghAudioProcessorEditor(VanGoghAudioProcessor& p)
    : AudioProcessorEditor(&p),
    processor(p),
    curveEditor(p),
    dcFilterAttachment(
        processor.getParameters(),
        "dcFilterEnabled",
        dcFilterButton),
    inputGainAttachment(
        processor.getParameters(),
        "inputGain",
        inputGainKnob),
    outputGainAttachment(
        processor.getParameters(),
        "outputGain",
        outputGainKnob) 
{
    processor.addChangeListener(this);

    addAndMakeVisible(curveEditor);

    // DC Filter button
    dcFilterButton.setButtonText("DC Filter");
    addAndMakeVisible(dcFilterButton);

    // Input Gain Knob
    inputGainKnob.setSliderStyle(
        juce::Slider::RotaryHorizontalVerticalDrag);
    inputGainKnob.setTextBoxStyle(
        juce::Slider::TextBoxBelow,
        false,
        80,
        20);

    inputGainKnob.setRotaryParameters(
        juce::MathConstants<float>::pi * 1.25f,
        juce::MathConstants<float>::pi * 2.75f,
        true);
    addAndMakeVisible(inputGainKnob);

    // Output Gain Knob
    outputGainKnob.setSliderStyle(
        juce::Slider::RotaryHorizontalVerticalDrag);
    outputGainKnob.setTextBoxStyle(
        juce::Slider::TextBoxBelow,
        false,
        80,
        20);

    outputGainKnob.setRotaryParameters(
        juce::MathConstants<float>::pi * 1.25f,
        juce::MathConstants<float>::pi * 2.75f,
        true);
    addAndMakeVisible(outputGainKnob);


    // Symmetrize button
    symmetrizeButton.setButtonText("Symmetrize");
    addAndMakeVisible(symmetrizeButton);
    symmetrizeButton.onClick = [this]
        {
            curveEditor.symmetrize();
        };

    // Reset Button
    resetButton.setButtonText("Reset");
    addAndMakeVisible(resetButton);
    resetButton.onClick = [this]
        {
            curveEditor.reset_to_linear();
        };

    // Randomize Button
    randomButton.setButtonText("Random");
    addAndMakeVisible(randomButton);
    randomButton.onClick = [this]
        {
            curveEditor.generate_random();
        };

    // Smooth Randomize Button
    smoothRandomButton.setButtonText("Smooth Random");
    addAndMakeVisible(smoothRandomButton);
    smoothRandomButton.onClick = [this]
        {
            curveEditor.generate_smooth_random();
        };


    // Save and Load Buttons
    Save.setButtonText("Save");
    addAndMakeVisible(Save);
    Save.onClick = [this]
        {
            SaveDialog();
        };

    Load.setButtonText("Load");
    addAndMakeVisible(Load);
    Load.onClick = [this]
        {
            LoadDialog();
        };


    setSize (900, 650);
    setResizable (false, false);
}

// Destructor
VanGoghAudioProcessorEditor::~VanGoghAudioProcessorEditor()
{
    processor.removeChangeListener(this);
}


// Gives CurveEditor Curve points that are set during construction of the processor
void VanGoghAudioProcessorEditor::changeListenerCallback(
    juce::ChangeBroadcaster* source)
{
    if (source == &processor)
    {
        curveEditor.setCurvePoints(
            processor.getCurvePoints());
    }
}

// Override Resized
void VanGoghAudioProcessorEditor::resized()
{
    // Curve Editor
    curveEditor.setBounds(
        50, 75,
        getWidth() - 100,
        getHeight() - 200);

    // Curve Editor buttons
    // Leave the upper-left corner clear for the title.
    Save.setBounds(170, 20, 70, 35);
    Load.setBounds(250, 20, 70, 35);
    randomButton.setBounds(330, 20, 100, 35);
    smoothRandomButton.setBounds(440, 20, 130, 35);
    resetButton.setBounds(580, 20, 100, 35);
    symmetrizeButton.setBounds(690, 20, 130, 35);

    // Audio Controls
    const int controlY = getHeight() - 125;
    const int controlSize = 110;

    inputGainKnob.setBounds(
        100, controlY, controlSize, controlSize);

    dcFilterButton.setBounds(
        395, controlY + 35, 110, 40);

    outputGainKnob.setBounds(
        690, controlY, controlSize, controlSize);
}

void VanGoghAudioProcessorEditor::paint (juce::Graphics& graphics)
{
    graphics.fillAll (juce::Colours::black);
    graphics.setColour (juce::Colours::whitesmoke);
    graphics.setFont(24.0f);

    auto titleBounds = getLocalBounds()
        .removeFromTop(50)
        .withTrimmedLeft(10)
        .withWidth(145);
    graphics.drawFittedText(
        "Van Gogh", titleBounds,
        juce::Justification::centredLeft, 1);
}

void VanGoghAudioProcessorEditor::SaveDialog()
{
    fileChooser = std::make_unique<juce::FileChooser>(
        "Save Van Gogh preset",
        juce::File{},
        "*.drawdist");

    const auto chooserFlags =
        juce::FileBrowserComponent::saveMode
        | juce::FileBrowserComponent::canSelectFiles
        | juce::FileBrowserComponent::warnAboutOverwriting;

    fileChooser->launchAsync(
        chooserFlags,
        [this](const juce::FileChooser& chooser)
        {
            const auto selectedFile = chooser.getResult();

            if (selectedFile != juce::File{})
            {
                const auto presetFile =
                    selectedFile.withFileExtension("drawdist");

                if (!processor.saveToFile(presetFile))
                {
                    juce::AlertWindow::showMessageBoxAsync(
                        juce::MessageBoxIconType::WarningIcon,
                        "Save failed",
                        "The preset could not be written to the selected file.");
                }
            }

            fileChooser.reset();
        });
}

void VanGoghAudioProcessorEditor::LoadDialog()
{
    fileChooser = std::make_unique<juce::FileChooser>(
        "Load Van Gogh preset",
        juce::File{},
        "*.drawdist");

    const auto chooserFlags =
        juce::FileBrowserComponent::openMode
        | juce::FileBrowserComponent::canSelectFiles;

    fileChooser->launchAsync(
        chooserFlags,
        [this](const juce::FileChooser& chooser)
        {
            const auto selectedFile = chooser.getResult();

            if (selectedFile != juce::File{}
                && !processor.loadFromFile(selectedFile))
            {
                juce::AlertWindow::showMessageBoxAsync(
                    juce::MessageBoxIconType::WarningIcon,
                    "Load failed",
                    "The selected file is not a valid Van Gogh preset.");
            }

            fileChooser.reset();
        });
}
