#ifndef SND_IOP_H
#define SND_IOP_H

// 989snd IOP sound-server command opcodes. Passed as the sceSifCallRpc
// `command` field to the sound server (snd_rpcServer) or the CD server
// (snd_cdRpcServer). The values are verified from the boot ELF's C call
// sites; where the Deadlocked PAL 989snd library
// (reference/dl/989snd/ee/989snd.c) issues the same opcode it does so from
// the same-named wrapper, which is how each name below was confirmed.
enum {
    SND_IOP_CMD_UNLOAD_BANK = 0x06,
    SND_IOP_CMD_RESOLVE_BANK_XREFS = 0x08,
    SND_IOP_CMD_SET_MASTER_VOLUME = 0x09,
    SND_IOP_CMD_SET_PLAYBACK_MODE = 0x0B,
    SND_IOP_CMD_SET_MIXER_MODE = 0x0D,
    SND_IOP_CMD_AUTO_REVERB = 0x10,
    SND_IOP_CMD_PLAY_SOUND = 0x11,
    SND_IOP_CMD_STOP_SOUND = 0x15,
    SND_IOP_CMD_PAUSE_ALL_SOUNDS_IN_GROUP = 0x16,
    SND_IOP_CMD_CONTINUE_ALL_SOUNDS_IN_GROUP = 0x17,
    SND_IOP_CMD_STOP_ALL_SOUNDS = 0x18,
    SND_IOP_CMD_SOUND_IS_STILL_PLAYING = 0x19,
    SND_IOP_CMD_SET_SOUND_PARAMS = 0x21,
    SND_IOP_CMD_INIT_VAG_STREAMING = 0x2A,
    SND_IOP_CMD_PLAY_VAG_STREAM = 0x2C,
    SND_IOP_CMD_PAUSE_VAG_STREAM = 0x2D,
    SND_IOP_CMD_CONTINUE_VAG_STREAM = 0x2E,
    SND_IOP_CMD_GET_VAG_STREAM_TIME_REMAINING = 0x32,
    SND_IOP_CMD_STOP_ALL_STREAMS = 0x34,
    SND_IOP_CMD_CHECK_CD_IDLE = 0x36,
    SND_IOP_CMD_BREAK_CD_STREAM_READ = 0x37,
    SND_IOP_CMD_CD_STREAM_READ = 0x38,
    SND_IOP_CMD_INIT_MOVIE_SOUND = 0x3B,
    SND_IOP_CMD_CLOSE_MOVIE_SOUND = 0x3C,
    SND_IOP_CMD_RESET_MOVIE_SOUND = 0x3D,
    SND_IOP_CMD_START_MOVIE_SOUND = 0x3E,
    SND_IOP_CMD_EXECUTE_BATCH = 0x4D,
    SND_IOP_CMD_SET_GROUP_VOICE_RANGE = 0x4E,
    SND_IOP_CMD_IS_VAG_STREAM_BUFFERED = 0x4F,
    SND_IOP_CMD_SET_REVERB = 0x50,
    SND_IOP_CMD_PREALLOC_REVERB_WORK_AREA = 0x51,
    SND_IOP_CMD_CD_BANK_LOAD = 0x57,
    SND_IOP_CMD_UPDATE_MOVIE_ADPCM = 0x5A,
    SND_IOP_CMD_GET_MOVIE_NAX = 0x5B,
};

// snd_StreamSafeCdSync modes.
enum {
    // Block until the CD is no longer busy.
    SND_CD_SYNC_MODE_WAIT = 0,
    // Non-blocking check: return 1 while the CD is still busy.
    SND_CD_SYNC_MODE_CHECK = 1,
};

#endif
