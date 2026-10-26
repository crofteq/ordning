# The warnings both the application and the test suite are built with. Included
# from each project's top level, so it applies to every target added after it.
#
# -Wconversion is the one that matters most here: money is long long öre and
# hours an int of hundredths, and a total narrowed into an int on the way would
# otherwise compile without a word.
#
# Warnings are not errors by default, so a newer compiler cannot break a build
# from the public sources. CI turns them into errors with
# CMAKE_COMPILE_WARNING_AS_ERROR (see the justfile's werror).
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(
        -Wall
        -Wextra
        -Wshadow
        -Wnon-virtual-dtor
        -Woverloaded-virtual
        -Wnull-dereference
        -Wconversion
        # Leaving out the trailing members of an aggregate that have defaults
        # of their own, as SalaryLine{ code, text, count, price } does, is
        # what those defaults are for.
        -Wno-missing-field-initializers
    )
endif()
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    add_compile_options(
        -Wduplicated-cond
        -Wlogical-op
    )
endif()
