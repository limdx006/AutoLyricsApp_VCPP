#pragma once
#include "common.h"

namespace playback_controls {
	enum class PlaybackAction {
		PlayPause,
		Next,
		Previous
	};

	bool is_playing();
	bool send_action(PlaybackAction action);
}
