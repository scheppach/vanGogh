#pragma once

#include <JuceHeader.h>
#include <array>
#include <vector>
#include "CurveModel.h"


class VanGoghAudioProcessor final : public juce::AudioProcessor, public juce::ChangeBroadcaster
{
public:
    VanGoghAudioProcessor();

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;
    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    void getStateInformation (juce::MemoryBlock& destinationData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState& getParameters();

    std::vector<CurvePoint> getCurvePoints() const;

    // UI interface
    bool submitCurvePoints(const std::vector<CurvePoint>& points);
    static std::vector<CurvePoint> createInitialCurvePoints();


    //Save/Load workflow functions
    juce::ValueTree createState();
    bool applyState(const juce::ValueTree& state);

    bool saveToFile(const juce::File& file);
    bool loadFromFile(const juce::File& file);
    
private:
    //Magic numbers
    // Number of entries in the lookup table for the response curve.
    static constexpr std::size_t curveLookupTableSize = 1025;
    // Knee initialization
    static constexpr float knee = 0.5f;


    // Declare Automatable Parameters
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    juce::AudioProcessorValueTreeState parameters;


    // Declare Oversampling
    juce::dsp::Oversampling<float> oversampling{
      2,
      2,
      juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR,
      true,
      true
    };

    //Declare DC Filter
    using DCFilter = juce::dsp::ProcessorDuplicator<
        juce::dsp::IIR::Filter<float>,
        juce::dsp::IIR::Coefficients<float>>;

    DCFilter dcFilter;


    // Helper to store curvepoints in parameters
    void storeCurvePoints(const std::vector<CurvePoint>& points);
    
    

    using CurveLookupTable = std::array<float, curveLookupTableSize>;

    // The audio thread alone reads this active table.
    CurveLookupTable curveLookupTable;

    // UI-built tables wait here until the audio thread adopts one.
    static constexpr int curveUpdateQueueSize = 8;
    std::array<CurveLookupTable, curveUpdateQueueSize> queuedCurveLookupTables;
    juce::AbstractFifo curveUpdateFifo{ curveUpdateQueueSize };

    // Helper for Lookup Table Handoff
    void adoptLatestCurveLookupTable() noexcept;


    // Helper that builds LookupTable and writes it in destination
    static void buildCurveLookupTable(
        const std::vector<CurvePoint>& points,
        CurveLookupTable& destination);

    // Reader function that reads the curve
    float lookupCurve(float input) const noexcept;

    
    bool queueCurveLookupTable(const std::vector<CurvePoint>& curve);


    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VanGoghAudioProcessor)
};
