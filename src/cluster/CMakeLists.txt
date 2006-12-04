# Make the list of src file
FILE(GLOB SRC_FILE *.c)

# Add flags (only for gcc user)
IF(${CMAKE_C_COMPILER} STREQUAL "gcc" )
	
	IF (CFLAGS)
		SET_SOURCE_FILES_PROPERTIES(
			${SRC_FILE}
			COMPILE_FLAGS ${CFLAGS})

	ENDIF (CFLAGS)

ENDIF(${CMAKE_C_COMPILER} STREQUAL "gcc" )

# Create the simbatch library
ADD_LIBRARY(simbatch SHARED ${SRC_FILE} ${INC_FILE})

TARGET_LINK_LIBRARIES(simbatch simgrid dl xml2 z m)

# Install library in the lib dir at the root of the project
INSTALL_TARGETS(/lib simbatch)

