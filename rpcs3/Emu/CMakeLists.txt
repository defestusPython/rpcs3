﻿add_library(rpcs3_emu
    cache_utils.cpp
    IdManager.cpp
    localized_string.cpp
    System.cpp
    system_config.cpp
    system_config_types.cpp
    system_progress.cpp
    system_utils.cpp
    VFS.cpp
    GDB.cpp
    title.cpp
    perf_meter.cpp
)

# prevent WolfSSL from warning about not having harden options
add_compile_definitions(WC_NO_HARDEN)

target_link_libraries(rpcs3_emu
    PRIVATE
        3rdparty::zlib 3rdparty::yaml-cpp
    PUBLIC
        3rdparty::libevdev 3rdparty::flatbuffers)


find_package(Threads REQUIRED)
target_link_libraries(rpcs3_emu
    PUBLIC Threads::Threads)

# For stdafx.h
target_include_directories(rpcs3_emu
    PUBLIC
        ${RPCS3_SRC_DIR})


# Utilities
target_sources(rpcs3_emu PRIVATE
    ../util/atomic.cpp
    ../util/media_utils.cpp
    ../util/logs.cpp
    ../util/yaml.cpp
    ../util/vm_native.cpp
    ../util/dyn_lib.cpp
    ../util/sysinfo.cpp
    ../util/cpu_stats.cpp
    ../../Utilities/bin_patch.cpp
    ../../Utilities/cheat_info.cpp
    ../../Utilities/cond.cpp
    ../../Utilities/Config.cpp
    ../../Utilities/File.cpp
    ../../Utilities/JIT.cpp
    ../../Utilities/LUrlParser.cpp
    ../../Utilities/mutex.cpp
    ../../Utilities/rXml.cpp
    ../../Utilities/sema.cpp
    ../../Utilities/StrFmt.cpp
    ../../Utilities/Thread.cpp
    ../../Utilities/version.cpp
)

target_include_directories(rpcs3_emu PUBLIC "${CMAKE_SOURCE_DIR}")

target_link_libraries(rpcs3_emu
    PUBLIC
        3rdparty::pugixml)

if(MSVC)
    set_source_files_properties("../../Utilities/JIT.cpp" PROPERTIES COMPILE_FLAGS /GR-)
else()
    set_source_files_properties("../../Utilities/JIT.cpp" PROPERTIES COMPILE_FLAGS -fno-rtti)
endif()

if(MSVC)
    set_source_files_properties("../util/yaml.cpp" PROPERTIES COMPILE_FLAGS /EHsc)
else()
    set_source_files_properties("../util/yaml.cpp" PROPERTIES COMPILE_FLAGS -fexceptions)
endif()

# Crypto
target_sources(rpcs3_emu PRIVATE
    ../Crypto/aes.cpp
    ../Crypto/aesni.cpp
    ../Crypto/ec.cpp
    ../Crypto/key_vault.cpp
    ../Crypto/lz.cpp
    ../Crypto/md5.cpp
    ../Crypto/sha1.cpp
    ../Crypto/sha256.cpp
    ../Crypto/unedat.cpp
    ../Crypto/unpkg.cpp
    ../Crypto/unself.cpp
    ../Crypto/utils.cpp
)

# Loader
target_sources(rpcs3_emu PRIVATE
    ../Loader/ELF.cpp
    ../Loader/mself.cpp
    ../Loader/PSF.cpp
    ../Loader/PUP.cpp
    ../Loader/TAR.cpp
    ../Loader/TROPUSR.cpp
    ../Loader/TRP.cpp
)


# Audio
target_sources(rpcs3_emu PRIVATE
    Audio/AudioDumper.cpp
    Audio/AudioBackend.cpp
    Audio/AL/OpenALBackend.cpp
)

if(USE_ALSA)
    find_package(ALSA)
    if(ALSA_FOUND)
        target_sources(rpcs3_emu PRIVATE Audio/ALSA/ALSABackend.cpp)
        target_link_libraries(rpcs3_emu PUBLIC 3rdparty::alsa)
    endif()
endif()

if(USE_PULSE AND PULSE_FOUND)
    target_sources(rpcs3_emu PRIVATE Audio/Pulse/PulseBackend.cpp)
    target_link_libraries(rpcs3_emu PUBLIC 3rdparty::pulse)
endif()

if(USE_FAUDIO)
    find_package(SDL2)
    if(SDL2_FOUND AND NOT SDL2_VERSION VERSION_LESS 2.0.9)
        target_sources(rpcs3_emu PRIVATE Audio/FAudio/FAudioBackend.cpp)
        target_link_libraries(rpcs3_emu PUBLIC 3rdparty::faudio)
    endif()
