file(GLOB _po_files *.po)
gettext_process_po_files(fr ALL INSTALL_DESTINATION ${CMAKE_INSTALL_LOCALEDIR} PO_FILES ${_po_files})
