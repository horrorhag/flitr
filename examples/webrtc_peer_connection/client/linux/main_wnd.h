/*
 *  Copyright 2012 The WebRTC Project Authors. All rights reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef WEBRTC_EXAMPLES_PEERCONNECTION_CLIENT_LINUX_MAIN_WND_H_
#define WEBRTC_EXAMPLES_PEERCONNECTION_CLIENT_LINUX_MAIN_WND_H_

#include <memory>
#include <string>

#include "webrtc/examples/peerconnection/client/main_wnd.h"
#include "webrtc/examples/peerconnection/client/peer_connection_client.h"

// Implements the main console UI of the peer connection client.
// implementation.
class SessionManager : public MainConsole {
 public:
  SessionManager(const char* server, int port, bool autoconnect, bool autocall);
  ~SessionManager();

  virtual void RegisterObserver(MainConsoleCallback* callback);
  virtual void SwitchToConnectUI();
  virtual void SwitchToPeerList(const Peers& peers);
  virtual void SwitchToStreamingUI();
  virtual void Message(std::string text);
  virtual MainConsole::UI current_ui();

  virtual void QueueUIThreadCallback(int msg_id, void* data);

  // Start the session
  bool Start();

  // Callback for when the console is destroyed.
  void OnDestroyed();

  // Callback for when the user clicks the "Connect" button.
  void OnStart();

  // Callback when the user double clicks a peer in order to initiate a
  // connection.
  void OnRowActivated(GtkTreeView* tree_view, GtkTreePath* path,
                      GtkTreeViewColumn* column);

 protected:
  MainConsoleCallback* callback_;
  std::string server_;
  std::string port_;
  bool autoconnect_;
  bool autocall_;
  std::unique_ptr<uint8_t[]> draw_buffer_;
  int draw_buffer_size_;
};

#endif  // WEBRTC_EXAMPLES_PEERCONNECTION_CLIENT_LINUX_MAIN_WND_H_
