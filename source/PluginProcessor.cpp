#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "CurveModel.h"
#include <cmath>



// Constructor
VanGoghAudioProcessor::VanGoghAudioProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput ("Input", juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)), 
    parameters(*this, nullptr, "Parameters", createParameterLayout())
{
    const auto initialPoints = createInitialCurvePoints();

    storeCurvePoints(initialPoints);
    buildCurveLookupTable(initialPoints, curveLookupTable);
}


// Helper that checks the queue and writes any finished table in the private lookup table
void VanGoghAudioProcessor::adoptLatestCurveLookupTable() noexcept
{
    const auto numReady = curveUpdateFifo.getNumReady();

    if (numReady == 0)
        return;

    int startIndex1, size1, startIndex2, size2;

    curveUpdateFifo.prepareToRead(numReady, startIndex1, size1, startIndex2, size2);

    if (size2 > 0)
    {
        curveLookupTable = queuedCurveLookupTables[startIndex2 + size2 - 1];
    }
    else if (size1 > 0)
    {
        curveLookupTable = queuedCurveLookupTables[startIndex1 + size1 - 1];
    }
    curveUpdateFifo.finishedRead(size1 + size2);
}

// UI interface, we assume that the list is ordered by ascending x values, assume that the x,y values are in -1,+1
// We also assume that at least there are two points in the vector with x_left = -1 and x_right = +1

bool VanGoghAudioProcessor::submitCurvePoints(const std::vector<CurvePoint>& points)
{
    if (!isCurveValid(points))
        return false;

    // Store points in parameters
    storeCurvePoints(points);

    return queueCurveLookupTable(points);
}


// Helper function to build lookup tables from points and writes to destination
void VanGoghAudioProcessor::buildCurveLookupTable(
    const std::vector<CurvePoint>& points,
    VanGoghAudioProcessor::CurveLookupTable& destination)
{
    if (points.size() < 2)
    {
        return;
    }
    std::size_t point_index = 0;
    for (std::size_t lookup_index = 0; lookup_index < curveLookupTableSize; ++lookup_index)
    {
        // Calculate index x position
        const float x = (static_cast<float>(lookup_index) / static_cast<float>(curveLookupTableSize - 1) - 0.5f) * 2.0f;

        while (point_index + 1 < points.size() - 1
            && x > juce::jlimit(-1.0f, 1.0f, points[point_index + 1].x))
        {
            ++point_index;
        }
        // Clamp points
        const auto leftX = juce::jlimit(-1.0f, 1.0f, points[point_index].x);
        const auto leftY = juce::jlimit(-1.0f, 1.0f, points[point_index].y);
        const auto rightX = juce::jlimit(-1.0f, 1.0f, points[point_index + 1].x);
        const auto rightY = juce::jlimit(-1.0f, 1.0f, points[point_index + 1].y);
        
        if (leftX != rightX)
        {
            float slope = (rightY - leftY) / (rightX - leftX);
            destination[lookup_index] = leftY + slope * (x - leftX);
        }
        else
        {
            destination[lookup_index] = 0.5f * (leftY + rightY);
        }
    }
}

// Initialize Curve points as soft knee
std::vector<CurvePoint> VanGoghAudioProcessor::createInitialCurvePoints()
{
    std::size_t half_samples = (number_generated_points + 1) / 2;
    std::vector<CurvePoint> points;
    if (knee >= 1)
    {
        points.push_back({ -1, -1 });
        points.push_back({ 1, 1 });
        return points;
    }

    // Set point that defines linear part
    points.push_back({ knee, knee });

    for (std::size_t index = 1; index < half_samples; index++)
    {
        float x = knee
            + (1.0f - knee)
            * static_cast<float>(index)
            / static_cast<float>(half_samples - 1);
        float y;
        const auto normalised_x =
            (x - knee) / (1.0f - knee);
        y = knee
            + (1.0f - knee)
            * (normalised_x
                - 0.5f * normalised_x * normalised_x);
        points.push_back({ x, y });
    }
    points = symmetrizedPositiveHalf(points);
    return points;
}