endif()

if(WIN32)
    target_include_directories(rpcs3_emu PUBLIC "${RPCS3_SRC_DIR}/../3rdparty/XAudio2Redist/include")
    target_link_libraries(rpcs3_emu PRIVATE "${RPCS3_SRC_DIR}/../3rdparty/XAudio2Redist/libs/xaudio2_9redist.lib")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /DELAYLOAD:xaudio2_9redist.dll")
    target_sources(rpcs3_emu PRIVATE
        Audio/XAudio2/XAudio2Backend.cpp
    )
endif()

target_link_libraries(rpcs3_emu
    PUBLIC
        3rdparty::openal)


# Cell
target_sources(rpcs3_emu PRIVATE
    Cell/MFC.cpp
    Cell/PPUAnalyser.cpp
    Cell/PPUDisAsm.cpp
    Cell/PPUFunction.cpp
    Cell/PPUInterpreter.cpp
    Cell/PPUModule.cpp
    Cell/PPUThread.cpp
    Cell/PPUTranslator.cpp
    Cell/RawSPUThread.cpp
    Cell/SPUAnalyser.cpp
    Cell/SPUASMJITRecompiler.cpp
    Cell/SPUDisAsm.cpp
    Cell/SPUInterpreter.cpp
    Cell/SPURecompiler.cpp
    Cell/SPUThread.cpp
    Cell/lv2/lv2.cpp
    Cell/lv2/sys_bdemu.cpp
    Cell/lv2/sys_btsetting.cpp
    Cell/lv2/sys_cond.cpp
    Cell/lv2/sys_console.cpp
    Cell/lv2/sys_crypto_engine.cpp
    Cell/lv2/sys_config.cpp
    Cell/lv2/sys_dbg.cpp
    Cell/lv2/sys_event.cpp
    Cell/lv2/sys_event_flag.cpp
    Cell/lv2/sys_fs.cpp
    Cell/lv2/sys_gamepad.cpp
    Cell/lv2/sys_gpio.cpp
    Cell/lv2/sys_hid.cpp
    Cell/lv2/sys_interrupt.cpp
    Cell/lv2/sys_io.cpp
    Cell/lv2/sys_lwcond.cpp
    Cell/lv2/sys_lwmutex.cpp
    Cell/lv2/sys_memory.cpp
    Cell/lv2/sys_mmapper.cpp
    Cell/lv2/sys_mutex.cpp
    Cell/lv2/sys_net.cpp
    Cell/lv2/sys_overlay.cpp
    Cell/lv2/sys_ppu_thread.cpp
    Cell/lv2/sys_process.cpp
    Cell/lv2/sys_prx.cpp
    Cell/lv2/sys_rsx.cpp
    Cell/lv2/sys_rsxaudio.cpp
    Cell/lv2/sys_rwlock.cpp
    Cell/lv2/sys_semaphore.cpp
    Cell/lv2/sys_spu.cpp
    Cell/lv2/sys_sm.cpp
    Cell/lv2/sys_ss.cpp
    Cell/lv2/sys_storage.cpp
    Cell/lv2/sys_time.cpp
    Cell/lv2/sys_timer.cpp
    Cell/lv2/sys_trace.cpp
    Cell/lv2/sys_tty.cpp
    Cell/lv2/sys_uart.cpp
    Cell/lv2/sys_usbd.cpp
    Cell/lv2/sys_vm.cpp
    Cell/Modules/cellAdec.cpp
    Cell/Modules/cellAtrac.cpp
    Cell/Modules/cellAtracMulti.cpp
    Cell/Modules/cellAudio.cpp
    Cell/Modules/cellAudioOut.cpp
    Cell/Modules/cellAuthDialog.cpp
    Cell/Modules/cellAvconfExt.cpp
    Cell/Modules/cellBgdl.cpp
    Cell/Modules/cellCamera.cpp
    Cell/Modules/cellCelp8Enc.cpp
    Cell/Modules/cellCelpEnc.cpp
    Cell/Modules/cellCrossController.cpp
    Cell/Modules/cellDaisy.cpp
    Cell/Modules/cellDmux.cpp
    Cell/Modules/cellDtcpIpUtility.cpp
    Cell/Modules/cellFiber.cpp
    Cell/Modules/cellFont.cpp
    Cell/Modules/cellFontFT.cpp
    Cell/Modules/cell_FreeType2.cpp
    Cell/Modules/cellFs.cpp
    Cell/Modules/cellGame.cpp
    Cell/Modules/cellGameExec.cpp
    Cell/Modules/cellGcmSys.cpp
    Cell/Modules/cellGem.cpp
    Cell/Modules/cellGifDec.cpp
    Cell/Modules/cellHttp.cpp
    Cell/Modules/cellHttpUtil.cpp
    Cell/Modules/cellImeJp.cpp
    Cell/Modules/cellJpgDec.cpp
    Cell/Modules/cellJpgEnc.cpp
    Cell/Modules/cellKb.cpp
    Cell/Modules/cellKey2char.cpp
    Cell/Modules/cellL10n.cpp
    Cell/Modules/cellLibprof.cpp
    Cell/Modules/cellMic.cpp
    Cell/Modules/cellMouse.cpp
    Cell/Modules/cellMsgDialog.cpp
    Cell/Modules/cellMusic.cpp
    Cell/Modules/cellMusicDecode.cpp
    Cell/Modules/cellMusicExport.cpp
    Cell/Modules/cellNetAoi.cpp
    Cell/Modules/cellNetCtl.cpp
    Cell/Modules/cellOskDialog.cpp
    Cell/Modules/cellOvis.cpp
    Cell/Modules/cellPad.cpp
    Cell/Modules/cellPamf.cpp
    Cell/Modules/cellPesmUtility.cpp
    Cell/Modules/cellPhotoDecode.cpp
    Cell/Modules/cellPhotoExport.cpp
    Cell/Modules/cellPhotoImport.cpp
    Cell/Modules/cellPngDec.cpp
    Cell/Modules/cellPngEnc.cpp
    Cell/Modules/cellPrint.cpp
    Cell/Modules/cellRec.cpp
    Cell/Modules/cellRemotePlay.cpp
    Cell/Modules/cellResc.cpp
    Cell/Modules/cellRtcAlarm.cpp
    Cell/Modules/cellRtc.cpp
    Cell/Modules/cellRudp.cpp
    Cell/Modules/cellSail.cpp
    Cell/Modules/cellSailRec.cpp
    Cell/Modules/cellSaveData.cpp
    Cell/Modules/cellScreenshot.cpp
    Cell/Modules/cellSearch.cpp
    Cell/Modules/cellSheap.cpp
    Cell/Modules/cellSpudll.cpp
    Cell/Modules/cellSpurs.cpp
    Cell/Modules/cellSpursJq.cpp
    Cell/Modules/cellSpursSpu.cpp
    Cell/Modules/cellSsl.cpp
    Cell/Modules/cellStorage.cpp
    Cell/Modules/cellSubDisplay.cpp
    Cell/Modules/cellSync2.cpp
    Cell/Modules/cellSync.cpp
    Cell/Modules/cellSysconf.cpp
    Cell/Modules/cellSysCache.cpp
    Cell/Modules/cellSysmodule.cpp
    Cell/Modules/cellSysutilAp.cpp
    Cell/Modules/cellSysutilAvc2.cpp
    Cell/Modules/cellSysutilAvc.cpp
    Cell/Modules/cellSysutilAvcExt.cpp
    Cell/Modules/cellSysutil.cpp
    Cell/Modules/cellSysutilMisc.cpp
    Cell/Modules/cellSysutilNpEula.cpp
    Cell/Modules/cellUsbd.cpp
    Cell/Modules/cellUsbpspcm.cpp
    Cell/Modules/cellUserInfo.cpp
    Cell/Modules/cellVdec.cpp
    Cell/Modules/cellVideoExport.cpp
    Cell/Modules/cellVideoOut.cpp
    Cell/Modules/cellVideoPlayerUtility.cpp
    Cell/Modules/cellVideoUpload.cpp
    Cell/Modules/cellVoice.cpp
    Cell/Modules/cellVpost.cpp
    Cell/Modules/cellWebBrowser.cpp
    Cell/Modules/libad_async.cpp
    Cell/Modules/libad_core.cpp
    Cell/Modules/libmedi.cpp
    Cell/Modules/libmixer.cpp
    Cell/Modules/libsnd3.cpp
    Cell/Modules/libsynth2.cpp
    Cell/Modules/sceNp2.cpp
    Cell/Modules/sceNpClans.cpp
    Cell/Modules/sceNpCommerce2.cpp
    Cell/Modules/sceNp.cpp
    Cell/Modules/sceNpMatchingInt.cpp
    Cell/Modules/sceNpSns.cpp
    Cell/Modules/sceNpTrophy.cpp
    Cell/Modules/sceNpTus.cpp
    Cell/Modules/sceNpUtil.cpp
    Cell/Modules/StaticHLE.cpp
    Cell/Modules/sys_crashdump.cpp
    Cell/Modules/sys_game.cpp
    Cell/Modules/sys_heap.cpp
    Cell/Modules/sys_io_.cpp
    Cell/Modules/sys_libc_.cpp
    Cell/Modules/sys_libc.cpp
    Cell/Modules/sys_lv2dbg.cpp
    Cell/Modules/sys_lwcond_.cpp
    Cell/Modules/sys_lwmutex_.cpp
    Cell/Modules/sys_mempool.cpp
    Cell/Modules/sys_mmapper_.cpp
    Cell/Modules/sys_net_.cpp
    Cell/Modules/sys_ppu_thread_.cpp
    Cell/Modules/sys_prx_.cpp
    Cell/Modules/sysPrxForUser.cpp
    Cell/Modules/sys_rsxaudio_.cpp
    Cell/Modules/sys_spinlock.cpp
    Cell/Modules/sys_spu_.cpp
    Cell/Modules/libfs_utility_init.cpp
)

