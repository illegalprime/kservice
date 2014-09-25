project("kplugin-update-index")

include(ECMMarkNonGuiExecutable)

add_executable(kplugin-update-index
    main.cpp
    kpluginindexer.cpp
)
add_executable(KF5::kplugin-update-index ALIAS kplugin-update-index)

# Mark it as non-gui so we won't create an app bundle on Mac OS X
ecm_mark_nongui_executable(kplugin-update-index)

target_link_libraries(kplugin-update-index
    Qt5::Core
    KF5::CoreAddons
    KF5::ConfigCore
)

install(TARGETS kplugin-update-index EXPORT KF5ServiceTargets ${KF5_INSTALL_TARGETS_DEFAULT_ARGS})