// Lookup function
float VanGoghAudioProcessor::lookupCurve(float input) const noexcept
{
    // Clamp Input to +1 and -1
    const auto clampedInput = juce::jlimit(-1.0f, 1.0f, input);

    // Calculate index
    const auto tablePosition =
        (clampedInput + 1.0f) * 0.5f
        * static_cast<float> (curveLookupTableSize - 1);

    const auto lowerIndex =
        static_cast<std::size_t> (tablePosition);

    if (lowerIndex == curveLookupTableSize - 1)
        return curveLookupTable[lowerIndex];

    const auto upperIndex = lowerIndex + 1;
    const auto interpolationAmount =
        tablePosition - static_cast<float> (lowerIndex);

    return curveLookupTable[lowerIndex]
        + interpolationAmount
        * (curveLookupTable[upperIndex]
            - curveLookupTable[lowerIndex]);
}



// Parameter Layout
juce::AudioProcessorValueTreeState::ParameterLayout
VanGoghAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ "inputGain", 1 },
        "Input Gain",
        juce::NormalisableRange<float> { -24.0f, 24.0f, 0.01f },
        0.0f,
        juce::AudioParameterFloatAttributes().withLabel("dB")));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ "outputGain", 1 },
        "Output Gain",
        juce::NormalisableRange<float> { -99.0f, 6.0f, 0.01f },
        0.0f,
        juce::AudioParameterFloatAttributes().withLabel("dB")));

    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID{ "dcFilterEnabled", 1 },
        "DC Filter",
        true));

    //bypass the distortion, only for debugging
    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID{ "Bypass", 1 },
        "Bypass",
        false));

    return layout;
}

const juce::String VanGoghAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool VanGoghAudioProcessor::acceptsMidi() const { return false; }
bool VanGoghAudioProcessor::producesMidi() const { return false; }
bool VanGoghAudioProcessor::isMidiEffect() const { return false; }
double VanGoghAudioProcessor::getTailLengthSeconds() const { return 0.0; }

int VanGoghAudioProcessor::getNumPrograms() { return 1; }
int VanGoghAudioProcessor::getCurrentProgram() { return 0; }
void VanGoghAudioProcessor::setCurrentProgram (int) {}
const juce::String VanGoghAudioProcessor::getProgramName (int) { return {}; }
void VanGoghAudioProcessor::changeProgramName (int, const juce::String&) {}


// Prepare to Play function

void VanGoghAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    oversampling.reset();
    oversampling.initProcessing(
        static_cast<size_t> (samplesPerBlock));

    const auto oversamplingFactor =
        static_cast<double> (oversampling.getOversamplingFactor());

    const auto oversampledSampleRate = sampleRate * oversamplingFactor;

    *dcFilter.state = *juce::dsp::IIR::Coefficients<float>::makeHighPass(
        oversampledSampleRate, 20.0f);

    dcFilter.prepare({
        oversampledSampleRate,
        static_cast<juce::uint32> (samplesPerBlock * oversamplingFactor),
        static_cast<juce::uint32> (getTotalNumOutputChannels())
        });

    dcFilter.reset();

    setLatencySamples(
        static_cast<int> (oversampling.getLatencyInSamples()));
}

void VanGoghAudioProcessor::releaseResources()
{
    // No explicitly owned resources to release
}

bool VanGoghAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto input = layouts.getMainInputChannelSet();
    const auto output = layouts.getMainOutputChannelSet();

    return input == output && (output == juce::AudioChannelSet::mono()
                            || output == juce::AudioChannelSet::stereo());
}

void VanGoghAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    // Adopts latest written table from the queue
    adoptLatestCurveLookupTable();

    juce::ScopedNoDenormals noDenormals;

    for (auto channel = getTotalNumInputChannels(); channel < getTotalNumOutputChannels(); ++channel)
        buffer.clear (channel, 0, buffer.getNumSamples());

    const auto inputGain = juce::Decibels::decibelsToGain(
        parameters.getRawParameterValue("inputGain")->load());

    const auto outputGain = juce::Decibels::decibelsToGain(
        parameters.getRawParameterValue("outputGain")->load());


    //Input stage
    buffer.applyGain(inputGain);


    //Oversampling
    auto block = juce::dsp::AudioBlock<float>(buffer);
    auto oversampledBlock = oversampling.processSamplesUp(block);


    //Apply Non linear curve of the lookup
    const auto bypass =
        parameters.getRawParameterValue("Bypass")->load() > 0.5f;

    if (!bypass)
    {
        for (size_t channel = 0; channel < oversampledBlock.getNumChannels(); ++channel)
        {
            auto* channelData = oversampledBlock.getChannelPointer(channel);

            for (size_t sample = 0; sample < oversampledBlock.getNumSamples(); ++sample)
            {
                channelData[sample] = lookupCurve(channelData[sample]);
            }
        }
    }

    //DC filter
    const auto isDcFilterEnabled =
        parameters.getRawParameterValue("dcFilterEnabled")->load() > 0.5f;

    if (isDcFilterEnabled)
        dcFilter.process(
            juce::dsp::ProcessContextReplacing<float>(oversampledBlock));
    
    //Downsampling
    oversampling.processSamplesDown(block);


    //Output stage
    buffer.applyGain(outputGain);
}

