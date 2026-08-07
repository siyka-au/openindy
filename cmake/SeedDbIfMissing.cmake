# Seed the runtime system DB from the checked-in template on first run only.
# A plain `copy_if_different` would blow away accumulated runtime state
# (imported plugins, projects, settings) on every relink of openIndy, since
# the runtime copy diverges from the pristine template the moment anything
# is imported/saved through the app - and copy_if_different's job is to make
# the destination match the source, which means overwriting it right back.
#
# Expects SRC_DB and DEST_DB to be passed via -D.

if(NOT EXISTS "${DEST_DB}")
    configure_file("${SRC_DB}" "${DEST_DB}" COPYONLY)
    message(STATUS "SeedDbIfMissing: seeded ${DEST_DB} from template")
endif()
