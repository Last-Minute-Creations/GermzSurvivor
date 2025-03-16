cmake_minimum_required(VERSION 3.14.0)
project(germz_survivor)

if(NOT AMIGA)
	message(SEND_ERROR "This project only compiles for Amiga")
endif()

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 23)

# ACE
set(AUDIO_MIXER_HW_CHANNEL_MODE "SINGLE")
set(AUDIO_MIXER_WORD_SIZED ON)
set(AUDIO_MIXER_ROUND_TO_32 ON)
set(AUDIO_MIXER_PROFILER OFF)
set(AUDIO_MIXER_HW_CHANNELS 3)
set(AUDIO_MIXER_SW_CHANNEL_COUNT 3)
set(AUDIO_MIXER_PERIOD 161)
set(ACE_SCROLLBUFFER_ENABLE_SCROLL_X OFF)
set(ACE_BOB_PRISTINE_BUFFER ON)
add_subdirectory(deps/ace ace)
add_subdirectory(deps/ace_audio_mixer ace_audio_mixer)
file(GLOB_RECURSE GAME_src src/*.c src/*.cpp src/*.h)

set(GAME_EXECUTABLE_STEM germz_survivor)
if(ELF2HUNK)
	set(GAME_EXECUTABLE ${GAME_EXECUTABLE_STEM}.elf)
	set(GAME_OUTPUT_EXECUTABLE ${GAME_EXECUTABLE_STEM}.exe)
	add_executable(${GAME_EXECUTABLE} ${GAME_src})
	target_link_options(${GAME_EXECUTABLE} PUBLIC -Wno-attributes)
	target_link_libraries(${GAME_EXECUTABLE} -Wl,-Map=${GAME_EXECUTABLE_STEM}.map)

	if (CMAKE_BUILD_TYPE STREQUAL "Release" OR CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
		set(ELF2HUNK_EXTRA_OPTS "-s")
	endif()

	add_custom_command(
		TARGET ${GAME_EXECUTABLE} POST_BUILD
		COMMAND ${ELF2HUNK} ${GAME_EXECUTABLE} ${GAME_OUTPUT_EXECUTABLE} ${ELF2HUNK_EXTRA_OPTS}
	)
	add_custom_command(
		TARGET ${GAME_EXECUTABLE} POST_BUILD
		COMMAND ${OBJDUMP} --disassemble -S ${GAME_EXECUTABLE} > ${GAME_EXECUTABLE_STEM}.s
	)
else()
	SET(GAME_EXECUTABLE ${GAME_EXECUTABLE_STEM})
	SET(GAME_OUTPUT_EXECUTABLE ${GAME_EXECUTABLE_STEM})
	add_executable(${GAME_EXECUTABLE} ${GAME_src})
endif()

target_include_directories(${GAME_EXECUTABLE} PRIVATE ${PROJECT_SOURCE_DIR}/src)
target_compile_options(${GAME_EXECUTABLE} PUBLIC -Wall -Wextra -Wimplicit-fallthrough=2)
target_compile_options(${GAME_EXECUTABLE} PRIVATE -Werror -Wno-error=unused-parameter -Wimplicit-fallthrough=2)
target_link_libraries(${GAME_EXECUTABLE} ace_audio_mixer ace)
if(GAME_DEBUG)
  target_compile_definitions(${GAME_EXECUTABLE} PRIVATE GAME_DEBUG)
endif()

set(RES_DIR ${CMAKE_CURRENT_LIST_DIR}/res)
set(DATA_DIR ${CMAKE_CURRENT_BINARY_DIR}/data)
set(GEN_DIR ${CMAKE_CURRENT_BINARY_DIR}/generated)

file(MAKE_DIRECTORY ${DATA_DIR})
file(MAKE_DIRECTORY ${GEN_DIR})

#----------------------------------------------------------------------- PALETTE

file(GLOB PALETTE_GPL ${RES_DIR}/game.gpl)
set(PALETTE_PLT ${DATA_DIR}/game.plt)

convertPalette(${GAME_EXECUTABLE} ${PALETTE_GPL} ${PALETTE_PLT})

#------------------------------------------------------------------------- TILES
file(MAKE_DIRECTORY ${GEN_DIR}/tiles)

extractBitmaps(TARGET ${GAME_EXECUTABLE} SOURCE ${RES_DIR}/mockup.png
	GENERATED_FILE_LIST "TILE_PATHS"
	DESTINATIONS
	# Corners
	${GEN_DIR}/tiles/0.png    0   0 16 16
	${GEN_DIR}/tiles/1.png   96   0 16 16
	${GEN_DIR}/tiles/2.png    0  96 16 16
	${GEN_DIR}/tiles/3.png   96  96 16 16
	# Top
	${GEN_DIR}/tiles/4.png   16   0 16 16
	${GEN_DIR}/tiles/5.png   32   0 16 16
	${GEN_DIR}/tiles/6.png   64   0 16 16
	# Left
	${GEN_DIR}/tiles/7.png    0  16 16 16
	${GEN_DIR}/tiles/8.png    0  32 16 16
	${GEN_DIR}/tiles/9.png    0  64 16 16
	# Right
	${GEN_DIR}/tiles/10.png  96  16 16 16
	${GEN_DIR}/tiles/11.png  96  48 16 16
	${GEN_DIR}/tiles/12.png  96  64 16 16
	# Bottom
	${GEN_DIR}/tiles/13.png  16  96 16 16
	${GEN_DIR}/tiles/14.png  32  96 16 16
	${GEN_DIR}/tiles/15.png  64  96 16 16
	# Fill
	${GEN_DIR}/tiles/16.png 176  16 16 16
	${GEN_DIR}/tiles/17.png 192  16 16 16
	${GEN_DIR}/tiles/18.png 208  16 16 16
	${GEN_DIR}/tiles/19.png 176  32 16 16
	${GEN_DIR}/tiles/20.png 192  32 16 16
	${GEN_DIR}/tiles/21.png 208  32 16 16
	${GEN_DIR}/tiles/22.png 176  48 16 16
	${GEN_DIR}/tiles/23.png 192  48 16 16
	${GEN_DIR}/tiles/24.png 208  48 16 16
)

convertTileset(
	TARGET ${GAME_EXECUTABLE} SIZE 16
	SOURCE ${GEN_DIR}/tiles
	INTERLEAVED DESTINATION ${GEN_DIR}/tiles.png
	TILE_PATHS ${TILE_PATHS}
)

#-------------------------------------------------------------------------- HERO

#------------------------------------------------------------------------ ZOMBIE

convertBitmaps(
	TARGET ${GAME_EXECUTABLE} PALETTE ${PALETTE_PLT} INTERLEAVED
	SOURCES
		${GEN_DIR}/tiles.png
	DESTINATIONS
		${DATA_DIR}/tiles.bm
)
