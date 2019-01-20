find_package(Gettext)
if(GETTEXT_FOUND)
    add_subdirectory(it)
    add_subdirectory(de)
    add_subdirectory(fr)
    add_subdirectory(es_AR)
else(GETTEXT_FOUND)
    message(
"------
                 NOTE: Gettext not found. Translations will *not* be installed.
------")
endif(GETTEXT_FOUND)
