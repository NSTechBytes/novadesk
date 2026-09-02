/* Copyright (C) 2026 OfficialNovadesk
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#pragma once

#include <string>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <queue>
#include <winrt/Windows.Media.Control.h>
#include "ImageUtils.h"

namespace winrt {
using namespace Windows::Media::Control;
}

/**
 * @brief Media playback statistics from the system media session.
 */
struct MediaStats {
  bool available = false;  ///< True if a media session is active.
  std::string player;      ///< Player application name.
  std::string artist;      ///< Artist name.
  std::string album;       ///< Album name.
  std::string title;       ///< Track title.
  std::string thumbnail;   ///< Path to album cover art.
  int duration = 0;        ///< Track duration in seconds.
  int position = 0;        ///< Current playback position in seconds.
  int progress = 0;        ///< Playback progress percentage.
  int state = 0;           ///< Playback state: 0=Stopped, 1=Playing, 2=Paused.
  int status = 0;          ///< Session status: 0=Closed, 1=Opened.
  bool shuffle = false;    ///< Shuffle mode enabled.
  bool repeat = false;     ///< Repeat mode enabled.
  std::string genres;      ///< Genre tags.
};

/**
 * @brief Supported media control actions.
 */
enum class MediaAction {
  Play,         ///< Start playback.
  Pause,        ///< Pause playback.
  PlayPause,    ///< Toggle play/pause.
  Stop,         ///< Stop playback.
  Next,         ///< Skip to next track.
  Previous,     ///< Skip to previous track.
  SetPosition,  ///< Seek to a position.
  SetShuffle,   ///< Toggle shuffle mode.
  SetRepeat     ///< Toggle repeat mode.
};

/**
 * @brief A queued media action with parameters.
 */
struct MediaActionItem {
  MediaAction action; ///< The action to perform.
  int value;          ///< Numeric parameter (e.g., position).
  bool flag;          ///< Boolean parameter (e.g., shuffle state).
};

/**
 * @brief Controls system media playback and retrieves media metadata.
 *
 * @note Runs a background worker thread for WinRT session management.
 *       Thread-safe for concurrent access from the UI and JS threads.
 */
class MediaController {
public:
  MediaController();
  ~MediaController();

  /**
   * @brief Gets the current media playback statistics.
   *
   * @return A snapshot of the current media state.
   */
  MediaStats GetStats();

  /**
   * @brief Queues a media control action.
   *
   * @param action The action to perform.
   * @param value Numeric parameter (default: 0).
   * @param flag Boolean parameter (default: false).
   */
  void QueueAction(MediaAction action, int value = 0, bool flag = false);

private:
  void WorkerThread();
  void UpdateData();
  void UpdateCover(
      const winrt::hstring &playerAppId,
      const winrt::GlobalSystemMediaTransportControlsSessionMediaProperties
          &props);
  void ProcessActions(std::queue<MediaActionItem> &pending);

  winrt::GlobalSystemMediaTransportControlsSessionManager m_manager{nullptr};

  std::mutex m_statsMutex;
  MediaStats m_stats; ///< Protected by m_statsMutex.

  std::mutex m_actionMutex;
  std::condition_variable m_actionCv;
  std::queue<MediaActionItem> m_actions; ///< Protected by m_actionMutex.
  bool m_stop = false;
  std::thread m_worker; ///< Background worker thread.

  ULONG_PTR m_gdiToken; ///< GDI+ initialization token.

  // Cover art tracking
  winrt::hstring m_lastCoverPath;
  std::wstring m_lastTrackId;

  // Position prediction (robust handling for Chrome/Edge)
  double m_localPosSec = 0.0;
  std::chrono::steady_clock::time_point m_localPosTime{};
  bool m_hasLocalPos = false;
  std::wstring m_prevSyncTrackId;
  int m_prevSyncDuration = 0;
  int64_t m_lockedStartTime = -1;
};
