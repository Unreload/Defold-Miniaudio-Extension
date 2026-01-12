#define DM_LOG_DOMAIN "MiniAudio"
#include <dmsdk/sdk.h>
#include <stdlib.h> // malloc, free, memcpy
#include <math.h>   // fabs

// miniaudio 구현부
#ifndef MINIAUDIO_IMPLEMENTATION
#define MINIAUDIO_IMPLEMENTATION
#endif
#include "miniaudio.h"

static ma_engine engine;

// ------------------------------------
// 구조체 및 전역 변수
// ------------------------------------

struct BGMTrack {
	ma_decoder decoder;
	ma_sound sound;
	void* data_buffer;
	size_t data_size;
	bool is_loaded;

	// 페이드 효과를 위한 변수
	bool is_fading;
	float fade_start_vol;
	float fade_target_vol;
	float fade_timer;
	float fade_duration;
};

struct SFXTrack {
	ma_decoder decoder;
	ma_sound sound;
	void* data_buffer;
	bool is_active;
};

#define SFX_POOL_SIZE 8

static BGMTrack bgm_track = { {0}, {0}, NULL, 0, false, false, 0, 0, 0, 0 };
static SFXTrack sfx_pool[SFX_POOL_SIZE];
static int sfx_cursor = 0;

static bool is_initialized = false;

// 글로벌 설정값 (0.0 ~ 1.0)
static float g_bgm_volume = 1.0f;
static float g_sfx_volume = 1.0f;

// ------------------------------------
// 유틸리티 함수
// ------------------------------------
float clamp(float v, float min, float max) {
	if (v < min) return min;
	if (v > max) return max;
	return v;
}

// ------------------------------------
// 1. 초기화 및 종료
// ------------------------------------
static int InitAudio(lua_State* L) {
	if (is_initialized) return 0;

	ma_result result = ma_engine_init(NULL, &engine);
	if (result != MA_SUCCESS) {
		dmLogError("Failed to initialize audio engine.");
		return 0;
	}

	// SFX 풀 초기화
	for (int i = 0; i < SFX_POOL_SIZE; ++i) {
		sfx_pool[i].is_active = false;
		sfx_pool[i].data_buffer = NULL;
	}

	is_initialized = true;
	dmLogInfo("Miniaudio Initialized.");
	return 0;
}

// ------------------------------------
// 2. BGM 관련 함수 (재생, 정지, 일시정지, 볼륨, 페이드)
// ------------------------------------

static int PlayBGM(lua_State* L) {
	if (!is_initialized) return 0;
	size_t len;
	const char* data = luaL_checklstring(L, 1, &len);

	// 기존 BGM 정리
	if (bgm_track.is_loaded) {
		ma_sound_uninit(&bgm_track.sound);
		ma_decoder_uninit(&bgm_track.decoder);
		if (bgm_track.data_buffer) free(bgm_track.data_buffer);
		bgm_track.is_loaded = false;
	}

	// 메모리 할당 및 복사
	bgm_track.data_buffer = malloc(len);
	if (!bgm_track.data_buffer) return 0;
	memcpy(bgm_track.data_buffer, data, len);

	// 디코더 및 사운드 초기화
	if (ma_decoder_init_memory(bgm_track.data_buffer, len, NULL, &bgm_track.decoder) != MA_SUCCESS) {
		free(bgm_track.data_buffer);
		return 0;
	}
	if (ma_sound_init_from_data_source(&engine, &bgm_track.decoder, 0, NULL, &bgm_track.sound) != MA_SUCCESS) {
		ma_decoder_uninit(&bgm_track.decoder);
		free(bgm_track.data_buffer);
		return 0;
	}

	ma_sound_set_looping(&bgm_track.sound, MA_TRUE);
	ma_sound_set_volume(&bgm_track.sound, g_bgm_volume); // 현재 글로벌 볼륨 적용
	ma_sound_start(&bgm_track.sound);

	bgm_track.is_loaded = true;
	bgm_track.is_fading = false; // 페이드 초기화

	return 0;
}

static int StopBGM(lua_State* L) {
	if (bgm_track.is_loaded) {
		ma_sound_stop(&bgm_track.sound);
		ma_sound_seek_to_pcm_frame(&bgm_track.sound, 0); // 위치 초기화
	}
	return 0;
}

static int PauseBGM(lua_State* L) {
	if (bgm_track.is_loaded) {
		ma_sound_stop(&bgm_track.sound); // Stop이지만 위치는 유지됨 (Resume시 이어짐)
	}
	return 0;
}

static int ResumeBGM(lua_State* L) {
	if (bgm_track.is_loaded) {
		ma_sound_start(&bgm_track.sound);
	}
	return 0;
}

static int SetBGMVolume(lua_State* L) {
	float vol = (float)luaL_checknumber(L, 1);
	g_bgm_volume = clamp(vol, 0.0f, 1.0f);

	if (bgm_track.is_loaded && !bgm_track.is_fading) {
		ma_sound_set_volume(&bgm_track.sound, g_bgm_volume);
	}
	return 0;
}