if(NOT MSVC)
    set_source_files_properties(Cell/PPUTranslator.cpp PROPERTIES COMPILE_FLAGS -fno-rtti)
endif()

target_link_libraries(rpcs3_emu
    PRIVATE
        3rdparty::stblib 3rdparty::libpng)


# CPU
target_sources(rpcs3_emu PRIVATE
    CPU/CPUThread.cpp
    CPU/CPUTranslator.cpp
)

target_link_libraries(rpcs3_emu
    PUBLIC 3rdparty::llvm 3rdparty::asmjit)


# Io
target_sources(rpcs3_emu PRIVATE
    Io/interception.cpp
    Io/KeyboardHandler.cpp
    Io/pad_config.cpp
    Io/pad_config_types.cpp
    Io/PadHandler.cpp
    Io/usb_device.cpp
    Io/Skylander.cpp
    Io/GHLtar.cpp
    Io/Buzz.cpp
    Io/Turntable.cpp
)

# Np
target_sources(rpcs3_emu PRIVATE
    NP/fb_helpers.cpp
    NP/np_contexts.cpp
    NP/np_handler.cpp
    NP/signaling_handler.cpp
    NP/np_structs_extra.cpp
    NP/rpcn_client.cpp
    NP/rpcn_config.cpp
)

# Memory
target_sources(rpcs3_emu PRIVATE
    Memory/vm.cpp
)


