cmake_minimum_required(VERSION 3.22)

project(VanGogh VERSION 0.1.0 LANGUAGES C CXX)

add_subdirectory(third_party/JUCE)

juce_add_plugin(VanGogh
    COMPANY_NAME "physical_gravity"
    BUNDLE_ID "com.physicalgravity.vangogh"
    PLUGIN_MANUFACTURER_CODE PGRV
    PLUGIN_CODE VNGH
    FORMATS VST3
    PRODUCT_NAME "Van Gogh"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    COPY_PLUGIN_AFTER_BUILD FALSE
    VST3_CATEGORIES "Fx" "Distortion")

target_sources(VanGogh
    PRIVATE
        source/PluginEditor.cpp
        source/PluginEditor.h
        source/PluginProcessor.cpp
        source/PluginProcessor.h
	source/CurveEditor.h
	source/CurveEditor.cpp
	source/CurveModel.h)

juce_generate_juce_header(VanGogh)

target_compile_features(VanGogh PRIVATE cxx_std_17)

target_compile_definitions(VanGogh
    PUBLIC
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0)

target_link_libraries(VanGogh
    PRIVATE
        juce::juce_audio_utils
	juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)
