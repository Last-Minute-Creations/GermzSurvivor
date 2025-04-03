cmake_minimum_required(VERSION 3.14.0)
project(germz_survivor)

if(NOT AMIGA)
	message(SEND_ERROR "This project only compiles for Amiga")
endif()

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 23)

# ACE
add_compile_definitions(FIXMATH_NO_OVERFLOW)
set(AUDIO_MIXER_HW_CHANNEL_MODE "SINGLE")
set(AUDIO_MIXER_WORD_SIZED ON)
set(AUDIO_MIXER_ROUND_TO_32 ON)
set(AUDIO_MIXER_PROFILER OFF)
set(AUDIO_MIXER_HW_CHANNELS 3)
set(AUDIO_MIXER_SW_CHANNEL_COUNT 3)
set(AUDIO_MIXER_PERIOD 161)
set(ACE_BOB_PRISTINE_BUFFER ON)
set(ACE_BOB_WRAP_Y OFF)
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
target_compile_options(${GAME_EXECUTABLE} PRIVATE -Werror)
target_link_libraries(${GAME_EXECUTABLE} ace_audio_mixer ace)
if(GAME_DEBUG)
  target_compile_definitions(${GAME_EXECUTABLE} PRIVATE GAME_DEBUG)
endif()

set(RES_DIR ${CMAKE_CURRENT_LIST_DIR}/res)
set(DATA_DIR ${CMAKE_CURRENT_BINARY_DIR}/data)
set(GEN_DIR ${CMAKE_CURRENT_BINARY_DIR}/generated)

