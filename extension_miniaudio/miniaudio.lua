-- extension_miniaudio/miniaudio.lua
local M = {}

-- C++ 확장 연결
local internal = miniaudio

function M.init()
	if internal then internal.init() end
end

-- ===========================
-- BGM (배경음악)
-- ===========================

-- 배경음악 재생 (경로)
function M.play_bgm(path)
	local data = sys.load_resource(path)
	if data and internal then internal.play_bgm(data) end
end

-- 배경음악 완전 정지 (처음으로 돌아감)
function M.stop_bgm()
	if internal then internal.stop_bgm() end
end

-- 일시 정지 (현재 위치 유지)
function M.pause_bgm()
	if internal then internal.pause_bgm() end
end

-- 재개 (일시 정지된 위치에서 시작)
function M.resume_bgm()
	if internal then internal.resume_bgm() end
end

-- BGM 볼륨 조절 (0.0 ~ 1.0)
function M.set_bgm_volume(volume)
	if internal then internal.set_bgm_volume(volume) end
end

-- BGM 페이드 효과
-- @param target_volume: 목표 볼륨 (0.0 ~ 1.0)
-- @param duration: 걸리는 시간 (초)
function M.fade_bgm(target_volume, duration)
	if internal then internal.fade_bgm(target_volume, duration) end
end

-- ===========================
-- SFX (효과음)
-- ===========================

-- 효과음 재생 (패닝 지원)
-- @param path: 파일 경로
-- @param pan: (선택) -1.0(왼쪽) ~ 1.0(오른쪽), 기본값 0.0
function M.play_sfx(path, pan)
	local data = sys.load_resource(path)
	pan = pan or 0.0
	if data and internal then internal.play_sfx(data, pan) end
end

-- SFX 전체 볼륨 조절 (0.0 ~ 1.0)
function M.set_sfx_volume(volume)
	if internal then internal.set_sfx_volume(volume) end
end

return M