// Helper: stores points as child in parameters
void VanGoghAudioProcessor::storeCurvePoints(const std::vector<CurvePoint>& points)
{
    auto curveState =
        parameters.state.getChildWithName("Curve");

    if (!curveState.isValid())
    {
        curveState = juce::ValueTree("Curve");
        parameters.state.addChild(curveState, -1, nullptr);
    }
    else
    {
        curveState.removeAllChildren(nullptr);
    }

    for (const auto& point : points)
    {
        juce::ValueTree pointState("Point");

        pointState.setProperty("x", point.x, nullptr);
        pointState.setProperty("y", point.y, nullptr);

        curveState.addChild(pointState, -1, nullptr);
    }
}


// Builds temporary curvePoint vector from canonical state in parameters, returns initial soft knee if the current parameter state is invalid
std::vector<CurvePoint> VanGoghAudioProcessor::getCurvePoints() const
{
    auto curveState =
        parameters.state.getChildWithName("Curve");
    const int numberOfPoints =
        curveState.getNumChildren();

    std::vector<CurvePoint> points;
    points.reserve(numberOfPoints);

    for (int index = 0; index < numberOfPoints; ++index)
    {
        const auto pointState = curveState.getChild(index);

        if (!pointState.hasType("Point"))
            continue;

        const float x =
            static_cast<float>(pointState.getProperty("x"));

        const float y =
            static_cast<float>(pointState.getProperty("y"));
        
        points.push_back({ x,y });
    }

    if (isCurveValid(points)) return points;
    else return createInitialCurvePoints();
}




bool VanGoghAudioProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor* VanGoghAudioProcessor::createEditor()
{
    return new VanGoghAudioProcessorEditor (*this);
}


// Used by DAW for state saving
void VanGoghAudioProcessor::getStateInformation(juce::MemoryBlock& destinationData)
{
    juce::ValueTree state = createState();

    if (const auto xml = state.createXml())
        copyXmlToBinary(*xml, destinationData);
}

// Used by DAW for state initialization after startup
void VanGoghAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    if (const auto xml = getXmlFromBinary(data, sizeInBytes))
    {
        const auto state = juce::ValueTree::fromXml(*xml);

        applyState(state);
    }
}

juce::ValueTree VanGoghAudioProcessor::createState()
{
    auto state = parameters.copyState();

    // Bypass is retained as a parameter for debugging, but is not part of
    // the user-facing state yet.
    state.getChildWithProperty("id", "Bypass")
        .setProperty("value", false, nullptr);

    return state;
}

// Validates state, changes curve, restores to initial knee if compromised, queues lookup table and sends message
bool VanGoghAudioProcessor::applyState(
    const juce::ValueTree& state)
{
    if (!state.isValid()
        || !state.hasType(parameters.state.getType()))
    {
        return false;
    }

    parameters.replaceState(state);

    // Ignore any previously saved or externally supplied bypass value.
    parameters.state.getChildWithProperty("id", "Bypass")
        .setProperty("value", false, nullptr);

    // Restores curve to initial knee if corrupted
    const auto restoredPoints = getCurvePoints();
    storeCurvePoints(restoredPoints);

    const bool queued =
        queueCurveLookupTable(restoredPoints);

    if (queued)
        sendChangeMessage();

    return queued;
}



