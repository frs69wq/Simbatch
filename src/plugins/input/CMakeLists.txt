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
ADD_LIBRARY(oar2-xml MODULE oar2-xml.c ${INC_FILE})

TARGET_LINK_LIBRARIES(wld dl simgrid simbatch)
TARGET_LINK_LIBRARIES(swf dl simgrid simbatch)
TARGET_LINK_LIBRARIES(oar2-xml xml2 dl simgrid simbatch)

# Install library in the ${CMAKE_INSTALL_PREFIX}/${INSTALL_DIR} directory
INSTALL_TARGETS(${INSTALL_DIR} swf wld oar2-xml)