file(MAKE_DIRECTORY ${DATA_DIR})
file(MAKE_DIRECTORY ${DATA_DIR}/sfx)
file(MAKE_DIRECTORY ${GEN_DIR})
file(GLOB COPY_FILES ${RES_DIR}/copied/*)
file(COPY ${COPY_FILES} DESTINATION ${DATA_DIR})

#-------------------------------------------------------------------------- FONT

convertFont(
	TARGET ${GAME_EXECUTABLE}
	SOURCE ${RES_DIR}/uni54 DESTINATION ${DATA_DIR}/uni54.fnt
)

#----------------------------------------------------------------------- PALETTE

file(GLOB PALETTE_SPRITE ${RES_DIR}/sprite.plt)
file(GLOB PALETTE_GPL ${RES_DIR}/survivor.gpl)
set(PALETTE_PLT ${DATA_DIR}/game.plt)
set(TRANSPARENCY_HEX "#880088")

convertPalette(${GAME_EXECUTABLE} ${PALETTE_GPL} ${PALETTE_PLT})

#----------------------------------------------------------------------- CURSORS

convertBitmaps(
	TARGET ${GAME_EXECUTABLE} PALETTE ${PALETTE_SPRITE}
	INTERLEAVED SOURCES
		${RES_DIR}/cursors/crosshair.png
	DESTINATIONS
		${DATA_DIR}/cursor_crosshair.bm
)

#-------------------------------------------------------------------------- MENU

extractBitmaps(TARGET ${GAME_EXECUTABLE} SOURCE ${RES_DIR}/killfriend.png
	GENERATED_FILE_LIST "KILLFRIEND_PATH"
	DESTINATIONS
	${GEN_DIR}/killfriend.png 32 31 256 192
)

#------------------------------------------------------------------------- TILES
file(MAKE_DIRECTORY ${GEN_DIR}/tiles)

extractBitmaps(TARGET ${GAME_EXECUTABLE} SOURCE ${RES_DIR}/tiles.png
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

#------------------------------------------------------------------------ PLAYER

file(MAKE_DIRECTORY ${GEN_DIR}/player_s)
file(MAKE_DIRECTORY ${GEN_DIR}/player_sw)
file(MAKE_DIRECTORY ${GEN_DIR}/player_nw)
file(MAKE_DIRECTORY ${GEN_DIR}/player_n)
file(MAKE_DIRECTORY ${GEN_DIR}/player_ne)
file(MAKE_DIRECTORY ${GEN_DIR}/player_se)

extractBitmaps(TARGET ${GAME_EXECUTABLE} SOURCE ${RES_DIR}/player/walk.png
	GENERATED_FILE_LIST "PLAYER_S_PATHS_WALK_AIM"
	DESTINATIONS
	# Walk + aim
	${GEN_DIR}/player_s/0.png    8  16 32 32
	${GEN_DIR}/player_s/1.png   56  16 32 32
	${GEN_DIR}/player_s/2.png  104  16 32 32
	${GEN_DIR}/player_s/3.png  152  16 32 32
	${GEN_DIR}/player_s/4.png  200  16 32 32
	${GEN_DIR}/player_s/5.png  248  16 32 32
	${GEN_DIR}/player_s/6.png  296  16 32 32
	${GEN_DIR}/player_s/7.png  344  16 32 32
)
extractBitmaps(TARGET ${GAME_EXECUTABLE} SOURCE ${RES_DIR}/player/walk_shoot.png
	GENERATED_FILE_LIST "PLAYER_S_PATHS_WALK_SHOOT"
	DESTINATIONS
	# Walk + shoot
	${GEN_DIR}/player_s/8.png    8  16 32 32
	${GEN_DIR}/player_s/9.png   56  16 32 32
	${GEN_DIR}/player_s/10.png 104  16 32 32
	${GEN_DIR}/player_s/11.png 152  16 32 32
	${GEN_DIR}/player_s/12.png 200  16 32 32
	${GEN_DIR}/player_s/13.png 248  16 32 32
	${GEN_DIR}/player_s/14.png 296  16 32 32
	${GEN_DIR}/player_s/15.png 344  16 32 32
)

convertTileset(
	TARGET ${GAME_EXECUTABLE} SIZE 32
	SOURCE ${GEN_DIR}/player_s
	INTERLEAVED DESTINATION ${GEN_DIR}/player_s.png
	TILE_PATHS ${PLAYER_S_PATHS_WALK_AIM} ${PLAYER_S_PATHS_WALK_SHOOT}
)

extractBitmaps(TARGET ${GAME_EXECUTABLE} SOURCE ${RES_DIR}/player/walk.png
	GENERATED_FILE_LIST "PLAYER_SW_PATHS_WALK_AIM"
	DESTINATIONS
	${GEN_DIR}/player_sw/0.png    8  80 32 32
	${GEN_DIR}/player_sw/1.png   56  80 32 32
	${GEN_DIR}/player_sw/2.png  104  80 32 32
	${GEN_DIR}/player_sw/3.png  152  80 32 32
	${GEN_DIR}/player_sw/4.png  200  80 32 32
	${GEN_DIR}/player_sw/5.png  248  80 32 32
	${GEN_DIR}/player_sw/6.png  296  80 32 32
	${GEN_DIR}/player_sw/7.png  344  80 32 32
)
extractBitmaps(TARGET ${GAME_EXECUTABLE} SOURCE ${RES_DIR}/player/walk_shoot.png
	GENERATED_FILE_LIST "PLAYER_SW_PATHS_WALK_SHOOT"
	DESTINATIONS
	${GEN_DIR}/player_sw/8.png    8  80 32 32
	${GEN_DIR}/player_sw/9.png   56  80 32 32
	${GEN_DIR}/player_sw/10.png 104  80 32 32
	${GEN_DIR}/player_sw/11.png 152  80 32 32
	${GEN_DIR}/player_sw/12.png 200  80 32 32
	${GEN_DIR}/player_sw/13.png 248  80 32 32
	${GEN_DIR}/player_sw/14.png 296  80 32 32
	${GEN_DIR}/player_sw/15.png 344  80 32 32
)

convertTileset(
	TARGET ${GAME_EXECUTABLE} SIZE 32
	SOURCE ${GEN_DIR}/player_sw
	INTERLEAVED DESTINATION ${GEN_DIR}/player_sw.png
	TILE_PATHS ${PLAYER_SW_PATHS_WALK_AIM} ${PLAYER_SW_PATHS_WALK_SHOOT}
)

extractBitmaps(TARGET ${GAME_EXECUTABLE} SOURCE ${RES_DIR}/player/walk.png
	GENERATED_FILE_LIST "PLAYER_NW_PATHS_WALK_AIM"
	DESTINATIONS
	${GEN_DIR}/player_nw/0.png    8 144 32 32
	${GEN_DIR}/player_nw/1.png   56 144 32 32
	${GEN_DIR}/player_nw/2.png  104 144 32 32
	${GEN_DIR}/player_nw/3.png  152 144 32 32
	${GEN_DIR}/player_nw/4.png  200 144 32 32
	${GEN_DIR}/player_nw/5.png  248 144 32 32
	${GEN_DIR}/player_nw/6.png  296 144 32 32
	${GEN_DIR}/player_nw/7.png  344 144 32 32
)
extractBitmaps(TARGET ${GAME_EXECUTABLE} SOURCE ${RES_DIR}/player/walk_shoot.png
	GENERATED_FILE_LIST "PLAYER_NW_PATHS_WALK_SHOOT"
	DESTINATIONS
	${GEN_DIR}/player_nw/8.png    8 144 32 32
	${GEN_DIR}/player_nw/9.png   56 144 32 32
	${GEN_DIR}/player_nw/10.png 104 144 32 32
	${GEN_DIR}/player_nw/11.png 152 144 32 32
	${GEN_DIR}/player_nw/12.png 200 144 32 32
	${GEN_DIR}/player_nw/13.png 248 144 32 32
	${GEN_DIR}/player_nw/14.png 296 144 32 32
	${GEN_DIR}/player_nw/15.png 344 144 32 32
)

convertTileset(
	TARGET ${GAME_EXECUTABLE} SIZE 32
	SOURCE ${GEN_DIR}/player_nw
	INTERLEAVED DESTINATION ${GEN_DIR}/player_nw.png
	TILE_PATHS ${PLAYER_NW_PATHS_WALK_AIM} ${PLAYER_NW_PATHS_WALK_SHOOT}
)

extractBitmaps(TARGET ${GAME_EXECUTABLE} SOURCE ${RES_DIR}/player/walk.png
	GENERATED_FILE_LIST "PLAYER_N_PATHS_WALK_AIM"
	DESTINATIONS
	${GEN_DIR}/player_n/0.png    8 208 32 32
	${GEN_DIR}/player_n/1.png   56 208 32 32
	${GEN_DIR}/player_n/2.png  104 208 32 32
	${GEN_DIR}/player_n/3.png  152 208 32 32
	${GEN_DIR}/player_n/4.png  200 208 32 32
	${GEN_DIR}/player_n/5.png  248 208 32 32
	${GEN_DIR}/player_n/6.png  296 208 32 32
	${GEN_DIR}/player_n/7.png  344 208 32 32
)
extractBitmaps(TARGET ${GAME_EXECUTABLE} SOURCE ${RES_DIR}/player/walk_shoot.png
	GENERATED_FILE_LIST "PLAYER_N_PATHS_WALK_SHOOT"
	DESTINATIONS
	${GEN_DIR}/player_n/8.png    8 208 32 32
	${GEN_DIR}/player_n/9.png   56 208 32 32
	${GEN_DIR}/player_n/10.png 104 208 32 32
	${GEN_DIR}/player_n/11.png 152 208 32 32
	${GEN_DIR}/player_n/12.png 200 208 32 32
	${GEN_DIR}/player_n/13.png 248 208 32 32
	${GEN_DIR}/player_n/14.png 296 208 32 32
	${GEN_DIR}/player_n/15.png 344 208 32 32
)

convertTileset(
	TARGET ${GAME_EXECUTABLE} SIZE 32
	SOURCE ${GEN_DIR}/player_n
	INTERLEAVED DESTINATION ${GEN_DIR}/player_n.png
	TILE_PATHS ${PLAYER_N_PATHS_WALK_AIM} ${PLAYER_N_PATHS_WALK_SHOOT}
)

extractBitmaps(TARGET ${GAME_EXECUTABLE} SOURCE ${RES_DIR}/player/walk.png
	GENERATED_FILE_LIST "PLAYER_NE_PATHS_WALK_AIM"
	DESTINATIONS
	${GEN_DIR}/player_ne/0.png    8 272 32 32
	${GEN_DIR}/player_ne/1.png   56 272 32 32
	${GEN_DIR}/player_ne/2.png  104 272 32 32
	${GEN_DIR}/player_ne/3.png  152 272 32 32
	${GEN_DIR}/player_ne/4.png  200 272 32 32
	${GEN_DIR}/player_ne/5.png  248 272 32 32
	${GEN_DIR}/player_ne/6.png  296 272 32 32
	${GEN_DIR}/player_ne/7.png  344 272 32 32
)
extractBitmaps(TARGET ${GAME_EXECUTABLE} SOURCE ${RES_DIR}/player/walk_shoot.png
	GENERATED_FILE_LIST "PLAYER_NE_PATHS_WALK_SHOOT"
	DESTINATIONS
	${GEN_DIR}/player_ne/8.png    8 272 32 32
	${GEN_DIR}/player_ne/9.png   56 272 32 32
	${GEN_DIR}/player_ne/10.png 104 272 32 32
	${GEN_DIR}/player_ne/11.png 152 272 32 32
	${GEN_DIR}/player_ne/12.png 200 272 32 32
	${GEN_DIR}/player_ne/13.png 248 272 32 32
	${GEN_DIR}/player_ne/14.png 296 272 32 32
	${GEN_DIR}/player_ne/15.png 344 272 32 32
)

convertTileset(
	TARGET ${GAME_EXECUTABLE} SIZE 32
	SOURCE ${GEN_DIR}/player_ne
	INTERLEAVED DESTINATION ${GEN_DIR}/player_ne.png
	TILE_PATHS ${PLAYER_NE_PATHS_WALK_AIM} ${PLAYER_NE_PATHS_WALK_SHOOT}
)

extractBitmaps(TARGET ${GAME_EXECUTABLE} SOURCE ${RES_DIR}/player/walk.png
	GENERATED_FILE_LIST "PLAYER_SE_PATHS_WALK_AIM"
	DESTINATIONS
	${GEN_DIR}/player_se/0.png    8  336 32 32
	${GEN_DIR}/player_se/1.png   56  336 32 32
	${GEN_DIR}/player_se/2.png  104  336 32 32
	${GEN_DIR}/player_se/3.png  152  336 32 32
	${GEN_DIR}/player_se/4.png  200  336 32 32
	${GEN_DIR}/player_se/5.png  248  336 32 32
	${GEN_DIR}/player_se/6.png  296  336 32 32
	${GEN_DIR}/player_se/7.png  344  336 32 32
)
extractBitmaps(TARGET ${GAME_EXECUTABLE} SOURCE ${RES_DIR}/player/walk_shoot.png
	GENERATED_FILE_LIST "PLAYER_SE_PATHS_WALK_SHOOT"
	DESTINATIONS
	${GEN_DIR}/player_se/8.png    8  336 32 32
	${GEN_DIR}/player_se/9.png   56  336 32 32
	${GEN_DIR}/player_se/10.png 104  336 32 32
	${GEN_DIR}/player_se/11.png 152  336 32 32
	${GEN_DIR}/player_se/12.png 200  336 32 32
	${GEN_DIR}/player_se/13.png 248  336 32 32
	${GEN_DIR}/player_se/14.png 296  336 32 32
	${GEN_DIR}/player_se/15.png 344  336 32 32
)

convertTileset(
	TARGET ${GAME_EXECUTABLE} SIZE 32
	SOURCE ${GEN_DIR}/player_se
	INTERLEAVED DESTINATION ${GEN_DIR}/player_se.png
	TILE_PATHS ${PLAYER_SE_PATHS_WALK_AIM} ${PLAYER_SE_PATHS_WALK_SHOOT}
)

#------------------------------------------------------------------------ ZOMBIE

file(MAKE_DIRECTORY ${GEN_DIR}/zombie_s)
file(MAKE_DIRECTORY ${GEN_DIR}/zombie_sw)
file(MAKE_DIRECTORY ${GEN_DIR}/zombie_nw)
file(MAKE_DIRECTORY ${GEN_DIR}/zombie_n)
file(MAKE_DIRECTORY ${GEN_DIR}/zombie_ne)
file(MAKE_DIRECTORY ${GEN_DIR}/zombie_se)

extractBitmaps(TARGET ${GAME_EXECUTABLE} SOURCE ${RES_DIR}/zombie/walk.png
	GENERATED_FILE_LIST "ZOMBIE_S_PATHS"
	DESTINATIONS
	${GEN_DIR}/zombie_s/0.png     16   20 16 24
	${GEN_DIR}/zombie_s/1.png     64   20 16 24
	${GEN_DIR}/zombie_s/2.png    112   20 16 24
	${GEN_DIR}/zombie_s/3.png    160   20 16 24
	${GEN_DIR}/zombie_s/4.png    208   20 16 24
	${GEN_DIR}/zombie_s/5.png    256   20 16 24
	${GEN_DIR}/zombie_s/6.png    304   20 16 24
	${GEN_DIR}/zombie_s/7.png    352   20 16 24
)

convertTileset(
	TARGET ${GAME_EXECUTABLE} SIZE 16 HEIGHT 24
	SOURCE ${GEN_DIR}/zombie_s
	INTERLEAVED DESTINATION ${GEN_DIR}/zombie_s.png
	TILE_PATHS ${ZOMBIE_S_PATHS}
)

extractBitmaps(TARGET ${GAME_EXECUTABLE} SOURCE ${RES_DIR}/zombie/walk.png
	GENERATED_FILE_LIST "ZOMBIE_SW_PATHS"
	DESTINATIONS
	${GEN_DIR}/zombie_sw/0.png     16   84 16 24
	${GEN_DIR}/zombie_sw/1.png     64   84 16 24
	${GEN_DIR}/zombie_sw/2.png    112   84 16 24
	${GEN_DIR}/zombie_sw/3.png    160   84 16 24
	${GEN_DIR}/zombie_sw/4.png    208   84 16 24
	${GEN_DIR}/zombie_sw/5.png    256   84 16 24
	${GEN_DIR}/zombie_sw/6.png    304   84 16 24
	${GEN_DIR}/zombie_sw/7.png    352   84 16 24
)

convertTileset(
	TARGET ${GAME_EXECUTABLE} SIZE 16 HEIGHT 24
	SOURCE ${GEN_DIR}/zombie_sw
	INTERLEAVED DESTINATION ${GEN_DIR}/zombie_sw.png
	TILE_PATHS ${ZOMBIE_SW_PATHS}
)

extractBitmaps(TARGET ${GAME_EXECUTABLE} SOURCE ${RES_DIR}/zombie/walk.png
	GENERATED_FILE_LIST "ZOMBIE_NW_PATHS"
	DESTINATIONS
	${GEN_DIR}/zombie_nw/0.png     16  148 16 24
	${GEN_DIR}/zombie_nw/1.png     64  148 16 24
	${GEN_DIR}/zombie_nw/2.png    112  148 16 24
	${GEN_DIR}/zombie_nw/3.png    160  148 16 24
	${GEN_DIR}/zombie_nw/4.png    208  148 16 24
	${GEN_DIR}/zombie_nw/5.png    256  148 16 24
	${GEN_DIR}/zombie_nw/6.png    304  148 16 24
	${GEN_DIR}/zombie_nw/7.png    352  148 16 24
)

convertTileset(
	TARGET ${GAME_EXECUTABLE} SIZE 16 HEIGHT 24
	SOURCE ${GEN_DIR}/zombie_nw
	INTERLEAVED DESTINATION ${GEN_DIR}/zombie_nw.png
	TILE_PATHS ${ZOMBIE_NW_PATHS}
)

extractBitmaps(TARGET ${GAME_EXECUTABLE} SOURCE ${RES_DIR}/zombie/walk.png
	GENERATED_FILE_LIST "ZOMBIE_N_PATHS"
	DESTINATIONS
	${GEN_DIR}/zombie_n/0.png     16  212 16 24
	${GEN_DIR}/zombie_n/1.png     64  212 16 24
	${GEN_DIR}/zombie_n/2.png    112  212 16 24
	${GEN_DIR}/zombie_n/3.png    160  212 16 24
	${GEN_DIR}/zombie_n/4.png    208  212 16 24
	${GEN_DIR}/zombie_n/5.png    256  212 16 24
	${GEN_DIR}/zombie_n/6.png    304  212 16 24
	${GEN_DIR}/zombie_n/7.png    352  212 16 24
)

convertTileset(
	TARGET ${GAME_EXECUTABLE} SIZE 16 HEIGHT 24
	SOURCE ${GEN_DIR}/zombie_n
	INTERLEAVED DESTINATION ${GEN_DIR}/zombie_n.png
	TILE_PATHS ${ZOMBIE_N_PATHS}
)

extractBitmaps(TARGET ${GAME_EXECUTABLE} SOURCE ${RES_DIR}/zombie/walk.png
	GENERATED_FILE_LIST "ZOMBIE_NE_PATHS"
	DESTINATIONS
	${GEN_DIR}/zombie_ne/0.png     16  276 16 24
	${GEN_DIR}/zombie_ne/1.png     64  276 16 24
	${GEN_DIR}/zombie_ne/2.png    112  276 16 24
	${GEN_DIR}/zombie_ne/3.png    160  276 16 24
	${GEN_DIR}/zombie_ne/4.png    208  276 16 24
	${GEN_DIR}/zombie_ne/5.png    256  276 16 24
	${GEN_DIR}/zombie_ne/6.png    304  276 16 24
	${GEN_DIR}/zombie_ne/7.png    352  276 16 24
)

convertTileset(
	TARGET ${GAME_EXECUTABLE} SIZE 16 HEIGHT 24
	SOURCE ${GEN_DIR}/zombie_ne
	INTERLEAVED DESTINATION ${GEN_DIR}/zombie_ne.png
	TILE_PATHS ${ZOMBIE_NE_PATHS}
)

extractBitmaps(TARGET ${GAME_EXECUTABLE} SOURCE ${RES_DIR}/zombie/walk.png
	GENERATED_FILE_LIST "ZOMBIE_SE_PATHS"
	DESTINATIONS
	${GEN_DIR}/zombie_se/0.png     16  340 16 24
	${GEN_DIR}/zombie_se/1.png     64  340 16 24
	${GEN_DIR}/zombie_se/2.png    112  340 16 24
	${GEN_DIR}/zombie_se/3.png    160  340 16 24
	${GEN_DIR}/zombie_se/4.png    208  340 16 24
	${GEN_DIR}/zombie_se/5.png    256  340 16 24
	${GEN_DIR}/zombie_se/6.png    304  340 16 24
	${GEN_DIR}/zombie_se/7.png    352  340 16 24
)

convertTileset(
	TARGET ${GAME_EXECUTABLE} SIZE 16 HEIGHT 24
	SOURCE ${GEN_DIR}/zombie_se
	INTERLEAVED DESTINATION ${GEN_DIR}/zombie_se.png
	TILE_PATHS ${ZOMBIE_SE_PATHS}
)

#----------------------------------------------------------------------- PICKUPS

file(MAKE_DIRECTORY ${GEN_DIR}/pickups)

extractBitmaps(TARGET ${GAME_EXECUTABLE} SOURCE ${RES_DIR}/pickups.png
	GENERATED_FILE_LIST "PICKUP_PATHS"
	DESTINATIONS
	${GEN_DIR}/pickups/0.png 3  61 16 12
	${GEN_DIR}/pickups/1.png 3  77 16 12
	${GEN_DIR}/pickups/2.png 3  93 16 12
	${GEN_DIR}/pickups/3.png 3 109 16 12
	${GEN_DIR}/pickups/4.png 3 125 16 12
)

convertTileset(
	TARGET ${GAME_EXECUTABLE} SIZE 16 HEIGHT 12
	SOURCE ${GEN_DIR}/pickups
	INTERLEAVED DESTINATION ${GEN_DIR}/pickups.png
	TILE_PATHS ${PICKUP_PATHS}
)

#----------------------------------------------------------------------- BULLETS

file(MAKE_DIRECTORY ${GEN_DIR}/bullets)

extractBitmaps(TARGET ${GAME_EXECUTABLE} SOURCE ${RES_DIR}/misc.png
	GENERATED_FILE_LIST "BULLETS_PATHS"
	DESTINATIONS
	${GEN_DIR}/bullets/0.png  270 104 16 6
	${GEN_DIR}/bullets/1.png  270 111 16 6
)

convertTileset(
	TARGET ${GAME_EXECUTABLE} SIZE 16 HEIGHT 6
	SOURCE ${GEN_DIR}/bullets
	INTERLEAVED DESTINATION ${GEN_DIR}/bullets.png
	TILE_PATHS ${BULLETS_PATHS}
)

#------------------------------------------------------------------------ STAINS

file(MAKE_DIRECTORY ${GEN_DIR}/stains)

extractBitmaps(TARGET ${GAME_EXECUTABLE} SOURCE ${RES_DIR}/misc.png
	GENERATED_FILE_LIST "STAINS_PATHS"
	DESTINATIONS
	${GEN_DIR}/stains/0.png 256 136 16 16
	${GEN_DIR}/stains/1.png 256 152 16 16
	${GEN_DIR}/stains/2.png 256 168 16 16
)

convertTileset(
	TARGET ${GAME_EXECUTABLE} SIZE 16
	SOURCE ${GEN_DIR}/stains
	INTERLEAVED DESTINATION ${GEN_DIR}/stains.png
	TILE_PATHS ${STAINS_PATHS}
)

#----------------------------------------------------------------- CONVERT TO BM

convertBitmaps(
	TARGET ${GAME_EXECUTABLE} PALETTE ${PALETTE_PLT}
	INTERLEAVED SOURCES
		${GEN_DIR}/tiles.png ${RES_DIR}/weapons.png ${GEN_DIR}/killfriend.png
	DESTINATIONS
		${DATA_DIR}/tiles.bm ${DATA_DIR}/weapons.bm ${DATA_DIR}/killfriend.bm
)

convertBitmaps(
	TARGET ${GAME_EXECUTABLE} PALETTE ${PALETTE_PLT} MASK_COLOR ${TRANSPARENCY_HEX}
	INTERLEAVED SOURCES
		${GEN_DIR}/player_s.png ${GEN_DIR}/player_sw.png ${GEN_DIR}/player_nw.png
		${GEN_DIR}/player_n.png ${GEN_DIR}/player_ne.png ${GEN_DIR}/player_se.png
	DESTINATIONS
		${DATA_DIR}/player_s.bm ${DATA_DIR}/player_sw.bm ${DATA_DIR}/player_nw.bm
		${DATA_DIR}/player_n.bm ${DATA_DIR}/player_ne.bm ${DATA_DIR}/player_se.bm
	MASKS
		${DATA_DIR}/player_s_mask.bm ${DATA_DIR}/player_sw_mask.bm ${DATA_DIR}/player_nw_mask.bm
		${DATA_DIR}/player_n_mask.bm ${DATA_DIR}/player_ne_mask.bm ${DATA_DIR}/player_se_mask.bm
)

convertBitmaps(
	TARGET ${GAME_EXECUTABLE} PALETTE ${PALETTE_PLT} MASK_COLOR ${TRANSPARENCY_HEX}
	INTERLEAVED SOURCES
		${GEN_DIR}/zombie_s.png ${GEN_DIR}/zombie_sw.png ${GEN_DIR}/zombie_nw.png
		${GEN_DIR}/zombie_n.png ${GEN_DIR}/zombie_ne.png ${GEN_DIR}/zombie_se.png
		${GEN_DIR}/bullets.png ${GEN_DIR}/stains.png ${RES_DIR}/edges.png ${GEN_DIR}/pickups.png
	DESTINATIONS
		${DATA_DIR}/zombie_s.bm ${DATA_DIR}/zombie_sw.bm ${DATA_DIR}/zombie_nw.bm
		${DATA_DIR}/zombie_n.bm ${DATA_DIR}/zombie_ne.bm ${DATA_DIR}/zombie_se.bm
		${DATA_DIR}/bullets.bm ${DATA_DIR}/stains.bm NONE ${DATA_DIR}/pickups.bm
	MASKS
		${DATA_DIR}/zombie_s_mask.bm ${DATA_DIR}/zombie_sw_mask.bm ${DATA_DIR}/zombie_nw_mask.bm
		${DATA_DIR}/zombie_n_mask.bm ${DATA_DIR}/zombie_ne_mask.bm ${DATA_DIR}/zombie_se_mask.bm
		${DATA_DIR}/bullets_mask.bm ${DATA_DIR}/stains_mask.bm ${DATA_DIR}/killfriend_edges.bm ${DATA_DIR}/pickups_mask.bm
)

#------------------------------------------------------------------------- AUDIO

convertAudio(
	TARGET ${GAME_EXECUTABLE} STRICT NORMALIZE PAD 2 DIVIDE_AMPLITUDE 3
	SOURCE ${RES_DIR}/sfx/bite_1.wav
	DESTINATION ${DATA_DIR}/sfx/bite_1.sfx
)
convertAudio(
	TARGET ${GAME_EXECUTABLE} STRICT NORMALIZE PAD 2 DIVIDE_AMPLITUDE 3
	SOURCE ${RES_DIR}/sfx/bite_2.wav
	DESTINATION ${DATA_DIR}/sfx/bite_2.sfx
)
convertAudio(
	TARGET ${GAME_EXECUTABLE} STRICT NORMALIZE PAD 2 DIVIDE_AMPLITUDE 3
	SOURCE ${RES_DIR}/sfx/impact_1.wav
	DESTINATION ${DATA_DIR}/sfx/impact_1.sfx
)
convertAudio(
	TARGET ${GAME_EXECUTABLE} STRICT NORMALIZE PAD 2 DIVIDE_AMPLITUDE 3
	SOURCE ${RES_DIR}/sfx/impact_2.wav
	DESTINATION ${DATA_DIR}/sfx/impact_2.sfx
)

convertAudio(
	TARGET ${GAME_EXECUTABLE} STRICT NORMALIZE PAD 2 DIVIDE_AMPLITUDE 3
	SOURCE ${RES_DIR}/sfx/rifle_shot_1.wav
	DESTINATION ${DATA_DIR}/sfx/rifle_shot_1.sfx
)
convertAudio(
	TARGET ${GAME_EXECUTABLE} STRICT NORMALIZE PAD 2 DIVIDE_AMPLITUDE 3
	SOURCE ${RES_DIR}/sfx/rifle_shot_2.wav
	DESTINATION ${DATA_DIR}/sfx/rifle_shot_2.sfx
)

convertAudio(
	TARGET ${GAME_EXECUTABLE} STRICT NORMALIZE PAD 2 DIVIDE_AMPLITUDE 3
	SOURCE ${RES_DIR}/sfx/assault_shot_1.wav
	DESTINATION ${DATA_DIR}/sfx/assault_shot_1.sfx
)
convertAudio(
	TARGET ${GAME_EXECUTABLE} STRICT NORMALIZE PAD 2 DIVIDE_AMPLITUDE 3
	SOURCE ${RES_DIR}/sfx/assault_shot_2.wav
	DESTINATION ${DATA_DIR}/sfx/assault_shot_2.sfx
)

convertAudio(
	TARGET ${GAME_EXECUTABLE} STRICT NORMALIZE PAD 2 DIVIDE_AMPLITUDE 3
	SOURCE ${RES_DIR}/sfx/smg_shot_1.wav
	DESTINATION ${DATA_DIR}/sfx/smg_shot_1.sfx
)
convertAudio(
	TARGET ${GAME_EXECUTABLE} STRICT NORMALIZE PAD 2 DIVIDE_AMPLITUDE 3
	SOURCE ${RES_DIR}/sfx/smg_shot_2.wav
	DESTINATION ${DATA_DIR}/sfx/smg_shot_2.sfx
)

convertAudio(
	TARGET ${GAME_EXECUTABLE} STRICT NORMALIZE PAD 2 DIVIDE_AMPLITUDE 3
	SOURCE ${RES_DIR}/sfx/shotgun_shot_1.wav
	DESTINATION ${DATA_DIR}/sfx/shotgun_shot_1.sfx
)
convertAudio(
	TARGET ${GAME_EXECUTABLE} STRICT NORMALIZE PAD 2 DIVIDE_AMPLITUDE 3
	SOURCE ${RES_DIR}/sfx/shotgun_shot_2.wav
	DESTINATION ${DATA_DIR}/sfx/shotgun_shot_2.sfx
)

convertAudio(
	TARGET ${GAME_EXECUTABLE} STRICT NORMALIZE PAD 2 DIVIDE_AMPLITUDE 3
	SOURCE ${RES_DIR}/sfx/reload_1.wav
	DESTINATION ${DATA_DIR}/sfx/reload_1.sfx
)

# Version stuff
string(TIMESTAMP YEAR "%y")
string(TIMESTAMP DAY "%d")
string(TIMESTAMP MONTH "%m")
MATH(EXPR VER_MAJOR "0 + ${YEAR}")
MATH(EXPR VER_MINOR "0 + ${MONTH}")
MATH(EXPR VER_FIX "0 + ${DAY}")
set(VERSION "${VER_MAJOR}.${VER_MINOR}.${VER_FIX}")
target_compile_definitions(${GAME_EXECUTABLE} PRIVATE GAME_VERSION="${VERSION}")

# Generating ZIP
set(GAME_PACKAGE_NAME "${CMAKE_PROJECT_NAME}_${VER_MAJOR}_${VER_MINOR}_${VER_FIX}")
add_custom_target(generateZip COMMAND
	${CMAKE_COMMAND} -E tar "cf" "${GAME_PACKAGE_NAME}.zip" --format=zip
	"${CMAKE_CURRENT_BINARY_DIR}/${GAME_OUTPUT_EXECUTABLE}"
	# "${CMAKE_CURRENT_BINARY_DIR}/${GAME_OUTPUT_EXECUTABLE}.info"
	"${DATA_DIR}"
	COMMENT "Generating ZIP file ${GAME_PACKAGE_NAME}.zip"
)