# RSX
target_sources(rpcs3_emu PRIVATE
    RSX/gcm_enums.cpp
    RSX/gcm_printing.cpp
    RSX/GSRender.cpp
    RSX/RSXFIFO.cpp
    RSX/rsx_methods.cpp
    RSX/RSXOffload.cpp
    RSX/RSXTexture.cpp
    RSX/RSXThread.cpp
    RSX/rsx_utils.cpp
    RSX/RSXDisAsm.cpp
    RSX/Common/BufferUtils.cpp
    RSX/Common/surface_store.cpp
    RSX/Common/TextureUtils.cpp
    RSX/Common/texture_cache.cpp
    RSX/Null/NullGSRender.cpp
    RSX/Overlays/overlay_animation.cpp
    RSX/Overlays/overlay_controls.cpp
    RSX/Overlays/overlay_edit_text.cpp
    RSX/Overlays/overlay_fonts.cpp
    RSX/Overlays/overlay_list_view.cpp
    RSX/Overlays/overlay_message_dialog.cpp
    RSX/Overlays/overlay_osk.cpp
    RSX/Overlays/overlay_osk_panel.cpp
    RSX/Overlays/overlay_perf_metrics.cpp
    RSX/Overlays/overlay_progress_bar.cpp
    RSX/Overlays/overlay_save_dialog.cpp
    RSX/Overlays/overlay_user_list_dialog.cpp
    RSX/Overlays/overlay_utils.cpp
    RSX/Overlays/overlays.cpp
    RSX/Overlays/overlay_shader_compile_notification.cpp
    RSX/Overlays/overlay_trophy_notification.cpp
    RSX/Overlays/Shaders/shader_loading_dialog.cpp
    RSX/Overlays/Shaders/shader_loading_dialog_native.cpp
    RSX/Program/CgBinaryFragmentProgram.cpp
    RSX/Program/CgBinaryVertexProgram.cpp
    RSX/Program/FragmentProgramDecompiler.cpp
    RSX/Program/GLSLCommon.cpp
    RSX/Program/program_util.cpp
    RSX/Program/ProgramStateCache.cpp
    RSX/Program/VertexProgramDecompiler.cpp
    RSX/Capture/rsx_capture.cpp
    RSX/Capture/rsx_replay.cpp
    RSX/GL/GLCommonDecompiler.cpp
    RSX/GL/GLCompute.cpp
    RSX/GL/GLDraw.cpp
    RSX/GL/GLFragmentProgram.cpp
    RSX/GL/GLGSRender.cpp
    RSX/GL/GLHelpers.cpp
    RSX/GL/GLOverlays.cpp
    RSX/GL/GLPipelineCompiler.cpp
    RSX/GL/GLPresent.cpp
    RSX/GL/GLRenderTargets.cpp
    RSX/GL/GLShaderInterpreter.cpp
    RSX/GL/GLTexture.cpp
    RSX/GL/GLVertexBuffers.cpp
    RSX/GL/GLVertexProgram.cpp
    RSX/GL/GLTextureCache.cpp
    RSX/GL/OpenGL.cpp
)

