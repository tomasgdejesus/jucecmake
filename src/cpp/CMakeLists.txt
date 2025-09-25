juce_add_plugin(audioplugin
    PLUGIN_MANUFACTURER_CODE Juce
    PLUGIN_CODE Dem0
    NEEDS_WEBVIEW2 TRUE
    FORMATS AU VST3 Standalone
    PRODUCT_NAME "audioplugin"
)

target_sources(audioplugin
    PRIVATE
        PluginProcessor.cpp
        PluginEditor.cpp
        LuaEnv.cpp
)

target_link_libraries(audioplugin
    PRIVATE
        juce::juce_audio_utils
        juce::juce_gui_extra
        libluajit
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

target_compile_definitions(audioplugin
    PRIVATE
        SOURCE_DIR="${CMAKE_SOURCE_DIR}"
        JUCE_WEB_BROWSER=1
        JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1
        JUCE_WEB_BROWSER_RESOURCE_PROVIDER_AVAILABLE=1
        VITE_DEV # Link WebBrowserComponent to Vite dev server
)