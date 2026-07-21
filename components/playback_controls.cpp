#include "playback_controls.h"

namespace playback_controls {
namespace {
	static bool g_is_playing = true;

	bool ensure_initialized()
	{
		static bool initialized = false;
		if (!initialized)
		{
			winrt::init_apartment();
			initialized = true;
		}
		return true;
	}

	GlobalSystemMediaTransportControlsSession get_current_session()
	{
		ensure_initialized();
		auto manager = GlobalSystemMediaTransportControlsSessionManager::RequestAsync().get();
		if (!manager)
			return nullptr;
		return manager.GetCurrentSession();
	}
}

	bool is_playing()
	{
		auto session = get_current_session();
		if (!session)
			return g_is_playing;

		auto playback = session.GetPlaybackInfo();
		g_is_playing = playback.PlaybackStatus() ==
			GlobalSystemMediaTransportControlsSessionPlaybackStatus::Playing;
		return g_is_playing;
	}

	bool send_action(PlaybackAction action)
	{
		auto session = get_current_session();
		if (!session)
			return false;

		switch (action)
		{
		case PlaybackAction::PlayPause:
			session.TryTogglePlayPauseAsync().get();
			break;
		case PlaybackAction::Next:
			session.TrySkipNextAsync().get();
			break;
		case PlaybackAction::Previous:
			session.TrySkipPreviousAsync().get();
			break;
		}

		return true;
	}
}