if(TARGET 3rdparty_vulkan)
    target_sources(rpcs3_emu PRIVATE
        RSX/VK/upscalers/fsr1/fsr_pass.cpp
        RSX/VK/vkutils/barriers.cpp
        RSX/VK/vkutils/buffer_object.cpp
        RSX/VK/vkutils/chip_class.cpp
        RSX/VK/vkutils/commands.cpp
        RSX/VK/vkutils/data_heap.cpp
        RSX/VK/vkutils/image.cpp
        RSX/VK/vkutils/image_helpers.cpp
        RSX/VK/vkutils/scratch.cpp
        RSX/VK/vkutils/sync.cpp
        RSX/VK/vkutils/memory.cpp
        RSX/VK/vkutils/device.cpp
        RSX/VK/vkutils/sampler.cpp
        RSX/VK/vkutils/shared.cpp
        RSX/VK/VKAsyncScheduler.cpp
        RSX/VK/VKCommandStream.cpp
        RSX/VK/VKCommonDecompiler.cpp
        RSX/VK/VKCompute.cpp
        RSX/VK/VKDMA.cpp
        RSX/VK/VKDraw.cpp
        RSX/VK/VKFormats.cpp
        RSX/VK/VKFragmentProgram.cpp
        RSX/VK/VKFramebuffer.cpp
        RSX/VK/VKGSRender.cpp
        RSX/VK/VKHelpers.cpp
        RSX/VK/VKMemAlloc.cpp
        RSX/VK/VKOverlays.cpp
        RSX/VK/VKPipelineCompiler.cpp
        RSX/VK/VKPresent.cpp
        RSX/VK/VKProgramPipeline.cpp
        RSX/VK/VKQueryPool.cpp
        RSX/VK/VKRenderPass.cpp
        RSX/VK/VKRenderTargets.cpp
        RSX/VK/VKResolveHelper.cpp
        RSX/VK/VKResourceManager.cpp
        RSX/VK/VKShaderInterpreter.cpp
        RSX/VK/VKTexture.cpp
        RSX/VK/VKVertexBuffers.cpp
        RSX/VK/VKVertexProgram.cpp
        RSX/VK/VKTextureCache.cpp
    )
endif()

target_link_libraries(rpcs3_emu
    PUBLIC
        3rdparty::ffmpeg
        3rdparty::opengl 3rdparty::stblib
        3rdparty::vulkan 3rdparty::glew
        3rdparty::libusb 3rdparty::wolfssl
    PRIVATE
        3rdparty::xxhash
)

if(APPLE)
    check_function_exists(clock_gettime HAVE_CLOCK_GETTIME)
    if (${HAVE_CLOCK_GETTIME})
	    target_compile_definitions(rpcs3_emu PUBLIC -DHAVE_CLOCK_GETTIME)
    endif()
endif()


if(USE_PRECOMPILED_HEADERS)
    if(COMMAND target_precompile_headers)
        target_precompile_headers(rpcs3_emu PRIVATE "${RPCS3_SRC_DIR}/stdafx.h")
    else()
        # Setup cotire
        option(UNITY_BUILD_EMU "Use unity build for rpcs3_emu target" OFF)

        set_target_properties(rpcs3_emu PROPERTIES
            COTIRE_CXX_PREFIX_HEADER_INIT "${RPCS3_SRC_DIR}/stdafx.h"
            COTIRE_ADD_UNITY_BUILD ${UNITY_BUILD_EMU})

        cotire(rpcs3_emu)
    endif()
endif()
