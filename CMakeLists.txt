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
file(MAKE_DIRECTORY ${DATA_DIR}/intro)
file(MAKE_DIRECTORY ${GEN_DIR})
file(GLOB COPY_FILES ${RES_DIR}/copied/* ${RES_DIR}/music/*)
file(COPY ${COPY_FILES} DESTINATION ${DATA_DIR})

#-------------------------------------------------------------------------- FONT

convertFont(
	TARGET ${GAME_EXECUTABLE}
	SOURCE ${RES_DIR}/uni54 DESTINATION ${DATA_DIR}/uni54.fnt
)
convertFont(
	TARGET ${GAME_EXECUTABLE}
	SOURCE ${RES_DIR}/uni54_small DESTINATION ${DATA_DIR}/uni54_small.fnt
)

#----------------------------------------------------------------------- PALETTE

file(GLOB PALETTE_SPRITE ${RES_DIR}/sprite.gpl)
file(GLOB PALETTE_GPL ${RES_DIR}/survivor.gpl)
file(GLOB PALETTE_INTRO_GPL ${RES_DIR}/intro.gpl)
set(PALETTE_PLT ${DATA_DIR}/game.plt)
set(TRANSPARENCY_HEX "#880088")

convertPalette(${GAME_EXECUTABLE} ${PALETTE_GPL} ${PALETTE_PLT})
convertPalette(${GAME_EXECUTABLE} ${PALETTE_INTRO_GPL} ${DATA_DIR}/intro.plt)

#------------------------------------------------------------------------- MUSIC

# mergeMods(
# 	TARGET ${GAME_EXECUTABLE} COMPRESS
# 	SOURCES
# 		${RES_DIR}/music/game.mod
# 		${RES_DIR}/music/menu.mod
# 		${RES_DIR}/music/intro.mod
# 	DESTINATIONS
# 		${DATA_DIR}/game.mod
# 		${DATA_DIR}/menu.mod
# 		${DATA_DIR}/intro.mod
# 	SAMPLE_PACK ${DATA_DIR}/music.samplepack
# )

#------------------------------------------------------------------------- INTRO

foreach(intro_idx RANGE 4)
	convertPalette(
		${GAME_EXECUTABLE}
		${RES_DIR}/intro/${intro_idx}.pal ${DATA_DIR}/intro/${intro_idx}.plt
	)
	convertBitmaps(
		TARGET ${GAME_EXECUTABLE} PALETTE ${RES_DIR}/intro/${intro_idx}.pal
		SOURCES ${RES_DIR}/intro/${intro_idx}.png
		INTERLEAVED DESTINATIONS ${DATA_DIR}/intro/${intro_idx}.bm
	)
endforeach()

#----------------------------------------------------------------------- CURSORS

file(MAKE_DIRECTORY ${GEN_DIR}/cursors)
extractBitmaps(TARGET ${GAME_EXECUTABLE} SOURCE ${RES_DIR}/misc.png
	GENERATED_FILE_LIST "CURSOR_PATHS"
	DESTINATIONS
	${GEN_DIR}/cursors/0.png 32 41 16 9
	${GEN_DIR}/cursors/1.png 32 28 16 9
	${GEN_DIR}/cursors/2.png 32  4 16 9
)

convertTileset(
	TARGET ${GAME_EXECUTABLE} SIZE 16 HEIGHT 9
	SOURCE ${GEN_DIR}/cursors
	INTERLEAVED DESTINATION ${GEN_DIR}/cursors.png
	TILE_PATHS ${CURSOR_PATHS}
)

convertBitmaps(
	TARGET ${GAME_EXECUTABLE} PALETTE ${PALETTE_SPRITE}
	INTERLEAVED SOURCES
		${GEN_DIR}/cursors.png
	DESTINATIONS
		${DATA_DIR}/cursors.bm
)

#-------------------------------------------------------------------------- MENU

extractBitmaps(TARGET ${GAME_EXECUTABLE} SOURCE ${RES_DIR}/killfriend.png
	GENERATED_FILE_LIST "KILLFRIEND_PATH"
	DESTINATIONS
	${GEN_DIR}/killfriend.png 32 31 256 192
)
extractBitmaps(TARGET ${GAME_EXECUTABLE} SOURCE ${RES_DIR}/logo.png
	GENERATED_FILE_LIST "LOGO_PATH"
	DESTINATIONS
	${GEN_DIR}/logo.png 106 57 112 48
)

#------------------------------------------------------------------------- TILES
file(MAKE_DIRECTORY ${GEN_DIR}/tiles)

extractBitmaps(TARGET ${GAME_EXECUTABLE} SOURCE ${RES_DIR}/tiles.png
	GENERATED_FILE_LIST "TILE_PATHS"
	DESTINATIONS
	# Regular
	${GEN_DIR}/tiles/0.png   0   0 32 32
	${GEN_DIR}/tiles/1.png   0  32 32 32
	${GEN_DIR}/tiles/2.png   0  64 32 32
	# Barrel
	${GEN_DIR}/tiles/3.png  32   0 32 32
	${GEN_DIR}/tiles/4.png  32  32 32 32
	${GEN_DIR}/tiles/5.png  32  64 32 32
	# Skeleton
	${GEN_DIR}/tiles/6.png  32  96 32 32
	${GEN_DIR}/tiles/7.png  32 128 32 32

	# # Simple
	# ${GEN_DIR}/tiles/0.png   0  96 32 32
	# ${GEN_DIR}/tiles/1.png   0 128 32 32
	# ${GEN_DIR}/tiles/2.png   0 160 32 32
	# ${GEN_DIR}/tiles/3.png   0 192 32 32
	# ${GEN_DIR}/tiles/4.png   0 224 32 32
)

convertTileset(
	TARGET ${GAME_EXECUTABLE} SIZE 32
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

extractBitmaps(TARGET ${GAME_EXECUTABLE} SOURCE deps/private_assets/player/walk.png
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
extractBitmaps(TARGET ${GAME_EXECUTABLE} SOURCE deps/private_assets/player/die.png
	GENERATED_FILE_LIST "PLAYER_S_PATHS_DIE"
	DESTINATIONS
	# Walk + aim
	${GEN_DIR}/player_s/8.png     8  16 32 32 # 0,0
	${GEN_DIR}/player_s/9.png    56  16 32 32 # 0,0
	${GEN_DIR}/player_s/10.png  104  17 32 32 # 0,1
	${GEN_DIR}/player_s/11.png  152  18 32 32 # 0,2
	${GEN_DIR}/player_s/12.png  200  22 32 32 # 0,6
	${GEN_DIR}/player_s/13.png  248  22 32 32 # 0,6
	${GEN_DIR}/player_s/14.png  296  25 32 32 # 0,9
	${GEN_DIR}/player_s/15.png  344  25 32 32 # 0,9
)
extractBitmaps(TARGET ${GAME_EXECUTABLE} SOURCE deps/private_assets/player/walk_shoot.png
	GENERATED_FILE_LIST "PLAYER_S_PATHS_WALK_SHOOT"
	DESTINATIONS
	# Walk + shoot
	${GEN_DIR}/player_s/16.png   8  16 32 32
	${GEN_DIR}/player_s/17.png  56  16 32 32
	${GEN_DIR}/player_s/18.png 104  16 32 32
	${GEN_DIR}/player_s/19.png 152  16 32 32
	${GEN_DIR}/player_s/20.png 200  16 32 32
	${GEN_DIR}/player_s/21.png 248  16 32 32
	${GEN_DIR}/player_s/22.png 296  16 32 32
	${GEN_DIR}/player_s/23.png 344  16 32 32
)

convertTileset(
	TARGET ${GAME_EXECUTABLE} SIZE 32
	SOURCE ${GEN_DIR}/player_s
	INTERLEAVED DESTINATION ${GEN_DIR}/player_s.png
	TILE_PATHS ${PLAYER_S_PATHS_WALK_AIM} ${PLAYER_S_PATHS_WALK_SHOOT} ${PLAYER_S_PATHS_DIE}
)

extractBitmaps(TARGET ${GAME_EXECUTABLE} SOURCE deps/private_assets/player/walk.png
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
extractBitmaps(TARGET ${GAME_EXECUTABLE} SOURCE deps/private_assets/player/die.png
	GENERATED_FILE_LIST "PLAYER_SW_PATHS_DIE"
	DESTINATIONS
	${GEN_DIR}/player_sw/8.png    8  80 32 32 #  0,0
	${GEN_DIR}/player_sw/9.png   56  80 32 32 #  0,0
	${GEN_DIR}/player_sw/10.png 104  80 32 32 #  0,0
	${GEN_DIR}/player_sw/11.png 152  80 32 32 #  0,0
	${GEN_DIR}/player_sw/12.png 198  80 32 32 # -2,0
	${GEN_DIR}/player_sw/13.png 243  80 32 32 # -5,0
	${GEN_DIR}/player_sw/14.png 289  81 32 32 # -7,1
	${GEN_DIR}/player_sw/15.png 337  81 32 32 # -7,1
)
extractBitmaps(TARGET ${GAME_EXECUTABLE} SOURCE deps/private_assets/player/walk_shoot.png
	GENERATED_FILE_LIST "PLAYER_SW_PATHS_WALK_SHOOT"
	DESTINATIONS
	${GEN_DIR}/player_sw/16.png   8  80 32 32
	${GEN_DIR}/player_sw/17.png  56  80 32 32
	${GEN_DIR}/player_sw/18.png 104  80 32 32
	${GEN_DIR}/player_sw/19.png 152  80 32 32
	${GEN_DIR}/player_sw/20.png 200  80 32 32
	${GEN_DIR}/player_sw/21.png 248  80 32 32
	${GEN_DIR}/player_sw/22.png 296  80 32 32
	${GEN_DIR}/player_sw/23.png 344  80 32 32
)

convertTileset(
	TARGET ${GAME_EXECUTABLE} SIZE 32
	SOURCE ${GEN_DIR}/player_sw
	INTERLEAVED DESTINATION ${GEN_DIR}/player_sw.png
	TILE_PATHS ${PLAYER_SW_PATHS_WALK_AIM} ${PLAYER_SW_PATHS_WALK_SHOOT} ${PLAYER_SW_PATHS_DIE}
)

extractBitmaps(TARGET ${GAME_EXECUTABLE} SOURCE deps/private_assets/player/walk.png
	GENERATED_FILE_LIST "PLAYER_NW_PATHS_WALK_AIM"
	DESTINATIONS
	${GEN_DIR}/player_nw/0.png   8 144 32 32
	${GEN_DIR}/player_nw/1.png  56 144 32 32
	${GEN_DIR}/player_nw/2.png 104 144 32 32
	${GEN_DIR}/player_nw/3.png 152 144 32 32
	${GEN_DIR}/player_nw/4.png 200 144 32 32
	${GEN_DIR}/player_nw/5.png 248 144 32 32
	${GEN_DIR}/player_nw/6.png 296 144 32 32
	${GEN_DIR}/player_nw/7.png 344 144 32 32
)
extractBitmaps(TARGET ${GAME_EXECUTABLE} SOURCE deps/private_assets/player/die.png
	GENERATED_FILE_LIST "PLAYER_NW_PATHS_DIE"
	DESTINATIONS
	${GEN_DIR}/player_nw/8.png    8 144 32 32 #  0,0
	${GEN_DIR}/player_nw/9.png   56 144 32 32 #  0,0
	${GEN_DIR}/player_nw/10.png 104 144 32 32 #  0,0
	${GEN_DIR}/player_nw/11.png 152 144 32 32 #  0,0
	${GEN_DIR}/player_nw/12.png 200 144 32 32 #  0,0
	${GEN_DIR}/player_nw/13.png 248 144 32 32 #  0,0
	${GEN_DIR}/player_nw/14.png 292 144 32 32 # -4,0
	${GEN_DIR}/player_nw/15.png 340 144 32 32 # -4,0
)
extractBitmaps(TARGET ${GAME_EXECUTABLE} SOURCE deps/private_assets/player/walk_shoot.png
	GENERATED_FILE_LIST "PLAYER_NW_PATHS_WALK_SHOOT"
	DESTINATIONS
	${GEN_DIR}/player_nw/16.png   8 144 32 32
	${GEN_DIR}/player_nw/17.png  56 144 32 32
	${GEN_DIR}/player_nw/18.png 104 144 32 32
	${GEN_DIR}/player_nw/19.png 152 144 32 32
	${GEN_DIR}/player_nw/20.png 200 144 32 32
	${GEN_DIR}/player_nw/21.png 248 144 32 32
	${GEN_DIR}/player_nw/22.png 296 144 32 32
	${GEN_DIR}/player_nw/23.png 344 144 32 32
)

convertTileset(
	TARGET ${GAME_EXECUTABLE} SIZE 32
	SOURCE ${GEN_DIR}/player_nw
	INTERLEAVED DESTINATION ${GEN_DIR}/player_nw.png
	TILE_PATHS ${PLAYER_NW_PATHS_WALK_AIM} ${PLAYER_NW_PATHS_WALK_SHOOT} ${PLAYER_NW_PATHS_DIE}
)

extractBitmaps(TARGET ${GAME_EXECUTABLE} SOURCE deps/private_assets/player/walk.png
	GENERATED_FILE_LIST "PLAYER_N_PATHS_WALK_AIM"
	DESTINATIONS
	${GEN_DIR}/player_n/0.png   8 208 32 32
	${GEN_DIR}/player_n/1.png  56 208 32 32
	${GEN_DIR}/player_n/2.png 104 208 32 32
	${GEN_DIR}/player_n/3.png 152 208 32 32
	${GEN_DIR}/player_n/4.png 200 208 32 32
	${GEN_DIR}/player_n/5.png 248 208 32 32
	${GEN_DIR}/player_n/6.png 296 208 32 32
	${GEN_DIR}/player_n/7.png 344 208 32 32
)
extractBitmaps(TARGET ${GAME_EXECUTABLE} SOURCE deps/private_assets/player/die.png
	GENERATED_FILE_LIST "PLAYER_N_PATHS_DIE"
	DESTINATIONS
	${GEN_DIR}/player_n/8.png    8 208 32 32 # 0,0
	${GEN_DIR}/player_n/9.png   56 208 32 32 # 0,0
	${GEN_DIR}/player_n/10.png 104 208 32 32 # 0,0
	${GEN_DIR}/player_n/11.png 152 208 32 32 # 0,0
	${GEN_DIR}/player_n/12.png 200 208 32 32 # 0,0
	${GEN_DIR}/player_n/13.png 248 208 32 32 # 0,0
	${GEN_DIR}/player_n/14.png 296 208 32 32 # 0,0
	${GEN_DIR}/player_n/15.png 344 208 32 32 # 0,0
)
extractBitmaps(TARGET ${GAME_EXECUTABLE} SOURCE deps/private_assets/player/walk_shoot.png
	GENERATED_FILE_LIST "PLAYER_N_PATHS_WALK_SHOOT"
	DESTINATIONS
	${GEN_DIR}/player_n/16.png   8 208 32 32
	${GEN_DIR}/player_n/17.png  56 208 32 32
	${GEN_DIR}/player_n/18.png 104 208 32 32
	${GEN_DIR}/player_n/19.png 152 208 32 32
	${GEN_DIR}/player_n/20.png 200 208 32 32
	${GEN_DIR}/player_n/21.png 248 208 32 32
	${GEN_DIR}/player_n/22.png 296 208 32 32
	${GEN_DIR}/player_n/23.png 344 208 32 32
)

convertTileset(
	TARGET ${GAME_EXECUTABLE} SIZE 32
	SOURCE ${GEN_DIR}/player_n
	INTERLEAVED DESTINATION ${GEN_DIR}/player_n.png
	TILE_PATHS ${PLAYER_N_PATHS_WALK_AIM} ${PLAYER_N_PATHS_WALK_SHOOT} ${PLAYER_N_PATHS_DIE}
)

extractBitmaps(TARGET ${GAME_EXECUTABLE} SOURCE deps/private_assets/player/walk.png
	GENERATED_FILE_LIST "PLAYER_NE_PATHS_WALK_AIM"
	DESTINATIONS
	${GEN_DIR}/player_ne/0.png   8 272 32 32
	${GEN_DIR}/player_ne/1.png  56 272 32 32
	${GEN_DIR}/player_ne/2.png 104 272 32 32
	${GEN_DIR}/player_ne/3.png 152 272 32 32
	${GEN_DIR}/player_ne/4.png 200 272 32 32
	${GEN_DIR}/player_ne/5.png 248 272 32 32
	${GEN_DIR}/player_ne/6.png 296 272 32 32
	${GEN_DIR}/player_ne/7.png 344 272 32 32
)
extractBitmaps(TARGET ${GAME_EXECUTABLE} SOURCE deps/private_assets/player/die.png
	GENERATED_FILE_LIST "PLAYER_NE_PATHS_DIE"
	DESTINATIONS
	${GEN_DIR}/player_ne/8.png    8 272 32 32 # 0,0
	${GEN_DIR}/player_ne/9.png   56 272 32 32 # 0,0
	${GEN_DIR}/player_ne/10.png 106 272 32 32 # 2,0
	${GEN_DIR}/player_ne/11.png 156 272 32 32 # 4,0
	${GEN_DIR}/player_ne/12.png 206 272 32 32 # 6,0
	${GEN_DIR}/player_ne/13.png 255 272 32 32 # 7,0
	${GEN_DIR}/player_ne/14.png 304 272 32 32 # 8,0
	${GEN_DIR}/player_ne/15.png 352 272 32 32 # 8,0
)
extractBitmaps(TARGET ${GAME_EXECUTABLE} SOURCE deps/private_assets/player/walk_shoot.png
	GENERATED_FILE_LIST "PLAYER_NE_PATHS_WALK_SHOOT"
	DESTINATIONS
	${GEN_DIR}/player_ne/16.png   8 272 32 32
	${GEN_DIR}/player_ne/17.png  56 272 32 32
	${GEN_DIR}/player_ne/18.png 104 272 32 32
	${GEN_DIR}/player_ne/19.png 152 272 32 32
	${GEN_DIR}/player_ne/20.png 200 272 32 32
	${GEN_DIR}/player_ne/21.png 248 272 32 32
	${GEN_DIR}/player_ne/22.png 296 272 32 32
	${GEN_DIR}/player_ne/23.png 344 272 32 32
)

convertTileset(
	TARGET ${GAME_EXECUTABLE} SIZE 32
	SOURCE ${GEN_DIR}/player_ne
	INTERLEAVED DESTINATION ${GEN_DIR}/player_ne.png
	TILE_PATHS ${PLAYER_NE_PATHS_WALK_AIM} ${PLAYER_NE_PATHS_WALK_SHOOT} ${PLAYER_NE_PATHS_DIE}
)

extractBitmaps(TARGET ${GAME_EXECUTABLE} SOURCE deps/private_assets/player/walk.png
	GENERATED_FILE_LIST "PLAYER_SE_PATHS_WALK_AIM"
	DESTINATIONS
	${GEN_DIR}/player_se/0.png   8  336 32 32
	${GEN_DIR}/player_se/1.png  56  336 32 32
	${GEN_DIR}/player_se/2.png 104  336 32 32
	${GEN_DIR}/player_se/3.png 152  336 32 32
	${GEN_DIR}/player_se/4.png 200  336 32 32
	${GEN_DIR}/player_se/5.png 248  336 32 32
	${GEN_DIR}/player_se/6.png 296  336 32 32
	${GEN_DIR}/player_se/7.png 344  336 32 32
)
extractBitmaps(TARGET ${GAME_EXECUTABLE} SOURCE deps/private_assets/player/die.png
	GENERATED_FILE_LIST "PLAYER_SE_PATHS_DIE"
	DESTINATIONS
	${GEN_DIR}/player_se/8.png    8  336 32 32 # 0,0
	${GEN_DIR}/player_se/9.png   56  336 32 32 # 0,0
	${GEN_DIR}/player_se/10.png 104  336 32 32 # 0,0
	${GEN_DIR}/player_se/11.png 152  336 32 32 # 0,0
	${GEN_DIR}/player_se/12.png 200  341 32 32 # 0,5
	${GEN_DIR}/player_se/13.png 248  340 32 32 # 0,4
	${GEN_DIR}/player_se/14.png 297  341 32 32 # 1,5
	${GEN_DIR}/player_se/15.png 345  341 32 32 # 1,5
)
extractBitmaps(TARGET ${GAME_EXECUTABLE} SOURCE deps/private_assets/player/walk_shoot.png
	GENERATED_FILE_LIST "PLAYER_SE_PATHS_WALK_SHOOT"
	DESTINATIONS
	${GEN_DIR}/player_se/16.png   8  336 32 32
	${GEN_DIR}/player_se/17.png  56  336 32 32
	${GEN_DIR}/player_se/18.png 104  336 32 32
	${GEN_DIR}/player_se/19.png 152  336 32 32
	${GEN_DIR}/player_se/20.png 200  336 32 32
	${GEN_DIR}/player_se/21.png 248  336 32 32
	${GEN_DIR}/player_se/22.png 296  336 32 32
	${GEN_DIR}/player_se/23.png 344  336 32 32
)

convertTileset(
	TARGET ${GAME_EXECUTABLE} SIZE 32
	SOURCE ${GEN_DIR}/player_se
	INTERLEAVED DESTINATION ${GEN_DIR}/player_se.png
	TILE_PATHS ${PLAYER_SE_PATHS_WALK_AIM} ${PLAYER_SE_PATHS_WALK_SHOOT} ${PLAYER_SE_PATHS_DIE}
)

#------------------------------------------------------------------------ ZOMBIE

file(MAKE_DIRECTORY ${GEN_DIR}/zombie_s)
file(MAKE_DIRECTORY ${GEN_DIR}/zombie_sw)
file(MAKE_DIRECTORY ${GEN_DIR}/zombie_nw)
file(MAKE_DIRECTORY ${GEN_DIR}/zombie_n)
file(MAKE_DIRECTORY ${GEN_DIR}/zombie_ne)
file(MAKE_DIRECTORY ${GEN_DIR}/zombie_se)

extractBitmaps(TARGET ${GAME_EXECUTABLE} SOURCE ${RES_DIR}/zombie/frames.png
	GENERATED_FILE_LIST "ZOMBIE_S_PATHS_WALK"
	DESTINATIONS
	${GEN_DIR}/zombie_s/0.png    8    4 16 24
	${GEN_DIR}/zombie_s/1.png   40    4 16 24
	${GEN_DIR}/zombie_s/2.png   72    4 16 24
	${GEN_DIR}/zombie_s/3.png  104    4 16 24
	${GEN_DIR}/zombie_s/4.png  136    4 16 24
	${GEN_DIR}/zombie_s/5.png  168    4 16 24
	${GEN_DIR}/zombie_s/6.png  200    4 16 24
	${GEN_DIR}/zombie_s/7.png  232    4 16 24
)
extractBitmaps(TARGET ${GAME_EXECUTABLE} SOURCE ${RES_DIR}/zombie/frames.png
	GENERATED_FILE_LIST "ZOMBIE_S_PATHS_DIE"
	DESTINATIONS
	${GEN_DIR}/zombie_s/8.png  264    4 16 24
	${GEN_DIR}/zombie_s/9.png  296    4 16 24
	${GEN_DIR}/zombie_s/10.png 328    4 16 24
)

convertTileset(
	TARGET ${GAME_EXECUTABLE} SIZE 16 HEIGHT 24
	SOURCE ${GEN_DIR}/zombie_s
	INTERLEAVED DESTINATION ${GEN_DIR}/zombie_s.png
	TILE_PATHS ${ZOMBIE_S_PATHS_WALK} ${ZOMBIE_S_PATHS_DIE}
)

extractBitmaps(TARGET ${GAME_EXECUTABLE} SOURCE ${RES_DIR}/zombie/frames.png
	GENERATED_FILE_LIST "ZOMBIE_SW_PATHS_WALK"
	DESTINATIONS
	${GEN_DIR}/zombie_sw/0.png    8   36 16 24
	${GEN_DIR}/zombie_sw/1.png   40   36 16 24
	${GEN_DIR}/zombie_sw/2.png   72   36 16 24
	${GEN_DIR}/zombie_sw/3.png  104   36 16 24
	${GEN_DIR}/zombie_sw/4.png  136   36 16 24
	${GEN_DIR}/zombie_sw/5.png  168   36 16 24
	${GEN_DIR}/zombie_sw/6.png  200   36 16 24
	${GEN_DIR}/zombie_sw/7.png  232   36 16 24
)
extractBitmaps(TARGET ${GAME_EXECUTABLE} SOURCE ${RES_DIR}/zombie/frames.png
	GENERATED_FILE_LIST "ZOMBIE_SW_PATHS_DIE"
	DESTINATIONS
	${GEN_DIR}/zombie_sw/8.png  264   36 16 24
	${GEN_DIR}/zombie_sw/9.png  296   36 16 24
	${GEN_DIR}/zombie_sw/10.png 328   36 16 24
)

convertTileset(
	TARGET ${GAME_EXECUTABLE} SIZE 16 HEIGHT 24
	SOURCE ${GEN_DIR}/zombie_sw
	INTERLEAVED DESTINATION ${GEN_DIR}/zombie_sw.png
	TILE_PATHS ${ZOMBIE_SW_PATHS_WALK} ${ZOMBIE_SW_PATHS_DIE}
)

extractBitmaps(TARGET ${GAME_EXECUTABLE} SOURCE ${RES_DIR}/zombie/frames.png
	GENERATED_FILE_LIST "ZOMBIE_NW_PATHS_WALK"
	DESTINATIONS
	${GEN_DIR}/zombie_nw/0.png    8  68 16 24
	${GEN_DIR}/zombie_nw/1.png   40  68 16 24
	${GEN_DIR}/zombie_nw/2.png   72  68 16 24
	${GEN_DIR}/zombie_nw/3.png  104  68 16 24
	${GEN_DIR}/zombie_nw/4.png  136  68 16 24
	${GEN_DIR}/zombie_nw/5.png  168  68 16 24
	${GEN_DIR}/zombie_nw/6.png  200  68 16 24
	${GEN_DIR}/zombie_nw/7.png  232  68 16 24
)
extractBitmaps(TARGET ${GAME_EXECUTABLE} SOURCE ${RES_DIR}/zombie/frames.png
	GENERATED_FILE_LIST "ZOMBIE_NW_PATHS_DIE"
	DESTINATIONS
	${GEN_DIR}/zombie_nw/8.png  264  68 16 24
	${GEN_DIR}/zombie_nw/9.png  296  68 16 24
	${GEN_DIR}/zombie_nw/10.png 328  68 16 24
)

convertTileset(
	TARGET ${GAME_EXECUTABLE} SIZE 16 HEIGHT 24
	SOURCE ${GEN_DIR}/zombie_nw
	INTERLEAVED DESTINATION ${GEN_DIR}/zombie_nw.png
	TILE_PATHS ${ZOMBIE_NW_PATHS_WALK} ${ZOMBIE_NW_PATHS_DIE}
)

extractBitmaps(TARGET ${GAME_EXECUTABLE} SOURCE ${RES_DIR}/zombie/frames.png
	GENERATED_FILE_LIST "ZOMBIE_N_PATHS_WALK"
	DESTINATIONS
	${GEN_DIR}/zombie_n/0.png    8  100 16 24
	${GEN_DIR}/zombie_n/1.png   40  100 16 24
	${GEN_DIR}/zombie_n/2.png   72  100 16 24
	${GEN_DIR}/zombie_n/3.png  104  100 16 24
	${GEN_DIR}/zombie_n/4.png  136  100 16 24
	${GEN_DIR}/zombie_n/5.png  168  100 16 24
	${GEN_DIR}/zombie_n/6.png  200  100 16 24
	${GEN_DIR}/zombie_n/7.png  232  100 16 24
)
extractBitmaps(TARGET ${GAME_EXECUTABLE} SOURCE ${RES_DIR}/zombie/frames.png
	GENERATED_FILE_LIST "ZOMBIE_N_PATHS_DIE"
	DESTINATIONS
	${GEN_DIR}/zombie_n/8.png  264  100 16 24
	${GEN_DIR}/zombie_n/9.png  296  100 16 24
	${GEN_DIR}/zombie_n/10.png 328  100 16 24
)

convertTileset(
	TARGET ${GAME_EXECUTABLE} SIZE 16 HEIGHT 24
	SOURCE ${GEN_DIR}/zombie_n
	INTERLEAVED DESTINATION ${GEN_DIR}/zombie_n.png
	TILE_PATHS ${ZOMBIE_N_PATHS_WALK} ${ZOMBIE_N_PATHS_DIE}
)

extractBitmaps(TARGET ${GAME_EXECUTABLE} SOURCE ${RES_DIR}/zombie/frames.png
	GENERATED_FILE_LIST "ZOMBIE_NE_PATHS_WALK"
	DESTINATIONS
	${GEN_DIR}/zombie_ne/0.png    8  132 16 24
	${GEN_DIR}/zombie_ne/1.png   40  132 16 24
	${GEN_DIR}/zombie_ne/2.png   72  132 16 24
	${GEN_DIR}/zombie_ne/3.png  104  132 16 24
	${GEN_DIR}/zombie_ne/4.png  136  132 16 24
	${GEN_DIR}/zombie_ne/5.png  168  132 16 24
	${GEN_DIR}/zombie_ne/6.png  200  132 16 24
	${GEN_DIR}/zombie_ne/7.png  232  132 16 24
)
extractBitmaps(TARGET ${GAME_EXECUTABLE} SOURCE ${RES_DIR}/zombie/frames.png
	GENERATED_FILE_LIST "ZOMBIE_NE_PATHS_DIE"
	DESTINATIONS
	${GEN_DIR}/zombie_ne/8.png  264  132 16 24
	${GEN_DIR}/zombie_ne/9.png  296  132 16 24
	${GEN_DIR}/zombie_ne/10.png 328  132 16 24
)

convertTileset(
	TARGET ${GAME_EXECUTABLE} SIZE 16 HEIGHT 24
	SOURCE ${GEN_DIR}/zombie_ne
	INTERLEAVED DESTINATION ${GEN_DIR}/zombie_ne.png
	TILE_PATHS ${ZOMBIE_NE_PATHS_WALK} ${ZOMBIE_NE_PATHS_DIE}
)

extractBitmaps(TARGET ${GAME_EXECUTABLE} SOURCE ${RES_DIR}/zombie/frames.png
	GENERATED_FILE_LIST "ZOMBIE_SE_PATHS_WALK"
	DESTINATIONS
	${GEN_DIR}/zombie_se/0.png    8  164 16 24
	${GEN_DIR}/zombie_se/1.png   40  164 16 24
	${GEN_DIR}/zombie_se/2.png   72  164 16 24
	${GEN_DIR}/zombie_se/3.png  104  164 16 24
	${GEN_DIR}/zombie_se/4.png  136  164 16 24
	${GEN_DIR}/zombie_se/5.png  168  164 16 24
	${GEN_DIR}/zombie_se/6.png  200  164 16 24
	${GEN_DIR}/zombie_se/7.png  232  164 16 24
)
extractBitmaps(TARGET ${GAME_EXECUTABLE} SOURCE ${RES_DIR}/zombie/frames.png
	GENERATED_FILE_LIST "ZOMBIE_SE_PATHS_DIE"
	DESTINATIONS
	${GEN_DIR}/zombie_se/8.png  264  164 16 24
	${GEN_DIR}/zombie_se/9.png  296  164 16 24
	${GEN_DIR}/zombie_se/10.png 328  164 16 24
)

convertTileset(
	TARGET ${GAME_EXECUTABLE} SIZE 16 HEIGHT 24
	SOURCE ${GEN_DIR}/zombie_se
	INTERLEAVED DESTINATION ${GEN_DIR}/zombie_se.png
	TILE_PATHS ${ZOMBIE_SE_PATHS_WALK} ${ZOMBIE_SE_PATHS_DIE}
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
	${GEN_DIR}/pickups/5.png 3 131 16 12
	${GEN_DIR}/pickups/6.png 3 147 16 12
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
		${GEN_DIR}/logo.png ${RES_DIR}/level_up.png
	DESTINATIONS
		${DATA_DIR}/tiles.bm ${DATA_DIR}/weapons.bm ${DATA_DIR}/killfriend.bm
		${DATA_DIR}/logo.bm ${DATA_DIR}/level_up.bm
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
