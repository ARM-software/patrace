add_executable(imageconv
    main.cpp
)

target_link_libraries(imageconv
    ${APP_LIBS}
    png # libpng (tested with version 1.2.42) in the system path will be used
)

install(TARGETS imageconv
    RUNTIME DESTINATION bin)