// BGM 페이드 (Target Volume, Duration Seconds)
static int FadeBGM(lua_State* L) {
	if (!bgm_track.is_loaded) return 0;

	float target_vol = (float)luaL_checknumber(L, 1);
	float duration = (float)luaL_checknumber(L, 2);

	bgm_track.fade_start_vol = ma_sound_get_volume(&bgm_track.sound);
	bgm_track.fade_target_vol = clamp(target_vol, 0.0f, 1.0f);
	bgm_track.fade_duration = duration;
	bgm_track.fade_timer = 0.0f;
	bgm_track.is_fading = true;

	// 페이드 목표가 글로벌 볼륨 설정도 변경함
	g_bgm_volume = bgm_track.fade_target_vol; 

	return 0;
}

// ------------------------------------
// 3. SFX 관련 함수 (재생, 볼륨, 패닝)
// ------------------------------------

static int PlaySFX(lua_State* L) {
	if (!is_initialized) return 0;

	size_t len;
	const char* data = luaL_checklstring(L, 1, &len);

	// 선택 인자: 패닝 (-1.0 ~ 1.0)
	float pan = 0.0f;
	if (lua_gettop(L) >= 2) {
		pan = (float)luaL_checknumber(L, 2);
	}

	SFXTrack* track = &sfx_pool[sfx_cursor];
	sfx_cursor = (sfx_cursor + 1) % SFX_POOL_SIZE;

	// 사용 중이면 정리
	if (track->is_active) {
		ma_sound_uninit(&track->sound);
		ma_decoder_uninit(&track->decoder);
		free(track->data_buffer);
		track->is_active = false;
	}

	track->data_buffer = malloc(len);
	if (!track->data_buffer) return 0;
	memcpy(track->data_buffer, data, len);

	if (ma_decoder_init_memory(track->data_buffer, len, NULL, &track->decoder) != MA_SUCCESS) {
		free(track->data_buffer);
		return 0;
	}
	if (ma_sound_init_from_data_source(&engine, &track->decoder, 0, NULL, &track->sound) != MA_SUCCESS) {
		ma_decoder_uninit(&track->decoder);
		free(track->data_buffer);
		return 0;
	}

	ma_sound_set_volume(&track->sound, g_sfx_volume);
	ma_sound_set_pan(&track->sound, clamp(pan, -1.0f, 1.0f)); // 패닝 적용
	ma_sound_start(&track->sound);
	track->is_active = true;

	return 0;
}

static int SetSFXVolume(lua_State* L) {
	float vol = (float)luaL_checknumber(L, 1);
	g_sfx_volume = clamp(vol, 0.0f, 1.0f);

	// 현재 재생 중인 모든 SFX에도 즉시 적용할지 여부 (여기선 즉시 적용)
	for (int i = 0; i < SFX_POOL_SIZE; ++i) {
		if (sfx_pool[i].is_active) {
			ma_sound_set_volume(&sfx_pool[i].sound, g_sfx_volume);
		}
	}
	return 0;
}

// ------------------------------------
// 4. Update 루프 (페이드 처리를 위해 필요)
// ------------------------------------
dmExtension::Result Update(dmExtension::Params* params) {
	if (bgm_track.is_loaded && bgm_track.is_fading) {
		// 간단한 dt 계산 (Defold는 기본적으로 60fps 근처)
		float dt = 1.0f / 60.0f; 

		bgm_track.fade_timer += dt;
		float t = bgm_track.fade_timer / bgm_track.fade_duration;

		if (t >= 1.0f) {
			t = 1.0f;
			bgm_track.is_fading = false;
		}

		// 선형 보간 (Lerp)
		float new_vol = bgm_track.fade_start_vol + (bgm_track.fade_target_vol - bgm_track.fade_start_vol) * t;
		ma_sound_set_volume(&bgm_track.sound, new_vol);
	}
	return dmExtension::RESULT_OK;
}

// ------------------------------------
// 5. 모듈 등록
// ------------------------------------
static const luaL_Reg Module_methods[] = {
	{"init", InitAudio},
	{"play_bgm", PlayBGM},
	{"stop_bgm", StopBGM},
	{"pause_bgm", PauseBGM},
	{"resume_bgm", ResumeBGM},
	{"set_bgm_volume", SetBGMVolume},
	{"fade_bgm", FadeBGM},

	{"play_sfx", PlaySFX},
	{"set_sfx_volume", SetSFXVolume},
	{0, 0}
};

static void LuaInit(lua_State* L) {
	luaL_register(L, "miniaudio", Module_methods);
	lua_pop(L, 1);
}

dmExtension::Result AppInitialize(dmExtension::AppParams* params) { return dmExtension::RESULT_OK; }
dmExtension::Result Initialize(dmExtension::Params* params) { LuaInit(params->m_L); return dmExtension::RESULT_OK; }
dmExtension::Result AppFinalize(dmExtension::AppParams* params) { return dmExtension::RESULT_OK; }

dmExtension::Result Finalize(dmExtension::Params* params) {
	if (bgm_track.is_loaded) {
		ma_sound_uninit(&bgm_track.sound);
		ma_decoder_uninit(&bgm_track.decoder);
		free(bgm_track.data_buffer);
	}
	for (int i = 0; i < SFX_POOL_SIZE; ++i) {
		if (sfx_pool[i].is_active) {
			ma_sound_uninit(&sfx_pool[i].sound);
			ma_decoder_uninit(&sfx_pool[i].decoder);
			free(sfx_pool[i].data_buffer);
		}
	}
	if (is_initialized) ma_engine_uninit(&engine);
	return dmExtension::RESULT_OK;
}

DM_DECLARE_EXTENSION(MiniAudioExt, "MiniAudioExt", AppInitialize, AppFinalize, Initialize, Update, 0, Finalize)
