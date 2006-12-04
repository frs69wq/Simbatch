# Make the list of src file
# FILE(GLOB SRC_FILE *.c)

# Add flags (only for gcc user)
IF(${CMAKE_C_COMPILER} STREQUAL "gcc" )
	
	IF (CFLAGS)
		MESSAGE( STATUS "Using ${CFLAGS} flags") 
		SET_SOURCE_FILES_PROPERTIES(
			${SRC_FILE}
			COMPILE_FLAGS ${CFLAGS})

	ENDIF (CFLAGS)

ENDIF(${CMAKE_C_COMPILER} STREQUAL "gcc" )

# Create plugins
ADD_LIBRARY(wld MODULE wld.c ${INC_FILE})
ADD_LIBRARY(swf MODULE swf.c ${INC_FILE})

TARGET_LINK_LIBRARIES(wld dl simgrid simbatch)
TARGET_LINK_LIBRARIES(swf dl simgrid simbatch)

# Install library in the lib dir at the root of the project
INSTALL_TARGETS(/lib/input swf wld)

