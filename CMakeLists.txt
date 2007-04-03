# Name of the project
PROJECT(SIMBATCH C)

# Affiche un message standard
MESSAGE(STATUS "Building Makefile for the Simbatch project")
MESSAGE(STATUS "If everything is ok, don't forget to do the following thing :") 

IF (UNIX)
	IF (APPLE)
		MESSAGE(STATUS "*************** MACOSX USER ***************")
		MESSAGE(STATUS "export DYLD_LIBRARY_PATH=$DYLD_LIBRARY_PATH:${SIMBATCH_SOURCE_DIR}/lib/")
		MESSAGE(STATUS "export DYLD_LIBRARY_PATH=$DYLD_LIBRARY_PATH:${SIMBATCH_SOURCE_DIR}/lib/input")
		MESSAGE(STATUS "export DYLD_LIBRARY_PATH=$DYLD_LIBRARY_PATH:${SIMBATCH_SOURCE_DIR}/lib/scheduler")
	ELSE (APPLE)
		MESSAGE(STATUS "*************** LINUX USER ***************") 
		MESSAGE(STATUS "export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:${SIMBATCH_SOURCE_DIR}/lib/")
		MESSAGE(STATUS "export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:${SIMBATCH_SOURCE_DIR}/lib/input")
		MESSAGE(STATUS "export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:${SIMBATCH_SOURCE_DIR}/lib/scheduler")
	ENDIF (APPLE)
ENDIF (UNIX)

MESSAGE(STATUS "or define the libraries path yourself for your specfic arch")

#-------------------------- SOME VARIABLES -------------------------------
# Fixe le path pour le make install
SET(CMAKE_INSTALL_PREFIX ${SIMBATCH_SOURCE_DIR})
SET(SIMGRID_HOME /usr/local CACHE PATH "Set the path for the Simgrid installation directory")

# --------------------- OPTIONS OF THE PROJECT -----------------------------
OPTION(OPT      "Enable optimisation flags"     OFF)
OPTION(WARN     "Enable some warning flags"     ON)
OPTION(PARANOID "Enable a lot of warning flags" OFF)

OPTION(DEBUG    "Enable debug mode"             OFF)
OPTION(NDEBUG   "Disable assert instructions"   OFF)
OPTION(GANTT    "Enable GANTT chart printing"   OFF)
OPTION(LOG      "Enable trace of the excution"  ON)
OPTION(OUTPUT   "Enable special output"         ON)
OPTION(VERBOSE  "Enable verbose Mode"           ON)

IF (DEBUG)
	ADD_DEFINITIONS(-DDEBUG)
ENDIF (DEBUG)

IF (NDEBUG)
    ADD_DEFINITIONS(-DNDEBUG)
ENDIF (NDEBUG)	

IF (GANTT)
    ADD_DEFINITIONS(-DGANTT)
ENDIF (GANTT)

IF (LOG)
    ADD_DEFINITIONS(-DLOG)
ENDIF (LOG)	

IF (OUTPUT)
    ADD_DEFINITIONS(-DOUTPUT)
ENDIF (OUTPUT)

IF (VERBOSE)
        ADD_DEFINITIONS(-DVERBOSE)
ENDIF (VERBOSE)
		
#----------------------- DEFINE SOME FLAGS ------------------------
IF (OPT)
	SET(O_FLAGS "-O2 -pipe")
ENDIF (OPT)

IF (WARN)
	SET (W_FLAGS "-Wall -Wstrict-prototypes -Wcast-align -Wmissing-prototypes -Wnonnull -Wmissing-prototypes -Wmissing-declarations")
ENDIF (WARN)

IF (PARANOID)
	SET (W_FLAGS "-Wshadow -Wcast-align -Wnonnull -Waggregate-return -Wmissing-prototypes -Wmissing-declarations -Wstrict-prototypes -Wredundant-decls -Wnested-externs -Wpointer-arith -Wwrite-strings -finline-functions -Wall")
ENDIF (PARANOID)

IF (OPT OR WARN)
	SET(CFLAGS "${O_FLAGS} ${W_FLAGS}")
ENDIF (OPT OR WARN)
IF (APPLE)
    SET(CFLAGS "-fnested-functions ${CFLAGS}")
ENDIF (APPLE)

# Add flags (only for gcc user)
#IF(${CMAKE_C_COMPILER} STREQUAL "gcc" )
MESSAGE(STATUS "flags used : ${CFLAGS}")
#ENDIF(${CMAKE_C_COMPILER} STREQUAL "gcc" )
										
#----------------------- DEFINE PATHES ---------------------------
INCLUDE_DIRECTORIES(/usr/include/libxml2 ${SIMBATCH_SOURCE_DIR}/include/simbatch ${SIMGRID_HOME}/include)
LINK_DIRECTORIES(/usr/lib/ ${SIMGRID_HOME}/lib)

# where to find source code
SUBDIRS(src/cluster)
SUBDIRS(src/plugins/input)
SUBDIRS(src/plugins/scheduler)

