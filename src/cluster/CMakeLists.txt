# Make the list of src file
FILE(GLOB SRC_FILE *.c)

IF(${CMAKE_C_COMPILER} MATCHES "(gcc|/usr/bin/gcc)" )
# Add flags (only for gcc user)
IF (CFLAGS)
	SET_SOURCE_FILES_PROPERTIES(
		${SRC_FILE}
		COMPILE_FLAGS ${CFLAGS})
ENDIF (CFLAGS)

ENDIF(${CMAKE_C_COMPILER} MATCHES "(gcc|/usr/bin/gcc)" )

# Create the simbatch library
ADD_LIBRARY(simbatch SHARED ${SRC_FILE} ${INC_FILE})

TARGET_LINK_LIBRARIES(simbatch simgrid dl xml2 z m)

# Install library in the lib dir at the root of the project
INSTALL_TARGETS(/lib simbatch)