bool VanGoghAudioProcessor::saveToFile(const juce::File& file)
{
    const juce::ValueTree tree = createState();

    auto root = juce::DynamicObject::Ptr(new juce::DynamicObject());

    root->setProperty("format", "VanGogh");
    root->setProperty("version", 1);

    root->setProperty(
        "inputGain",
        tree.getChildWithProperty(
            "id", "inputGain")
        .getProperty("value"));

    root->setProperty(
        "outputGain", tree.getChildWithProperty(
            "id", "outputGain")
        .getProperty("value"));

    const auto dcFilterValue =
        tree.getChildWithProperty(
            "id", "dcFilterEnabled")
        .getProperty("value");

    root->setProperty(
        "dcFilterEnabled",
        static_cast<double>(dcFilterValue) > 0.5);

    juce::Array<juce::var> curveArray;

    const auto curveState =
        tree.getChildWithName("Curve");

    for (int index = 0; index < curveState.getNumChildren(); ++index)
    {
        const auto pointState =
            curveState.getChild(index);

        if (!pointState.hasType("Point"))
            continue;

        auto pointObject = juce::DynamicObject::Ptr(new juce::DynamicObject());

        pointObject->setProperty(
            "x", pointState.getProperty("x"));

        pointObject->setProperty(
            "y", pointState.getProperty("y"));

        curveArray.add(juce::var(pointObject));
    }

    root->setProperty("curve", juce::var(curveArray));

    const juce::var json(root);
    const auto jsonText =
        juce::JSON::toString(json, true);

    return file.replaceWithText(jsonText);
}


bool VanGoghAudioProcessor::loadFromFile(const juce::File& file)
{
    // Parse file
    const auto jsonText =
        file.loadFileAsString();

    const auto json =
        juce::JSON::parse(jsonText);

    if (json.isVoid() || !json.isObject())
        return false;

    // convert to dynamic object, checks whether is it well formed
    const auto root = json.getDynamicObject();
    if (root == nullptr)
        return false;
    if (!root->hasProperty("inputGain")
        || !root->hasProperty("outputGain")
        || !root->hasProperty("dcFilterEnabled")
        || !root->hasProperty("curve"))
    {
        return false;
    }
    if (root->getProperty("format") != "VanGogh")
        return false;
    
    const auto inputGain =
        root->getProperty("inputGain");

    const auto outputGain =
        root->getProperty("outputGain");

    const auto dcFilterEnabled =
        root->getProperty("dcFilterEnabled");

    if (!dcFilterEnabled.isBool())
    {
        return false;
    }


    // Copy current state to construct new state
    juce::ValueTree state = createState();

    // Replaces named properties
    state.getChildWithProperty(
        "id", "inputGain")
        .setProperty("value", inputGain, nullptr);

    state.getChildWithProperty(
        "id", "outputGain")
        .setProperty("value", outputGain, nullptr);

    state.getChildWithProperty(
        "id", "dcFilterEnabled")
        .setProperty("value", dcFilterEnabled, nullptr);

    const auto curveVar = root->getProperty("curve");

    if (!curveVar.isArray())
        return false;

    const auto* curveArray = curveVar.getArray();

    // Current curve state
    auto curveState = state.getChildWithName("Curve");

    if (!curveState.isValid())
    {
        // Creates new curve in state
        curveState = juce::ValueTree("Curve");
        state.addChild(curveState, -1, nullptr);
    }
    else
    {
        curveState.removeAllChildren(nullptr);
    }

    // Iterate through the loaded array
    for (const auto& item : *curveArray)
    {
        if (!item.isObject())
            return false;

        const auto pointObject = item.getDynamicObject();

        if (pointObject == nullptr)
            return false;

        const juce::var x = pointObject->getProperty("x");
        const juce::var y = pointObject->getProperty("y");

        // Check whether read in values are numbers and finite
        if (!(x.isInt()  || x.isInt64() || x.isDouble()) ||
            !(y.isInt() || y.isInt64() || y.isDouble()) )
                return false;
        if (!std::isfinite(static_cast<double>(x)) || !std::isfinite(static_cast<double>(y)))
            return false;



        juce::ValueTree pointState("Point");

        pointState.setProperty("x", static_cast<float>(x), nullptr);
        pointState.setProperty("y", static_cast<float>(y), nullptr);

        curveState.addChild(
            pointState,
            -1,
            nullptr);
    }
    return applyState(state);
}




// Returns false if queue is full
bool VanGoghAudioProcessor::queueCurveLookupTable(const std::vector<CurvePoint>& curve)
{
    int startIndex1, size1, startIndex2, size2;

    juce::ignoreUnused(startIndex2, size2);

    curveUpdateFifo.prepareToWrite(
        1, startIndex1, size1, startIndex2, size2);

    if (size1 != 1)
        return false;

    buildCurveLookupTable(
        curve, queuedCurveLookupTables[startIndex1]);

    curveUpdateFifo.finishedWrite(1);
    return true;
}


juce::AudioProcessorValueTreeState&
VanGoghAudioProcessor::getParameters()
{
    return parameters;
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new VanGoghAudioProcessor();
}
